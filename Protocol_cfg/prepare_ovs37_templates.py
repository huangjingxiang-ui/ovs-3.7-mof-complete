#!/usr/bin/env python3
"""Create OVS 3.7 template files from a verified OVS 3.7-MOF tree.

This is a maintainer tool.  Normal protocol generation uses
``assemble_ovs37.py`` and never needs access to the golden tree.
"""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


FILES = {
    "flow_h": "include/openvswitch/flow.h",
    "flow_c": "lib/flow.c",
    "packets_c": "lib/packets.c",
    "ofp_actions_c": "lib/ofp-actions.c",
    "xlate_c": "ofproto/ofproto-dpif-xlate.c",
    "meta_flow_c": "lib/meta-flow.c",
    "odp_execute_c": "lib/odp-execute.c",
    "match_c": "lib/match.c",
    "openflow_10_h": "include/openflow/openflow-1.0.h",
}


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one occurrence, found {count}")
    return text.replace(old, new, 1)


def between(text: str, start: str, end: str, token: str, label: str) -> str:
    a = text.find(start)
    if a < 0:
        raise RuntimeError(f"{label}: start marker not found: {start!r}")
    b = text.find(end, a)
    if b < 0:
        raise RuntimeError(f"{label}: end marker not found: {end!r}")
    return text[:a] + token + text[b:]


def make_templates(golden: Path, out: Path, cfg: Path) -> None:
    generated = cfg / "PyPrj" / "cfile"

    flow_h = (golden / FILES["flow_h"]).read_text(encoding="utf-8")
    pad_start = flow_h.index("    ovs_be64 flow_pad_end[")
    pad_end = flow_h.index("};", pad_start) + 3
    flow_h = flow_h[:pad_start] + "@@FLOW_PAD@@\n" + flow_h[pad_end:]
    custom_start = flow_h.index("\t struct stmac_dst{")
    footer_start = flow_h.index("\t#ifdef __cplusplus", custom_start)
    flow_h = flow_h[:custom_start] + "@@FLOW_DECLS@@" + flow_h[footer_start:]

    flow_c = (golden / FILES["flow_c"]).read_text(encoding="utf-8")
    map_start = flow_c.index("    /* MOF protocol field maps */")
    map_last = flow_c.rfind("    FLOWMAP_SET_MOF", map_start,
                           flow_c.index("\n}\n", map_start))
    map_end = flow_c.index("\n", map_last) + 1
    flow_c = flow_c[:map_start] + "@@FLOWMAP@@" + flow_c[map_end:]
    parser_start = flow_c.index("size_t parser_dl(")
    while parser_start and flow_c[parser_start - 1] in " \t":
        parser_start -= 1
    canonical_flow_tail = flow_c[parser_start:]
    flow_c = flow_c[:parser_start] + "@@FLOW_C_TAIL@@"

    packets = (golden / FILES["packets_c"]).read_text(encoding="utf-8")
    packets_1 = (generated / "packets" / "packets_c_1.c").read_text(encoding="utf-8")
    packets_2 = (generated / "packets" / "packets_c_2.c").read_text(encoding="utf-8")
    packets = replace_once(packets, packets_1, "@@PACKETS_1@@", "packets-1")
    packets = replace_once(packets, packets_2, "@@PACKETS_2@@", "packets-2")

    ofp_actions = (golden / FILES["ofp_actions_c"]).read_text(encoding="utf-8")
    ofp_block = (generated / "ofp-actions.c").read_text(encoding="utf-8")
    ofp_actions = replace_once(ofp_actions, ofp_block, "@@OFP_ACTIONS@@", "ofp-actions")

    xlate = (golden / FILES["xlate_c"]).read_text(encoding="utf-8")
    offsets = (generated / "ofproto-dpif-xlate" / "xlate-offset.c").read_text(encoding="utf-8")
    xlate = replace_once(xlate, offsets, "@@XLATE_OFFSETS@@", "xlate-offsets")
    action_start = xlate.index("        case OFPACT_SEG_ROUTING: {")
    action_end = xlate.index("        }\n", action_start) + len("        }\n")
    xlate = xlate[:action_start] + "@@XLATE_SRV6_ACTION@@" + xlate[action_end:]
    helper_start = xlate.index("void \nxlate_set_flow_value_srv6(")
    helper_end = xlate.index("\n}\n", helper_start) + len("\n}\n")
    xlate = xlate[:helper_start] + "@@XLATE_SRV6_HELPER@@" + xlate[helper_end:]

    meta_flow = (golden / FILES["meta_flow_c"]).read_text(encoding="utf-8")
    meta_start = meta_flow.index("char * \nmf_parse_set_mof(")
    meta_flow = meta_flow[:meta_start] + "@@META_FLOW@@"

    odp_execute = (golden / FILES["odp_execute_c"]).read_text(encoding="utf-8")
    odp_block = (generated / "odp-execute" / "case_SEG_ROUTING.c").read_text(encoding="utf-8")
    odp_execute = replace_once(odp_execute, odp_block, "@@ODP_SEG_ROUTING@@", "odp-seg")

    match = (golden / FILES["match_c"]).read_text(encoding="utf-8")
    match = between(match, "void\nmatch_format_mof(",
                    "/* Converts 'match' to a string", "@@MATCH_FORMAT@@", "match")

    openflow = (golden / FILES["openflow_10_h"]).read_text(encoding="utf-8")
    of10_block = (generated / "openflow-1.0.h").read_text(encoding="utf-8")
    openflow = replace_once(openflow, of10_block, "@@OPENFLOW10_MATCH@@", "openflow-1.0")

    rendered = {
        FILES["flow_h"]: flow_h,
        FILES["flow_c"]: flow_c,
        FILES["packets_c"]: packets,
        FILES["ofp_actions_c"]: ofp_actions,
        FILES["xlate_c"]: xlate,
        FILES["meta_flow_c"]: meta_flow,
        FILES["odp_execute_c"]: odp_execute,
        FILES["match_c"]: match,
        FILES["openflow_10_h"]: openflow,
    }
    for rel, text in rendered.items():
        path = out / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8", newline="\n")

    for rel in ("include/odp-netlink.h", "lib/meta-flow.inc"):
        path = out / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes((golden / rel).read_bytes())

    canonical_dir = out / "canonical"
    canonical_dir.mkdir(parents=True, exist_ok=True)
    (canonical_dir / "flow-tail.c").write_text(
        canonical_flow_tail, encoding="utf-8", newline="\n")
    digest = hashlib.sha256()
    yaml_root = cfg / "header_describe"
    for path in sorted(p for p in yaml_root.rglob("*")
                       if p.is_file() and not p.name.startswith("._")
                       and p.suffix.lower() in (".yml", ".yaml")):
        digest.update(path.relative_to(yaml_root).as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    (canonical_dir / "protocol-yaml.sha256").write_text(
        digest.hexdigest() + "\n", encoding="ascii", newline="\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--golden", required=True, type=Path)
    parser.add_argument("--output", type=Path,
                        default=Path(__file__).parent / "templates" / "ovs-3.7")
    args = parser.parse_args()
    make_templates(args.golden.resolve(), args.output.resolve(), Path(__file__).parent)
    print(f"OVS 3.7 templates created in {args.output.resolve()}")


if __name__ == "__main__":
    main()

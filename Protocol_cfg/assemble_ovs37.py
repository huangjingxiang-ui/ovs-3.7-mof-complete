#!/usr/bin/env python3
"""Render generated protocol fragments into an OVS 3.7 source tree."""

from __future__ import annotations

import argparse
import hashlib
import os
import re
from pathlib import Path


FILES = (
    "include/openvswitch/flow.h",
    "lib/flow.c",
    "lib/packets.c",
    "lib/ofp-actions.c",
    "ofproto/ofproto-dpif-xlate.c",
    "lib/meta-flow.c",
    "lib/odp-execute.c",
    "lib/match.c",
    "include/openflow/openflow-1.0.h",
    "include/odp-netlink.h",
    "lib/meta-flow.inc",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def tab_indent(text: str) -> str:
    return "".join(("\t" + line if line.strip() else line)
                   for line in text.splitlines(keepends=True))


def split_srv6(text: str) -> tuple[str, str]:
    action_start = text.index("        case OFPACT_SEG_ROUTING: {")
    action_end = text.index("        }\n", action_start) + len("        }\n")
    helper_start = text.index("void \nxlate_set_flow_value_srv6(")
    return text[action_start:action_end], text[helper_start:]


def yaml_digest(root: Path) -> str:
    digest = hashlib.sha256()
    for path in sorted(p for p in root.rglob("*")
                       if p.is_file() and not p.name.startswith("._")
                       and p.suffix.lower() in (".yml", ".yaml")):
        digest.update(path.relative_to(root).as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def atomic_write(path: Path, data: str, newline: str = "\n") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + ".mof-new")
    tmp.write_text(data, encoding="utf-8", newline=newline)
    os.replace(tmp, path)


def render(config_dir: Path, target: Path) -> None:
    templates = config_dir / "templates" / "ovs-3.7"
    cfile = config_dir / "PyPrj" / "cfile"
    srv6_action, srv6_helper = split_srv6(
        read(cfile / "ofproto-dpif-xlate" / "xlate-srv6.c"))

    generated_flow_tail = "".join(read(cfile / name) for name in (
        "parser.c", "miniflow_extract.c", "hash.c"))
    canonical_dir = templates / "canonical"
    expected_yaml = read(canonical_dir / "protocol-yaml.sha256").strip()
    if yaml_digest(config_dir / "header_describe") == expected_yaml:
        flow_tail = read(canonical_dir / "flow-tail.c")
    else:
        flow_tail = generated_flow_tail

    declarations = "".join(tab_indent(read(cfile / name)) for name in (
        "header.c", "smallstruct.c", "unionprotols.c", "enumpmap.c", "flow.h"))
    declarations = re.sub(r"\n{3,}", "\n\n", declarations)
    flowmap = read(cfile / "flowmap_set_mof.c").replace("\t", "    ")
    flowmap = "    /* MOF protocol field maps */\n" + flowmap

    values = {
        "@@FLOW_PAD@@": read(cfile / "ori_flow_pad.c").rstrip(),
        "@@FLOW_DECLS@@": declarations,
        "@@FLOWMAP@@": flowmap,
        "@@FLOW_C_TAIL@@": flow_tail,
        "@@PACKETS_1@@": read(cfile / "packets" / "packets_c_1.c"),
        "@@PACKETS_2@@": read(cfile / "packets" / "packets_c_2.c"),
        "@@OFP_ACTIONS@@": read(cfile / "ofp-actions.c"),
        "@@XLATE_OFFSETS@@": read(cfile / "ofproto-dpif-xlate" / "xlate-offset.c"),
        "@@XLATE_SRV6_ACTION@@": srv6_action,
        "@@XLATE_SRV6_HELPER@@": srv6_helper.rstrip() + "\n",
        "@@META_FLOW@@": read(cfile / "meta-flow.c"),
        "@@ODP_SEG_ROUTING@@": read(cfile / "odp-execute" / "case_SEG_ROUTING.c"),
        "@@MATCH_FORMAT@@": read(cfile / "match.c"),
        "@@OPENFLOW10_MATCH@@": read(cfile / "openflow-1.0.h"),
    }

    for rel in FILES:
        source = templates / rel
        if not source.exists():
            raise RuntimeError(f"missing OVS 3.7 template: {source}")
        if rel in ("include/odp-netlink.h", "lib/meta-flow.inc"):
            data = read(source)
        else:
            data = read(source)
            for token, value in values.items():
                if token in data:
                    data = data.replace(token, value)
            if "@@" in data:
                raise RuntimeError(f"unexpanded template token in {rel}")
        newline = "\r\n" if rel in (
            "lib/ofp-actions.c", "ofproto/ofproto-dpif-xlate.c") else "\n"
        atomic_write(target / rel, data, newline)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", required=True, type=Path,
                        help="OVS 3.7 source tree to update")
    args = parser.parse_args()
    target = args.target.resolve()
    if not (target / "configure.ac").is_file():
        raise SystemExit(f"not an OVS source tree: {target}")
    render(Path(__file__).parent.resolve(), target)
    print(f"OVS 3.7-MOF sources generated in {target}")


if __name__ == "__main__":
    main()

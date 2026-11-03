#!/usr/bin/env python3
"""Create reviewable Markdown/HTML reports from retained real VM evidence."""
import pathlib,json,html,base64
root=pathlib.Path(__file__).resolve().parents[1]
e=root/'evidence/2026-11-03'
results=json.loads((e/'results.json').read_text(encoding='utf-8'))
lines=['# OVS 3.7 MOF 与 DPDK 23.11 功能复验报告','',
'测试日期：2026-11-03。对象：ovs-3.7-mof-complete。环境：Ubuntu 单虚拟机、2 个逻辑 CPU、约 4 GiB 内存、DPDK 23.11.0、OVS 3.7.0。', '',
'## 结论','',
'8 项主要数据面功能通过真实 DPDK PCAP PMD 实测，输出报文逐包满足字节断言。MOF 规则下发、回读、删除通过。本轮定位并修复 MOF 创建规则时遗漏 ofproto 引用导致的清理和退出断言；修复后的退出日志未再出现该断言。', '',
'这是一轮功能冒烟验证，未开展性能、长期稳定性、物理网卡或控制器测试。IPv4 与 IPv6 转换、最长前缀匹配、CRC 按已有约定排除，不能据此声称所有功能全部通过。','',
'## 重构和构建','',
'重新运行 Protocol_cfg/configure.sh，输出 Configuration successful。生成器负责的 11 个文件与受保护 final 基线逐字节相同。随后执行 make clean、configure --with-dpdk=shared、make -j2，返回成功；补充修复后再次编译成功。final 保持只读。本轮新增 ofproto/ofproto.c 引用修复不属于这 11 个生成文件，因此完整工程相对 final 多出该修复。','',
'另将待发布 Git 文件生成源码归档，在虚拟机全新目录 /home/ovs/mof-release-check-VeeKGd/ovs-3.7-mof-complete 内运行一键构建。重构、从零编译、二进制校验和 11 文件对照均成功，日志末尾为 RELEASE_CHECK_SUCCESS。打包检查同时修复了空输出目录未创建、Instructions_ori 模板被忽略及 utilities/docker/Makefile 被误排除的问题。','',
'## 方法','',
'使用工程内 ovsdb-server、ovs-vswitchd、ovs-vsctl、ovs-ofctl 和 ovs-appctl。独立数据库位于 /tmp/mof-smoke-*，桥为 netdev，两个 Interface 均为 type=dpdk，设备使用 net_pcap PMD。dpdk_initialized=true，并在 dpif/show 中确认 DPDK 入口、出口；不是仅用普通 Linux 端口验证。','',
'输入为 100 包 IPv4 UDP PCAP，源 MAC 02:00:00:00:00:0a，目的 MAC 00:11:22:33:44:55。循环回放解决流表准备期间有限输入耗尽问题。数据包数随时序变化，不用于推导吞吐率或丢包率。','',
'| 用例 | 输出包数 | 满足字节断言包数 | 规则删除后为空 | 退出断言 | 结果 |',
'|---|---:|---:|---|---|---|']
for r in results:
    lines.append(f"| {r['name']} | {r['packets']} | {r['verified_packets']} | {'是' if r['delete_pass'] else '否'} | {'有' if r['shutdown_assertion'] else '无'} | {'通过' if r['pass_test'] and r['delete_pass'] and not r['shutdown_assertion'] else '失败'} |")
lines += ['','## 每项流表与实包证据','']
for r in results:
    lines += ['### '+r['name'],'','下发规则：','','```text',*r['flows'],'```','','回读与命中：','','```text',r['flow_dump'].strip(),'```']
    if r['first_hex']: lines += ['','输出首包原始字节：','','```text',r['first_hex'],'```']
    lines += ['',f"原始证据：[PCAP](evidence/2026-11-03/{r['name']}/{r['name']}.pcap)；[JSON](evidence/2026-11-03/{r['name']}/results.json)。",'']
lines += ['## 本轮发现及修复','',
'首次连续切换端口测试在 oftable_destroy() 触发 classifier_is_empty 断言；独立测试退出时触发 ovs_refcount_unref() 的 old_refcount > 0 断言。对照 OVS 3.7 原生 ofproto_rule_create() 后发现 MOF 创建分支没有取得 ofproto 引用，而共用析构函数会释放引用。','',
'修复为在 ofproto_rule_create_mof() 分配规则前调用 ofproto_try_ref()，失败时返回错误，并在规则分配失败时调用 ofproto_unref()。补丁保存在源码 ofproto/ofproto.c 及 patches/ovs37-mof-rule-refcount.patch，受保护 final 未修改。修复后的 8 项独立复验均无退出断言；同一实例连续创建、删除端口和流表的回归为 8/8，通过后正常退出，见 lifecycle.log。增加独立 DPDK file-prefix 后又执行一次同样回归，仍为 8/8，见 lifecycle-prefix.log。','',
'修复前日志保存在 smoke.log、isolated-smoke-v2.log、final-smoke.log；修复后日志为 fixed-smoke.log，汇总为 results.json。','',
'## 测试截图','',
'以下为 Ubuntu 虚拟机终端的真实屏幕截图。终端使用 show_evidence.py 展示本轮保留的 JSON、流表回读和 PCAP 证据；截图展示记录复核过程，不表示截图时仍有测试 daemon 持续运行。', '',
'### 功能汇总','',
'![功能汇总](evidence/2026-11-03/screenshots/01-summary.png)','',
'### 流表命中与多表处理','',
'![MOF 流表](evidence/2026-11-03/screenshots/02-flows.png)','',
'### 实际输出报文','',
'![MAC VLAN 和字节修改](evidence/2026-11-03/screenshots/03-packets.png)','',
'### DPDK 和构建结果','',
'![DPDK 初始化和全新源码构建](evidence/2026-11-03/screenshots/04-dpdk.png)','',
'## 复现','',
'完整步骤、环境依赖、DPDK 安装、大页恢复、构建脚本参数、自定义流表示例及故障排查见 [README](README.md)。', '',
'```bash','cd /home/ovs/ovs-3.7-mof-complete','bash build-ovs37-mof.sh --clean --jobs 2','sudo python3 scripts/functional_smoke.py','sudo python3 scripts/functional_smoke.py lifecycle','```','']
text='\n'.join(lines)
(root/'功能复验报告.md').write_text(text,encoding='utf-8')
import markdown
body=markdown.markdown(text,extensions=['tables','fenced_code'])
for screenshot in (e/'screenshots').glob('*.png'):
    relative=screenshot.relative_to(root).as_posix()
    body=body.replace('src="'+relative+'"','src="data:image/png;base64,'+base64.b64encode(screenshot.read_bytes()).decode('ascii')+'"')
(root/'功能复验报告.html').write_text('<!doctype html><meta charset="utf-8"><title>OVS 3.7 MOF 功能复验</title><style>body{max-width:1100px;margin:40px auto;font:16px/1.65 sans-serif;padding:0 24px;color:#18212b}pre{white-space:pre-wrap;overflow-wrap:anywhere;background:#f3f5f7;padding:16px}table{border-collapse:collapse}td,th{border:1px solid #ccd3db;padding:8px 12px}img{max-width:100%;height:auto}h2{margin-top:36px}code{font-size:90%}</style>'+body,encoding='utf-8')
print(root/'功能复验报告.md')

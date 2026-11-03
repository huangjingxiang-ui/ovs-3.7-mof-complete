# OVS 3.7-MOF 完整重构工程

本工程将 OVS 2.16-MOF 的协议 YAML/Python 生成器迁移到 OVS 3.7。使用当前 `Protocol_cfg/header_describe` YAML，生成器可重新构造 OVS 3.7-MOF 数据面源码。

公开仓库：[huangjingxiang-ui/ovs-3.7-mof-complete](https://github.com/huangjingxiang-ui/ovs-3.7-mof-complete)。功能验证详情见 [复验报告](功能复验报告.md)，含 Ubuntu 终端截图、流表和 PCAP 证据。

```bash
git clone https://github.com/huangjingxiang-ui/ovs-3.7-mof-complete.git
cd ovs-3.7-mof-complete
```

2026-11-03 的实测确认主要动作能够通过 DPDK 23.11 PCAP PMD 实际改包，流表可以下发、回读、删除。本轮补齐 `ofproto_rule_create_mof()` 遗漏的 `ofproto` 引用处理，修复删除规则及退出时的断言。详见 `evidence/2026-11-03` 及测试报告。结果适用于本次单机功能范围，不能作为生产稳定性结论。IPv4/IPv6 转换、最长前缀匹配、CRC 和控制器集成不在本轮范围。

## 目录

- `openvswitch-3.7.0-mof`：待生成、编译和运行的 OVS 3.7-MOF 源码。
- `Protocol_cfg`：YAML 协议描述、Python 代码生成器、OVS 3.7 版本化模板及 Java/ONOS 生成结果。
- `ovs-frontend`：原 MOF 工程的前端与复合动作文件。
- `scripts`：OVS 3.7 + DPDK 23.11 编译脚本。

## 生成方法

```bash
cd Protocol_cfg
./configure.sh ../openvswitch-3.7.0-mof
```

成功时最后输出 `Configuration successful!`。也可使用环境变量：

```bash
OVS_SRC=/absolute/path/to/openvswitch-3.7.0-mof ./configure.sh
```

生成器会拒绝把目标设为 `openvswitch-3.7.0 (final)`，该目录只能作为黄金基线进行比较。

## OVS 3.7 迁移要点

- `struct flow_mof` 协议区起始偏移：536 字节。
- OVS 3.7 原生 `struct flow` 尾部偏移：736 字节。
- `FLOW_WC_SEQ`：43。
- OVS 3.7 的 xlate 动作分支与 SRv6 辅助函数位于不同区域，现由 `assemble_ovs37.py` 分槽渲染。
- `include/odp-netlink.h` 和 `lib/meta-flow.inc` 使用 OVS 3.7 版本化模板，不再复制 2.16 文件。

## 一键重构与编译

Ubuntu 中准备好完整的 `openvswitch-3.7.0-mof` 源码，并确保 DPDK 23.11 可被 `pkg-config` 找到后，在工程根目录运行：

```bash
chmod +x build-ovs37-mof.sh
./build-ovs37-mof.sh
```

脚本依次执行环境检查、根据 YAML 重构 MOF 源码、配置 OVS、编译并验证生成的二进制。默认不会停止当前 OVS，也不会安装或覆盖系统文件。

常用选项：

```bash
# 清理后完整重编译
./build-ovs37-mof.sh --clean --jobs 2

# 编译后额外运行 OVS 源码测试套件
./build-ovs37-mof.sh --check

# 编译成功后安装到系统（此步骤才会使用 sudo）
./build-ovs37-mof.sh --install
```

运行 `./build-ovs37-mof.sh --help` 可查看全部选项。已有的 `scripts/init.sh` 和 `scripts/build.sh` 保留用于兼容原有流程。

## Ubuntu 环境准备

示例工作目录为 `/home/ovs/ovs-3.7-mof-complete`，运行命令时按实际位置调整。源码和生成过程使用普通用户；安装、分配大页和运行 DPDK 测试需要 sudo。

```bash
sudo apt-get update
sudo apt-get install -y build-essential autoconf automake libtool pkg-config \
  python3 python3-yaml python3-scapy python3-pyelftools \
  libssl-dev libcap-ng-dev libnuma-dev libpcap-dev meson ninja-build
```

已安装 DPDK 23.11 的机器直接检查：

```bash
export PKG_CONFIG_PATH=/usr/local/lib/x86_64-linux-gnu/pkgconfig:${PKG_CONFIG_PATH:-}
pkg-config --modversion libdpdk
sudo ldconfig
```

预期版本为 `23.11` 或 `23.11.x`。若尚未安装，在 DPDK 23.11 源码目录运行以下步骤；必须在编译 DPDK 前安装 `libpcap-dev`，使 PCAP PMD 可用：

```bash
meson setup build --prefix=/usr/local -Dexamples=all
ninja -C build -j2
sudo ninja -C build install
sudo ldconfig
pkg-config --modversion libdpdk
```

## 从源码到可运行二进制

完整目录应同时包含 `Protocol_cfg`、`ovs-frontend`、`scripts`、`openvswitch-3.7.0-mof`。旧的 `generator-package.tar.gz` 只包含生成器配套文件，不能单独作为完整源码交付物。

```bash
cd /home/ovs/ovs-3.7-mof-complete
# 推荐首次使用清理编译，避免复制目录中残留旧的目标文件。
bash build-ovs37-mof.sh --clean --jobs 2
./openvswitch-3.7.0-mof/vswitchd/ovs-vswitchd --version
```

脚本默认 `--with-dpdk=shared`，与本轮环境一致；静态链接可使用 `DPDK_LINK=static bash build-ovs37-mof.sh --clean --jobs 2`。安装是可选步骤：`bash build-ovs37-mof.sh --install`。功能测试直接使用工程内的二进制，无须先安装，避免调用 PATH 中另一个 OVS。

仅生成和对照源码：

```bash
bash Protocol_cfg/configure.sh "$PWD/openvswitch-3.7.0-mof"
bash scripts/verify_against_final.sh "$PWD/openvswitch-3.7.0-mof" \
  '/home/ovs/openvswitch-3.7.0 (final)'
```

最后一个命令只适用于有 `final` 黄金基线的原虚拟机。它比较生成器负责的 11 个文件，而不是整个源码目录。改变 YAML 后，不再预期与旧基线逐字节一致，需要重新验证布局、解析和改包结果。

## 单机 DPDK 功能验证

拓扑为 `输入 PCAP -> DPDK net_pcap PMD -> OVS netdev MOF -> DPDK net_pcap PMD -> 输出 PCAP`。不绑定 `ens33`，也不需要物理网卡、第二台机器或控制器。此方法覆盖真实 DPDK 收发路径，但不覆盖 VFIO、物理网卡驱动、vhost-user 和性能。

两核、约 4 GiB 内存的虚拟机可先准备 384 个 2 MiB 大页。大页配置重启后通常需要恢复：

```bash
sudo mkdir -p /dev/hugepages
mountpoint -q /dev/hugepages || sudo mount -t hugetlbfs nodev /dev/hugepages
sudo sysctl -w vm.nr_hugepages=384
grep -E 'HugePages_Total|HugePages_Free|Hugepagesize' /proc/meminfo

cd /home/ovs/ovs-3.7-mof-complete
sudo python3 scripts/functional_smoke.py
# 也可单独验证某项
sudo python3 scripts/functional_smoke.py mod_fields
# 在同一实例内连续切换 8 项，回归端口和规则清理
sudo python3 scripts/functional_smoke.py lifecycle
```

脚本默认逐项创建独立数据库和 daemon，`lifecycle` 模式则复用同一实例，打印 `Evidence directory: /tmp/mof-smoke-...`。查看该目录的 `results.json`、`*-flows.txt`、`*.pcap` 和 `vswitchd.log`。`pass_test` 表示改包断言，`delete_pass` 表示规则删除后回读为空，`shutdown_assertion` 表示退出日志是否出现断言；必须一起判断，不能仅看 `PASS`。任何数据面、删除或退出检查失败都会返回非零状态。

输入使用循环回放，以避免有限包在流表准备前耗尽。输出包数随运行时序变化，只用于确认非零输出和逐包字节，不用于测丢包率或性能。端口统计和规则计数存在刷新时差，不能直接要求与 PCAP 数量相同。

```bash
sudo tcpdump -nn -e -r /tmp/mof-smoke-具体目录/mod_fields.pcap | head
sudo cat /tmp/mof-smoke-具体目录/results.json
sudo tail -100 /tmp/mof-smoke-具体目录/vswitchd.log
```

## 自定义流表命令示例

以下命令用于已经运行的 MOF 网桥，入口 OpenFlow 端口号 1、出口端口号 2。`B` 为网桥名；独立测试实例的管理 socket 目录由 `OVS_RUNDIR` 指定。需要保持 daemon 运行时才能执行命令，自动 smoke 脚本结束后会退出其测试实例。

```bash
OFCTL="$PWD/openvswitch-3.7.0-mof/utilities/ovs-ofctl"
B=br-dpdk-mof
sudo "$OFCTL" add-flow-mof "$B" \
 'priority=200,in_port=1,actions=mod_fields(mac,mac_dst=0x0200000000cc),output:2'
sudo "$OFCTL" dump-flows-mof "$B"
sudo "$OFCTL" del-flows-mof "$B"
```

普通 `dump-flows` 不能完整显示 MOF 动作，应使用 `dump-flows-mof`。流表安装成功还要通过输出报文检查动作是否执行。

| 功能 | actions 示例 | 验证标准 |
|---|---|---|
| 改目的 MAC | `mod_fields(mac,mac_dst=0x0200000000cc),output:2` | 输出目的 MAC 为 `02:00:00:00:00:cc` |
| 添加 VLAN | `add_protocol(vlan1,vlan1_tpid=0x8100,vlan1_qid=0x0064),output:2` | 插入 VLAN 100，报文长增加 4 字节 |
| 删除 VLAN | `del_protocol(vlan1),output:2` | 已存在 VLAN 被删除；测试先在 Table 0 添加，再在 Table 1 删除 |
| 移动协议 | `move_protocol(src=vlan1,dst=vlan2),goto_table:1` | Table 1 按 `vlan2_qid=0x0064` 匹配输出；Table 0 先添加 vlan1 |
| 多表跳转 | `mod_fields(mac,mac_dst=0x0200000000cc),goto_table:1` | Table 1 按新 `mac_dst` 匹配并输出 |
| 字节动作 | `add_bytes:0x0200000000cc0200000000aa0800,output:2` | 目的/源 MAC 和 EtherType 前 14 字节与指定内容一致 |
| 丢弃 | `drop` | 有规则命中，输出 PCAP 中没有报文 |

表中使用的协议名和字段名来自当前 YAML。多表完整命令可直接参考 `scripts/functional_smoke.py` 中的 `cases`，以及实测 `results.json` 的 `flows` 字段。

## 常见问题和限制

- `dpdk_initialized=true` 不能单独证明 daemon 仍存活；同时检查进程、管理 socket 和 `dpif/show`。
- `sudo ovs-ctl: 找不到命令`：旧安装布局可使用 `/usr/local/share/openvswitch/scripts/ovs-ctl` 的绝对路径。本项目自动测试直接启动工程内 daemon。
- `libdpdk not found`：检查 `PKG_CONFIG_PATH` 和 `pkg-config --modversion libdpdk`；共享库错误时执行 `sudo ldconfig`。
- `Cannot get hugepage information`：恢复 hugetlbfs 挂载和大页，并确认实际分配数量。
- `net_pcap` 创建失败：检查 DPDK 是否构建 PCAP PMD，以及输入文件存在、端口 `error` 字段和 daemon 日志。
- 如果退出出现引用计数断言、反复端口变更出现表销毁断言：确认运行的是本工程重新编译后的二进制。旧 `final` 基线仍保留原始行为；本轮在 `ofproto_rule_create_mof()` 中补齐 `ofproto_try_ref()` 及分配失败时的 `ofproto_unref()`，修复记录和前后日志在证据目录。

## 测试报告与截图

本轮报告为根目录的 `功能复验报告.md` 和 `功能复验报告.html`。HTML 已内嵌截图，可单独打开查看；Markdown 使用 `evidence/2026-11-03/screenshots` 中的原始截图。截图为 Ubuntu 终端读取本轮保留的结果、流表回读、PCAP 和构建日志；原始日志和 PCAP 同时保留以便复核。

可在虚拟机终端复核证据：

```bash
python3 scripts/show_evidence.py summary
python3 scripts/show_evidence.py flows
python3 scripts/show_evidence.py packets
python3 scripts/show_evidence.py dpdk
```

重新生成报告需要可选依赖 `python3-markdown`：`sudo apt-get install python3-markdown`，然后执行 `python3 scripts/create_validation_report.py`。这不是构建 OVS 所需的依赖。

## 代码许可和发布

OVS 上游许可见 `openvswitch-3.7.0-mof/LICENSE`、`NOTICE` 等原始文件；DPDK 单独安装，不把已编译 DPDK 库或机器凭据放进仓库。MOF 扩展和旧 Python/前端工程保持原始来源与许可声明，未额外授予新许可。发布代码应保留已有版权和许可文件。

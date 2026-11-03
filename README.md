# OVS 3.7-MOF 完整工程

本工程包含 OVS 3.7-MOF 源码、协议 YAML、Python 重构器、前端及编译和功能测试脚本。重构器根据 `Protocol_cfg/header_describe` 中的协议描述生成 MOF 数据面代码，并使用 DPDK 23.11 进行收发和自定义动作处理。

仓库地址：[huangjingxiang-ui/ovs-3.7-mof-complete](https://github.com/huangjingxiang-ui/ovs-3.7-mof-complete)。测试过程和结果见 [功能测试报告](功能复验报告.md)。

## 目录结构

| 目录或文件 | 用途 |
|---|---|
| `openvswitch-3.7.0-mof/` | OVS 3.7-MOF 完整源码 |
| `Protocol_cfg/header_describe/` | 协议 YAML 描述 |
| `Protocol_cfg/PyPrj/` | Python 协议代码生成器 |
| `Protocol_cfg/templates/ovs-3.7/` | OVS 3.7 版本化模板 |
| `ovs-frontend/` | MOF 前端和复合动作配置 |
| `scripts/` | 构建及自动功能测试脚本 |
| `patches/` | 移植修复补丁 |
| `build-ovs37-mof.sh` | 一键重构和编译入口 |
| `功能复验报告.md` | 功能测试报告 |

## 1 获取代码与安装依赖

在 Ubuntu 中执行：

```bash
git clone https://github.com/huangjingxiang-ui/ovs-3.7-mof-complete.git
cd ovs-3.7-mof-complete

sudo apt-get update
sudo apt-get install -y build-essential autoconf automake libtool pkg-config \
  python3 python3-yaml python3-scapy python3-pyelftools \
  libssl-dev libcap-ng-dev libnuma-dev libpcap-dev meson ninja-build
```

后续示例使用 `/home/ovs/ovs-3.7-mof-complete` 作为工程路径，请按实际位置调整。

## 2 准备 DPDK 23.11

已安装 DPDK 的机器先检查：

```bash
export PKG_CONFIG_PATH=/usr/local/lib/x86_64-linux-gnu/pkgconfig:${PKG_CONFIG_PATH:-}
pkg-config --modversion libdpdk
sudo ldconfig
```

预期输出 `23.11` 或 `23.11.x`。尚未安装时，进入 DPDK 23.11 源码目录执行：

```bash
meson setup build --prefix=/usr/local -Dexamples=all
ninja -C build -j2
sudo ninja -C build install
sudo ldconfig
pkg-config --modversion libdpdk
```

编译 DPDK 前应先安装 `libpcap-dev`，自动功能测试需要 PCAP PMD。

## 3 重构并编译 OVS 3.7-MOF

```bash
cd /home/ovs/ovs-3.7-mof-complete
bash build-ovs37-mof.sh --clean --jobs 2
./openvswitch-3.7.0-mof/vswitchd/ovs-vswitchd --version
```

脚本依次检查依赖、运行 YAML 重构器、配置 DPDK、编译 OVS，并校验生成的程序和 MOF 流表命令。默认使用共享 DPDK 库，编译完成后版本信息应包含：

```text
ovs-vswitchd (Open vSwitch) 3.7.0
DPDK 23.11.0
```

常用选项：

```bash
# 常规重构和增量编译
bash build-ovs37-mof.sh --jobs 2

# 编译成功后安装，默认安装前缀为 /usr/local
bash build-ovs37-mof.sh --install

# 额外运行 OVS 源码测试套件
bash build-ovs37-mof.sh --check

# 使用静态 DPDK 库重新配置和编译
DPDK_LINK=static bash build-ovs37-mof.sh --clean --jobs 2
```

只重构源码时，运行：

```bash
bash Protocol_cfg/configure.sh "$PWD/openvswitch-3.7.0-mof"
```

成功后输出 `Configuration successful!`。修改 YAML 后重新执行重构与编译。生成器自动创建中间输出目录，并生成配套 Java 文件。全部编译参数见 `bash build-ovs37-mof.sh --help`。

## 4 准备大页并启动 OVS-DPDK

以下配置用于本次两核、约 4 GiB 内存的虚拟机，384 个 2 MiB 大页共占 768 MiB。重启后重新确认挂载和分配状态。

```bash
sudo mkdir -p /dev/hugepages
mountpoint -q /dev/hugepages || sudo mount -t hugetlbfs nodev /dev/hugepages
sudo sysctl -w vm.nr_hugepages=384
grep -E 'HugePages_Total|HugePages_Free|Hugepagesize' /proc/meminfo
```

安装 OVS 后，执行以下步骤启动 OVSDB 并初始化 DPDK。这组命令会重启当前 OVS：

```bash
sudo /usr/local/share/openvswitch/scripts/ovs-ctl stop
sudo /usr/local/share/openvswitch/scripts/ovs-ctl \
  --no-ovs-vswitchd start --system-id=random

sudo ovs-vsctl --no-wait set Open_vSwitch . \
  other_config:dpdk-init=true \
  other_config:dpdk-lcore-mask=0x1 \
  other_config:pmd-cpu-mask=0x2 \
  other_config:dpdk-socket-mem=128 \
  other_config:dpdk-hugepage-dir=/dev/hugepages \
  other_config:dpdk-extra="--single-file-segments"

sudo /usr/local/share/openvswitch/scripts/ovs-ctl \
  --no-ovsdb-server --db-sock=/usr/local/var/run/openvswitch/db.sock start

sudo ovs-vsctl get Open_vSwitch . dpdk_initialized
sudo ovs-appctl dpif/show
```

`dpdk_initialized` 应为 `true`，同时确认 `ovs-appctl` 能连接运行中的进程。

## 5 自动功能测试

脚本直接使用工程内编译出的二进制，创建独立数据库和 `br-mof-check` 网桥。输入 PCAP 经 DPDK PCAP PMD 进入 OVS，执行 MOF 流表后写入输出 PCAP。

```bash
cd /home/ovs/ovs-3.7-mof-complete

# 逐项测试 8 个功能
sudo python3 scripts/functional_smoke.py

# 单独测试修改目的 MAC
sudo python3 scripts/functional_smoke.py mod_fields

# 在同一实例中连续测试端口和流表的创建、删除
sudo python3 scripts/functional_smoke.py lifecycle
```

支持的单项名称为 `output`、`mod_fields`、`add_protocol`、`del_protocol`、`move_protocol`、`goto_table`、`add_bytes`、`drop`。

脚本打印本次结果目录 `/tmp/mof-smoke-...`，其中包含流表回读、JSON 结果、输出 PCAP 和 daemon 日志。检查 `pass_test`、`delete_pass` 为 `true`，且 `shutdown_assertion` 为 `false`。脚本结束时退出自己的测试实例。

```bash
sudo cat /tmp/mof-smoke-具体目录/results.json
sudo tcpdump -nn -e -r /tmp/mof-smoke-具体目录/mod_fields.pcap | head
sudo tail -100 /tmp/mof-smoke-具体目录/vswitchd.log
```

测试使用循环回放，输出包数随运行时序变化，检查依据是实际输出字节。流表计数、端口统计和 PCAP 分别读取，不要求快照数值相等。

## 6 手工下发流表

以下示例使用已运行的 `br-dpdk-mof`，入口 OpenFlow 端口号为 1、出口为 2。先配置网桥及输入、输出端口，并用 `sudo ovs-ofctl show br-dpdk-mof` 核对端口号。

### 下发命令

```bash
sudo ovs-ofctl add-flow-mof br-dpdk-mof \
  "priority=200,in_port=1,actions=mod_fields(mac,mac_dst=0x0200000000cc),output:2"
```

### 查看流表

```bash
sudo ovs-ofctl dump-flows-mof br-dpdk-mof
```

回显中的 `cookie`、`duration`、`n_packets`、`n_bytes` 是查询结果，不是下发命令的一部分。MOF 动作使用 `dump-flows-mof` 查看。

### 删除流表

```bash
sudo ovs-ofctl del-flows-mof br-dpdk-mof
```

该命令删除此测试网桥中的 MOF 规则。其他动作和多表处理的完整下发命令见 [功能测试报告](功能复验报告.md)。

自动测试使用源码目录中的 `utilities/ovs-ofctl`，命令形式相同；脚本整体已通过 sudo 启动，内部调用不重复添加 sudo。

## 7 常见问题

- `sudo: ovs-ctl: 找不到命令`：使用 `/usr/local/share/openvswitch/scripts/ovs-ctl` 的绝对路径。
- `libdpdk not found`：检查 `PKG_CONFIG_PATH` 和 `pkg-config --modversion libdpdk`；共享库加载失败时执行 `sudo ldconfig`。
- `Cannot get hugepage information`：检查 hugetlbfs 挂载及实际分配的大页数量。
- PCAP 端口创建失败：确认 DPDK 已编译 PCAP PMD，输入文件存在，并查看 Interface 的 `error` 字段和 daemon 日志。
- 权限不足：管理通过 root 启动的 OVS 时使用 sudo。

## 8 移植实现与许可

OVS 3.7 的协议区布局使用 536/736 字节偏移和 `FLOW_WC_SEQ=43`。重构器通过版本化模板写入数据面源码，并同步生成 Java 序列化代码。`ofproto_rule_create_mof()` 中补齐了规则持有的 `ofproto` 引用，修复删除规则及退出时的断言；补丁见 `patches/ovs37-mof-rule-refcount.patch`。

OVS 上游许可见 `openvswitch-3.7.0-mof/LICENSE`、`NOTICE` 等原始文件。MOF 扩展及 Python、前端工程保留原有版权和许可声明。

#!/usr/bin/env python3
"""Isolated, real DPDK PCAP smoke tests; run with sudo python3, no NIC binding."""
import json, os, pathlib, subprocess, time, tempfile, re, sys
from scapy.all import Ether, IP, UDP, Raw, Dot1Q, wrpcap, rdpcap

if len(sys.argv)==1:
    codes=[subprocess.call([sys.executable,__file__,name]) for name in ('output','mod_fields','add_protocol','del_protocol','move_protocol','goto_table','add_bytes','drop')]
    sys.exit(any(codes))
lifecycle_mode=sys.argv[1]=='lifecycle'

ROOT=pathlib.Path(__file__).resolve().parents[1]
SRC=ROOT/'openvswitch-3.7.0-mof'
OUT=pathlib.Path(tempfile.mkdtemp(prefix='mof-smoke-',dir='/tmp'))
os.environ['OVS_RUNDIR']=str(OUT)
os.environ['OVS_LOGDIR']=str(OUT)
BR='br-mof-check'
def run(*args,check=True):
    p=subprocess.run([str(x) for x in args],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
    if check and p.returncode: raise RuntimeError(f'{args}: {p.stdout}')
    return p.stdout
def vs(*args): return run(SRC/'utilities/ovs-vsctl','--db=unix:'+str(OUT/'db.sock'),*args)
def of(*args): return run(SRC/'utilities/ovs-ofctl',*args)
def app(*args): return run(SRC/'utilities/ovs-appctl','-t',OUT/'vswitchd.ctl',*args)
print('Evidence directory:',OUT,flush=True)
wrpcap(str(OUT/'input.pcap'),[Ether(dst='00:11:22:33:44:55',src='02:00:00:00:00:0a')/IP(src='192.0.2.1',dst='198.51.100.1')/UDP(sport=12345,dport=23456)/Raw(b'MOF-functional-test-payload')]*100)
results=[]
try:
    run(SRC/'ovsdb/ovsdb-tool','create',OUT/'conf.db',SRC/'vswitchd/vswitch.ovsschema')
    run(SRC/'ovsdb/ovsdb-server',OUT/'conf.db','--remote=punix:'+str(OUT/'db.sock'),'--pidfile='+str(OUT/'db.pid'),'--unixctl='+str(OUT/'db.ctl'),'--detach','--log-file='+str(OUT/'db.log'))
    vs('--no-wait','init')
    vs('--no-wait','set','Open_vSwitch','.','other_config:dpdk-init=true','other_config:dpdk-lcore-mask=0x1','other_config:pmd-cpu-mask=0x2','other_config:dpdk-socket-mem=128','other_config:dpdk-hugepage-dir=/dev/hugepages',f'other_config:dpdk-extra=--no-pci --single-file-segments --file-prefix={OUT.name}')
    run(SRC/'vswitchd/ovs-vswitchd','unix:'+str(OUT/'db.sock'),'--pidfile='+str(OUT/'vswitchd.pid'),'--unixctl='+str(OUT/'vswitchd.ctl'),'--detach','--log-file='+str(OUT/'vswitchd.log'))
    print(run(SRC/'vswitchd/ovs-vswitchd','--version'),flush=True)
    print('dpdk_initialized:',vs('get','Open_vSwitch','.','dpdk_initialized'),flush=True)
    vs('add-br',BR,'--','set','Bridge',BR,'datapath_type=netdev','fail_mode=secure')
    cases=[
      ('output',['priority=200,in_port=1,actions=output:2'],lambda p:bytes(p)==bytes(rdpcap(str(OUT/'input.pcap'))[0])),
      ('mod_fields',['priority=200,in_port=1,actions=mod_fields(mac,mac_dst=0x0200000000cc),output:2'],lambda p:p.dst=='02:00:00:00:00:cc'),
      ('add_protocol',['priority=200,in_port=1,actions=add_protocol(vlan1,vlan1_tpid=0x8100,vlan1_qid=0x0064),output:2'],lambda p:Dot1Q in p and p[Dot1Q].vlan==100),
      ('del_protocol',['priority=200,in_port=1,actions=add_protocol(vlan1,vlan1_tpid=0x8100,vlan1_qid=0x0064),goto_table:1','table=1,priority=200,in_port=1,actions=del_protocol(vlan1),output:2'],lambda p:Dot1Q not in p and bytes(p)==bytes(rdpcap(str(OUT/'input.pcap'))[0])),
      ('move_protocol',['priority=200,in_port=1,actions=add_protocol(vlan1,vlan1_tpid=0x8100,vlan1_qid=0x0064),move_protocol(src=vlan1,dst=vlan2),goto_table:1','table=1,priority=200,vlan2_qid=0x0064,actions=output:2'],lambda p:Dot1Q in p and p[Dot1Q].vlan==100),
      ('goto_table',['priority=200,in_port=1,actions=mod_fields(mac,mac_dst=0x0200000000cc),goto_table:1','table=1,priority=200,mac_dst=0x0200000000cc,actions=output:2'],lambda p:p.dst=='02:00:00:00:00:cc'),
      ('add_bytes',['priority=200,in_port=1,actions=add_bytes:0x0200000000cc0200000000aa0800,output:2'],lambda p:p.dst=='02:00:00:00:00:cc' and p.src=='02:00:00:00:00:aa' and p.type==0x800),
      ('drop',['priority=200,in_port=1,actions=drop'],lambda p:False),
    ]
    for index,(name,flows,verify) in enumerate(cases):
        if not lifecycle_mode and name!=sys.argv[1]: continue
        of('del-flows-mof',BR)
        capture=OUT/(name+'.pcap')
        vs('add-port',BR,'pcap-out','--','set','Interface','pcap-out','type=dpdk',f'options:dpdk-devargs=net_pcap{index*2},tx_pcap={capture}','ofport_request=2')
        vs('add-port',BR,'pcap-in','--','set','Interface','pcap-in','type=dpdk',f'options:dpdk-devargs=net_pcap{index*2+1},rx_pcap={OUT}/input.pcap,infinite_rx=1','ofport_request=1')
        for flow in flows: of('add-flow-mof',BR,flow)
        time.sleep(.08)
        stats=of('dump-flows-mof',BR)
        port=vs('get','Interface','pcap-in','statistics')
        datapath=app('dpif/show')
        print(name,stats,port,flush=True)
        (OUT/(name+'-flows.txt')).write_text(stats+'\n'+port)
        of('del-flows-mof',BR)
        after_delete=of('dump-flows-mof',BR)
        print('DELETE',name,repr(after_delete),flush=True)
        print(datapath,flush=True)
        # Infinite replay avoids the one-shot PCAP startup race. Stop the
        # daemon after observing the hit to flush the capture before reading.
        if lifecycle_mode:
            vs('--if-exists','del-port',BR,'pcap-in')
            vs('--if-exists','del-port',BR,'pcap-out')
        else:
            run(SRC/'utilities/ovs-appctl','-t',OUT/'vswitchd.ctl','exit',check=False)
        time.sleep(.2)
        packets=rdpcap(str(capture)) if capture.exists() and capture.stat().st_size>24 else []
        good=sum(bool(verify(p)) for p in packets) if name!='drop' else 0
        hit=any(int(x)>0 for x in re.findall(r'n_packets=(\d+)',stats))
        passed=hit and (len(packets)==0 if name=='drop' else good==len(packets) and good>0)
        results.append(dict(name=name,pass_test=passed,packets=len(packets),verified_packets=good,flows=flows,first_hex=bytes(packets[0]).hex() if packets else '',flow_dump=stats,delete_pass=not after_delete.strip(),datapath=datapath,shutdown_assertion='assertion' in (OUT/'vswitchd.log').read_text()))
        print('RESULT',name,'PASS' if passed else 'FAIL','packets=',len(packets),'verified=',good,flush=True)
    (OUT/'results.json').write_text(json.dumps(results,indent=2))
finally:
    run(SRC/'utilities/ovs-appctl','-t',OUT/'vswitchd.ctl','exit',check=False)
    run(SRC/'utilities/ovs-appctl','-t',OUT/'db.ctl','exit',check=False)
time.sleep(.2)
for r in results: r['shutdown_assertion']='assertion' in (OUT/'vswitchd.log').read_text()
(OUT/'results.json').write_text(json.dumps(results,indent=2))
print('SUMMARY',sum(r['pass_test'] for r in results),'/',len(results),flush=True)
sys.exit(0 if results and all(r['pass_test'] and r['delete_pass'] and not r['shutdown_assertion'] for r in results) else 1)

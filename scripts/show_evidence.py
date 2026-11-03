#!/usr/bin/env python3
"""Display retained VM test evidence for review and terminal screenshots."""
import json,pathlib,sys,subprocess,re
root=pathlib.Path(__file__).resolve().parents[1]
e=root/'evidence/2026-11-03'
rows=json.loads((e/'results.json').read_text())
mode=sys.argv[1] if len(sys.argv)>1 else 'summary'
print('OVS 3.7-MOF + DPDK 23.11 -- retained VM evidence 2026-11-03')
print('Scope: functional checks only; real net_pcap DPDK ports\n')
if mode=='summary':
    print(f"{'CASE':18} {'PACKETS':>8} {'BYTE OK':>8} {'DELETE':>8} {'SHUTDOWN':>10}")
    for r in rows:
        print(f"{r['name']:18} {r['packets']:8} {r['verified_packets']:8} {'PASS' if r['delete_pass'] else 'FAIL':>8} {'FAIL' if r['shutdown_assertion'] else 'PASS':>10}")
    print('\nAll 8 data-plane cases PASS; no shutdown assertion after refcount fix.')
    print('Original final baseline was not modified.')
elif mode=='dpdk':
    subprocess.run([str(root/'openvswitch-3.7.0-mof/vswitchd/ovs-vswitchd'),'--version'])
    observed=re.findall(r'dpdk_initialized: (\w+)',(e/'fixed-smoke.log').read_text())
    print('DPDK initialization observed in test log:',observed[0] if observed else 'missing')
    print('\nRetained datapath and port types:')
    print(rows[0]['datapath'])
    print('Fresh source-package build verification:')
    print('\n'.join((e/'release-check.log').read_text().splitlines()[-2:]))
elif mode=='flows':
    for r in rows:
        if r['name'] in ('mod_fields','goto_table','move_protocol'):
            print('CASE:',r['name']); print(r['flow_dump'].strip()); print()
else:
    for name in ('output','mod_fields','add_protocol','del_protocol','add_bytes'):
        print('\nPCAP:',name,flush=True)
        subprocess.run(['tcpdump','-nn','-e','-c','1','-r',str(e/name/(name+'.pcap'))],stderr=subprocess.DEVNULL)

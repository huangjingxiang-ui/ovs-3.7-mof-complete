curl --location --globoff 'http://127.0.0.1:8181/onos/v1/flowobjectives/of:0000000000000022/forward?appId=org.onosproject.openflow' \
--header 'Content-Type: application/json' \
--header 'Authorization: Basic b25vczpyb2Nrcw==' \
--data '{
    "flag": "SPECIFIC",
    "priority": 40000, 
    "timeout": 0, 
    "isPermanent": true, 
    "deviceId": "of:0000000000000022", 
    "tableId": 0,
    "operation": "ADD", 
    "selector": { 
        "criteria": [
            {
                "type": "PDXP_VERSION",
                "pdxp_version": "99/FF" 
            },
            {
                "type": "PDXP_DEST_ADDR",
                "pdxp_dest_addr": "55667788/FFFFFFFF"
            }
        ]
    },
    "treatment": { 
        "instructions": [
            {
                "type": "OUTPUT", 
                "port": 2
            }
        ]
    }
}'

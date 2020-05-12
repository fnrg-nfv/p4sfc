{
    "chain_desc": [
        {
            "name": "Monitor",
            "click_config": {
                "param1": "abc"    
            },
            "location": "s1"
        },
        {
            "name": "Firewall",
            "click_config": {
                "param1": "abc"    
            },
            "location": "s2"
        },
        {
            "name": "IPRewriter",
            "click_file_name": "nat-p4-encap",
            "click_config": {
                "param1": "abc"    
            },
            "location": "s3"
        }
    ],
    "route": [
        "ingress",
        "s1",
        "s2",
        "s3",
        "egress"
    ]
}
max(1589247626519, 1589247626500, 1589274523663) - 1589247626317
max(1589248085043, 1589248085022, 1589274982193) - 1589248084874

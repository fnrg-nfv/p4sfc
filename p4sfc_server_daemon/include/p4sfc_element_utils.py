class IPRewriter(object):

    @staticmethod
    def build_IpRewriter_exact_entry(p4info_helper, entry_info):
        if 'change_src_addr_and_port' in entry_info['action_name']:
            table_entry = p4info_helper.buildTableEntry(
                table_name= entry_info['table_name'],
                match_fields={
                    "hdr.sfc.chainId": entry_info['chain_id'],
                    "meta.stageId": entry_info['stage_id'],
                    "hdr.ipv4.srcAddr": entry_info["src_addr"],
                    "hdr.ipv4.dstAddr": entry_info["dst_addr"],
                    "hdr.ipv4.protocol": entry_info["protocol"],
                    "hdr.tcp_udp.srcPort": entry_info["src_port"],
                    "hdr.tcp_udp.dstPort": entry_info["dst_port"],
                },
                action_name= entry_info['action_name'],
                action_params={
                    "srcAddr": entry_info["action_params_src_addr"],
                    "srcPort": entry_info["action_params_src_port"],
                }
            )
            return table_entry
        else:
            table_entry = p4info_helper.buildTableEntry(
                table_name= entry_info['table_name'],
                match_fields={
                    "hdr.sfc.chainId": entry_info['chain_id'],
                    "meta.stageId": entry_info['stage_id'],
                    "hdr.ipv4.srcAddr": entry_info["src_addr"],
                    "hdr.ipv4.dstAddr": entry_info["dst_addr"],
                    "hdr.ipv4.protocol": entry_info["protocol"],
                    "hdr.tcp_udp.srcPort": entry_info["src_port"],
                    "hdr.tcp_udp.dstPort": entry_info["dst_port"],
                },
                action_name= entry_info['action_name'],
                action_params={
                    "dstAddr": entry_info["action_params_dst_addr"],
                    "dstPort": entry_info["action_params_dst_port"],
                }
            )
            return table_entry

class Element(object):
    pass


class IPRewriter(Element):

    @staticmethod
    def get_element_name():
        return "ipRewriter"

    @staticmethod
    def build_new_entry(p4info_helper, entry_info):
        match_fields = entry_info['match_fields']
        action_params = entry_info['action_params']
        if 'change_src_addr_and_port' in entry_info['action_name']:
            table_entry = p4info_helper.buildTableEntry(
                table_name=entry_info['table_name'],
                match_fields={
                    "hdr.sfc.chainId": entry_info['chain_id'],
                    "meta.stageId": entry_info['stage_id'],
                    "hdr.ipv4.srcAddr": match_fields["src_addr"],
                    "hdr.ipv4.dstAddr": match_fields["dst_addr"],
                    "hdr.ipv4.protocol": match_fields["protocol"],
                    "hdr.tcp_udp.srcPort": match_fields["src_port"],
                    "hdr.tcp_udp.dstPort": match_fields["dst_port"],
                },
                action_name=entry_info['action_name'],
                action_params={
                    "srcAddr": action_params["src_addr"],
                    "srcPort": action_params["src_port"],
                }
            )
            return table_entry
        else:
            table_entry = p4info_helper.buildTableEntry(
                table_name=entry_info['table_name'],
                match_fields={
                    "hdr.sfc.chainId": entry_info['chain_id'],
                    "meta.stageId": entry_info['stage_id'],
                    "hdr.ipv4.srcAddr": match_fields["src_addr"],
                    "hdr.ipv4.dstAddr": match_fields["dst_addr"],
                    "hdr.ipv4.protocol": match_fields["protocol"],
                    "hdr.tcp_udp.srcPort": match_fields["src_port"],
                    "hdr.tcp_udp.dstPort": match_fields["dst_port"],
                },
                action_name=entry_info['action_name'],
                action_params={
                    "dstAddr": action_params["action_params_dst_addr"],
                    "dstPort": action_params["action_params_dst_port"],
                }
            )
            return table_entry


class Monitor(Element):
    @staticmethod
    def get_element_name():
        return 'monitor'

    @staticmethod
    def build_new_entry(p4info_helper, entry_info):
        table_entry = p4info_helper.buildTableEntry(
            table_name=entry_info['table_name'],
            match_fields={
                "hdr.sfc.chainId": entry_info['chain_id'],
                "meta.stageId": entry_info['stage_id'],
            },
            action_name=entry_info['action_name'],
        )
        return table_entry

# class Firewall(Element):
#     @staticmethod
#     def get_element_name():
#         return 'firewall'

#     @staticmethod
#     def build_new_entry(p4info_helper, entry_info):
#         table_entry = p4info_helper.buildTableEntry(
#             table_name=entry_info['table_name'],
#             match_fields={
#                 "hdr.sfc.chainId": entry_info['chain_id'],
#                 "meta.stageId": entry_info['stage_id'],
#             },
#             action_name=entry_info['action_name'],
#         )
#         return table_entry

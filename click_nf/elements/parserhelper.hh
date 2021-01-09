#ifndef PARSERHELPER_HH
#define PARSERHELPER_HH

#include <stdint.h>
#include <click/glue.hh>
#include <click/string.hh>
#include <click/ipaddress.hh>
#include <click/args.hh>

struct SingleAddress
{
    // these are all in network order
    uint32_t ip;
    uint32_t ip_mask;
    uint16_t port;
    uint16_t port_mask;

    static inline SingleAddress parse(String &word)
    {
        SingleAddress ret;
        ret.port = 0;
        ret.port_mask = 0;

        if (word == "-")
        {
            ret.ip = 0;
            ret.ip_mask = 0;
        }
        else
        {
            int middle_pos = word.find_left(":");

            String ip_str;
            String port_str = nullptr;
            if (middle_pos == -1)
                ip_str = word;
            else
            {
                ip_str = word.substring(0, middle_pos);
                port_str = word.substring(middle_pos + 1);
            }

            IPAddress ip;
            IPAddress mask;
            bool result;
            if (ip_str.find_left("/") != -1)
            {
                result = IPPrefixArg().parse(ip_str, ip, mask);
                ip &= mask;
            }
            else
            {
                result = IPAddressArg().parse(ip_str, ip);
                mask = IPAddress::make_broadcast();
            }
#ifdef DEBUG
            click_chatter("%d %.*s => %x %x", result, ip_str.length(), ip_str.data(), ip.addr(), mask.addr());
#endif

            ret.ip = ip.addr();
            ret.ip_mask = mask.addr();

            if (port_str)
            {
                int port;
                if (IntArg().parse(port_str, port))
                {
                    ret.port = htons(port);
                    ret.port_mask = 0xffff;
                }
            }
        }
        return ret;
    }
};

#endif
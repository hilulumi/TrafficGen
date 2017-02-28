/*
 * Flow.cpp
 *
 *  Created on: Feb 26, 2017
 *      Author: root
 */

#include "Flow.hpp"
#include <cstring>
#include <arpa/inet.h>


namespace Traffic{

uint16_t checksum (uint16_t *addr, int len)
{
  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }


  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}

uint16_t tcpudp4_checksum(IPV4_h *IP, uint16_t* buf, uint16_t len, uint8_t protocol)
{
	uint32_t sum = 0;

	sum += (IP->saddr >> 16) & 0xffff;
	sum += (IP->saddr) & 0xffff;
	sum += (IP->daddr >> 16) & 0xffff;
	sum += (IP->daddr) & 0xffff;
	sum += htons(protocol);
	sum += htons(len);

	while(len > 1){
		sum += *(buf++);
		len -=2;
	}

	if(len > 0){
		sum += *((uint8_t*)(buf));
	}

	while(sum >> 16){
		sum = (sum & 0xffff) + (sum >> 16);
	}

	sum = ~sum;
	if(protocol == IPPROTO_UDP)
		return ((uint16_t)sum == 0x0000) ? 0xffff : ((uint16_t)sum);
	else
		return ((uint16_t)sum);
}

uint16_t tcpudp6_checksum(IPV6_h* IP, uint16_t* buf, uint16_t len, uint8_t protocol){
	uint32_t sum = 0;

	for(int i=0; i<8; i++)
		sum += IP->ip6_src.s6_addr16[i];
	for(int i=0; i<8; i++)
			sum += IP->ip6_dst.s6_addr16[i];

	uint32_t L = len;
	sum += (L >> 16) & 0xffff;
	sum += (L & 0xffff);
	sum += htons(protocol);

	while(len > 1){
		sum += *(buf++);
		len -=2;
	}

	if(len > 0){
		sum += *((uint8_t*)(buf));
	}

	while(sum >> 16){
		sum = (sum & 0xffff) + (sum >> 16);
	}

	sum = ~sum;
	if(protocol == IPPROTO_UDP)
		return ((uint16_t)sum == 0x0000) ? 0xffff : ((uint16_t)sum);
	else
		return ((uint16_t)sum);
}

Raw_Packet::Raw_Packet(ETHER_h& header, size_t len)
{
	pkt_len = len;
	frame = new uint8_t[len+ETHERLEN];
	ether = frame;
	std::memcpy(ether, &header, len);
	cur = ether + ETHERLEN;
}

IPV4_t::IPV4_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
		Raw_Packet(header, len)
{
	ip = cur;
	ip->saddr = src.ip.s_addr;
	ip->daddr = dst.ip.s_addr;
	ip->ihl = IPV4LEN / sizeof(uint32_t);
	ip->version = 4;
	ip->tos = 0;
	ip->tot_len = htons(len);
	ip->id = htons(0);
	ip->frag_off = 0;
	ip->ttl = 255;
	//ip->protocol = IPPROTO_TCP; decide at next layer
	ip->check = 0;
	//ip->check = checksum((uint16_t*)ip, IPV4LEN);
	cur = ip + IPV4LEN;
}

bool IPV4_t::isIP4()
{
	return true;
}

void IPV4_t::reset_next_protocol(u_int8_t protocol)
{
	ip->protocol = IPPROTO_TCP;
}

void IPV4_t::reset_pkt(int start_id, int sent_pkt, int flow_length=0, unsigned long sent_byte=0){
	ip->id = htons(start_id + sent_pkt);
	ip->check = 0;
	ip->check = checksum((uint16_t*)ip, IPV4LEN);
}

IPV6_t::IPV6_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
		Raw_Packet(header, len)
{
	ip = cur;
	ip->ip6_flow = htonl ((6 << 28) | (0 << 20) | 0);
	//p->ip6_nxt = IPPROTO_TCP; decide at next layer
	ip->ip6_hops = 255;
	ip->ip6_plen = htons(len - ETHERLEN - IPV6LEN);
	ip->ip6_src.s6_addr16[0] = 0;
	ip->ip6_src.s6_addr16[1] = 0;
	ip->ip6_src.s6_addr16[2] = 0;
	ip->ip6_src.s6_addr16[3] = 0;
	ip->ip6_src.s6_addr16[4] = 0;
	ip->ip6_src.s6_addr16[5] = htons(65535);
	ip->ip6_src.s6_addr16[6] = htons(ntohl(src.ip.s_addr) >> 16);
	ip->ip6_src.s6_addr16[7] = htons(ntohl(src.ip.s_addr) & 65535);
	ip->ip6_dst.s6_addr16[0] = 0;
	ip->ip6_dst.s6_addr16[1] = 0;
	ip->ip6_dst.s6_addr16[2] = 0;
	ip->ip6_dst.s6_addr16[3] = 0;
	ip->ip6_dst.s6_addr16[4] = 0;
	ip->ip6_dst.s6_addr16[5] = htons(65535);
	ip->ip6_dst.s6_addr16[6] = htons(ntohl(dst.ip.s_addr) >> 16);
	ip->ip6_dst.s6_addr16[7] = htons(ntohl(dst.ip.s_addr) & 65535);
	cur = ip + IPV6LEN;
}

bool IPV6_t::isIP4()
{
	return false;
}

void IPV6_t::reset_next_protocol(u_int8_t protocol)
{
	ip->ip6_nxt = IPPROTO_TCP;
}

void IPV6_t::reset_pkt(int start_id, int sent_pkt, int flow_length=0, unsigned long sent_byte=0){
	uint16_t id = (uint16_t)start_id + (uint16_t)sent_pkt;

	ip->ip6_flow = htonl ((6 << 28) | (id << 20) | 0);
}

template<class T>
TCP_t<T>::TCP_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
	T(src, dst, header, len)
{
	tcp = cur;
	T::reset_next_protocal(IPPROTO_TCP);
	tcp->th_sport = htons(src.port);
	tcp->th_dport = htons(dst.port);
	tcp->th_seq = htonl(0);
	tcp->th_ack = htonl(0);
	tcp->th_x2 = 0;
	tcp->th_off = TCPLEN / 4;
	tcp->th_flags = TH_ACK;
	tcp->th_win = htons(65535);
	tcp->th_urp = htons(0);
	tcp->th_sum = 0;
	cur = tcp + TCPLEN;
	// Move checksum to reset_pkt
	/*if(T::isIP4()){
		tcp->th_sum = tcpudp4_checksum(T::ip, (uint16_t*)tcp, len - IPV4LEN, IPPROTO_TCP);
	}
	else{

		tcp->th_sum = tcpudp6_checksum(T::ip, (uint16_t*)tcp, len - IPV6LEN, IPPROTO_TCP);
	}*/
}

template<class T>
void TCP_t<T>::reset_pkt(int start_id, int sent_pkt, int flow_length, unsigned long sent_byte){
	T::reset_pkt(start_id, sent_pkt);
	if((sent_pkt > 0) && (sent_pkt < flow_length)){
		tcp->th_seq = htonl(start_id + sent_byte);
		tcp->th_ack = htonl(start_id + sent_byte + 500);
	}
	else if(sent_pkt == 0){
		tcp->th_flags = TH_SYN | TH_ACK;
		tcp->th_seq = htonl(start_id + sent_byte);
		tcp->th_ack = htonl(start_id + sent_byte + 500);
	}
	else if(sent_pkt==flow_length-1){
		tcp->th_flags = TH_FIN | TH_ACK;
		tcp->th_ack = htonl(start_id + sent_byte + 500);
	}
	if(T::isIP4()){
			tcp->th_sum = tcpudp4_checksum(T::ip, (uint16_t*)tcp, T::pkt_len - IPV4LEN, IPPROTO_TCP);
	}
	else{

		tcp->th_sum = tcpudp6_checksum(T::ip, (uint16_t*)tcp, T::pkt_len - IPV6LEN, IPPROTO_TCP);
	}

}

template<class T>
UDP_t<T>::UDP_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
			T(src, dst, header, len)
{
	udp = cur;
	T::reset_next_protocal(IPPROTO_UDP);
	udp->source = htons(src.port);
	udp->dest = htons(dst.port);
	udp->check = 0;
	/*if(T::isIP4()){
		udp->len = len - IPV4LEN;
		udp->check = tcpudp4_checksum(T::ip, (uint16_t*)udp, udp->len, IPPROTO_UDP);
	}
	else{
		udp->len = len - IPV6LEN;
		udp->check = tcpudp6_checksum(T::ip, (uint16_t*)udp, udp->len, IPPROTO_UDP);
	}*/
	cur = udp + UDPLEN;
}

template<class T>
void UDP_t<T>::reset_pkt(int start_id, int sent_pkt, int flow_length=0, unsigned long sent_byte=0){
	if(T::isIP4()){
			udp->len = len - IPV4LEN;
			udp->check = tcpudp4_checksum(T::ip, (uint16_t*)udp, udp->len, IPPROTO_UDP);
	}
	else{
		udp->len = len - IPV6LEN;
		udp->check = tcpudp6_checksum(T::ip, (uint16_t*)udp, udp->len, IPPROTO_UDP);
	}
}


ICMP_t::ICMP_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
		IPV4_t(src, dst, header, len)
{
	icmp = cur;
	icmp->type = ICMP_ECHO;
	icmp->code = 0;
	icmp->un.echo.id = htons(0);
	icmp->un.echo.sequence = htons(0);
	icmp->checksum = 0;
	//icmp->checksum = checksum((uint16_t*)icmp, len - IPV4LEN);
	cur = icmp + ICMPLEN;
}

void ICMP_t::reset_pkt(int start_id, int sent_pkt, int flow_length=0, unsigned long sent_byte=0){
	icmp->un.echo.id = htons(start_id);
	icmp->un.echo.sequence = htons(sent_pkt);
	icmp->checksum = checksum((uint16_t*)icmp, pkt_len - IPV4LEN);
}


ICMP6_t::ICMP6_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
		IPV6_t(src, dst, header, len)
{
	icmp = cur;
	icmp->icmp6_type = ICMP6_ECHO_REQUEST;
	icmp->icmp6_code = 0;
	icmp->icmp6_id = htons(1000);
	icmp->icmp6_seq = htons(0);
	icmp->icmp6_cksum = 0;
	//icmp->icmp6_cksum = tcpudp6_checksum(ip, (uint16_t*)icmp, (uint16_t)(len - IPV6LEN), IPPROTO_ICMP);
	cur = icmp + ICMP6LEN;
}

void ICMP6_t::reset_pkt(int start_id, int sent_pkt, int flow_length=0, unsigned long sent_byte=0){
	icmp->icmp6_id = htons(start_id);
	icmp->icmp6_seq = htons(sent_pkt);
	icmp->icmp6_cksum = tcpudp6_checksum(ip, (uint16_t*)icmp, (uint16_t)(pkt_len - IPV6LEN), IPPROTO_ICMP);
}

}

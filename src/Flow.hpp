/*
 * Flow.hpp
 *
 *  Created on: Feb 21, 2017
 *      Author: ubuntu14
 */

#ifndef SRC_FLOW_HPP_
#define SRC_FLOW_HPP_

#include "Host_IP.hpp"
#include <cstdint>
#include <time.h>
#include <random>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
namespace Traffic{

typedef struct ether_header	ETHER_h;
typedef struct iphdr		IPV4_h;
typedef struct ip6_hdr		IPV6_h;
typedef struct tcphdr		TCP_h;
typedef struct udphdr		UDP_h;
typedef struct icmphdr		ICMP_h;
typedef struct icmp6_hdr	ICMP6_h;

const size_t ETHERLEN = sizeof(ETHER_h);
const size_t IPV4LEN = sizeof(IPV4_h);
const size_t IPV6LEN = sizeof(IPV6_h);
const size_t TCPLEN = sizeof(TCP_h);
const size_t UDPLEN = sizeof(UDP_h);
const size_t ICMPLEN = sizeof(ICMP_h);
const size_t ICMP6LEN = sizeof(ICMP6_h);

enum class Protocol {TCP, UDP, ICMP, ICMP6};


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

uint16_t tcpudp4_checksum(uint8_t* ipheader, uint16_t* buf, uint16_t len, uint8_t protocol)
{
	uint32_t sum = 0;
	IPV4_h *IP = (IPV4_h*)ipheader;

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

uint16_t tcpudp6_checksum(uint8_t* ipheader, uint16_t* buf, uint16_t len, uint8_t protocol){
	uint32_t sum = 0;
	IPV6_h* IP = (IPV6_h*)ipheader;

	for(int i=0; i<8; i++)
		sum += IP->ip6_src.s6_addr16[i];
	for(int i=0; i<8; i++)
			sum += IP->ip6_dst.s6_addr16[i];

	sum += htons(len);
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


class Raw_Packet{
protected:
	ETHER_h* ether;
	uint8_t* frame;
	uint8_t* cur;
	size_t pkt_len;
public:
	Raw_Packet(ETHER_h& header, size_t len){
		pkt_len = len;
		frame = new uint8_t[len+ETHERLEN];
		ether = (ETHER_h*)frame;
		std::memcpy(ether, &header, len);
		cur = (uint8_t*)ether + ETHERLEN;
	}
	virtual ~Raw_Packet(){
		ether = NULL;
		cur = NULL;
		delete [] frame;
	}
	virtual bool isIP4(){
		return true;
	}
	virtual void reset_next_protocol(u_int8_t)	{}
	virtual void reset_pkt(int a, int b, int c, unsigned long d)	{}
	size_t getLen()	{
		return pkt_len;
	}
	uint8_t* getframe()	{
		return frame;
	}
};

class IPV4_t : public Raw_Packet{
protected:
	IPV4_h* ip;
public:
	IPV4_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
		Raw_Packet(header, len)
	{
		ip = (IPV4_h*)cur;
		ip->saddr = src.getaddr();
		ip->daddr = dst.getaddr();
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
		cur = (uint8_t*)ip + IPV4LEN;
	}
	virtual ~IPV4_t(){
		ip = NULL;
	}
	bool isIP4(){
		return true;
	}
	void reset_next_protocol(u_int8_t protocol){
		ip->protocol = protocol;
	}
	virtual void reset_pkt(int start_id, int sent_pkt, int flow_length=0, unsigned long sent_byte=0){
		//std::cout << ip->saddr <<std::endl;
		ip->id = htons(start_id + sent_pkt);
		ip->check = 0;
		ip->check = checksum((uint16_t*)ip, IPV4LEN);
		//std::cout << ip->saddr <<std::endl;
	}
};

class IPV6_t : public Raw_Packet{
protected:
	IPV6_h* ip;
public:
	IPV6_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
		Raw_Packet(header, len)
	{
		ip = (IPV6_h*)cur;
		ip->ip6_flow = htonl ((6 << 28) | (0 << 20) | 0);
		//p->ip6_nxt = IPPROTO_TCP; decide at next layer
		ip->ip6_hops = 255;
		ip->ip6_plen = htons(len - IPV6LEN);
		ip->ip6_src.s6_addr16[0] = 0;
		ip->ip6_src.s6_addr16[1] = 0;
		ip->ip6_src.s6_addr16[2] = 0;
		ip->ip6_src.s6_addr16[3] = 0;
		ip->ip6_src.s6_addr16[4] = 0;
		ip->ip6_src.s6_addr16[5] = htons(65535);
		ip->ip6_src.s6_addr16[6] = htons(ntohl(src.getaddr()) >> 16);
		ip->ip6_src.s6_addr16[7] = htons(ntohl(src.getaddr()) & 0xffff);
		ip->ip6_dst.s6_addr16[1] = 0;
		ip->ip6_dst.s6_addr16[2] = 0;
		ip->ip6_dst.s6_addr16[3] = 0;
		ip->ip6_dst.s6_addr16[4] = 0;
		ip->ip6_dst.s6_addr16[5] = htons(65535);
		ip->ip6_dst.s6_addr16[6] = htons(ntohl(dst.getaddr()) >> 16);
		ip->ip6_dst.s6_addr16[7] = htons(ntohl(dst.getaddr()) & 0xffff);
		cur = (uint8_t*)ip + IPV6LEN;
	}
	virtual ~IPV6_t(){
		ip = NULL;
	}
	bool isIP4(){
		return false;
	}
	void reset_next_protocol(u_int8_t protocol){
		ip->ip6_nxt = protocol;
	}
	virtual void reset_pkt(int start_id, int sent_pkt, int flow_length=0, unsigned long sent_byte=0){
		uint16_t id = (uint16_t)start_id + (uint16_t)sent_pkt;
		ip->ip6_flow = htonl ((6 << 28) | (id << 20) | 0);
	}
};


template<class T>
class TCP_t : public T{
protected:
	TCP_h* tcp;
public:
	TCP_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
		T(src, dst, header, len)
	{
		tcp = (TCP_h*)T::cur;
		T::reset_next_protocol(IPPROTO_TCP);
		tcp->th_sport = htons(src.getport());
		tcp->th_dport = htons(dst.getport());
		tcp->th_seq = htonl(0);
		tcp->th_ack = htonl(0);
		tcp->th_x2 = 0;
		tcp->th_off = TCPLEN / 4;
		tcp->th_flags = TH_ACK;
		tcp->th_win = htons(65535);
		tcp->th_urp = htons(0);
		tcp->th_sum = 0;
		T::cur = (uint8_t*)tcp + TCPLEN;
		// Move checksum to reset_pkt
		/*if(T::isIP4()){
			tcp->th_sum = tcpudp4_checksum(T::ip, (uint16_t*)tcp, len - IPV4LEN, IPPROTO_TCP);
		}
		else{

			tcp->th_sum = tcpudp6_checksum(T::ip, (uint16_t*)tcp, len - IPV6LEN, IPPROTO_TCP);
		}*/
	}
	virtual ~TCP_t(){
		tcp = NULL;
	}
	virtual void reset_pkt(int start_id, int sent_pkt, int flow_length, unsigned long sent_byte){
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
				tcp->th_sum = tcpudp4_checksum((uint8_t*)(T::ip), (uint16_t*)tcp, T::pkt_len - IPV4LEN, IPPROTO_TCP);
		}
		else{
			tcp->th_sum = tcpudp6_checksum((uint8_t*)(T::ip), (uint16_t*)tcp, T::pkt_len - IPV6LEN, IPPROTO_TCP);
		}

	}
};

template<class T>
class UDP_t : public T{
protected:
	UDP_h* udp;
public:
	UDP_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
			T(src, dst, header, len)
	{
		udp = (UDP_h*)T::cur;
		T::reset_next_protocol(IPPROTO_UDP);
		udp->source = htons(src.getport());
		udp->dest = htons(dst.getport());
		udp->check = 0;
		/*if(T::isIP4()){
			udp->len = len - IPV4LEN;
			udp->check = tcpudp4_checksum(T::ip, (uint16_t*)udp, udp->len, IPPROTO_UDP);
		}
		else{
			udp->len = len - IPV6LEN;
			udp->check = tcpudp6_checksum(T::ip, (uint16_t*)udp, udp->len, IPPROTO_UDP);
		}*/
		T::cur = (uint8_t*)udp + UDPLEN;
	}
	virtual ~UDP_t(){
		udp = NULL;
	}
	virtual void reset_pkt(int start_id, int sent_pkt, int flow_length, unsigned long sent_byte){
		T::reset_pkt(start_id, sent_pkt);
		if(T::isIP4()){
			udp->len = htons(T::pkt_len - IPV4LEN);
			udp->check = tcpudp4_checksum((uint8_t*)(T::ip), (uint16_t*)udp, T::pkt_len - IPV4LEN, IPPROTO_UDP);
		}
		else{
			udp->len = htons(T::pkt_len - IPV6LEN);
			udp->check = tcpudp6_checksum((uint8_t*)(T::ip), (uint16_t*)udp, T::pkt_len - IPV6LEN, IPPROTO_UDP);
		}
	}
};

class ICMP_t : public IPV4_t{
protected:
	ICMP_h* icmp;
public:
	ICMP_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
		IPV4_t(src, dst, header, len)
	{
		IPV4_t::reset_next_protocol(IPPROTO_ICMP);
		icmp = (ICMP_h*)cur;
		icmp->type = ICMP_ECHO;
		icmp->code = 0;
		icmp->un.echo.id = htons(0);
		icmp->un.echo.sequence = htons(0);
		icmp->checksum = 0;
		//icmp->checksum = checksum((uint16_t*)icmp, len - IPV4LEN);
		cur = (uint8_t*)icmp + ICMPLEN;
	}
	virtual ~ICMP_t(){
		icmp = NULL;
	}
	virtual void reset_pkt(int start_id, int sent_pkt, int flow_length=0, unsigned long sent_byte=0){
		IPV4_t::reset_pkt(start_id, sent_pkt);
		icmp->un.echo.id = htons(start_id);
		icmp->un.echo.sequence = htons(sent_pkt);
		icmp->checksum = checksum((uint16_t*)icmp, pkt_len - IPV4LEN);
	}

};

class ICMP6_t : public IPV6_t{
protected:
	ICMP6_h* icmp;
public:
	ICMP6_t(Host_IP& src, Host_IP& dst, ETHER_h& header, size_t len):
		IPV6_t(src, dst, header, len)
	{
		IPV6_t::reset_next_protocol(IPPROTO_ICMPV6);
		icmp = (ICMP6_h*)cur;
		icmp->icmp6_type = ICMP6_ECHO_REQUEST;
		icmp->icmp6_code = 0;
		icmp->icmp6_id = htons(1000);
		icmp->icmp6_seq = htons(0);
		icmp->icmp6_cksum = 0;
		//icmp->icmp6_cksum = tcpudp6_checksum(ip, (uint16_t*)icmp, (uint16_t)(len - IPV6LEN), IPPROTO_ICMP);
		cur = (uint8_t*)icmp + ICMP6LEN;
	}
	virtual ~ICMP6_t(){
		icmp = NULL;
	}
	virtual void reset_pkt(int start_id, int sent_pkt, int flow_length=0, unsigned long sent_byte=0){
		icmp->icmp6_id = htons(start_id);
		icmp->icmp6_seq = htons(sent_pkt);
		icmp->icmp6_cksum = tcpudp6_checksum((uint8_t*)ip, (uint16_t*)icmp, (uint16_t)(pkt_len - IPV6LEN), IPPROTO_ICMPV6);
	}
};

class Flow{
private:
	int timer;		// epoll fd
	int flow_length;
	int sent_pkt;	//pkt already sent
	int start_id;
	unsigned long sent_byte;
	struct timespec arrival_time; //flow arrival time
	std::default_random_engine generator;
	ETHER_h ether;
	Host_IP src, dst;
	bool ipv4;
	Protocol type;
	Raw_Packet *pkt; // current pkt to be sent

public:
	//Flow();
	//~Flow();
};


}

#endif /* SRC_FLOW_HPP_ */

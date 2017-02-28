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

class Raw_Packet{
protected:
	ETHER_h* ether;
	uint8_t* frame;
	uint8_t* cur;
	size_t pkt_len;
public:
	Raw_Packet(ETHER_h&, size_t);
	virtual ~Raw_Packet();
	virtual bool isIP4();
	virtual void reset_next_protocol(u_int8_t);
	virtual void reset_pkt(int, int, int, unsigned long);
};

class IPV4_t : public Raw_Packet{
protected:
	IPV4_h* ip;
public:
	IPV4_t(Host_IP&, Host_IP&, ETHER_h&, size_t);
	virtual ~IPV4_t();
	bool isIP4();
	virtual void reset_next_protocol(u_int8_t);
	virtual void reset_pkt(int, int, int, unsigned long);
};

class IPV6_t : public Raw_Packet{
protected:
	IPV6_h* ip;
public:
	IPV6_t(Host_IP&, Host_IP&, ETHER_h&, size_t);
	virtual ~IPV6_t();
	bool isIP4();
	void reset_next_protocol(u_int8_t);
	virtual void reset_pkt(int, int, int, unsigned long);
};


template<class T>
class TCP_t : public T{
protected:
	TCP_h* tcp;
public:
	TCP_t(Host_IP&, Host_IP&, ETHER_h&, size_t);
	virtual ~TCP_t();
	virtual void reset_pkt(int, int, int, unsigned long);
};

template<class T>
class UDP_t : public T{
protected:
	UDP_h* udp;
public:
	UDP_t(Host_IP&, Host_IP&, ETHER_h&, size_t);
	virtual ~UDP_t();
	virtual void reset_pkt(int, int, int, unsigned long);
};

class ICMP_t : public IPV4_t{
protected:
	ICMP_h* icmp;
public:
	ICMP_t(Host_IP&, Host_IP&, ETHER_h&, size_t);
	virtual ~ICMP_t();
	virtual void reset_pkt(int, int, int, unsigned long);
};

class ICMP6_t : public IPV6_t{
protected:
	ICMP6_h* icmp;
public:
	ICMP6_t(Host_IP&, Host_IP&, ETHER_h&, size_t);
	virtual ~ICMP6_t();
	virtual void reset_pkt(int, int, int, unsigned long);
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

/*
 * Flow.hpp
 *
 *  Created on: Feb 21, 2017
 *      Author: ubuntu14
 */

#ifndef SRC_FLOW_HPP_
#define SRC_FLOW_HPP_

#include "Host_IP.hpp"
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


class Raw_Packet{
private:
	struct

public:


};

class Flow{
private:
	int timer;
	int remain_pkt;
	struct timespec arrival_time;
	Host_IP src, dst;
	std::default_random_engine generator;
	Raw_Packet pkt;

public:
	Flow();
	Flow(Host_IP, Host_IP, int);
	~Flow();
	void SetFlow(Host_IP, Host_IP, int);
};

Flow::Flow(){

}

Flow::Flow(Host_IP s, Host_IP d, int flow_len){

}

void Flow::SetFlow(Host_IP s, Host_IP d, int flow_len){

}

Flow::~Flow(){

}



#endif /* SRC_FLOW_HPP_ */

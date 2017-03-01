/*
 * Flow.hpp
 *
 *  Created on: Feb 21, 2017
 *      Author: ubuntu14
 */

#ifndef SRC_FLOW_HPP_
#define SRC_FLOW_HPP_

#include "Raw_Pkt.hpp"
#include <time.h>
#include <random>



namespace Traffic{


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
	Flow(int timer_v, ETHER_h& ether_v){
		std::uniform_int_distribution<int> D(1,16384);
		generator.seed(std::random_device{}());
		start_id = D(generator);
		timer = timer_v;
		clock_gettime(CLOCK_REALTIME, &(arrival_time));
		std::memcpy(&ether, &ether_v, ETHERLEN);
		ipv4 = true;
		type = Protocol::TCP;
		flow_length = 0;
		src = Host_IP();
		dst = Host_IP();
		sent_pkt = 0;
		sent_byte = 0;
		pkt = NULL;
	}
	~Flow(){
		delete pkt;
		pkt = NULL;
	}
	/*Call From Dispather*/
	void setFlowTime(struct timespec& time){
		arrival_time.tv_nsec = time.tv_nsec;
		arrival_time.tv_sec = time.tv_sec;
	}
	/*Call From Worker*/
	void setFlow(Host_IP& shost, Host_IP& dhost, bool v4, Protocol t, int len){
		std::uniform_int_distribution<int> D(1,16384);
		start_id = D(generator);
		sent_pkt = 0;
		sent_byte = 0;
		src = shost;
		dst = dhost;
		ipv4 = v4;
		type = t;
		flow_length = len;
	}
	void setPkt(size_t pktlen){
		switch(type){
			case Protocol::TCP:
				if(ipv4) pkt = new TCP_t<IPV4_t>(src, dst, ether, pktlen);
				else pkt = new TCP_t<IPV6_t>(src, dst, ether, pktlen);
				break;

			case Protocol::UDP:
				if(ipv4) pkt = new UDP_t<IPV4_t>(src, dst, ether, pktlen);
				else pkt = new UDP_t<IPV6_t>(src, dst, ether, pktlen);
				break;

			case Protocol::ICMP:
				if(ipv4) pkt = new ICMP_t(src, dst, ether, pktlen);
				else pkt = new ICMP6_t(src, dst, ether, pktlen);
				break;

			default:
				if(ipv4) pkt = new IPV4_t(src, dst, ether, pktlen);
				else pkt = new IPV6_t(src, dst, ether, pktlen);
		}
	}
};


}

#endif /* SRC_FLOW_HPP_ */

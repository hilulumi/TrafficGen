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
#include <algorithm>


namespace Traffic{


class Flow{
private:
	int timer;		// epoll fd
	int flow_length;
	int sent_pkt;	//pkt already sent
	int start_id;
	unsigned long sent_byte;
	ETHER_h ether;
	Host_IP src, dst;
	bool ipv4;
	Protocol type;
	Raw_Packet *pkt; // current pkt to be sent

public:
	std::default_random_engine generator;
	struct itimerspec arrival_time; //flow arrival time
	Flow(int timer_v, ETHER_h& ether_v){
		std::uniform_int_distribution<int> D(1,16384);
		generator.seed(std::random_device{}());
		start_id = D(generator);
		timer = timer_v;
		ether = ether_v;
		ipv4 = true;
		type = Protocol::UNKNOWN;
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
	Raw_Packet* takePkt(){
		Raw_Packet *x = pkt;
		if(x == NULL)
			return x;
		pkt = NULL;
		return x;
	}
	int getTimerfd(){
		return timer;
	}
	int getRemain(){
		return flow_length - sent_pkt;
	}

	/*
	 * Set the New Flow including the first packet and the protocols
	 * P.S. Call From Worker
	 */
	void setFlow(Host_IP& shost, Host_IP& dhost, int len, std::vector<std::vector<double>>& PktDist){
		std::uniform_int_distribution<int> D(1,16384);
		generator.seed(std::random_device{}());
		start_id = D(generator);
		sent_pkt = 0;
		sent_byte = 0;
		src = shost;
		dst = dhost;
		flow_length = len;
		type = Protocol::UNKNOWN;
		int pktlen = getPktLen(PktDist);
		setProtocol(pktlen, PktDist);
		genPkt(pktlen);
	}
	/*
	 * Assign a new packet to the flow
	 */
	void genPkt(size_t pktlen){
		if(pkt != NULL){
			std::cout<<"Pkt Non deleted\n";
			delete pkt;
			pkt = NULL;
		}
		size_t l3, l4;
		switch(type){
			case Protocol::TCP:
				if(ipv4) pkt = new TCP_t<IPV4_t>(src, dst, ether, pktlen);
				else pkt = new TCP_t<IPV6_t>(src, dst, ether, pktlen);
				l4 = TCPLEN;
				break;

			case Protocol::UDP:
				if(ipv4) pkt = new UDP_t<IPV4_t>(src, dst, ether, pktlen);
				else pkt = new UDP_t<IPV6_t>(src, dst, ether, pktlen);
				l4 = UDPLEN;
				break;

			case Protocol::ICMP:
				if(ipv4) pkt = new ICMP_t(src, dst, ether, pktlen);
				else pkt = new ICMP6_t(src, dst, ether, pktlen);
				l4 = ICMPLEN;
				break;

			default:
				if(ipv4) pkt = new IPV4_t(src, dst, ether, pktlen);
				else pkt = new IPV6_t(src, dst, ether, pktlen);
				l4 = 0;
		}
		l3 = ipv4 ? IPV4LEN : IPV6LEN;
		pkt->reset_pkt(start_id, sent_pkt, flow_length, sent_byte);
		sent_pkt++;
		sent_byte += (pktlen - l3 - l4);

	}
	/*
	 * Get a length for the new packet
	 */
	int getPktLen(std::vector<std::vector<double>>& PktDist){
		int startIdx;
		if(((type == Protocol::UDP || type == Protocol::ICMP) && ipv4) || type == Protocol::UNKNOWN)
			startIdx = 0; //start len 28
		else if(ipv4)
			startIdx = 12; //start len 40
		else if(type !=
				Protocol::TCP) startIdx = 20;  //start len 48
		else
			startIdx = 32; //start len 60

		std::uniform_real_distribution<double> D(PktDist[0][startIdx], PktDist[0][PktDist[0].size()-1]);
		return (std::upper_bound(PktDist[0].begin()+startIdx, PktDist[0].end(), D(generator)) - PktDist[0].begin() + 28);
	}

	void setProtocol(int pktlen, std::vector<std::vector<double>>& PktDist){
		int used_v, p_idx;
		auto setType = [](int idx, Protocol& t, bool& v4)mutable{
			switch(idx){
				case 0:{t = Protocol::TCP; v4 = true;}break;
				case 1:{t = Protocol::UDP; v4 = true;}break;
				case 2:{t = Protocol::ICMP; v4 = true;}break;
				case 3:{t = Protocol::ICMP; v4 = false;}break;
				case 4:{t = Protocol::ICMP; v4 = false;}break;
				case 5:{t = Protocol::ICMP; v4 = false;}break;
				default:{t = Protocol::UNKNOWN;}
			}
		};

		if(pktlen < 40) used_v = 1;
		else if(pktlen < 48) used_v = 2;
		else if(pktlen < 60) used_v = 3;
		else used_v = 4;
		std::uniform_real_distribution<double> D(0, PktDist[used_v][PktDist[used_v].size()-1]);
		p_idx = std::upper_bound(PktDist[used_v].begin(), PktDist[used_v].end(), D(generator)) - PktDist[used_v].begin();
		setType(p_idx, type, ipv4);
		if(type == Protocol::UNKNOWN){
			std::cout << "Wrong Packet Distribution File\n";
			exit(-1);
		}
	}
};


}

#endif /* SRC_FLOW_HPP_ */

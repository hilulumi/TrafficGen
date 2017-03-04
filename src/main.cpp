/*
 * main.cpp
 *
 *  Created on: Feb 18, 2017
 *      Author: phoebe
 */
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "Flow.hpp"
#include "Threadpool.hpp"
#include "Helper.hpp"


#define LONGOPT "s:F:c:f:L:l:p:t:i:"
#define DEFAULT_DISTR "weibull(1,1)"

u_int8_t DstMac[6] = {0x34, 0x97, 0xF6, 0x33, 0x5E, 0xE9};

using namespace std;

int ResolveModel(string S, Threadpool::Prob::Model &t, double &a, double &b){
    char *x = new char[S.size()];
    strncpy(x, S.c_str(), S.size());
    char* pch;
    char *pinta, *pintb;
    pch = strtok (x,"(");
    pinta = strtok (NULL," ,");
    pintb = strtok (NULL," )");
    pch = strtok (pch," ");
    string tmp(pch);
    a = atof(pinta);
    b = atof(pintb);
    if(tmp == "weibull")
    	t = Threadpool::Prob::Model::Weibull;
    else if(tmp == "lognormal")
    	t = Threadpool::Prob::Model::Lognormal;
    else if(tmp == "poisson")
    	t = Threadpool::Prob::Model::Poisson;
    else
    	return -1;

    return 0;
}


int main(int argc, char* argv[]){
	int c;
	int ActiveFlows = 1, duration = 10;
	char *ServerHostFile = NULL, *ClientHostFile = NULL, *IfName = NULL, *PktFile=NULL;
	std::string FlowArrival(DEFAULT_DISTR);
	std::string PktArrival(DEFAULT_DISTR);
	std::string FlowLen(DEFAULT_DISTR);
	//std::string PktLen(DEFAULT_DISTR);    To be done: packet length using distribution models


	std::default_random_engine generator(std::random_device{}());

	while(1){

		static struct option long_options[]={
			{"server hosts",	required_argument,	0,	's'},
			{"active flows",	required_argument,	0,	'F'},
			{"client hosts",	required_argument,	0,	'c'},
			{"flow arrival",	required_argument,	0,	'f'},
			{"flow length",		required_argument,	0,	'L'},
			{"pkt length",		required_argument,	0,	'l'},
			{"pkt arrival",		required_argument,	0,	'p'},
			{"duration",		required_argument,	0,	't'},
			{"interface",		required_argument,	0,	'i'},
			{0,	0,	0,	0}
		};

		c = getopt_long(argc, argv, LONGOPT, long_options, NULL);

		if(c == -1)
			break;

		switch(c){
			case 's':
				if(optarg)
					ServerHostFile = optarg;

				break;

			case 'F':
				ActiveFlows = atoi(optarg);
				break;

			case 'c':
				if(optarg)
					ClientHostFile = optarg;
				break;

			case 'f':
				FlowArrival = optarg;
				break;

			case 'L':
				FlowLen = optarg;
				break;

			case 'l':
				//PktLen = optarg; To be done: packet length using distribution models
				PktFile = optarg;
				break;

			case 'p':
				PktArrival = optarg;
				break;

			case 't':
				duration =atoi(optarg);
				break;

			case 'i':
				IfName = optarg;
				break;

			case '?':
				break;

			default:
				break;
		}
	}

	if(PktFile == NULL){
			std::cout << "Must specify a Packet Distribution File\n";
			exit(-1);
	}

	/**************************Set Max File Discriptor***************************/
	struct rlimit rlim;
	getrlimit(RLIMIT_NOFILE, &rlim);
	rlim.rlim_cur = (ActiveFlows+100<rlim.rlim_max)?ActiveFlows+100:rlim.rlim_max;
	setrlimit(RLIMIT_NOFILE, &rlim);
	//getrlimit(RLIMIT_NOFILE, &rlim);

	/*****************************	Get Hosts	*********************************/
	int sockfd;
	struct sockaddr_ll socket_address;
	struct ifreq Interface;
	vector<Host_IP> Servers, Clients;
	vector<Host_IP>::iterator host_it;
	Host_IP tmphost;
	char buf[30];
	Traffic::ETHER_h ether;

	/*Get Server Host(s)*/
	sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
	if(sockfd < 0){
		perror("socket");
		exit(-1);
	}

	if(ServerHostFile == NULL){
		if(IfName != NULL){
			memset(&Interface, 0, sizeof(Interface));
			strncpy(Interface.ifr_name, IfName, IFNAMSIZ-1);
			if(ioctl(sockfd, SIOCGIFADDR, &Interface) < 0){
				perror("Interface, SIOCGIFADDR");
				exit(-1);
			}
			tmphost.setaddr(((struct sockaddr_in*)&Interface.ifr_addr)->sin_addr.s_addr);
			Servers.push_back(tmphost);

			/*Get Host MAC address*/
			memset(&Interface, 0, sizeof(Interface));
			strncpy(Interface.ifr_name, IfName, IFNAMSIZ-1);
			if(ioctl(sockfd, SIOCGIFHWADDR, &Interface)< 0){
				perror("Interface, SIOCGIFHWADDR");
				exit(-1);
			}
			memcpy(ether.ether_shost, Interface.ifr_hwaddr.sa_data, 6);
			memcpy(ether.ether_dhost, DstMac, 6);

			/*
			 * unsigned char mac_address[6];
			 * memcpy(mac_address, Interface.ifr_hwaddr.sa_data, 6);
			 * printf("%02x:%02x:%02x:%02x:%02x:%02x\n",mac_address[0],mac_address[1],mac_address[2],mac_address[3],mac_address[4],mac_address[5]);
			 */
		}
		else{
			std::cout << "Must specify either Interface or Server hosts\n";
			exit(-1);
		}

	}
	else{
		std::fstream file(ServerHostFile, std::fstream::in);
		while(file.getline(buf, 30)){
			tmphost.sethost(std::string(buf));
			if(std::find(Servers.begin(), Servers.end(), tmphost) == Servers.end())
				Servers.push_back(tmphost);
		}
	}
	memset(&Interface, 0, sizeof(struct ifreq));
	strncpy(Interface.ifr_name, IfName, 10);
	if (ioctl(sockfd, SIOCGIFINDEX, &Interface) < 0){
		perror("SIOCGIFINDEX");
		exit(-1);
	}

	socket_address.sll_ifindex = Interface.ifr_ifindex;
	socket_address.sll_halen = ETH_ALEN;
	socket_address.sll_addr[0] = DstMac[0];
	socket_address.sll_addr[1] = DstMac[1];
	socket_address.sll_addr[2] = DstMac[2];
	socket_address.sll_addr[3] = DstMac[3];
	socket_address.sll_addr[4] = DstMac[4];
	socket_address.sll_addr[5] = DstMac[5];

	/*
	 * for(std::vector<Host_IP>::const_iterator i=Servers.begin(); i!=Servers.end(); i++)
	 * 		std::cout<< ntohl(i->getaddr()) << ':' << ntohs(i->getport())<<endl;
	 * cout<<endl<<endl;
	 */

	/*Get Client Hosts*/
	if(ClientHostFile == NULL){
		int clientnum = 1 + (ActiveFlows-1)/Servers.size();
		std::uniform_int_distribution<unsigned long> d1(1,4294967295);
		std::uniform_int_distribution<unsigned short> d2(1,65535);
		int i=0;

		while(i<clientnum){
			tmphost.sethost(htonl(d1(generator)), htons(d2(generator)));
			if(std::find(Clients.begin(), Clients.end(), tmphost) == Clients.end()){
				Clients.push_back(tmphost);
				i++;
			}
		}
	}
	else{
		std::fstream file(ClientHostFile, std::fstream::in);
		while(file.getline(buf, 30)){
			tmphost.sethost(std::string(buf));
			if(std::find(Clients.begin(), Clients.end(), tmphost) == Clients.end())
				Clients.push_back(tmphost);
		}
	}
	/*
	 * for(std::vector<Host_IP>::const_iterator i=Clients.begin(); i!=Clients.end(); i++)
	 * 		std::cout<< ntohl(i->getaddr()) << ':' << ntohs(i->getport())<<endl;
	 */

	/************************Get Packet length and protocol distribution***********************/
	vector<vector<double>> PktDist;
	fstream PktDistF;

	PktDistF.open(PktFile, std::fstream::in);
	std::string line;
	while(std::getline(PktDistF, line)){
		std::istringstream iss(line);
		double number = 0.0;
	    std::vector<double> W;
	    while (iss >> number)
	    	W.push_back(number);

	    // add line to vector
		PktDist.push_back(W);
	}
	/*
	 * std::cout.precision(14);
	 * for(int i=0; i<PktDist.size();i++){
	 * 	for(int j=0; j<PktDist[i].size();j++)
	 * 		std::cout<<PktDist[i][j]<<" ";
	 * 	std::cout<<std::endl;
	 * }
	 */


	/************************	  	Resolve Distribution Models		************************/
	Threadpool::Prob::Distribution FlowArr_D, PktArr_D, FlowLen_D;
	Threadpool::Prob::Model FlowArr_M, PktArr_M, FlowLen_M;
	double FlowArr_a, FlowArr_b, PktArr_a, PktArr_b, FlowLen_a, FlowLen_b;
	/*
	 *  To be done: packet length using distribution models
	 *
	 *  Threadpool::Prob::Model PktLen_M;
	 *  int PktLen_a, PktLen_b;
	 */

	if(ResolveModel(FlowArrival, FlowArr_M, FlowArr_a, FlowArr_b) != 0){
		std::cout << "Wrong Distribution Model Name : FlowArrival\n";
		exit(-1);
	}
	if(ResolveModel(PktArrival, PktArr_M, PktArr_a, PktArr_b) != 0){
			std::cout << "Wrong Distribution Model Name : PktArrival\n";
			exit(-1);
	}
	if(ResolveModel(FlowLen, FlowLen_M, FlowLen_a, FlowLen_b) != 0){
			std::cout << "Wrong Distribution Model Name : FlowLen\n";
			exit(-1);
	}

	Threadpool::Prob::getmodel(FlowArr_D, FlowArr_M, FlowArr_a, FlowArr_b);
	Threadpool::Prob::getmodel(PktArr_D, PktArr_M, PktArr_a, PktArr_b);
	Threadpool::Prob::getmodel(FlowLen_D, FlowLen_M, FlowLen_a, FlowLen_b);

	/************************	  	Main Job		************************/

	std::uniform_int_distribution<int> ServerIdx(0,Servers.size()-1);
	std::uniform_int_distribution<int> ClientIdx(0,Clients.size()-1);
	struct epoll_event *events = new struct epoll_event[ActiveFlows+1];
	struct epoll_event tmpev;
	struct itimerspec timer, monitorT;
	int epollfd, monitor, nfds;
	bool finflag = false;


	clock_gettime(CLOCK_REALTIME, &(timer.it_value));
	timer.it_value.tv_sec = timer.it_value.tv_sec+ 5;
	timer.it_interval.tv_sec = duration;
	timer.it_interval.tv_nsec = 0;
	epollfd = epoll_create(1);

	monitorT = timer;
	monitorT.it_value.tv_sec = monitorT.it_value.tv_sec + duration;
	monitor = timerfd_create(CLOCK_REALTIME, 0);
	tmpev.data.ptr = (void*)new Traffic::Flow(monitor, ether);
	tmpev.events = EPOLLIN|EPOLLET;
	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, monitor, &tmpev) == -1) {
		perror("epoll_ctl: fd");
		exit(-1);
	}
	timerfd_settime(monitor, TFD_TIMER_ABSTIME, &monitorT, NULL);

	Threadpool::Pool pool(FlowArr_D, FlowLen_D);

	for(int i; i<ActiveFlows; i++){
		struct epoll_event *ev = new struct epoll_event[1];
		int timefd = timerfd_create(CLOCK_REALTIME, 0);
		int sidx, cidx;
		Traffic::Flow *flow = new Traffic::Flow(timefd, ether);
		sidx = ServerIdx(flow->generator);
		cidx = ClientIdx(flow->generator);
		flow->setFlow(Servers[sidx], Clients[cidx],  FlowLen_D(flow->generator), PktDist);
		timer.it_value.tv_nsec += FlowArr_D(flow->generator)*1000000;
		timer.it_value.tv_sec += timer.it_value.tv_nsec/1000000000;
		timer.it_value.tv_nsec = timer.it_value.tv_nsec%1000000000;
		flow->arrival_time = timer;
		ev->data.ptr = (void*)flow;
		ev->events = EPOLLIN|EPOLLET;
		if(epoll_ctl(epollfd, EPOLL_CTL_ADD, flow->getTimerfd(), ev) == -1) {
		    perror("epoll_ctl: fd");
		    exit(-1);
		}
		timerfd_settime(flow->getTimerfd(), TFD_TIMER_ABSTIME, &flow->arrival_time, NULL);
		//cout<<"Set Flow "<<timer.it_value.tv_sec<<"s "<<timer.it_value.tv_nsec/1000000<<" ms"<<endl;
		//sleep(1);
	}

	while(!finflag){
		nfds = epoll_wait(epollfd, events, ActiveFlows+1, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			exit(-1);
		}
		for(int j=0; j<nfds;j++){
			Traffic::Flow* flow_ev = (Traffic::Flow*)(events[j].data.ptr);
			Traffic::Raw_Packet *pkt = flow_ev->takePkt();
			if(flow_ev->getTimerfd()==monitor){
				finflag = true;
				cout<<"Terminating...\n";
				break;
			}
			/*Generate Send Pkt job*/
			if(pkt != NULL){
				auto job = new Threadpool::Job::callback([pkt, &socket_address](Threadpool::Worker& w)
						{
							const std::unique_ptr<Traffic::Raw_Packet> data(pkt);
							if (sendto(w.getSocket(), data->getframe(), data->getLen()+Traffic::ETHERLEN, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0){
								perror("Send Fail \n");
								exit(-1);
							}
							//else cout<<"sent "<<data->getLen()<<endl;

							return Threadpool::Job::Type::NORMAL;
						});
				pool.push(job);
			}
			/*Set a new flow*/
			if(flow_ev->getRemain() != 0){
				auto job = new Threadpool::Job::callback([flow_ev, &PktDist](Threadpool::Worker& w)
						{
							int len = flow_ev->getPktLen(PktDist);
							flow_ev->genPkt(len);
							flow_ev->arrival_time.it_value.tv_nsec += w.InterPktDist(flow_ev->generator)*1000000;
							flow_ev->arrival_time.it_value.tv_sec += flow_ev->arrival_time.it_value.tv_nsec/1000000000;
							flow_ev->arrival_time.it_value.tv_nsec = flow_ev->arrival_time.it_value.tv_nsec%1000000000;
							timerfd_settime(flow_ev->getTimerfd(), TFD_TIMER_ABSTIME, &(flow_ev->arrival_time), NULL);
							//cout<<"GenPkt "<<flow_ev->arrival_time.it_value.tv_sec<<" "<<flow_ev->arrival_time.it_value.tv_nsec/1000000<<endl;
							return Threadpool::Job::Type::NORMAL;
						});
				pool.push(job);
			}
			else{
				timer.it_value.tv_nsec += FlowArr_D(flow_ev->generator)*1000000;
				timer.it_value.tv_sec += timer.it_value.tv_nsec/1000000000;
				timer.it_value.tv_nsec = timer.it_value.tv_nsec%1000000000;
				flow_ev->arrival_time = timer;
				auto job = new Threadpool::Job::callback([flow_ev, epollfd, &Servers, &Clients, &PktDist](Threadpool::Worker& w)
						{
							std::uniform_int_distribution<int> SIdx(0,Servers.size()-1);
							std::uniform_int_distribution<int> CIdx(0,Clients.size()-1);
							flow_ev->setFlow(Servers[SIdx(flow_ev->generator)], Clients[CIdx(flow_ev->generator)],  w.FlowLenDist(flow_ev->generator), PktDist);
							timerfd_settime(flow_ev->getTimerfd(), TFD_TIMER_ABSTIME, &(flow_ev->arrival_time), NULL);
							//cout<<"Set Flow "<<flow_ev->arrival_time.it_value.tv_sec<<" "<<flow_ev->arrival_time.it_value.tv_nsec/1000000<<endl;
							return Threadpool::Job::Type::NORMAL;
						});
				pool.push(job);
			}
			//cout<<"Size: "<<pool.JobNum() << "\n";
		}//events forloop
	}//epoll wait while loop

	pool.terminate();


	return 0;
}







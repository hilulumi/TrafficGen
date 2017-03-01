/*
 * main.cpp
 *
 *  Created on: Feb 18, 2017
 *      Author: phoebe
 */
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <sstream>
#include <cstdio>
#include "Flow.hpp"
#include "Threadpool.hpp"
#define LONGOPT "s:F:c:f:L:l:p:t:i:"
#define DEFAULT_DISTR "weibull, 1, 1"

using namespace std;



int main(int argc, char* argv[]){
	int c, sockfd;
	int ServerNum = 1, ActiveFlows = 1, duration = 10;
	char *ServerHostFile = NULL, *ClientHostFile = NULL, *IfName = NULL, *PktFile=NULL;
	std::string FlowArrival(DEFAULT_DISTR);
	std::string PktArrival(DEFAULT_DISTR);
	std::string FlowLen(DEFAULT_DISTR);
	std::string PktLen(DEFAULT_DISTR);
	struct ifreq Interface;
	//struct ip iphdr;
	vector<Host_IP> Servers, Clients;
	vector<Host_IP>::iterator host_it;
	Host_IP tmphost;
	char buf[30];
	vector<vector<double>> PktDist;
	fstream PktDistF;

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
				PktLen = optarg;
				break;

			case 'p':
				PktFile = optarg;
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

	sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
	if(sockfd < 0){
		perror("socket");
		exit(-1);
	}

	/*Get Server Host(s)*/
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
			//unsigned char mac_address[6];
			//memcpy(mac_address, Interface.ifr_hwaddr.sa_data, 6);
			//printf("%02x:%02x:%02x:%02x:%02x:%02x\n",mac_address[0],mac_address[1],mac_address[2],mac_address[3],mac_address[4],mac_address[5]);
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
	/*
		for(std::vector<Host_IP>::const_iterator i=Servers.begin(); i!=Servers.end(); i++)
			std::cout<< ntohl(i->getaddr()) << ':' << ntohs(i->getport())<<endl;
		cout<<endl<<endl;
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
		for(std::vector<Host_IP>::const_iterator i=Clients.begin(); i!=Clients.end(); i++)
			std::cout<< ntohl(i->getaddr()) << ':' << ntohs(i->getport())<<endl;
	*/

	/*Get Packet length and protocol distribution*/
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
	    std::cout.precision(14);
		for(int i=0; i<PktDist.size();i++){
			for(int j=0; j<PktDist[i].size();j++)
				std::cout<<PktDist[i][j]<<" ";
			std::cout<<std::endl;
		}

	 */

	/*TEST*/
	Traffic::ETHER_h E;
	Traffic::Flow A(0, E);

	return 0;

}







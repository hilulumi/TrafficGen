/*
 * main.cpp
 *
 *  Created on: Feb 18, 2017
 *      Author: phoebe
 */
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <net/if.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <string>

#include "Host_IP.hpp"

#define LONGOPT "S:s::F:c::f:L:l:p:t:i:"
#define DEFAULT_DISTR "weibull, 1, 1"

using namespace std;



int main(int argc, char* argv[]){
	int c, sockfd;
	int ServerNum = 1, ActiveFlows = 1, duration = 10;
	char *ServerHostFile = NULL, *ClientHostFile = NULL, *IfName = NULL;
	std::string FlowArrival(DEFAULT_DISTR);
	std::string PktArrival(DEFAULT_DISTR);
	std::string FlowLen(DEFAULT_DISTR);
	std::string PktLen(DEFAULT_DISTR);
	struct ifreq Interface;
	struct ip iphdr;


	while(1){

		static struct option long_options[]={
			{"server num",		required_argument,	0,	'S'},
			{"server hosts",	optional_argument,	0,	's'},
			{"active flows",	required_argument,	0,	'F'},
			{"client hosts",	optional_argument,	0,	'c'},
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
			case 'S':
				ServerNum = atoi(optarg);
				break;

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
				PktLen = optarg;
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
		}
	}

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
		}
	}


}







/*
 * Host.hpp
 *
 *  Created on: Feb 19, 2017
 *      Author: phoebe
 */
#include <arpa/inet.h>
#include <string>

#ifndef HOST_IP_HPP_
#define HOST_IP_HPP_



class Host_IP{
private:
	struct sockaddr_in host;
public:
	Host_IP();
	Host_IP(unsigned long, unsigned short);
	Host_IP(std::string, unsigned short);
	Host_IP(std::string);
	~Host_IP();
	std::string getaddr();
	void setaddr(std::string);
	unsigned short getport();
	void setport(unsigned short);
	void gethost(struct sockaddr_in &);
};

Host_IP::Host_IP(){

}

Host_IP::Host_IP(unsigned long addr, unsigned short port){

}

Host_IP::Host_IP(std::string addr, unsigned short port){

}

Host_IP::Host_IP(std::string addr_port){

}

Host_IP::~Host_IP(){

}

std::string Host_IP::getaddr(){

}

void Host_IP::setaddr(std::string addr){

}

unsigned short Host_IP::getport(){

}

void Host_IP::setport(unsigned short port){

}

void Host_IP::gethost(struct sockaddr_in &){

}

#endif /* HOST_IP_HPP_ */

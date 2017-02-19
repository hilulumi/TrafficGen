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
	struct in_addr ip;
	unsigned short port;
public:
	Host_IP();
	Host_IP(unsigned long, unsigned short);
	Host_IP(std::string, unsigned short);
	Host_IP(std::string);
	~Host_IP();
	unsigned long getaddr();
	void setaddr(std::string);
	unsigned short getport();
	void setport(unsigned short);
	void sethost(std::string);
};

Host_IP::Host_IP(){
	setport(p);
	setaddr("192.168.0.11");
}

Host_IP::Host_IP(unsigned long addr, unsigned short p){
	setport(p);
	ip.s_addr = htonl(addr);
}

Host_IP::Host_IP(std::string addr, unsigned short p){
	setaddr(addr);
	setport(p);
}

Host_IP::Host_IP(std::string h){
	sethost(h);
}

Host_IP::~Host_IP(){

}

unsigned long Host_IP::getaddr(){
	return ip.s_addr;
}

void Host_IP::setaddr(std::string addr){
	inet_aton(addr.c_str(), &ip);
}

unsigned short Host_IP::getport(){
	return port;
}

void Host_IP::setport(unsigned short p){
	port = htons(p);
}

void Host_IP::sethost(std::string h){
	size_t split = h.find_last_of(':');

	inet_aton(h.substr(0,split).c_str(), &ip);
	port = htons(atoi(h.substr(split+1).c_str()));
}

#endif /* HOST_IP_HPP_ */

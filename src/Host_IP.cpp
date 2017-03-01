/*
 * Host_IP.cpp
 *
 *  Created on: Feb 28, 2017
 *      Author: root
 */

#include "Host_IP.hpp"


Host_IP::Host_IP(){
	setport(8989);
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

unsigned long int Host_IP::getaddr()const{
	return ip.s_addr;
}

void Host_IP::setaddr(std::string addr){
	inet_aton(addr.c_str(), &ip);
}

void Host_IP::setaddr(unsigned long addr){
	ip.s_addr = addr;
}

unsigned short int Host_IP::getport() const{
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

void Host_IP::sethost(unsigned long addr, unsigned short p){
	setaddr(addr);
	setport(p);
}

bool Host_IP::operator ==(const Host_IP &H) const{
	if((ip.s_addr != H.getaddr()) || (port != H.getport()))
		return false;

	return true;
}

Host_IP& Host_IP::operator=(const Host_IP& host){
	if(this != &host){
		ip.s_addr = host.ip.s_addr;
		port = host.port;
	}
	return *this;
}

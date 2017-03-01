/*
 * Host.hpp
 *
 *  Created on: Feb 19, 2017
 *      Author: phoebe
 */

#ifndef HOST_IP_HPP_
#define HOST_IP_HPP_

#include <string>
#include <arpa/inet.h>
class Host_IP{
private:
	struct in_addr ip;
	unsigned short int port;
public:
	Host_IP();
	Host_IP(unsigned long, unsigned short);
	Host_IP(std::string, unsigned short);
	Host_IP(std::string);
	~Host_IP();
	unsigned long int getaddr() const;
	void setaddr(std::string);
	void setaddr(unsigned long);
	unsigned short int getport() const;
	void setport(unsigned short);
	void sethost(std::string);
	void sethost(unsigned long, unsigned short);
	bool operator ==(const Host_IP &) const;
	Host_IP& operator=(const Host_IP&);
};



#endif /* HOST_IP_HPP_ */

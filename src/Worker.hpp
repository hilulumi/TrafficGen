/*
 * Worker.hpp
 *
 *  Created on: Mar 3, 2017
 *      Author: root
 */

#ifndef SRC_WORKER_HPP_
#define SRC_WORKER_HPP_

#include <random>
#include <functional>
#include <thread>

namespace Threadpool{

namespace Prob{
enum class Model {Weibull, Lognormal, Poisson};
typedef std::function<int(std::default_random_engine&)> Distribution;
void getmodel(Prob::Distribution&, Prob::Model, int, int);
}

class Pool;
class Worker{
	friend class Pool;

private:
	int sockfd;
	std::thread wthread;

public:
	Prob::Distribution InterPktDist;
	Prob::Distribution FlowLenDist;
	Worker(Pool&, const Prob::Distribution&, const Prob::Distribution&);
	/*
	 * Do not use self-defined destructor, which can disable implicit generation of the move constructor
	 */
	void execute(Pool&);

	int getSocket();

};

}

#endif /* SRC_WORKER_HPP_ */

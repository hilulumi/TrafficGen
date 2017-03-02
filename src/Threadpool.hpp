/*
 * Treadpool.hpp
 *
 *  Created on: Feb 22, 2017
 *      Author: ubuntu14
 */

#ifndef THREADPOOL_HPP_
#define THREADPOOL_HPP_

#include <vector>
#include <mutex>
#include <thread>
#include <functional>
#include <condition_variable>
#include <boost/lockfree/queue.hpp>


namespace Threadpool{

class Worker;
class Threadpool;

namespace Job{
enum class Type {NORMAL, FINCALL};
typedef std::function<Type(Worker&)> callback;
}

namespace Prob{
enum class Model {Weibull, Lognormal, Poisson};
typedef std::function<int(std::default_random_engine&)> Distribution;
}

void getmodel(Prob::Distribution& Dist, Prob::Model i, int a, int b){
	using namespace Prob;
    switch(i){
       case Model::Weibull:{
    	   std::weibull_distribution<double> A(a,b);
    	   Dist = [A](std::default_random_engine& R) mutable {return A(R);};
       }
       break;
       case Model::Lognormal:{
            std::lognormal_distribution<double> B(a,b);
            Dist = [B](std::default_random_engine& R) mutable {return B(R);};
       }
       break;
       case Model::Poisson:{
            std::poisson_distribution<int> C(a);
            Dist = [C](std::default_random_engine& R) mutable {return C(R);};
       }
              break;
       default:{
           std::uniform_int_distribution<int> D(a,b);
           Dist = [D](std::default_random_engine& R) mutable {return D(R);};
       }
    }
};


class Worker{
	friend class Treadpool;

private:
	int sockfd;
	std::thread wthread;
public:
	Prob::Distribution InterPktDist;
	Prob::Distribution FlowLenDist;
	Worker(Threadpool& pool, Prob::Distribution& InterPkt, Prob::Distribution& FlowLen){
		if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
			perror("socket");
		}
		InterPktDist = InterPkt;
		FlowLenDist = FlowLen;
		wthread = std::thread(std::bind(&Worker::execute, this, std::ref(pool)));
	}
	int getSocket(){
		return sockfd;
	}
	void execute(Threadpool& pool){

	}
};

class Threadpool{
	friend class Worker;

private:
	 std::unique_ptr<boost::lockfree::queue<Job::callback*>> JobQ;
	std::mutex job_mutex;
	std::condition_variable job_signal;
	std::vector<Worker> WorkerQ;
public:
	Threadpool(Prob::Distribution& InterPkt, Prob::Distribution& FlowLen,
			std::size_t worker_count = std::thread::hardware_concurrency()):
		JobQ(new boost::lockfree::queue<Job::callback*>(0))
	{
		for(size_t i=0; i<worker_count; i++){
			WorkerQ.push_back(Worker(*this, InterPkt, FlowLen));
		}
	}


};

}

#endif /* THREADPOOL_HPP_ */






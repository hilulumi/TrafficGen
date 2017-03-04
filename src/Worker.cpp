/*
 * Worker.cpp
 *
 *  Created on: Mar 3, 2017
 *      Author: root
 */
#include "Worker.hpp"
#include "Threadpool.hpp"
#include <netinet/ether.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace Threadpool{

void Prob::getmodel(Prob::Distribution& Dist, Prob::Model i, double a, double b)
{
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
}

Worker::Worker(Pool& pool, const Prob::Distribution& InterPkt, const Prob::Distribution& FlowLen):
		wthread(new std::thread(&Worker::execute, this, std::ref(pool))){
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
		perror("socket");
	}

	InterPktDist = InterPkt;
	FlowLenDist = FlowLen;
}

void Worker::execute(Pool& pool){
	// no explicit destructor, kill itself
	const std::unique_ptr<Worker> self(this);
	while(1){
		Job::callback* job;
		Job::Type jtype = Job::Type::NORMAL;
		while(jtype != Job::Type::FINCALL && pool.JobQ->pop(job)){
			--pool.Qsize;
			//suicide when done
			const std::unique_ptr<Job::callback> work(job);
			jtype = (*work)(*this);
		}

		if(jtype == Job::Type::FINCALL)
			return;
		std::unique_lock<std::mutex> lock(pool.job_mutex);
		pool.job_signal.wait(lock, [&pool](){return pool.WorkerQ.size()>0;});

	}
}

int Worker::getSocket(){
	return sockfd;
}

}




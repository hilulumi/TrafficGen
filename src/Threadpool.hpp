/*
 * TFPool.hpp
 *
 *  Created on: Mar 2, 2017
 *      Author: root
 */

#ifndef SRC_THREADPOOL_HPP_
#define SRC_THREADPOOL_HPP_

#include <vector>
#include <mutex>
#include <condition_variable>
#include <boost/lockfree/queue.hpp>
#include "Worker.hpp"

namespace Threadpool{

namespace Job{
enum class Type {NORMAL, FINCALL};
typedef std::function<Type(Worker&)> callback;
}

class Pool{
	friend Worker;

private:
	std::unique_ptr<boost::lockfree::queue<Job::callback*>> JobQ;
	std::mutex job_mutex;
	std::condition_variable job_signal;
	std::vector<Worker> WorkerQ;

public:
	Pool( Prob::Distribution& InterPkt, Prob::Distribution& FlowLen,
			std::size_t worker_count = std::thread::hardware_concurrency()):
				JobQ(new boost::lockfree::queue<Job::callback*>(0))
	{

		for(size_t i=0; i<worker_count; i++){
			WorkerQ.push_back(Worker(*this, InterPkt, FlowLen));
		}

	}
	~Pool(){
		terminate();
	}
	void push(Job::callback* job){
		JobQ->push(job);
		job_signal.notify_one();
	}
	void terminate(){
		for(size_t i=0; i<WorkerQ.size(); i++){
			push(new Job::callback([](Worker& X){return Job::Type::FINCALL;}));
		}

		for(size_t i=0; i<WorkerQ.size(); i++){
			WorkerQ[i].wthread.join();
		}
		WorkerQ.clear();
		JobQ.~unique_ptr();
	}

};

}


#endif /* SRC_THREADPOOL_HPP_ */

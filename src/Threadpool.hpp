/*
 * Treadpool.hpp
 *
 *  Created on: Feb 22, 2017
 *      Author: ubuntu14
 */

#ifndef THREADPOOL_HPP_
#define THREADPOOL_HPP_

#include <mutex>
#include <thread>
#include <functional>
#include <boost/lockfree/queue.hpp>

namespace Threadpool{

class Worker;
class Threadpool;

namespace Job{
enum class Type {NORMAL, FINCALL};
typedef std::function<Type(Worker&)> callback;
}

class Worker{
	friend class Treadpool;

private:

public:

};

class Threadpool{
	friend class Worker;

private:
	boost::lockfree::queue<Job::callback*> JobQ;
	std::mutex job_signal;
public:

};

}

#endif /* THREADPOOL_HPP_ */






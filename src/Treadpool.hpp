/*
 * Treadpool.hpp
 *
 *  Created on: Feb 22, 2017
 *      Author: ubuntu14
 */

#ifndef THREADPOOL_HPP_
#define THREADPOOL_HPP_

#include <thread>
#include <functional>
#include <boost/lockfree/queue.hpp>

namespace Treadpool{

namespace Job{
enum class Type {NORMAL, FINCALL};
std::function<Job::Type()> callback;
}

class Treadpool;

class Worker{
	friend class Treadpool;
private:

public:

};

class Treadpool{
	friend class Worker;
private:

public:

};

}






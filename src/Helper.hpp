/*
 * Helper.hpp
 *
 *  Created on: Mar 1, 2017
 *      Author: root
 */

#ifndef SRC_HELPER_HPP_
#define SRC_HELPER_HPP_

namespace Treadpool{

template<class T>
class CaptureHelper{
private:
	mutable T value;
	CaptureHelper& operator=(CaptureHelper&& x) = delete;
	CaptureHelper& operator=(const CaptureHelper& x) = delete;

public:
	CaptureHelper(T&& x) : value(std::move(x)){}
	CaptureHelper(const CaptureHelper& x) : value(std::move(x.value)){}
	T& Value() { return value; }
	const T& Value() const { return value; }
};

template<class T>
CaptureHelper<T> getCaptureHelper(T&& x){
	return CaptureHelper<T>(std::move(x));
}

}




#endif /* SRC_HELPER_HPP_ */

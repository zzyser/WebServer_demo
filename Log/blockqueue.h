// #ifndef BLOCKQUEUE_H
// #define BLOCKQUEUE_H

#include <deque>
#include <condition_variable>
#include <mutex>
#include <sys/time.h>
#include <assert.h>

using namespace std;

template<typename T>
class BlockQueue{
	private:
		deque<T> deq_;
		mutex mtx_;
		bool isClose_;
		size_t capacity_;
		condition_variable condConsumer_;
		condition_variable condProducer_;
	public:
		explicit BlockQueue(size_t maxsize = 1000);
		~BlockQueue();
		bool empty();
		bool full();
		void push_back(const T& item);
		void push_front(const T& item);
		bool pop(T& item);
		bool pop(T& item,int timeout);
		void clear();
		T front();
		T back();
		size_t capacity();
		size_t size();

		void flush();
		void Close();
};

template<typename T>
BlockQueue<T>::BlockQueue(size_t maxsize) : capacity_(maxsize){
	assert(maxsize > 0);
	isClose_ = false;
}

template<typename T>
BlockQueue<T>::~BlockQueue(){
	Close();
}

template<typename T>
void BlockQueue<T>::Close(){
	clear();
	isClose_ = true;
	condConsumer_.notify_all();
	condProducer_.notify_all();
}

template<typename T>
void BlockQueue<T>::clear(){
	lock_guard<mutex> locker(mtx_);
	deq_.clear();
}

template<typename T>
bool BlockQueue<T> :: empty(){
	lock_guard<mutex> locker(mtx_);
	return deq_.empty();
}

template<typename T>
bool BlockQueue<T>::full(){
	lock_guard<mutex> locker(mtx_);
	return deq_.size() >= capacity_;
}

template<typename T>
void BlockQueue<T>::push_back(const T& item){
	unique_lock<mutex> locker(mtx_);
	while(deq_.size() >= capacity_){
		condProducer_.wait(locker);
	}
	deq_.push_back(item);
	condConsumer_.notify_one();
}

template<typename T>
void BlockQueue<T>::push_front(const T& item){
	unique_lock<mutex> locker(mtx_);
	while(deq_.empty()){
		condConsumer_.wait(locker);
	}
	deq_.push_front(item);
	condConsumer_.notify_one();
}

template<typename T>
bool BlockQueue<T>:: pop(T& item){
	unique_lock<mutex> locker(mtx_);
	while(deq_.empty()){
		condConsumer_.wait(locker);
	}
	item = deq_.front();
	deq_.pop_front();
	condProducer_.notify_one();
	return true;
}

template<typename T>
bool BlockQueue<T>::pop(T &item,int timeout){
	unique_lock<mutex> locker(mtx_);
	while(deq_.empty()){
		if(condConsumer_.wait_for(locker,chrono::seconds(timeout))==cv_status::timeout){
			return false;
		}
		if(isClose_){
			return false;
		}
	}
	item = deq_.front();
	deq_.pop_front();
	condProducer_.notify_one();
	return true;
}

template<typename T>
T BlockQueue<T>:: front(){
	lock_guard<mutex> locker(mtx_);
	return deq_.front();
}

template<typename T>
T BlockQueue<T>:: back(){
	lock_guard<mutex> locker(mtx_);
	return deq_.back();
}

template<typename T>
size_t BlockQueue<T>:: size(){
	lock_guard<mutex> locker(mtx_);
	return deq_.size();
}

template<typename T>
void BlockQueue<T>::flush(){
	condConsumer_.notify_one();
}

//#endif 

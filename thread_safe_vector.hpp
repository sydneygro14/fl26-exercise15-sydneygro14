// thread_safe_vector.hpp: starter code -- please refer to in-class example for ThreadSafeQueue

#ifndef THREAD_SAFE_VECTOR_HPP
#define THREAD_SAFE_VECTOR_HPP

#include <vector>
#include <mutex>
#include <condition_variable>
#include <utility>

template <typename T>
class ThreadSafeVector
{
public:
  // default constructor, the underlying vector starts empty
  ThreadSafeVector() = default;

  // copying is disabled because mutexes and condition variables cannot be copied
  ThreadSafeVector(const ThreadSafeVector&) = delete;
  ThreadSafeVector& operator=(const ThreadSafeVector&) = delete;

  // adds an item to the back of the vector and wakes one waiting consumer
  void push_back(const T& item)
  {
    {
      // lock only while modifying the vector, released at the end of this scope
      std::lock_guard<std::mutex> lock(mutex_);
      data_.push_back(item);
    }

    // notifies after unlocking so the woken thread can grab the mutex right away
    cond_.notify_one();
  }

  // blocks until the vector has an item, then removes the last item and stores it in value
  void wait_and_pop_back(T& value)
  {
    // unique_lock is required because the condition variable has to unlock while waiting
    std::unique_lock<std::mutex> lock(mutex_);

    // the predicate form of wait guards against spurious wakeups and stolen items
    cond_.wait(lock, [this]{ return !data_.empty(); });

    // moves the last item out to the caller and shrinks the vector by one
    value = std::move(data_.back());
    data_.pop_back();
  }

  // removes the last item without blocking, returns false if the vector was empty
  bool try_pop_back(T& value)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    // returns immediately instead of waiting when there is nothing to pop
    if (data_.empty()) {
      return false;
    }

    value = std::move(data_.back());
    data_.pop_back();
    return true;
  }

  // returns whether the vector is empty at the moment of the call
  bool empty() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.empty();
  }

  // returns how many items the vector holds at the moment of the call
  std::size_t size() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
  }

private:
  // the plain vector that holds the data, only touched while the mutex is held
  std::vector<T> data_;

  // mutable so the const member functions above are still allowed to lock it
  mutable std::mutex mutex_;

  // signals waiting consumers that an item has been added
  std::condition_variable cond_;
};

#endif

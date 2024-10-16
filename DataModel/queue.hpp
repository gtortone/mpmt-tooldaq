#ifndef QUEUE_HPP
#define QUEUE_HPP

// C++ implementation of thread-safe queue

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>

// Thread-safe queue
template <typename T>
class TSQueue {
private:
   // Underlying queue
   std::queue<T> m_queue;

   // mutex for thread synchronization
   std::mutex m_mutex;

   // Condition variable for signaling
   std::condition_variable m_cond;

public:
   // Pushes an element to the queue
   void push(T item)
   {

      // Acquire lock
      std::unique_lock<std::mutex> lock(m_mutex);

      // Add item
      m_queue.push(item);

      // Notify one thread that
      // is waiting
      m_cond.notify_one();
   }

   // Pops an element off the queue
   T pop()
   {
      // acquire lock
      std::unique_lock<std::mutex> lock(m_mutex);

      // wait until queue is not empty
      m_cond.wait(lock,
               [this]() { return !m_queue.empty(); });

      // retrieve item
      T item = m_queue.front();
      m_queue.pop();

      // return item
      return item;
   }

   T pop(bool *time_expired)
   {
      // acquire lock
      std::unique_lock<std::mutex> lock(m_mutex);

      m_cond.wait_for(lock, std::chrono::seconds(1), [this]() { return !m_queue.empty(); });

      if(!m_queue.empty()) {
         T item = m_queue.front();
         m_queue.pop();
         *time_expired = false;
         return item;
      }
         
      *time_expired = true;
      return T();
   }

   T front()
   {
      // acquire lock
      std::unique_lock<std::mutex> lock(m_mutex);

      // wait until queue is not empty
      m_cond.wait(lock,
               [this]() { return !m_queue.empty(); });

      return m_queue.front();      
   }

   int size() { return m_queue.size(); }

   void clear()
   {
      std::queue<T> empty;
      std::swap(m_queue, empty);
   }
};

#if 0

// Driver code
int main()
{
   TSQueue<int> q;

   // Push some data
   q.push(1);
   q.push(2);
   q.push(3);

   // Pop some data
   std::cout << q.pop() << std::endl;
   std::cout << q.pop() << std::endl;
   std::cout << q.pop() << std::endl;

   return 0;
}

#endif

#endif

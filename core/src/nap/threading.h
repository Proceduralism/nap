#pragma once


#include <functional>
#include <thread>
#include <vector>
#include <nap/blockingconcurrentqueue.h>

namespace nap {
    
    /**
     * Thread safe queue for tasks that are encapsulated in function objects.
     * It is possible to enqueue tasks at the end of the queue and to execute the tasks in the queue.
     */
    class TaskQueue {
    public:
        // a task in the queue is a function object
        using Task = std::function<void()>;
        
    public:
        // constructor takes maximum number of items that can be in the queue at a time
        TaskQueue(unsigned int maxQueueItems = 20) : queue(maxQueueItems) { }
        // add a task to the end of the queue
        void enqueue(Task task) { queue.enqueue(task); }
        
        /**
         * If the queue is empty, this function blocks until tasks are enqueued and executes them.
         * If the queue is not empty all the tasks are executed.
         */
        void processBlocking();
        // executes all tasks currently in the queue
        void process();
        
    private:
        moodycamel::BlockingConcurrentQueue<Task> queue;
        Task nextTask = nullptr;
    };
    
    
    /**
     * A single thread that runs its own task queue
     */
    class WorkerThread {
    public:
        WorkerThread(unsigned int maxQueueItems = 20);
        ~WorkerThread();
        
        // enqueues a task to be performed on this thread
        void execute(TaskQueue::Task task) { taskQueue.enqueue(task); }
        
    private:
        std::unique_ptr<std::thread> thread = nullptr;
        std::atomic<bool> stop;
        TaskQueue taskQueue;
    };
    
    
    /**
     * A pool of threads that can be used to perform multiple tasks at the same time
     */
    class ThreadPool {
    public:
        ThreadPool(unsigned int numberOfThreads = 1, unsigned int maxQueueItems = 20);
        ~ThreadPool();
        
        // enqueues a task to be performed on the next idle thread
        void execute(TaskQueue::Task task) { taskQueue.enqueue(task); }        
        
        // sets stopping to true and joins and exits all threads in the pool
        void shutDown();
        
        // resizes the number of threads in the pool, joins and exits all existing threads first!
        void resize(int numberOfThreads);
        
        // returns the number ot threads in the pool
        int getThreadCount() const { return int(threads.size()); }
        
        // returns wether this thread is shutting down
        bool isStopping() const { return (stop == true); }
        
    private:
        void addThread();
        
        std::vector<std::thread> threads;
        std::atomic<bool> stop;
        TaskQueue taskQueue;
    };
    
    
}
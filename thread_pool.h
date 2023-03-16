/**
 * @author jess@mediaexcel.com
 * @version 1.0
 * @attention When the queue size of a thread pool created with the 
 * 'CreateThreadPool' function reaches its maximum capacity, 
 * the 'EnqueueTask' method may block the calling thread 
 * until a worker thread becomes available to execute the queued task.
*/
#pragma once

typedef void* SyncObject;
typedef void* ThreadPool;

#if __cplusplus
extern "C" {
#endif
/**
 * @brief create thread pool
 * @param num_threads number of threads
 * @param max_queue_size maximum task queue size
 * @return thread pool instance
*/
ThreadPool CreateThreadPool(int num_threads, int max_queue_size);

/**
 * @brief destroy thread pool instance
*/
void DestroyThreadPool(ThreadPool pool);

/**
 * @brief push new task to the thread pool
 * @param pool thread pool instance
 * @param function function pointer to be called
 * @param arg function argument
 * 
 * @attention (IMPORTANT!!) To avoid deadlock, you must call 'WaitForSyncObject' with its return value.
*/
SyncObject EnqueueTask(ThreadPool pool, void (*function)(void *), void *arg);

/**
 * @brief block until task is done
 * @param obj sync object which is fetched from 'EnqueuTask'
*/
void WaitForSyncObject(SyncObject obj);

#if __cplusplus
}
#endif

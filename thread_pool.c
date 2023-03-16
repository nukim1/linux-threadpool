#include "thread_pool.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

typedef struct {
  sem_t mutex;
  sem_t *empty;
} sync_object;

typedef struct {
  void (*function)(void *);
  void *arg;
} thread_task;

typedef struct {
  thread_task *tasks;
  int head, tail, count, size;
  pthread_mutex_t mutex;
  sem_t sem_empty;
  sem_t sem_full;
  bool shutdown;
  pthread_t *threads;
  int num_threads;
  sync_object *sync_objects;
} thread_pool;

static inline bool __block_on_semaphore(sem_t* sem) 
{
  int ret;
  do { ret = sem_wait(sem); } while (ret == -1 && errno == EINTR);
  return ret ? false : true;
}

static inline bool __post_semaphore(sem_t* sem)
{
  return sem_post(sem) ? false : true;
}

static void *worker(void *arg)
{
  thread_pool *pool = (thread_pool *)arg;

  while (true)
  {
    __block_on_semaphore(&pool->sem_full);
    pthread_mutex_lock(&pool->mutex);

    if (pool->shutdown)
    {
      pthread_mutex_unlock(&pool->mutex);
      pthread_exit(NULL);
    }

    int qidx = pool->head;
    thread_task task = pool->tasks[qidx];
    pool->head = (pool->head + 1) % pool->size;
    pool->count--;
    pthread_mutex_unlock(&pool->mutex);

    task.function(task.arg);

    sync_object *sync_object = &pool->sync_objects[qidx];
    __post_semaphore(&sync_object->mutex);
    // __post_semaphore(&pool->sem_empty); it is shifted to the WaitForSyncObject
  }

  return NULL;
}

extern ThreadPool CreateThreadPool(int num_threads, int max_queue_size)
{
  thread_pool *pool = (thread_pool *)malloc(sizeof(thread_pool));
  pool->tasks = (thread_task *)malloc(sizeof(thread_task) * max_queue_size);
  pool->head = 0;
  pool->tail = 0;
  pool->count = 0;
  pool->size = max_queue_size;
  pthread_mutex_init(&pool->mutex, NULL);
  sem_init(&pool->sem_empty, 0, max_queue_size);
  sem_init(&pool->sem_full, 0, 0);
  pool->shutdown = false;
  pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
  pool->num_threads = num_threads;
  pool->sync_objects = (sync_object *)malloc(sizeof(sync_object) * max_queue_size);

  for (int i = 0; i < max_queue_size; i++)
  {
    sem_init(&pool->sync_objects[i].mutex, 0, 0);
    pool->sync_objects[i].empty = &pool->sem_empty;
  }

  for (int i = 0; i < num_threads; i++)
  {
    pthread_create(&pool->threads[i], NULL, worker, pool);
  }

  return (ThreadPool)pool;
}

extern void DestroyThreadPool(ThreadPool _pool)
{
  thread_pool *pool = (thread_pool*)_pool;
  pool->shutdown = true;

  for(int i = 0; i < pool->num_threads; i++)
  {
    sem_post(&pool->sem_full);
  }
  for (int i = 0; i < pool->num_threads; i++)
  {
    pthread_join(pool->threads[i], NULL);
  }

  free(pool->threads);
  free(pool->tasks);

  for (int i = 0; i < pool->size; i++)
  {
    sem_destroy(&pool->sync_objects[i].mutex);
  }

  free(pool->sync_objects);

  pthread_mutex_destroy(&pool->mutex);
  sem_destroy(&pool->sem_full);
  sem_destroy(&pool->sem_empty);

  free(pool);
}

extern SyncObject EnqueueTask(ThreadPool _pool, void (*function)(void *), void *arg)
{
  thread_pool *pool = (thread_pool*)_pool;

  __block_on_semaphore(&pool->sem_empty);
  pthread_mutex_lock(&pool->mutex);

  if (pool->shutdown)
  {
    pthread_mutex_unlock(&pool->mutex);
    return (SyncObject)NULL;
  }

  int qidx = pool->tail;
  pool->tasks[qidx].function = function;
  pool->tasks[qidx].arg = arg;
  pool->tail = (pool->tail + 1) % pool->size;
  pool->count++;
  pthread_mutex_unlock(&pool->mutex);

  sem_post(&pool->sem_full);
  return (SyncObject)(&pool->sync_objects[qidx]);
}

extern void WaitForSyncObject(SyncObject obj)
{
  sync_object* sync_obj = (sync_object*)obj;

  if (obj != NULL)
  {
    __block_on_semaphore(&sync_obj->mutex);
    __post_semaphore(sync_obj->empty);
  }
}


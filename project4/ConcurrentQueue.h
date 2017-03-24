#ifndef CONCURRENT_QUEUE
#define CONCURRENT_QUEUE

#include <queue>
#include <pthread.h>

template <class T>
class ConcurrentQueue {

    public:
        ConcurrentQueue() {
            pthread_mutex_init(&m, NULL);
            pthread_cond_init(&c, NULL);
        }
        void enqueue(T data) {
            pthread_mutex_lock(&m);
            q.push(data);
            pthread_cond_broadcast(&c);
            pthread_mutex_unlock(&m);
        }
        T dequeue() {
            pthread_mutex_lock(&m);
            while(q.empty()) {
                pthread_cond_wait(&c, &m);
            }
            T data = q.front();
            q.pop();
            pthread_mutex_unlock(&m);
            return data;
        }
        bool empty() {
            pthread_mutex_lock(&m);
            bool e = q.empty();
            pthread_mutex_unlock(&m);
            return e;
        }
        size_t getSize() {
            pthread_mutex_unlock(&m);
            size_t s = q.size();
            pthread_mutex_unlock(&m);
            return s;
        }

    private:
        std::queue<T> q;
        pthread_mutex_t m;
        pthread_cond_t c;
};
#endif
#ifndef CONCURRENT_QUEUE
#define CONCURRENT_QUEUE

#include <queue>
#include <pthread.h>

template <class T>
class ConcurrentQueue {

    public:
        bool volatile stop;

        ConcurrentQueue() {
            pthread_mutex_init(&m, NULL);
            pthread_cond_init(&c, NULL);
            stop = false;
        }
        void enqueue(T data) {
            pthread_mutex_lock(&m);
            q.push(data);
            pthread_cond_broadcast(&c);
            pthread_mutex_unlock(&m);
        }
        T dequeue() {
            pthread_mutex_lock(&m);
            while(q.empty() && !stop) {
                pthread_cond_wait(&c, &m);
            }
            T data = T();
            if(stop) {
                // if we want to stop, return a default value
                // instead of trying to get a value
                pthread_mutex_unlock(&m);
                return data;
            }
            data = q.front();
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
            pthread_mutex_lock(&m);
            size_t s = q.size();
            pthread_mutex_unlock(&m);
            return s;
        }
        void stopQueue() {
            pthread_mutex_lock(&m);
            stop = true;
            pthread_cond_broadcast(&c);
            pthread_mutex_unlock(&m);
        }

    private:
        std::queue<T> q;
        pthread_mutex_t m;
        pthread_cond_t c;
};
#endif
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
            size = 0;
        }
        void enqueue(T data) {
            pthread_mutex_lock(&m);
            q.push(data);
            size++;
            pthread_cond_signal(&c);
            pthread_mutex_unlock(&m);
        }
        T dequeue() {
            pthread_mutex_lock(&m);
            while(q.empty()) {
                pthread_cond_wait(&c, &m);
            }
            T data = q.front();
            q.pop();
            size--;
            pthread_mutex_unlock(&m);
            return data;
        }
        bool empty() {
            if(size > 0) return false;
            return true;
        }
        int getSize() {
            return size;
        }

    private:
        std::queue<T> q;
        int size;
        pthread_mutex_t m;
        pthread_cond_t c;
};
#endif
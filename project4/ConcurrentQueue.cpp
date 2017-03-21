#include <queue>
#include <pthread.h>
#include "ConcurrentQueue.h"

using namespace std;

template <class T>
void ConcurrentQueue<T>::enqueue(T data) {
    pthread_mutex_lock(&m);
    q.push(data);
    pthread_cond_signal(&c);
    pthread_mutex_unlock(&m);
}

template <class T>
T ConcurrentQueue<T>::dequeue() {
    pthread_mutex_lock(&m);
    while(q.empty()) {
        pthread_cond_wait(&c, &m);
    }
    T data = q.front();
    q.pop();
    pthread_mutex_unlock(&m);
    return data;
}
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
<< endl; exit(EXIT_FAILURE); }

const int DATA_COUNT = 1000000;

int data[DATA_COUNT];

bool ready = false;

pthread_mutex_t mutex; // Мьютекс

void cond()
{
    int err;
    bool flag = false;
    
    while(!flag) 
    {
        err = pthread_mutex_lock(&mutex);
        if(err != 0)
            err_exit(err, "Cannot lock mutex");
        
        flag = ready;

        err = pthread_mutex_unlock(&mutex);
        if(err != 0)
            err_exit(err, "Cannot unlock mutex");
    }       
}

void *thread_job1(void *arg)
{
    cond();

    int sum = 0;

    for(int i = 0; i < DATA_COUNT; ++i)
    {
        sum += data[i];
    }

    cout << "Sum: " << sum << '\n';
}

void *thread_job2(void *arg)
{
    cond();
    
    int diff = 0;

    for(int i = 0; i < DATA_COUNT; ++i)
    {
        diff -= data[i];
    }

    cout << "Diff: " << diff << '\n';
}

void *thread_job3(void *arg)
{
    int err;

    for(int i = 0; i < DATA_COUNT; ++i)
    {
        data[i] = rand() % 1000 + 1;
    }

    err = pthread_mutex_lock(&mutex);
    if(err != 0)
        err_exit(err, "Cannot lock mutex");
    
    ready = true;

    err = pthread_mutex_unlock(&mutex);
    if(err != 0)
        err_exit(err, "Cannot unlock mutex");
}

int main()
{
    srand(time(NULL));
    pthread_t thread1, thread2, thread3; // Идентификаторы потоков
    int err; // Код ошибки

    // Инициализируем мьютекс
    err = pthread_mutex_init(&mutex, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize mutex");

    // Создаём потоки
    err = pthread_create(&thread1, NULL, thread_job1, NULL);
    if(err != 0)
        err_exit(err, "Cannot create thread 1");

    err = pthread_create(&thread2, NULL, thread_job2, NULL);
    if(err != 0)
        err_exit(err, "Cannot create thread 2");

    err = pthread_create(&thread3, NULL, thread_job3, NULL);
    if(err != 0)
        err_exit(err, "Cannot create thread 3");

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    // Освобождаем ресурсы, связанные с мьютексом
    pthread_mutex_destroy(&mutex);
}
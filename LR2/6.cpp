#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <pthread.h>

using namespace std;

const int SIZE_STORAGE = 4;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
<< endl; exit(EXIT_FAILURE); }

enum store_state
{
    EMPTY, FULL
};

auto state = store_state::EMPTY;

pthread_mutex_t mutex;
pthread_cond_t cond1, cond2;

int store;

void* producer(void* arg)
{
    int err;

    while (true)
    {
        // Захватываем мьютекс и ожидаем освобождения склада
        err = pthread_mutex_lock(&mutex);
        if (err != 0)
            err_exit(err, "Cannot lock mutex");

        while (state == FULL)
        {
            err = pthread_cond_wait(&cond2, &mutex);
            if (err != 0)
                err_exit(err, "Cannot wait on condition variable");
        }
        // Получен сигнал, что на складе не осталось товаров.

        // Производим новый товар.
        store = SIZE_STORAGE;
        cout << "Склад пополнен" << endl;
        state = FULL;

        // Посылаем сигнал, что на складе появился товар.
        err = pthread_cond_signal(&cond1);
        if (err != 0)
            err_exit(err, "Cannot send signal");

        err = pthread_mutex_unlock(&mutex);
        if (err != 0)
            err_exit(err, "Cannot unlock mutex");
    }
}

void* consumer(void* arg)
{
    int err;
    //
    while (true)
    {
        // Захватываем мьютекс и ожидаем появления товаров на складе
        err = pthread_mutex_lock(&mutex);
        if (err != 0)
            err_exit(err, "Cannot lock mutex");

        while (state == EMPTY)
        {
            err = pthread_cond_wait(&cond1, &mutex);
            if (err != 0)
                err_exit(err, "Cannot wait on condition variable");
        }
        // Получен сигнал, что на складе имеется товар.

        // Потребляем его.
        cout << "Consuming number " << pthread_self() << "..." << endl;
        store -= 1;

        if (store <= 0)
        {
            state = EMPTY;

            // Посылаем сигнал, что на складе не осталось товаров.
            err = pthread_cond_signal(&cond2);
            if (err != 0)
                err_exit(err, "Cannot send signal");
        }

        err = pthread_mutex_unlock(&mutex);
        if (err != 0)
            err_exit(err, "Cannot unlock mutex");

        sleep(1);
    }
}

int main()
{
    int err;
    //
    pthread_t thread_producer, thread_consumer_1, thread_consumer_2, thread_consumer_3;
    // Идентификаторы потоков
    // Инициализируем мьютекс и условную переменную
    err = pthread_cond_init(&cond1, NULL);
    if (err != 0)
        err_exit(err, "Cannot initialize condition variable");

    err = pthread_cond_init(&cond2, NULL);
    if (err != 0)
        err_exit(err, "Cannot initialize condition variable");

    err = pthread_mutex_init(&mutex, NULL);
    if (err != 0)
        err_exit(err, "Cannot initialize mutex");

    // Создаём потоки
    err = pthread_create(&thread_producer, NULL, producer, NULL);
    if (err != 0)
        err_exit(err, "Cannot create thread producer");

    err = pthread_create(&thread_consumer_1, NULL, consumer, NULL);
    if (err != 0)
        err_exit(err, "Cannot create thread consumer 1");

    err = pthread_create(&thread_consumer_2, NULL, consumer, NULL);
    if (err != 0)
        err_exit(err, "Cannot create thread consumer 2");

    err = pthread_create(&thread_consumer_3, NULL, consumer, NULL);
    if (err != 0)
        err_exit(err, "Cannot create thread consumer 3");

    // Дожидаемся завершения потоков
    pthread_join(thread_producer, NULL);
    pthread_join(thread_consumer_1, NULL);
    pthread_join(thread_consumer_2, NULL);
    pthread_join(thread_consumer_3, NULL);

    // Освобождаем ресурсы, связанные с мьютексом
    // и условной переменной
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond1);
    pthread_cond_destroy(&cond2);
}

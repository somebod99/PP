#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
 << endl; exit(EXIT_FAILURE); }

enum store_state {EMPTY, FULL};
store_state state = store_state::FULL;

pthread_mutex_t mutex;

int store;

bool ready = false;

void cond()
{   
    int err;

    err = pthread_mutex_trylock(&mutex);
    if (err == 0)
    {
        ready = false;

        while(!ready) {} 

        err = pthread_mutex_unlock(&mutex);
        if(err != 0)
            err_exit(err, "Cannot unlock mutex"); 
    }    
}

void *producer(void *arg)
{
 int err;
 //
 while(true) 
    {
        cout << "Start Producer thread\n";
        cond();
        cout << "End Producer thread\n";

        cout << "Ready start Producer thread\n";
        ready = true;
        cout << "Ready end Producer thread\n";
    }

}
void *consumer(void *arg)
{
    while(true) 
    {
        cout << "Start Consumer thread\n";
        cond();
        // Получен сигнал, что на складе имеется товар.
        // Потребляем его.
        cout << "End Consumer thread\n";
        // Посылаем сигнал, что на складе не осталось товаров.
        cout << "Ready start Consumer thread\n";
        ready = true;
        sleep(1);
        cout << "Ready end Consumer thread\n";
    }
}

int main()
{
    int err;
    //
    pthread_t thread1, thread2; // Идентификаторы потоков
    
    err = pthread_mutex_init(&mutex, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize mutex");
    // Создаём потоки
    err = pthread_create(&thread1, NULL, producer, NULL);
    if(err != 0)
        err_exit(err, "Cannot create thread 1");
    sleep(1);
    err = pthread_create(&thread2, NULL, consumer, NULL);
    if(err != 0)
        err_exit(err, "Cannot create thread 2");
    // Дожидаемся завершения потоков
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    // Освобождаем ресурсы, связанные с мьютексом
    // и условной переменной
    pthread_mutex_destroy(&mutex);
} 
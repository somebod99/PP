#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>

using namespace std;

/* Функция, которую будет исполнять созданный поток */
void *thread_job(void *arg)
{
    cout << "Thread is running..." << '\n';

    pthread_t thread = pthread_self();
    pthread_attr_t attr;

    int err;
    err = pthread_getattr_np(thread, &attr);

    if(err != 0)
    {
        cout << "Cannot get thread attribute: " << strerror(err) << endl;
        exit(-1);
    }

    size_t stack_size;
    err = pthread_attr_getstacksize(&attr, &stack_size);

    if(err != 0)
    {
        cout << "Cannot get satck size: " << strerror(err) << endl;
        exit(-1);
    }

    cout << "Stack size: " << stack_size << '\n';

    cout << "Thread is finishing..." << '\n';
}

int main()
{
    // Определяем переменные: идентификатор потока и код ошибки
    pthread_t thread;
    pthread_attr_t thread_attr;
    int err;
    // Инициализируем переменную для хранения атрибутов потока
    err = pthread_attr_init(&thread_attr);
    // Если при инициализации переменной атрибутов произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(err != 0)
    {
        cout << "Cannot create thread attribute: " << strerror(err) << endl;
        exit(-1);
    }

    // Устанавливаем минимальный размер стека для потока (в байтах)
    err = pthread_attr_setstacksize(&thread_attr, 5*1024*1024);
    // Если при установке размера стека произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(err != 0)
    {
        cout << "Setting stack size attribute failed: " << strerror(err) << endl;
        exit(-1);
    }

    // Создаём поток
    err = pthread_create(&thread, &thread_attr, thread_job, NULL);
    // Если при создании потока произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(err != 0)
    {
        cout << "Cannot create a thread: " << strerror(err) << endl;
        exit(-1);
    }

    // Освобождаем память, занятую для хранения атрибутов потока
    pthread_attr_destroy(&thread_attr);
    
    // Ожидаем завершения созданного потока перед завершением
    // работы программы
    pthread_exit(NULL);
}
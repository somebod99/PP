#include <cstdlib>
#include <iostream>
#include <cmath>
#include <cstring>
#include <pthread.h>

using namespace std;

typedef double (*func_ptr_type)(double);

// Структура для передачи параметров в функцию потока
struct Param
{
    func_ptr_type func_ptr;
    double* array_ptr;
    int count_elements;
};

/* Функция, которую будет исполнять созданный поток */
void *thread_job(void *arg)
{
    Param* params = (Param*)arg;

    for(int i = 0; i < params->count_elements; ++i)
    {
        params->array_ptr[i] = params->func_ptr(params->array_ptr[i]);
    }
}

void print_array(double* array, const int size)
{
    for(int i = 0; i < size; ++i)
    {
        cout << "Array[" << i << "] = " << array[i] << '\n';
    }
}

func_ptr_type get_func(const int func)
{
    switch(func)
    {
    case 0:
        return &exp;
    case 1:
        return &sqrt;
    case 2:
        return &round;
    case 3:
        return &cos;
    case 4:
        return &sin;
    case 5:
        return &log2;
    case 6:
        return &abs;
    default:
        return NULL;
    }
}

void print_menu()
{
    cout << "\nEnter function to apply to an array:\n";
    cout << "0 - exp"   << '\n';
    cout << "1 - sqrt"  << '\n';
    cout << "2 - round" << '\n';
    cout << "3 - cos"   << '\n';
    cout << "4 - sin"   << '\n';
    cout << "5 - log2"  << '\n';
    cout << "6 - abs"   << '\n';
    cout << "(0-6): ";
}

int init_threads(Param* const params, pthread_t* const threads, double* const array, const int array_size, const int count_threads, func_ptr_type const func_ptr)
{
    // Количество элементов, обрабатываемых одним потоком
    int count_elements = array_size / count_threads;

    int err;

    for(int i = 0; i < count_threads; ++i)
    {
        params[i].func_ptr = func_ptr;
        params[i].count_elements = count_elements;
        params[i].array_ptr = &array[count_elements * i];

        // Добавление количества обрабатываемых элементов последнему потоку
        // при нецелом делении количества элементов во все массиве на количество потоков
        if(i == count_threads - 1)
        {
            params[i].count_elements += array_size % count_threads;  
        }

        // Создание потока
        err = pthread_create(&threads[i], NULL, thread_job, (void*)&params[i]);

        // Если при создании потока произошла ошибка
        if(err != 0)
        {
            return -1;
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        cout << "Wrong count arguments\n";
        exit(-1);
    }

    int count_threads = atoi(argv[1]);
    int array_size = atoi(argv[2]);

    if(array_size <= 0 || count_threads <= 0)
    {
        cout << "Wrong array size or count threads\n";
        exit(-1);
    }

    if(array_size > count_threads)
    {
        count_threads = array_size;
    }

    print_menu();

    char c_func;
    cin >> c_func;

    // Указатель на функцию
    func_ptr_type func_ptr = get_func(c_func - '0');

    if(!func_ptr)
    {
        cout << "Unexpected function\n";
        exit(-1);
    }

    srand(time(NULL));

    double* array = new double[array_size];

    for(int i = 0; i < array_size; ++i)
    {
        array[i] = (rand() % 1001) / 10.0 - 50.0;
    }
    
    cout << "\nArray before changing:\n";
    print_array(array, array_size);

    // Массив аргументов, которые будут переданы в функции потоков
    Param* params = new Param[count_threads];
    // Массив потоков
    pthread_t* threads = new pthread_t[count_threads];

    // Создание и инициализация потоков
    int err = init_threads(params, threads, array, array_size, count_threads, func_ptr);

    if(err != 0)
    {
        cout << "Cannot create a thread: " << strerror(err) << endl;
        exit(-1);
    }

    // Ждем завершения всех созданных потоков
    for(int i = 0; i < count_threads; ++i)
    {
        pthread_join(threads[i], NULL);
    }
    
    cout << "\nArray after changing:\n";
    print_array(array, array_size);

    cout << '\n';

    delete[] params;
    delete[] array;
    delete[] threads;
}
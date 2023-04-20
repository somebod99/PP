#include <omp.h>
#include <iostream>
#include <cmath>
#include <chrono>

#define _USE_MATH_DEFINES

using namespace std;

int f(int b)
{
    double sum = 0.0;
    for(int i = 0; i < 1e6; ++i)
    {
        sum += pow(sin(sum / M_PI), 3) + pow(sum, 10) * cos(M_PI * sum);
    }

    return b;
}

int main()
{
    int a[100], b[100];

    // Инициализация массива b
    for(int i = 0; i < 100; i++)
        b[i] = i;
    
    // Директива OpenMP для распараллеливания цикла

    auto begin = chrono::steady_clock::now();

    #pragma omp parallel for
    for(int i = 0; i < 100; i++)
    {
        a[i] = f(b[i]);
        b[i] = 2 * a[i];
    }

    int result = 0;
    
    // Далее значения a[i] и b[i] используются, например, так:
    #pragma omp parallel for reduction(+ : result)
    for(int i = 0; i<100; i++)
        result += (a[i] + b[i]);

    auto end = chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    
    cout << "Result = " << result << '\n';
    cout << "Time: " << elapsed_ms.count() << '\n';
    //
    return 0;
}
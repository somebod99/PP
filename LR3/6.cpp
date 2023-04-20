#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <omp.h>
#include <cmath>

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>

#include <chrono>

#include "CImg.h"

#define PI 3.14159265

using namespace cimg_library;
using namespace std;

typedef vector<float> Array;
typedef vector<Array> Matrix;


Matrix getGaussKernel(int size, double sigma)
{
    Matrix kernel(size, Array(size));

    double sum = 0.0;

    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            kernel[i][j] = exp(-(i * i + j * j) / (2 * sigma * sigma)) /
                (2 * PI * sigma * sigma);
            sum += kernel[i][j];
        }
    }

    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) kernel[i][j] /= sum;

    return kernel;
}

void applyFilter(Matrix& filter)
{
    CImg<float> img("C:\\Users\\gukas\\Desktop\\wolf.jpg");

    CImg<float> endImg(img.width(), img.height(), 1, 3);

    endImg.fill(0);

    int height = img.height();
    int width = img.width();

    int filterHeight = filter.size();
    int filterWidth = filter[0].size();

    int newImageHeight = height - filterHeight + 1;
    int newImageWidth = width - filterWidth + 1;

    
    auto begin = std::chrono::steady_clock::now();

    omp_set_num_threads(25);

    //#pragma omp parallel for
    for (int d = 0; d < 3; d++)
    {
        int id = omp_get_thread_num();
        cout << "ID: " << id << endl;
        #pragma omp parallel for
        for (int i = 0; i < newImageWidth; i++)
        {
            for (int j = 0; j < newImageHeight; j++)
            {
                for (int w = i; w < i + filterWidth; w++)
                {
                    for (int h = j; h < j + filterHeight; h++)
                    {
                        endImg(i, j, 0, d) += filter[w - i][h - j] * img(w, h, 0, d);
                    }
                }
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    std::cout << "The time: " << elapsed_ms.count() << " ms\n";
    
    endImg.display("Gaussian blur");
}
int main(int argc, char* argv[])
{
    int radius = 50;
    float sigma = 10;

    Matrix filter = getGaussKernel(radius, sigma);

    applyFilter(filter);

    return 0;
}

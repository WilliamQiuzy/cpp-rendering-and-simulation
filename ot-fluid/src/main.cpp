#define _CRT_SECURE_NO_WARNINGS

#include "Vector.h"

#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <memory>
#include <unistd.h>
#include "Voronoi.h"
#include "OptimalTransport.h"
#include "Fluid.h"

int main() {
    Fluid fluid(50);
    std::cout << "start computing" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    fluid.compute();

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "end computing" << std::endl;
    std::cout << "Elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    return 0;

}

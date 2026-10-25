#ifndef FLUID_H
#define FLUID_H

#include "OptimalTransport.h"

class Fluid {
public:
    Fluid() {}
    Fluid(int N);

    void compute();

private:
    std::vector<Vector> particles_;
    std::vector<Vector> velocities_;
    
    int N;

    void initialize(int N);

};

#endif // FLUID_H
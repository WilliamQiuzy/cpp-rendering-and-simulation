# include "Fluid.h"

#include <iostream>
#include <fstream>
#include <filesystem>


Fluid::Fluid(int N) {
    initialize(N);
}

void Fluid::initialize(int N) {
    particles_.resize(N);
    velocities_.resize(N);
    for (int i = 0; i < N; ++i) {
        particles_[i] = Vector(uniform_gen(engine), uniform_gen(engine), 0);
        velocities_[i] = Vector(0, 0, 0);
    }
    return ;
}


void Fluid::compute() {
    const int totalFrames = 400;
    const double mass = 200;
    const double epsilon = 0.004 * 0.004;
    const double timeStep = 0.003;
    const Vector gravity(0, -9.81);

    int particleCount = particles_.size();
    std::filesystem::create_directories("frames");

    for (int frame = 0; frame < totalFrames; ++frame) {
        std::vector<double> initW(particleCount, 1.0);
        OptimalTransport solver(particles_, initW);

        int res = solver.compute_fluid();

        std::vector<Polygon> voronoiDiagram = solver.getSolution().voronoi;
        save_frame(voronoiDiagram, "frames/animation", frame);
        std::cout << "frame output: " << frame << std::endl;


        for (int i = 0; i < particleCount; ++i) {
            Vector springForce;
            if (res==0) {
                springForce = (voronoiDiagram[i].computeCentroid() - particles_[i]) / epsilon;
            } else {
                springForce = Vector(0, 0, 0);
            }
            
            Vector totalForce = springForce + gravity * mass;
            velocities_[i] = velocities_[i] +  timeStep * totalForce / mass;
            particles_[i] = particles_[i] +  timeStep * velocities_[i];

            if (particles_[i][0] < 0) {
                particles_[i][0] = -particles_[i][0];
                velocities_[i][0] = 0;
            }
            if (particles_[i][1] < 0) {
                particles_[i][1] = -particles_[i][1];
                velocities_[i][1] = 0;
            }
            if (particles_[i][0] >= 1) {
                particles_[i][0] = 2 - particles_[i][0];
                velocities_[i][0] = 0;
            }
            if (particles_[i][1] >= 1) {
                particles_[i][1] = 2 - particles_[i][1];
                velocities_[i][1] = 0;
            }
        }
    }
}

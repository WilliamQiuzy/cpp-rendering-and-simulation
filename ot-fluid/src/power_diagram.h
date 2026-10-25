#ifndef POWERDIAGRAM_H
#define POWERDIAGRAM_H

#include "Voronoi.h"

class PowerDiagram : public Voronoi {
public:
    PowerDiagram(std::vector<Vector> points, std::vector<double> weights) : Voronoi(points), weights(weights) {}
    void compute();
    void compute_fluid();
    std::vector<double> weights;
    Polygon clip_circle(Polygon& poly,Polygon& clip) const;
    Polygon clip_fluid(Polygon& cell, int i, int j, const Vector* points, const double* weights);


protected:
    Polygon clipPolygon(const Polygon& poly, const Vector& P0, const Vector& Pi, int i0, int ii) const;
    Polygon compute_voronoi_cell(int idx) const;
    Polygon compute_voronoi_cell_fluid(int idx) ;


};

#endif // POWERDIAGRAM_H
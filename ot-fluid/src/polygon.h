#pragma once
#include <vector>
#include "Vector.h"

#ifndef POLYGON_H
#define POLYGON_H


class Polygon {  
public:
    std::vector<Vector> vertices;
    double computeArea() const;
    double computeSquaredDistance(const Vector& point) const;
    Vector computeCentroid() const;

};  

#endif // POLYGON_H
 
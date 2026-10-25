#include "Polygon.h"

double Polygon::computeArea() const {
    double area = 0.0;
    int num = vertices.size();

    if (num < 2) {
        return 0;
    }

    for (int i = 0; i < num; ++i) {
        int next = (i + 1) % num;
        double x1 = vertices[i][0];
        double y1 = vertices[i][1];

        double x2 = vertices[next][0];
        double y2 = vertices[next][1];
        
        area += (x1 * y2) - (x2 * y1);
    }

    return std::abs(area / 2.0);
}



double Polygon::computeSquaredDistance(const Vector& point) const {
    if (vertices.size() < 3) {return 0;}

    double totalValue = 0;

    for (size_t i = 1; i < vertices.size() - 1; ++i) {
        Vector vertex0 = vertices[0];
        Vector vertex1 = vertices[i];
        Vector vertex2 = vertices[i + 1];

        Vector triangle[3] = { vertex0, vertex1, vertex2 };

        double triangleValue = 0;

        for (int k = 0; k < 3; ++k) {
            for (int l = k; l < 3; ++l) {
                Vector diffK = triangle[k] - point;
                Vector diffL = triangle[l] - point;
                triangleValue += dot(diffK, diffL);
            }
        }

        Vector edge1 = vertex1 - vertex0;
        Vector edge2 = vertex2 - vertex0;
        double area = 0.5 * std::abs(edge1[1] * edge2[0] - edge1[0] * edge2[1]);

        totalValue += (triangleValue / 6) * area;
    }

    return totalValue;
}


Vector Polygon::computeCentroid() const {
    Vector centroid(0, 0, 0);
    double area = computeArea();
    int numVertices = vertices.size();

    if (area == 0) {
        if (numVertices != 0) {
            return vertices[0];
        } else {
            return Vector(0, 0, 0);
        }
    }

    for (int i = 0; i < numVertices; ++i) {
        int nextIndex = (i + 1) % numVertices;
        centroid =  centroid + (vertices[i] + vertices[nextIndex]) * cross2d(vertices[i], vertices[nextIndex]);
    }

    return centroid / (-6 * area);
}
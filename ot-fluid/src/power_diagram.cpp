#include "PowerDiagram.h"

Polygon PowerDiagram::clipPolygon(const Polygon& poly, const Vector& P0, const Vector& Pi, int i0, int ii) const {
    Polygon res;
    Vector M = (P0 + Pi) / 2;
    Vector diff = Pi - P0;
    Vector Mprime = M + (weights[i0] - weights[ii]) * diff / (2 * diff.norm2());

    for (size_t i = 0; i < poly.vertices.size(); ++i) {
        Vector A;
        if (i==0) {
            A = poly.vertices.back();
        } else {
            A = poly.vertices[i - 1];
        }

        Vector B = poly.vertices[i];
        double t = dot(Mprime - A, Pi - P0) / dot(B - A, Pi - P0);
        Vector P = A + t * (B - A);

        double distA_P0 = (A - P0).norm2() - weights[i0];
        double distA_Pi = (A - Pi).norm2() - weights[ii];
        double distB_P0 = (B - P0).norm2() - weights[i0];
        double distB_Pi = (B - Pi).norm2() - weights[ii];

        if (distB_P0 < distB_Pi) {
            if (distA_P0 > distA_Pi) {
                res.vertices.push_back(P);
            }
            res.vertices.push_back(B);
        } else if (distA_P0 < distA_Pi) {
            res.vertices.push_back(P);
        }

    }
    return res;
}


Polygon PowerDiagram::clip_fluid(Polygon& cell, int i, int j, const Vector* points, const double* weights) {
    Polygon clippedPolygon;
    int num = cell.vertices.size();

    for (int k = 0; k < num; ++k) {
        Vector A;
        if (k-1 >= 0) {
            A = cell.vertices[k - 1];
        }  else {
            A = cell.vertices[num - 1];
        }
        Vector B = cell.vertices[k];

        Vector midPoint = (points[i] + points[j]) / 2;
        midPoint = midPoint + (weights[i] - weights[j]) / (2 * (points[i] - points[j]).norm2()) * (points[j] - points[i]);

        double t = dot(midPoint - A, points[j] - points[i]) / dot(B - A, points[j] - points[i]);
        Vector interP = A + t * (B - A);

        bool c2 = ((B - points[i]).norm2() - weights[i]) <= ((B - points[j]).norm2() - weights[j]);
        bool c1 = ((A - points[i]).norm2() - weights[i]) > ((A - points[j]).norm2() - weights[j]);

        if (c2) {
            if (c1) {
                clippedPolygon.vertices.push_back(interP);
            }
            clippedPolygon.vertices.push_back(B);
        } else if (!c2 && !c1) {
            clippedPolygon.vertices.push_back(interP);
        }
    }

    return clippedPolygon;
}

Polygon PowerDiagram::compute_voronoi_cell(int idx) const {
    Polygon cell;
    cell.vertices = {
        Vector(0, 0, 0),
        Vector(1, 0, 0),
        Vector(1, 1, 0),
        Vector(0, 1, 0)
    };

    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        if (i != idx) {
            cell = clipPolygon(cell, points[idx], points[i], idx, i);
        }
    }
    return cell;
}

Polygon PowerDiagram::clip_circle(Polygon& poly, Polygon& clip) const {
    Polygon res = poly;
    int clipSize = clip.vertices.size();

    for (int i = 0; i < clipSize; ++i) {
        Vector p1 = clip.vertices[i];
        Vector p2 = clip.vertices[(i + 1) % clipSize];
        Vector u = p1;
        Vector n(p2[1] - p1[1], p1[0] - p2[0]);
        
        int resSize = res.vertices.size();
        Polygon temp;

        for (int k = 0; k < resSize; ++k) {
            Vector A;
            if (k-1>=0) {
                A = res.vertices[k - 1];
            } else {
                A = res.vertices[resSize - 1];
            }
            Vector B = res.vertices[k];

            double t = dot(u - A, n) / dot(B - A, n);
            Vector p = A + t * (B - A);

            if (dot(u - B, n) <= 0) {
                if (dot(u - A, n) > 0) {
                    temp.vertices.push_back(p);
                }
                temp.vertices.push_back(B);
            } else if (dot(u - A, n) <= 0) {
                temp.vertices.push_back(p);
            }
        }
        res = temp;
    }
    return res;
}

Polygon PowerDiagram::compute_voronoi_cell_fluid(int idx) {
    Polygon cell;
    cell.vertices = {
        Vector(0, 0, 0),
        Vector(0, 1, 0),
        Vector(1, 1, 0),
        Vector(1, 0, 0)
    };
    // std::cout << "weights: " << weights[idx] << " " << weights[points.size()] << "\n ";

    for (size_t i = 0; i < points.size() + 1; ++i) {
        cell = clip_fluid(cell, idx, i, points.data(), weights.data());
    }

    Polygon disk;
    int numVertices = 200;
    disk.vertices.resize(numVertices);

    double radius = sqrt(weights[idx] - weights[points.size()]);

    for (int j = 0; j < numVertices; ++j) {
        double angle = j / static_cast<double>(numVertices) * 2 * M_PI;
        disk.vertices[j][0] = cos(angle) * radius + points[idx][0];
        disk.vertices[j][1] = -sin(angle) * radius + points[idx][1];
        disk.vertices[j][2] = 0;


    }

    cell = clip_circle(cell, disk);



    return cell;
}



void PowerDiagram::compute() {
    voronoi.resize(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        voronoi[i] = compute_voronoi_cell(i);
    }
}

void PowerDiagram::compute_fluid() {
    voronoi.resize(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        voronoi[i] = compute_voronoi_cell_fluid(i);
    }
    return ;
}
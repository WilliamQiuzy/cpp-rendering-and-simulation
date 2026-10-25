#define _CRT_SECURE_NO_WARNINGS 1
#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <memory>
#include <unistd.h>
#include <algorithm>


 
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
 
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
 
#define NB_RAY 64
#define RAY_DEPTH 5

static std::default_random_engine engine(42);
static std::uniform_real_distribution<double> uniform_gen(0, 1);


class Vector {
public:
    explicit Vector(double x = 0, double y = 0, double z = 0) {
        data[0] = x;
        data[1] = y;
        data[2] = z;
    }
    double norm2() const {
        return data[0] * data[0] + data[1] * data[1] + data[2] * data[2];
    }
    double norm() const {
        return sqrt(norm2());
    }
    void normalize() {
        double n = norm();
        data[0] /= n;
        data[1] /= n;
        data[2] /= n;
    }
    double operator[](int i) const { return data[i]; };
    double& operator[](int i) { return data[i]; };
    double data[3];
};
 

Vector operator+(const Vector& a, const Vector& b) {
    return Vector(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
}
Vector operator-(const Vector& a, const Vector& b) {
    return Vector(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}
Vector operator*(const double a, const Vector& b) {
    return Vector(a*b[0], a*b[1], a*b[2]);
}
Vector operator*(const Vector& a, const double b) {
    return Vector(a[0]*b, a[1]*b, a[2]*b);
}
Vector operator/(const Vector& a, const double b) {
    return Vector(a[0] / b, a[1] / b, a[2] / b);
}
double dot(const Vector& a, const Vector& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
Vector cross(const Vector& a, const Vector& b) {
    return Vector(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
}
Vector operator-(const Vector& a) {
    return Vector(-a[0], -a[1], -a[2]);
}
Vector operator*(const Vector& a, const Vector& b) {
    return Vector(a[0]*b[0], a[1]*b[1], a[2]*b[2]);
}


class Ray {
public:
    Vector O;
    Vector u;

};

class Geometry {
public:
    Vector albedo;
    int id;
    double n2=1;
    bool mirror=false;
    bool transparent=false;

    virtual bool intersect(const Ray& r, Vector& P, Vector& N, double& t, int& id) const = 0;
    virtual void set_id(int i) {
        id = i;
    }

    virtual void set_mirror(bool m) {
        mirror = m;
    }

    virtual void set_transparent(bool t) {
        transparent = t;
    }

    virtual void set_n2(double n) {
        n2 = n;
    }

    virtual void set_albedo(const Vector& color) {
        albedo = color;
    }
    
};


class Sphere: public Geometry {
public:
    Vector center;
    double radius;

    Sphere(const Vector& c, double r, const Vector& color, int i) : center(c), radius(r)  {
        albedo = color;
        id = i;
        n2 = 1;
        mirror = false;
        transparent = false;
    }

    bool intersect(
        const Ray& r,
        Vector& P,
        Vector& N,
        double& t,
        int& idt
    ) const {
        Vector L = r.O - center;

        double b = dot(r.u, L);
        double c = dot(L, L) - radius * radius;

        double disc = b*b - c;
        if (disc < 0.0) {
            return false;  // no real roots: miss
        }

        double sqrtD = std::sqrt(disc);
        double t0 = -b - sqrtD;
        double t1 = -b + sqrtD;

        if (t1 < 0.0) {
            return false;
        }

        t = (t0 >= 0.0) ? t0 : t1;

        P = r.O + r.u * t;
        Vector diff = P - center;
        double len = diff.norm();
        if (len > 1e-12) {
            N = diff / len;              
        } else {
            N = Vector(0,0,0);
        }

        idt = id;
        return true;
    }
};

class BoundingBox {
public:
    Vector min;
    Vector max;
    
    bool intersect(const Ray& r, double& distance) const {
    Vector invDir(1.0/r.u[0], 1.0/r.u[1], 1.0/r.u[2]);

    Vector t0 = (min - r.O) * invDir;
    Vector t1 = (max - r.O) * invDir;

    Vector tEntry, tExit;
    for (int i = 0; i < 3; ++i) {
        tEntry[i] = std::min(t0[i], t1[i]);
        tExit [i] = std::max(t0[i], t1[i]);
    }

    double t_enter = std::max( std::max(tEntry[0], tEntry[1]), tEntry[2] );
    double t_exit  = std::min( std::min(tExit [0], tExit [1]), tExit [2] );

    if (t_enter > t_exit || t_exit < 0.0) {
        return false;
    }

    distance = t_enter;
    return true;
}
};

class Node {
public:
    BoundingBox bbox;
    int start;
    int end; 
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    std::vector<int> indices;
};


class TriangleIndices {
public:
    TriangleIndices(int vtxi = -1, int vtxj = -1, int vtxk = -1, int ni = -1, int nj = -1, int nk = -1, int uvi = -1, int uvj = -1, int uvk = -1, int group = -1, bool added = false) : vtxi(vtxi), vtxj(vtxj), vtxk(vtxk), uvi(uvi), uvj(uvj), uvk(uvk), ni(ni), nj(nj), nk(nk), group(group) {
    };
    int vtxi, vtxj, vtxk; // indices within the vertex coordinates array
    int uvi, uvj, uvk;  // indices within the uv coordinates array
    int ni, nj, nk;  // indices within the normals array
    int group;       // face group
};
 
 
class TriangleMesh: public Geometry{
public:
  ~TriangleMesh() {}
    TriangleMesh() {};
    
    int readOBJ(const char* obj) {
 
        char matfile[255];
        char grp[255];
 
        FILE* f;
        f = fopen(obj, "r");

        if (f == NULL) {
            perror("Error opening file");
            return -1;
        }

        int curGroup = -1;
        while (!feof(f)) {
            char line[255];
            if (!fgets(line, 255, f)) break;
 
            std::string linetrim(line);
            linetrim.erase(linetrim.find_last_not_of(" \r\t") + 1);
            strcpy(line, linetrim.c_str());
 
            if (line[0] == 'u' && line[1] == 's') {
                sscanf(line, "usemtl %[^\n]\n", grp);
                curGroup++;
            }
 
            if (line[0] == 'v' && line[1] == ' ') {
                Vector vec;
 
                Vector col;
                if (sscanf(line, "v %lf %lf %lf %lf %lf %lf\n", &vec[0], &vec[1], &vec[2], &col[0], &col[1], &col[2]) == 6) {
                    col[0] = std::min(1., std::max(0., col[0]));
                    col[1] = std::min(1., std::max(0., col[1]));
                    col[2] = std::min(1., std::max(0., col[2]));
 
                    vertices.push_back(vec);
                    vertexcolors.push_back(col);
 
                } else {
                    sscanf(line, "v %lf %lf %lf\n", &vec[0], &vec[1], &vec[2]);
                    vertices.push_back(vec);
                }
            }
            if (line[0] == 'v' && line[1] == 'n') {
                Vector vec;
                sscanf(line, "vn %lf %lf %lf\n", &vec[0], &vec[1], &vec[2]);
                normals.push_back(vec);
            }
            if (line[0] == 'v' && line[1] == 't') {
                Vector vec;
                sscanf(line, "vt %lf %lf\n", &vec[0], &vec[1]);
                uvs.push_back(vec);
            }
            if (line[0] == 'f') {
                TriangleIndices t;
                int i0, i1, i2, i3;
                int j0, j1, j2, j3;
                int k0, k1, k2, k3;
                int nn;
                t.group = curGroup;
 
                char* consumedline = line + 1;
                int offset;
 
                nn = sscanf(consumedline, "%u/%u/%u %u/%u/%u %u/%u/%u%n", &i0, &j0, &k0, &i1, &j1, &k1, &i2, &j2, &k2, &offset);
                if (nn == 9) {
                    if (i0 < 0) t.vtxi = vertices.size() + i0; else t.vtxi = i0 - 1;
                    if (i1 < 0) t.vtxj = vertices.size() + i1; else t.vtxj = i1 - 1;
                    if (i2 < 0) t.vtxk = vertices.size() + i2; else t.vtxk = i2 - 1;
                    if (j0 < 0) t.uvi = uvs.size() + j0; else   t.uvi = j0 - 1;
                    if (j1 < 0) t.uvj = uvs.size() + j1; else   t.uvj = j1 - 1;
                    if (j2 < 0) t.uvk = uvs.size() + j2; else   t.uvk = j2 - 1;
                    if (k0 < 0) t.ni = normals.size() + k0; else    t.ni = k0 - 1;
                    if (k1 < 0) t.nj = normals.size() + k1; else    t.nj = k1 - 1;
                    if (k2 < 0) t.nk = normals.size() + k2; else    t.nk = k2 - 1;
                    indices.push_back(t);
                } else {
                    nn = sscanf(consumedline, "%u/%u %u/%u %u/%u%n", &i0, &j0, &i1, &j1, &i2, &j2, &offset);
                    if (nn == 6) {
                        if (i0 < 0) t.vtxi = vertices.size() + i0; else t.vtxi = i0 - 1;
                        if (i1 < 0) t.vtxj = vertices.size() + i1; else t.vtxj = i1 - 1;
                        if (i2 < 0) t.vtxk = vertices.size() + i2; else t.vtxk = i2 - 1;
                        if (j0 < 0) t.uvi = uvs.size() + j0; else   t.uvi = j0 - 1;
                        if (j1 < 0) t.uvj = uvs.size() + j1; else   t.uvj = j1 - 1;
                        if (j2 < 0) t.uvk = uvs.size() + j2; else   t.uvk = j2 - 1;
                        indices.push_back(t);
                    } else {
                        nn = sscanf(consumedline, "%u %u %u%n", &i0, &i1, &i2, &offset);
                        if (nn == 3) {
                            if (i0 < 0) t.vtxi = vertices.size() + i0; else t.vtxi = i0 - 1;
                            if (i1 < 0) t.vtxj = vertices.size() + i1; else t.vtxj = i1 - 1;
                            if (i2 < 0) t.vtxk = vertices.size() + i2; else t.vtxk = i2 - 1;
                            indices.push_back(t);
                        } else {
                            nn = sscanf(consumedline, "%u//%u %u//%u %u//%u%n", &i0, &k0, &i1, &k1, &i2, &k2, &offset);
                            if (i0 < 0) t.vtxi = vertices.size() + i0; else t.vtxi = i0 - 1;
                            if (i1 < 0) t.vtxj = vertices.size() + i1; else t.vtxj = i1 - 1;
                            if (i2 < 0) t.vtxk = vertices.size() + i2; else t.vtxk = i2 - 1;
                            if (k0 < 0) t.ni = normals.size() + k0; else    t.ni = k0 - 1;
                            if (k1 < 0) t.nj = normals.size() + k1; else    t.nj = k1 - 1;
                            if (k2 < 0) t.nk = normals.size() + k2; else    t.nk = k2 - 1;
                            indices.push_back(t);
                        }
                    }
                }
 
                consumedline = consumedline + offset;
 
                while (true) {
                    if (consumedline[0] == '\n') break;
                    if (consumedline[0] == '\0') break;
                    nn = sscanf(consumedline, "%u/%u/%u%n", &i3, &j3, &k3, &offset);
                    TriangleIndices t2;
                    t2.group = curGroup;
                    if (nn == 3) {
                        if (i0 < 0) t2.vtxi = vertices.size() + i0; else    t2.vtxi = i0 - 1;
                        if (i2 < 0) t2.vtxj = vertices.size() + i2; else    t2.vtxj = i2 - 1;
                        if (i3 < 0) t2.vtxk = vertices.size() + i3; else    t2.vtxk = i3 - 1;
                        if (j0 < 0) t2.uvi = uvs.size() + j0; else  t2.uvi = j0 - 1;
                        if (j2 < 0) t2.uvj = uvs.size() + j2; else  t2.uvj = j2 - 1;
                        if (j3 < 0) t2.uvk = uvs.size() + j3; else  t2.uvk = j3 - 1;
                        if (k0 < 0) t2.ni = normals.size() + k0; else   t2.ni = k0 - 1;
                        if (k2 < 0) t2.nj = normals.size() + k2; else   t2.nj = k2 - 1;
                        if (k3 < 0) t2.nk = normals.size() + k3; else   t2.nk = k3 - 1;
                        indices.push_back(t2);
                        consumedline = consumedline + offset;
                        i2 = i3;
                        j2 = j3;
                        k2 = k3;
                    } else {
                        nn = sscanf(consumedline, "%u/%u%n", &i3, &j3, &offset);
                        if (nn == 2) {
                            if (i0 < 0) t2.vtxi = vertices.size() + i0; else    t2.vtxi = i0 - 1;
                            if (i2 < 0) t2.vtxj = vertices.size() + i2; else    t2.vtxj = i2 - 1;
                            if (i3 < 0) t2.vtxk = vertices.size() + i3; else    t2.vtxk = i3 - 1;
                            if (j0 < 0) t2.uvi = uvs.size() + j0; else  t2.uvi = j0 - 1;
                            if (j2 < 0) t2.uvj = uvs.size() + j2; else  t2.uvj = j2 - 1;
                            if (j3 < 0) t2.uvk = uvs.size() + j3; else  t2.uvk = j3 - 1;
                            consumedline = consumedline + offset;
                            i2 = i3;
                            j2 = j3;
                            indices.push_back(t2);
                        } else {
                            nn = sscanf(consumedline, "%u//%u%n", &i3, &k3, &offset);
                            if (nn == 2) {
                                if (i0 < 0) t2.vtxi = vertices.size() + i0; else    t2.vtxi = i0 - 1;
                                if (i2 < 0) t2.vtxj = vertices.size() + i2; else    t2.vtxj = i2 - 1;
                                if (i3 < 0) t2.vtxk = vertices.size() + i3; else    t2.vtxk = i3 - 1;
                                if (k0 < 0) t2.ni = normals.size() + k0; else   t2.ni = k0 - 1;
                                if (k2 < 0) t2.nj = normals.size() + k2; else   t2.nj = k2 - 1;
                                if (k3 < 0) t2.nk = normals.size() + k3; else   t2.nk = k3 - 1;                             
                                consumedline = consumedline + offset;
                                i2 = i3;
                                k2 = k3;
                                indices.push_back(t2);
                            } else {
                                nn = sscanf(consumedline, "%u%n", &i3, &offset);
                                if (nn == 1) {
                                    if (i0 < 0) t2.vtxi = vertices.size() + i0; else    t2.vtxi = i0 - 1;
                                    if (i2 < 0) t2.vtxj = vertices.size() + i2; else    t2.vtxj = i2 - 1;
                                    if (i3 < 0) t2.vtxk = vertices.size() + i3; else    t2.vtxk = i3 - 1;
                                    consumedline = consumedline + offset;
                                    i2 = i3;
                                    indices.push_back(t2);
                                } else {
                                    consumedline = consumedline + 1;
                                }
                            }
                        }
                    }
                }
 
            }
 
        }
        fclose(f);
        return 0;
 
    }


    bool intersect(
        const Ray& r,
        Vector& P,
        Vector& N,
        double& t,
        int& id
    ) const {
        // First test the mesh’s top‐level bbox
        double t0;
        if (!bbox.intersect(r, t0)) 
            return false;

        bool   hit    = false;
        double bestT  = std::numeric_limits<double>::max();

        // Traverse BVH
        std::vector<const Node*> stack;
        stack.reserve(32);
        stack.push_back(root.get());

        while (!stack.empty()) {
            const Node* node = stack.back();
            stack.pop_back();

            // If this is an internal node, push children whose bboxes intersect
            if (node->left || node->right) {
                double dist;
                if (node->left  && node->left->bbox.intersect(r, dist))  stack.push_back(node->left.get());
                if (node->right && node->right->bbox.intersect(r, dist)) stack.push_back(node->right.get());
                continue;
            }

            // Leaf: test all triangles
            for (int i = node->start; i < node->end; ++i) {
                const auto& ti = indices[i];
                const Vector& V0 = vertices[ti.vtxi];
                const Vector& V1 = vertices[ti.vtxj];
                const Vector& V2 = vertices[ti.vtxk];

                // Edges
                Vector E1 = V1 - V0;
                Vector E2 = V2 - V0;

                // Möller–Trumbore
                Vector Pvec = cross(r.u, E2);
                double det  = dot(E1, Pvec);
                if (fabs(det) < 1e-8) continue;
                double invDet = 1.0 / det;

                Vector Tvec = r.O - V0;
                double u = dot(Tvec, Pvec) * invDet;
                if (u < 0.0 || u > 1.0) continue;

                Vector Qvec = cross(Tvec, E1);
                double v = dot(r.u, Qvec) * invDet;
                if (v < 0.0 || u + v > 1.0) continue;

                double tt = dot(E2, Qvec) * invDet;
                if (tt <= 1e-8 || tt >= bestT) continue;

                // We have a new best hit
                bestT = tt;
                P     = r.O + r.u * tt;

                // compute and normalize normal
                N = cross(E1, E2);
                double len = N.norm();
                if (len > 1e-12) N = N / len;

                id  = this->id;
                hit = true;
            }
        }

        if (hit) {
            t = bestT;
        }
        return hit;
    }


    bool set_position(const Vector& pos) {
        for (int i = 0; i < vertices.size(); i++) {
            vertices[i] = vertices[i] + pos;
        }
        return true;
    }

    void resize_model(double scale) {
        for (int i = 0; i < vertices.size(); i++) {
            vertices[i] = vertices[i] * scale;
        }
    }

    BoundingBox get_boundingbox( int start, int end) {
        BoundingBox box;
        box.min = Vector(10E10, 10E10, 10E10);
        box.max = Vector(-10E10, -10E10, -10E10);
        for (int i = start; i < end; i++) {
            for (int j = 0; j < 3; j++) {
                if (vertices[indices[i].vtxi][j] < box.min[j]) box.min[j] = vertices[indices[i].vtxi][j];
                if (vertices[indices[i].vtxi][j] > box.max[j]) box.max[j] = vertices[indices[i].vtxi][j];
                if (vertices[indices[i].vtxj][j] < box.min[j]) box.min[j] = vertices[indices[i].vtxj][j];
                if (vertices[indices[i].vtxj][j] > box.max[j]) box.max[j] = vertices[indices[i].vtxj][j];
                if (vertices[indices[i].vtxk][j] < box.min[j]) box.min[j] = vertices[indices[i].vtxk][j];
                if (vertices[indices[i].vtxk][j] > box.max[j]) box.max[j] = vertices[indices[i].vtxk][j];
            }
        }
        return box;
    }

    void compute_bvh(Node* node, int start, int end) {
        node->bbox  = get_boundingbox(start, end);
        node->start = start;
        node->end   = end;

        int count = end - start;
        if (count <= 8) {               // leaf‐size threshold
            node->left = nullptr;
            node->right= nullptr;
            return;
        }

        Vector diag = node->bbox.max - node->bbox.min;
        int axis = 0;
        if (diag[1] > diag[axis]) axis = 1;
        if (diag[2] > diag[axis]) axis = 2;

        int mid = start + count/2;
        std::nth_element(
            indices.begin() + start,
            indices.begin() + mid,
            indices.begin() + end,
            [&](const TriangleIndices &A, const TriangleIndices &B){
                Vector cA = ( vertices[A.vtxi]
                            + vertices[A.vtxj]
                            + vertices[A.vtxk] ) / 3.0;
                Vector cB = ( vertices[B.vtxi]
                            + vertices[B.vtxj]
                            + vertices[B.vtxk] ) / 3.0;
                return cA[axis] < cB[axis];
            }
        );

        node->left  = std::make_unique<Node>();
        node->right = std::make_unique<Node>();
        compute_bvh(node->left.get(),  start, mid);
        compute_bvh(node->right.get(), mid,   end);
    }

 
    std::vector<TriangleIndices> indices;
    std::vector<Vector> vertices;
    std::vector<Vector> normals;
    std::vector<Vector> uvs;
    std::vector<Vector> vertexcolors;
    BoundingBox bbox;
    std::shared_ptr<Node> root = std::make_shared<Node>();
    
};


struct Light{
    Vector L;
    double I;
};


Vector random_cos(const Vector& N) {
    double r1 = uniform_gen(engine);
    double r2 = uniform_gen(engine);

    double x = cos(2 * M_PI * r1) * sqrt(1 - r2);
    double y = sin(2 * M_PI * r1) * sqrt(1 - r2);
    double z = sqrt(r2);

    Vector T1;

    if ((std::abs(N[0]) < std::abs(N[1])) && std::abs(N[0]) <= std::abs(N[2])) {
        T1 = Vector(0, N[2], -N[1]);
    } else {
        if (std::abs(N[1]) <= std::abs(N[0]) && std::abs(N[1]) <= std::abs(N[2])) {
            T1 = Vector(N[2], 0, -N[0]);
        } else {
            T1 = Vector(N[1], -N[0], 0);
        }
    }

    T1.normalize();


    return x * T1 + y * cross(N, T1) + z * N;
}

void boxMuller(double stdev , double &x, double &y) {
    double r1 = uniform_gen(engine);
    double r2 = uniform_gen(engine);
    x = sqrt(-2. * log(r1))*cos(2.*M_PI*r2)*stdev;
    y = sqrt(-2. * log(r1))*sin(2.*M_PI*r2)*stdev;
}




class Scene {
public:
    Light light;
    std::vector<std::unique_ptr<Geometry>> objects;



    void add(std::unique_ptr<Geometry> obj) {
        objects.push_back(std::move(obj));
    }



    void setLight(const Light& l) {
        light = l;
    }

    Geometry* getGeometry(int id) const {
        for (const auto& obj : objects) {
            if (obj->id == id) {
                return obj.get();
            }
        }
        return nullptr;
    }


    bool intersect(const Ray& r, Vector& P, Vector& N, double &t, int &id) const {
        id = -1;
        bool found = false;
        double t_res = 10E10;
        for (int i = 0; i < objects.size(); i++) {
            Vector Ptmp, Ntmp;
            double ttmp;
            int idtmp;

            
            if (objects[i]->intersect(r, Ptmp, Ntmp, ttmp, idtmp)) {
                if (ttmp < t_res) {
                    t_res = ttmp;
                    P = Ptmp;
                    N = Ntmp;
                    id = idtmp;
                    found = true;
                }
            }
        }
        t = t_res;
        return found;  
    }


    Vector get_color(const Ray& r, int depth) {
        if (depth < 0) return Vector(0.,0.,0.);

        Vector P, N;
        int id;
        double t;
        if (intersect(r, P, N, t, id)) {
            Geometry* obj = getGeometry(id);

            if (obj->mirror) {
                return shade_mirror(r, P, N, depth);
            }
            else if (obj->transparent) {
                return shade_transparent(r, P, N, obj, depth);
            }
            else {
                return shade_diffuse(P, N, obj, depth);
            }
        } else {
            return Vector(0.,0.,0.);
        }
    }

    Vector shade_mirror(const Ray& r, const Vector& P, const Vector& N, int depth) {
        Vector R = r.u - 2 * dot(r.u, N) * N;
        Ray r_reflected = {P + 0.0001 * N, R};
        return get_color(r_reflected, depth - 1);
    }

    Vector direct_lighting(const Vector& P, const Vector& N, const Geometry* obj) {
        Vector L = light.L - P;
        double d = L.norm();
        L = L / d;
        Vector P_eps = P + 0.0001 * N;
        Ray shadowRay = {P_eps, L};
        Vector Ptmp, Ntmp;
        double ttmp;
        int idtmp;
        bool hit = intersect(shadowRay, Ptmp, Ntmp, ttmp, idtmp);
        double vis = (!hit || ttmp >= d) ? 1.0 : 0.0;
        return (light.I / (4 * M_PI * d * d))
            * obj->albedo / M_PI
            * std::max(0.0, dot(N, L))
            * vis;
    }

    Vector indirect_lighting(const Vector& P, const Vector& N, const Geometry* obj, int depth) {
        Ray indirect_ray = {P + 0.0001 * N, random_cos(N)};
        return get_color(indirect_ray, depth - 1) * obj->albedo;
    }

    Vector shade_diffuse(const Vector& P, const Vector& N, const Geometry* obj, int depth) {
        Vector direct = direct_lighting(P, N, obj);
        Vector indirect = indirect_lighting(P, N, obj, depth);
        return direct + indirect;
    }

    Vector shade_transparent(
        const Ray& r, const Vector& P, const Vector& N_in,
        const Geometry* obj, int depth
    ) {
        Vector N = N_in;
        double n1 = 1.0, n2 = obj->n2;
        if (dot(r.u, N) > 0) {
            std::swap(n1, n2);
            N = -N;
        }
        double n = n1 / n2;
        double cos_theta = -dot(r.u, N);

        double k0 = pow((n1 - n2) / (n1 + n2), 2);
        double R0 = k0 + (1 - k0) * pow(1 - std::abs(dot(N, r.u)), 5);
        double u = uniform_gen(engine);

        if (u < R0) {
            Ray reflected = {P + 0.0001 * N, r.u - 2 * dot(r.u, N) * N};
            return get_color(reflected, depth - 1);
        } else {
            double rad = 1 - n * n * (1 - cos_theta * cos_theta);
            if (rad < 0) {
                Ray reflected = {P + 0.0001 * N, r.u - 2 * dot(r.u, N) * N};
                return get_color(reflected, depth - 1);
            } else {
                Vector T_normal = -sqrt(rad) * N;
                Vector T_tangent = n * (r.u - cos_theta * N);
                Ray refracted = {P - 0.0001 * N, T_normal + T_tangent};
                return get_color(refracted, depth - 1);
            }
        }
    }
};


Vector calculate_direction_camera(double i, double j, const Vector& C, int W, int H, double distance) {
    double x = (C[0] + i + 0.5 - W / 2) ;
    double y = (C[1] + j + 0.5 - H / 2) ;
    double z = C[2] + distance;
    Vector P(x, y, z);
    return (P - C) / (P - C).norm();
}

std::vector<unsigned char> gamma_correct(std::vector<unsigned char>& image, double gamma) {
    std::vector<unsigned char> image_gamma(image.size(), 0);
    for (int i = 0; i < image.size(); i++) {
        image_gamma[i] = pow(image[i] / 255.0, 1.0 / gamma) * 255;
    }
    return image_gamma;

}

int id_max = 0;
int assign_id() {
    return id_max++;
}


// =====================================MAIN=========================================================

int main() {

    // char cwd[1024];
    // if (getcwd(cwd, sizeof(cwd)) != NULL) {
    //     std::cout << "Current working directory: " << cwd << std::endl;
    // } else {
    //     perror("getcwd() error");
    //     return -1;
    // }

    auto start = std::chrono::high_resolution_clock::now();
    int W = 512;
    int H = 512;
    int NB_path = NB_RAY;
    int depth = RAY_DEPTH;

    double fov  = 60.0  * M_PI  / 180.0 ;
    double z = -W / (2.0 * tan(fov / 2.0));

    Sphere S(Vector(0,0,0), 10, Vector(0.4,0.4,0.4), assign_id());
    Light light = {Vector(-10, 20, 40), 1E7};

    Sphere red(Vector(0,1000,0), 940, Vector(0.5,0,0), assign_id());
    Sphere pink(Vector(0,0,1000), 940, Vector(0.5,0,0.5), assign_id());
    Sphere blue(Vector(0,-1000,0), 990, Vector(0,0,0.5), assign_id());
    Sphere green(Vector(0,0,-1000), 940, Vector(0,0.5,0), assign_id());
    Sphere yellow(Vector(1000,0,0), 940, Vector(0.5,0.5,0), assign_id());
    Sphere cyan(Vector(-1000,0,0), 940, Vector(0,0.5,0.5), assign_id());
    Sphere mirror_sphere(Vector(25,0,0), 10, Vector(0.5,0.5,0.5), assign_id());
    Sphere transparent_sphere(Vector(-25,0,0), 10, Vector(0.5,0.5,0.5), assign_id());

    
    mirror_sphere.set_mirror(true);
    transparent_sphere.set_transparent(true);
    transparent_sphere.set_n2(1.5);


    Scene scene;
    // scene.add(red);
    // scene.add(pink);
    // scene.add(blue);
    // scene.add(green);
    // scene.add(S);
    // scene.add(mirror_sphere);
    // scene.add(transparent_sphere);

    scene.add(std::make_unique<Sphere>(red));
    scene.add(std::make_unique<Sphere>(pink));
    scene.add(std::make_unique<Sphere>(blue));
    scene.add(std::make_unique<Sphere>(green));

    // Uncomment to add spheres to the scene
    // scene.add(std::make_unique<Sphere>(S)); //white sphere
    // scene.add(std::make_unique<Sphere>(mirror_sphere));  // mirror sphere
    // scene.add(std::make_unique<Sphere>(transparent_sphere));  // transparent sphere

    scene.setLight(light);


    TriangleMesh mesh = TriangleMesh();
    int code = mesh.readOBJ("model/cat.obj");
    if(code != 0) {
        std::cout << "Error reading obj file" << std::endl;
        return -1;
    }

    mesh.set_id(assign_id());
    mesh.set_albedo(Vector(1.0,1.0,1.0));
    mesh.set_position(Vector(0,-13,-20));
    mesh.resize_model(0.8);
    mesh.bbox = mesh.get_boundingbox(0, mesh.indices.size());
    mesh.compute_bvh(mesh.root.get(), 0, mesh.indices.size());

    scene.add(std::make_unique<TriangleMesh>(mesh));
    scene.add(std::make_unique<Sphere>(yellow));
    scene.add(std::make_unique<Sphere>(cyan));

    std::vector<unsigned char> image(W * H * 3, 0);


    #pragma omp parallel for 
    for (int i = 0; i < H; i++) {
        // std::cout << "i: " << i << std::endl;
        for (int j = 0; j < W; j++) {
            Vector Q(0,7,55);
            int x = j;
            int y = H - i - 1;

            Vector P, N;
            double ttmp;
            int obj_id;

            Vector color = Vector(0,0,0);

            for (int k = 0; k < NB_path; k++) {
                double x_i, y_i;
                boxMuller(0.4, x_i, y_i);
                Vector U = calculate_direction_camera(x+x_i, y+y_i, Q, W, H, z);
                Ray r = {Q, U};

                color = color +  scene.get_color(r, depth);
            }
            
            color = color / NB_path;

            image[(i * W + j) * 3 + 0] = std::min(255., color[0]) ;
            image[(i * W + j) * 3 + 1] = std::min(255., color[1]) ;
            image[(i * W + j) * 3 + 2] = std::min(255., color[2]) ;

        }
    }
    
    image = gamma_correct(image, 2.2);
   
    stbi_write_png("image.png", W, H, 3, &image[0], 0);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time: " << elapsed.count() << std::endl;
 
    return 0;
}
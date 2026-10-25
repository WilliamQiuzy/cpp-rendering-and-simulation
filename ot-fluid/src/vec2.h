#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <memory>
#include <unistd.h>

#ifndef VECTOR_H
#define VECTOR_H

static std::default_random_engine engine(44);
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

Vector operator+(const Vector& a, const Vector& b);
Vector operator-(const Vector& a, const Vector& b);
Vector operator*(double scalar, const Vector& a);
Vector operator*(const Vector& a, double scalar);
Vector operator/(const Vector& a, double scalar);
double dot(const Vector& a, const Vector& b);
Vector cross(const Vector& a, const Vector& b);
Vector operator-(const Vector& a);
Vector operator*(const Vector& a, const Vector& b);
double det(const Vector& a, const Vector& b);
double cross2d(const Vector& a, const Vector& b);

#endif // VECTOR_H

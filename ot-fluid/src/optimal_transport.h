#ifndef OPTIMALTRANSPORT_H
#define OPTIMALTRANSPORT_H

#include "PowerDiagram.h"
#include "lbfgs.h"


class OptimalTransport {
public:
    OptimalTransport(const std::vector<Vector>& points,
                     const std::vector<double>& weights)
      : points_(points)
      , weights_(weights)
      , solution_(points_, weights_)
    {}
    
    static lbfgsfloatval_t evaluateCallback(void* instance, const lbfgsfloatval_t* x, lbfgsfloatval_t* g, int n, lbfgsfloatval_t step);
    lbfgsfloatval_t evaluate(const lbfgsfloatval_t* x, lbfgsfloatval_t* g, int n, lbfgsfloatval_t step);

    static lbfgsfloatval_t optimizeFluidCallback(void* instance, const lbfgsfloatval_t* x, lbfgsfloatval_t* g, int n, lbfgsfloatval_t step);
    lbfgsfloatval_t optimizeFluid(const lbfgsfloatval_t* x, lbfgsfloatval_t* g, int n, lbfgsfloatval_t step);
    
    static int progressCallback(void* instance, const lbfgsfloatval_t* x, const lbfgsfloatval_t* g, lbfgsfloatval_t fx, lbfgsfloatval_t xnorm, lbfgsfloatval_t gnorm, lbfgsfloatval_t step, int n, int k, int ls);
    int progress(const lbfgsfloatval_t* x, const lbfgsfloatval_t* g, lbfgsfloatval_t fx, lbfgsfloatval_t xnorm, lbfgsfloatval_t gnorm, lbfgsfloatval_t step, int n, int k, int ls);

    void compute();
    int compute_fluid();

    void save(const std::string& filename) const {
        solution_.save(filename);
    }

    std::vector<Vector> getPoints() const {
        return points_;
    }

    std::vector<double> getWeights() const {
        return weights_;
    }

    void setPoints(const std::vector<Vector>& points) {
        points_ = points;
    }

    void setWeights(const std::vector<double>& weights) {
        weights_ = weights;
    }

    PowerDiagram getSolution() const {
        return solution_;
    }

    double calculateA(double triArea, const Vector& v0, const Vector& v1, const Vector& v2, const Vector& pt);

    




private:
    std::vector<Vector> points_;    
    std::vector<double> weights_;
    PowerDiagram    solution_;
};


#endif // OPTIMALTRANSPORT_H
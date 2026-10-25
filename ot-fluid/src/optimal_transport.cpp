#include "OptimalTransport.h"   


lbfgsfloatval_t OptimalTransport::evaluateCallback(void* instance, const lbfgsfloatval_t* x, lbfgsfloatval_t* g, int n, lbfgsfloatval_t step) {
    return reinterpret_cast<OptimalTransport*>(instance)->evaluate(x, g, n, step);
}   

lbfgsfloatval_t OptimalTransport::evaluate(const lbfgsfloatval_t* x, lbfgsfloatval_t* g, int n, lbfgsfloatval_t step) {
    std::vector<double> weights(x, x + n);
    PowerDiagram powerDiagram(points_, weights);
    powerDiagram.compute();
    
    lbfgsfloatval_t  fx = 0.0;
   

    for(int i = 0; i < n; i++) {
        double carea = powerDiagram.voronoi[i].computeArea();
        g[i] = carea - weights_[i];

        fx += powerDiagram.voronoi[i].computeSquaredDistance(powerDiagram.points[i]);
        fx += weights_[i] * x[i];
        fx -= x[i] * carea;
    }
    return -fx;
}

int OptimalTransport::progressCallback(void* instance, const lbfgsfloatval_t* x, const lbfgsfloatval_t* g, lbfgsfloatval_t fx, lbfgsfloatval_t xnorm, lbfgsfloatval_t gnorm, lbfgsfloatval_t step, int n, int k, int ls) {
    return reinterpret_cast<OptimalTransport*>(instance)->progress(x, g, fx, xnorm, gnorm, step, n, k, ls);
}

int OptimalTransport::progress(const lbfgsfloatval_t* x, const lbfgsfloatval_t* g, lbfgsfloatval_t fx, lbfgsfloatval_t xnorm, lbfgsfloatval_t gnorm, lbfgsfloatval_t step, int n, int k, int ls) {
    return 0;
}



void OptimalTransport::compute() {
    lbfgsfloatval_t fx;
    lbfgsfloatval_t *x = lbfgs_malloc(points_.size());
    for(size_t i = 0; i < points_.size(); i++) {
        x[i] = 1;
    }
    int n = points_.size();
    int ret = lbfgs(n, x, &fx, evaluateCallback, progressCallback, this, NULL);
    std::cout << "L-BFGS optimization terminated with status code " << ret << "\n";
    std::cout << "fx = " << fx << "\n";
    std::vector<double> weights(x, x + n);
    PowerDiagram powerDiagram(points_, weights);
    powerDiagram.compute();
    solution_ = powerDiagram;
    lbfgs_free(x);

}


lbfgsfloatval_t OptimalTransport::optimizeFluidCallback(void* instance, const lbfgsfloatval_t* x, lbfgsfloatval_t* g, int n, lbfgsfloatval_t step) {
    return reinterpret_cast<OptimalTransport*>(instance)->optimizeFluid(x, g, n, step);
}

lbfgsfloatval_t OptimalTransport::optimizeFluid(const lbfgsfloatval_t* x, lbfgsfloatval_t* g, int n, lbfgsfloatval_t step) {
    lbfgsfloatval_t fx = 0.0;
    std::vector<Vector> pts(points_.begin(), points_.end());
    std::vector<double> wts(x, x + n);

    PowerDiagram pd(pts, wts);
    pd.compute_fluid();

    std::vector<Polygon> voronoiCells = pd.voronoi;

    const double fluidFrac = 0.4;
    const double airFrac = 1.0 - fluidFrac;
    const double lambda = fluidFrac / (n - 1);
    double totalFluidArea = 0.0;

    for (int i = 0; i < n - 1; ++i) {
        double cellArea = voronoiCells[i].computeArea();
        totalFluidArea += cellArea;
        g[i] = -(lambda - cellArea);

        double A = 0.0;
        const std::vector<Vector>& verts = voronoiCells[i].vertices;
        int numVerts = verts.size();
        if (numVerts < 3) continue;

        for (int j = 1; j < numVerts - 1; ++j) {
            Vector v0 = verts[0];
            Vector v1 = verts[j];
            Vector v2 = verts[j + 1];
            double triArea = std::abs(0.5 * det(v1 - v0, v2 - v0));

            A += calculateA(triArea, v0, v1, v2, pts[i]);
        }

        fx += -(A - x[i] * cellArea + lambda * x[i]);
    }

    double airArea = 1.0 - totalFluidArea;
    g[n - 1] = -(airFrac - airArea);
    fx += -(-x[n - 1] * airArea + airFrac * x[n - 1]);

    return fx;
}

double OptimalTransport::calculateA(double triArea, const Vector& v0, const Vector& v1, const Vector& v2, const Vector& pt)  {
    double A = 0.0;
    Vector verts[3] = { v0, v1, v2 };

    for (size_t p = 0; p < 3; ++p) {
        for (size_t q = p; q < 3; ++q) {
            A += triArea / 6 * dot(verts[p] - pt, verts[q] - pt);
        }
    }

    return A;
}




int OptimalTransport::compute_fluid() {
    // while (!success) {
        lbfgsfloatval_t fx;
        lbfgsfloatval_t *x = lbfgs_malloc(points_.size() + 1);
        for (size_t i = 0; i < points_.size(); ++i) {
            x[i] = 1.0 / points_.size();
        }
        x[points_.size()] = 0;


        lbfgs_parameter_t param;
        lbfgs_parameter_init(&param);
        param.linesearch = LBFGS_LINESEARCH_BACKTRACKING_ARMIJO;
        param.max_iterations = 10000;

        int n = points_.size() + 1;
        int ret = lbfgs(n, x, &fx, optimizeFluidCallback, progressCallback, this, &param);
        std::cout << "L-BFGS optimization terminated with status code " << ret << "\n";
        std::cout << "fx = " << fx << "\n";
        std::vector<double> weights(x, x + n);
        PowerDiagram powerDiagram(points_, weights);
        powerDiagram.compute_fluid();
        solution_ = powerDiagram;

        lbfgs_free(x);
    
    return ret ;

}

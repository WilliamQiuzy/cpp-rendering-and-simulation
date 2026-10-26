# PhotonFlow

[![CI](https://github.com/WilliamQiuzy/photonflow/actions/workflows/ci.yml/badge.svg)](https://github.com/WilliamQiuzy/photonflow/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**A C++17 physics engine for light transport and incompressible fluids.** PhotonFlow
renders scenes with unbiased Monte Carlo path tracing and simulates free-surface fluids
with semi-discrete optimal transport. No graphics API or engine dependency: geometry,
acceleration structures, shading, power diagrams and the transport dual are implemented
here; only PNG output (stb) and the L-BFGS optimizer (liblbfgs) are third-party.

<p align="center">
  <img src="docs/cat.png" width="32%" alt="Path-traced mesh in a room of colored walls">
  <img src="docs/cat_spheres.png" width="32%" alt="Same scene with a glass sphere and a mirror sphere">
  <img src="docs/fluid.gif" width="32%" alt="Incompressible fluid simulated with optimal transport">
</p>

## Modules

### `render`: Monte Carlo path tracing
- Global illumination with cosine-weighted importance sampling and next-event estimation
  from point lights.
- Specular mirrors and dielectrics with Schlick's Fresnel term and total internal reflection.
- Triangle meshes from Wavefront OBJ, accelerated by a median-split bounding volume
  hierarchy with Möller-Trumbore intersection.
- OpenMP-parallel rendering with a per-pixel random generator: no shared state between
  threads, and bit-identical images across runs and thread counts.

### `fluid`: optimal-transport fluid solver
- Power diagrams built by Sutherland-Hodgman clipping, intersected with per-particle disks
  to model the air phase and produce a free surface.
- Transport weights from the concave dual of the semi-discrete problem, maximized with
  L-BFGS; the gradient is the gap between each cell's area and its target.
- Incompressibility through a stiff spring toward each cell's centroid (Gallouët and
  Mérigot, 2018) with semi-implicit Euler integration.

## Performance

Measured on a 10-core Apple laptop, Release build with OpenMP.

| Workload | Time |
|---|---|
| 512 x 512, 64 samples/pixel, 5 bounces, 3,954-triangle mesh | 9.8 s |
| Same scene at 256 samples/pixel (images above) | 38 s |
| Fluid, 50 particles, 400 frames, transport solve every frame | 87 s |

## Testing

`ctest` runs 15 unit tests and two end-to-end smoke tests on Ubuntu and macOS in CI:

- the BVH returns exactly the closest hit found by brute force over 500 random triangles;
- cosine-weighted sampling matches its analytic mean, E[cos θ] = 2/3;
- polygon integrals match closed forms, and Voronoi cells tile the domain exactly;
- power weights move the bisector to the analytic position;
- the transport solver gives every particle the same area to within 1e-3.

## Build

Requires CMake 3.16+ and a C++17 compiler. OpenMP is optional.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

On macOS with Apple Clang, add OpenMP for multithreading:

```bash
brew install libomp
cmake -S . -B build -DOpenMP_ROOT=$(brew --prefix libomp)
```

## Usage

```bash
./build/render/photonflow-render --samples 256 --output render.png
./build/render/photonflow-render --glass-sphere --mirror-sphere --output spheres.png

./build/fluid/photonflow-fluid --particles 50 --frames 400 --output-dir frames
ffmpeg -framerate 25 -i frames/frame_%04d.png -pix_fmt yuv420p fluid.mp4
```

## Layout

```
render/   vec3, ray, rng, shape, sphere, aabb, triangle_mesh (OBJ + BVH), scene, camera, CLI
fluid/    vec2, polygon, power_diagram, optimal_transport (L-BFGS dual), fluid, frame_writer, CLI
tests/    unit tests and CLI smoke tests (CTest)
third_party/   stb_image_write, liblbfgs
```

## References

- T. Gallouët and Q. Mérigot. A Lagrangian scheme à la Brenier for the incompressible Euler
  equations. *Foundations of Computational Mathematics*, 2018.
- B. Lévy. A numerical algorithm for L2 semi-discrete optimal transport in 3D. *ESAIM: M2AN*, 2015.
- T. Möller and B. Trumbore. Fast, minimum storage ray-triangle intersection. *Journal of
  Graphics Tools*, 1997.
- C. Schlick. An inexpensive BRDF model for physically-based rendering. *Computer Graphics
  Forum*, 1994.

## Third-party code and assets

- [stb_image_write](https://github.com/nothings/stb) (public domain or MIT).
- [liblbfgs](https://github.com/chokkan/liblbfgs) by Naoaki Okazaki (MIT), see
  `third_party/liblbfgs/COPYING`.
- The cat model was downloaded from cadnav.com.

## License

MIT, see [LICENSE](LICENSE).

# SeismicPackage
Just some random seismic algorithms. Ray tracing, wave propagation, time processing, etc... I hope it is useful for somebody.

## CMake build

```bash
cmake -S . -B build
cmake --build build -j
```

- Executables are emitted to `bin/`, libraries to `lib/`.
- `SEPDataViewer` and `SEGYDataViewer` are managed by CMake; they are built when OpenGL/GLUT development dependencies are available.


# Face Engine in Face Pass
## Preparation

Dependencies (ONNX Runtime and openpnp-capture) are automatically fetched via CMake FetchContent. No manual installation required.

## Build
```bash
cd face_engine
```

```bash
mkdir -p build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
```

## Run
In `run.sh` file, replace each image path with yours. Example in `run.sh` file:
```bash
./face-pass \
    /path/to/image1 \
    /path/to/image2
```

The face detection result with be saved at `result<index>.bmp` and the matching & spoofing results will be printed to the terminal.

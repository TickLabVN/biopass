# Face Engine in Face Pass
## Preparation

ONNX Runtime is automatically fetched via CMake FetchContent. Camera capture links a pinned libcamera built from
source (see `auth/BundleLibcamera.cmake`) plus the system libjpeg-turbo package (`libturbojpeg0-dev`) found
via pkg-config — install libjpeg-turbo and libcamera's build tools (`meson`, `ninja-build`, `python3-jinja2`,
`python3-yaml`, `python3-ply`, `libudev-dev`) before configuring.

## Build
```bash
cmake -S auth -B auth/build -DBUILD_TESTS=ON
cmake --build auth/build --target face_engine_test -j$(nproc)
```

## Run
Compare two images from the `auth` build directory:
```bash
./auth/build/test/face_engine/face_engine_test \
    /path/to/image1 \
    /path/to/image2
```

Optional arguments:
```bash
./auth/build/test/face_engine/face_engine_test \
    --threshold 0.5 \
    --det-model /path/to/yolov8n-face.onnx \
    --reg-model /path/to/edgeface_s_gamma_05.onnx \
    --save-crops --crop1 result1.jpg --crop2 result2.jpg \
    /path/to/image1 \
    /path/to/image2
```

Notes:
- Default models are loaded from `auth/face/models`.
- The CLI detects all faces and uses the largest one in each image for comparison.
- Similarity score and match result are printed to the terminal.

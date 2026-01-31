# Face Engine in Face Pass
## Preparation
### OpenCV
Install opencv for C++: [Read Installation for C++](https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html) 

### Libtorch
```bash
wget https://download.pytorch.org/libtorch/nightly/cpu/libtorch-cxx11-abi-static-without-deps-2.2.0.dev20231031+cpu.zip
unzip libtorch-cxx11-abi-static-without-deps-2.2.0.dev20231031+cpu.zip
```

## Build
```bash
cd face_engine
```

In `build.sh` file, replace path of DCMAKE_PREFIX_PATH with absolute path to `libtorch` folder. 
```bash
-DCMAKE_PREFIX_PATH=/path/to/libtorch
```

Run:
```bash
bash build.sh
```

## Run
In `run.sh` file, replace each image path with yours. Example in `run.sh` file:
```bash
./face-pass \
    /path/to/image1 \
    /path/to/image2
```

The face detection result with be saved at `result<index>.jpg` and the matching & spoofing results will be printed to the terminal.
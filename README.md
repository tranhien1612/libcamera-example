# libcamera-example

## Install libs
```
     sudo apt install build-essential cmake git libcamera-dev libopencv-dev
```
```
    git clone https://github.com/tranhien1612/libcamera-example.git
    cd libcamera-example
    mkdir build && cd build
    cmake .. && make
    sudo make install
```

## Build source code
```
    cd libcamera-example
    g++ main.cpp -o main `pkg-config --cflags --libs  libcamera  opencv4` -llccv
```

# libcamera-example

## Install libs
```
    git clone https://github.com/kbarni/LCCV.git
    cd LCCV
    mkdir build && cd build
    cmake .. && make
    sudo make install
```

## Build source code
```
    g++ main.cpp -o main `pkg-config --cflags --libs  libcamera  opencv4` -llccv
```

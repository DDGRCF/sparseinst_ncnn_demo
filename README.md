
<h1 align="center">

🚀SparseInst🚀 NCNN Deployment

</h1>

# Introduction

This repo is based on [SparseInst](https://github.com/hustvl/SparseInst). SparseInst is a fast and high precise instance segmemation alogrithm.

# Preview

<div align="center">

<img src="./assets/preview.jpg"/> 

</div>


# Usage

## Compile NCNN
You can reference [ncnn_docs](https://github.com/Tencent/ncnn/wiki/how-to-build). There is for linux backend.

1. clone repo
```shell
git clone https://github.com/Tencent/ncnn.git
cd ncnn
git submodule update --init
```
2. install dependencies
```shell
sudo apt install build-essential git cmake libprotobuf-dev protobuf-compiler libvulkan-dev vulkan-utils libopencv-dev
```

3. build
```shell
mkdir -p build && cd build
cmake -DNCNN_VULKAN=ON -DNCNN_BUILD_EXAMPLES=ON -DNCNN_PYTHON=ON -DNCNN_BUILD_TESTS=ON ..
make -j$(proc)
make install
```

## Clone Repo

```shell
git clone https://github.com/DDGRCF/sparseinst_ncnn_demo.git
cd sparseinst_ncnn_demo
mkdir -p build && cd build
cmake .. -DNCNN_DIR=/path/to/ncnn/build/install/lib/cmake/ncnn
make -j$(nproc)
```


## Download Model

```shell
latest_version=v1.0.0
wget https://github.com/DDGRCF/sparseinst_ncnn_demo/releases/download/${latest_version}/sparseinst-sim-opt.param
wget https://github.com/DDGRCF/sparseinst_ncnn_demo/releases/download/${latest_version}/sparseinst-sim-opt.bin
```

## Run Repo
```shell
param_path=/path/to/your/param
bin_path=/path/to/your/bin
image_path=/path/to/your/image
save_path=/path/to/your/save_image
./sparseinst_ncnn_demo ${param_path} ${bin_path} ${image_path} ${save_path}
# NOTE: image_path can be dir or file.
```

# License
This repo is under [MIT LICENSE](./LICENSE)

# Thanks

* [ncnn](https://github.com/Tencent/ncnn)
* [sparseinst](https://github.com/hustvl/SparseInst)
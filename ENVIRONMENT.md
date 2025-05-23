# Fresh setup of Azure VM for FHE-HA Integrated Workflow

VM : [Standard NC4as T4 v3 (4 vCPUs, 28 GB memory)](https://learn.microsoft.com/en-us/azure/virtual-machines/sizes/gpu-accelerated/ncast4v3-series?tabs=sizeaccelerators)

Image: Ubuntu 22.04

### 0. Connect to the VM:
```
ssh -i ~/.ssh/Encrypt_HAtestVM_keys.pem azureuser@20.73.185.100
```

### 1. Environment preparation:

#### 1.1 Environment tools:

```
sudo apt update &&\
sudo apt-get install gcc g++ make
```

#### 1.2 NVIDIA Driver (560.28.03) & CUDA toolkit (12.6.0):
```
wget https://developer.download.nvidia.com/compute/cuda/12.6.0/local_installers/cuda_12.6.0_560.28.03_linux.run
sudo sh cuda_12.6.0_560.28.03_linux.run
```

#### 1.3 Docker:
```
sudo apt install apt-transport-https ca-certificates curl software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt update
sudo apt install docker-ce
```

#### 1.4 NVIDIA container toolkit:
```
curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg   && curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list |     sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' |     sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list
sudo sed -i -e '/experimental/ s/^#//g' /etc/apt/sources.list.d/nvidia-container-toolkit.list
sudo apt-get update
sudo apt-get install -y nvidia-container-toolkit
```

#### 1.5 Configure Docker Runtime:
```
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker
```

#### 1.6 CMake v3.27.6:
```
wget https://github.com/Kitware/CMake/releases/download/v3.27.6/cmake-3.27.6-linux-x86_64.sh
sudo chmod +x cmake-3.27.6-linux-x86_64.sh
sudo ./cmake-3.27.6-linux-x86_64.sh --skip-license --prefix=/usr/local
sudo rm cmake-3.27.6-linux-x86_64.sh
```

#### 1.7 Include the machine ssh agent to Github (optional):

Create an ssh key for a github account:
`ssh-keygen -t ed25519 -C "user.name@email.com"`
Show the generated ssh key:
`cat ~/.ssh/id_ed25519.pub`
Copy this key to clip board and paste in "New SSH Key" form in Github (Settings, SSH and GPG keys, New SSH key)!



### (Optional) Environment Functional Test 

#### Clone OpenFHE UniMan
```
git clone git@github.com:beehive-lab/openfhe-uniman.git
cd openfhe-uniman
git checkout opt
```

#### Build OpenFHE UniMan
```
cmake	-DCMAKE_BUILD_TYPE=Debug \
        -DWITH_OPENMP=OFF \
        -DWITH_CUDA=ON \
        -DCMAKE_VERBOSE_MAKEFILE=ON \
        -DCUDA_PATH=/usr/local/cuda \
        -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda \
        -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc \
        -S ~/openfhe-uniman \
        -B ~/openfhe-uniman/cmake-build-debug-cuda

cmake	--build ~/openfhe-uniman/cmake-build-debug-cuda \
        --target simple-mult-24 \
        -- -j $(nproc)
```

#### Run simple-mult-24
```
./cmake-build-debug-cuda/bin/examples/encrypt/simple-mult-24
```
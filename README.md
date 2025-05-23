### Environment Preparation:
Instructions [here](ENVIRONMENT.md)

### 1. Connect to the Encrypt-HAvm:
```
ssh -i ~/.ssh/Encrypt_HAtestVM_keys.pem azureuser@20.73.185.100
```
You nedd 2 terminals :
- Terminal 1 is for manipulating containers. 
- Terminal 2 is for data sharing.

### Clone the repository:
*(in Terminal 1)*
```
git clone git@github.com:JPBultel/FHE-HA-Integration.git
```

Checkout to uniman branch :
```
cd ~/FHE-HA-Integration/
git checkout uniman
```

### 2. Encryption:

#### 2.1 Build the Encryption tool:
*(in Terminal 1)* 
```
cd ~/FHE-HA-Integration/HEHAEnc &&\
sudo docker build -t heha-enc .
```
#### 2.2 Run the Encryption tool:
*(in Terminal 1)*
```
sudo docker run -it --name heha-enc heha-enc
```
In the docker terminal run:
```
./encrypt-fintech-setup
```

#### 2.3 Share the keys and the encrypted data:
*(in Terminal 2)*
```
cd ~/FHE-HA-Integration/HEHAMain/build/data &&\
sudo docker cp heha-enc:/bdt/build/results/cryptocontext.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/key-public.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/key-eval-mult.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_data0.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_data1.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_data2.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_data3.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_data4.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_data5.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_data6.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_tree0.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_tree1.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_tree2.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_tree3.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_tree4.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_tree5.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/encrypted_tree6.txt . &&\
cd ../../../HEHADec/build/data &&\
sudo docker cp heha-enc:/bdt/build/results/cryptocontext.txt . &&\
sudo docker cp heha-enc:/bdt/build/results/key-private.txt .
```

#### 2.4 Close the running docker container with in Terminal 1 with `Ctrl-D`.

************************************************************************************
### 3. Perform the homomorphic evaluation:
************************************************************************************

#### 3.1 Build the Evaluation Tool:
*(in Terminal 1)*
```
cd ~/FHE-HA-Integration/HEHAMain &&\
sudo docker build -t heha-main .
```

#### 3.2 Run the Evaluation tool:
*(in Terminal 1)*
```
sudo docker run --gpus all -it --name heha-main heha-main
```
In the running docker terminal execute:
```
./encrypt-fintech-analytics
```

#### 3.3 Share the encrypted result:
*(in Terminal 2)*
```
sudo docker cp heha-main:/bdt/build/results/output_ciphertext.txt ~/FHE-HA-Integration/HEHADec/build/data/output_ciphertext.txt
```
#### 3.4 Exit:
*(in Terminal 1)*

ctrl-D

************************************************************************************
### 4 Decrypt the result:
************************************************************************************

#### 4.1 Build the Decryption Tool:
*(in Terminal 1)*
```
cd ~/FHE-HA-Integration/HEHADec &&\
sudo docker build -t heha-dec .
```

#### 4.2 Run the Decryption Tool:
*(in Terminal 1)*
```
sudo docker run -it --name heha-dec heha-dec
```
In the running docker terminal execute:
```
./encrypt-fintech-getresult
```


#### Old, Notes:

The code has been updated. Now the binary files produced are supposed to be executed without argument: ./name_of_the_file

There are 4 folders. 
- HEHAEnc is a setup/encrytion tool.
- HEHADec is a decryption tool.
- HEHAMain is the analytic component that works on encrypted data.
- accfhe-fintech is a pre-filled integration environment.

The clear inputs (binary decision tree and data tree) are now txt files inside HEHAEnc/build/data (instead of being hard-coded like in the previous version of the code).

TODO
1) Update the CUDA parts of the dockerfiles in HEHAEnc, HEHAMain, HEHADec and accfhe-fintech/Docker for compliance with the ENCRYPT platform.
2) Build and run a container from HEHAMain (docker build (...), then docker run -it (...)).
3) Extract the binary file encrypt-fintech-analytics (generated inside this container) and copy it in accfhe-fintech/app.
4) Push the folder accfhe-fintech to the ENCRYPT platform.
# fhe-ha

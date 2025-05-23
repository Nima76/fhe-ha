# *Wrapper* tool

The purpose of the **Wrapper** tool is to easily integrate computations supported by Privacy-Preserving Technologies (PPTs) into the ENCRYPT platform. It achieves this by managing the creation of containerized applications, without the need for direct alterations of the script.

The **Wrapper** tool performs 3 operations: 
* Store the data uploaded by the data owner
* Start the computation
* Send the result to the platform

## Structure of *Wrapper* Tool
Within the [PPT_common](/fhe-fintech) directory, there are all the essential resources required for building the Wrapper Docker image for different computations. Thus, for the image, we employ the same template but with minor variations.

Specifically, there are common resources for all computations, such as
the '[wrapper.py](/fhe-fintech/wrapper.py)' and '[server.py](/fhe-fintech/server.py)' scripts, and resources related to a particular computation, including:

* Dockerfiles for creating Docker images in the [Docker](/fhe-fintech/Docker) subdirectory.
* Files required for the computation in the subdirectories `<ppt_domain_computation>` (e.g., [tee_fintech_training](/fhe-fintech/app)). 



# How to use the *Wrapper* tool to integrate the PPT in the ENCRYPT Platform


## STEP 1 - Configuration
Create a specific directory for the computation named `<ppt_domain_computation>` (e.g., [tee_fintech_training](/fhe-fintech/app)), within the [PPT_common](/fhe-fintech) directory. Inside this folder, all the resources necessary for the computation (e.g., the binary, configuration files) must be available.

## STEP 2 - Build
### Dockerfile
Copy the template [Dockerfile](/fhe-fintech/Docker/Dockerfile.template) for your specific computation, and set the `SRC_DIR` environment variable
```
# Folder that contains the computation script and resources
ENV SRC_DIR="${PPT_DIR}/{here_your_directory}" 
```
For example:
```
ENV SRC_DIR="${PPT_DIR}/app" 
```

The Dockerfile name must follows a pattern `<Dockerfile>` (e.g. [Dockerfile](/fhe-fintech/Docker/Dockerfile))

### 'docker build'
Execute the *docker build* command from the parent folder of **PPT_common** since we are copying it into the container.

For example, from the [encrypt-project](/) folder run:
```
cd encrypt-project
docker build -t encryptdev/fhe_fintech:0.1 -f .//Docker/Dockerfile.tee.fintech.training .
```

The Docker image name must follows a pattern `encryptdev/<ppt_domain_computation>:tag` (e.g. `encryptdev/app:0.1`)


## STEP 3 - Set Environment Variables and Run
For the execution of the application we need 2 environment variables:

* **API_URL** *= the URL of the API endpoint for uploading the computation result*
* **CMD** *= the command to start the privacy-preserving tecnology whose arguments must be split with commas*

Example:

```
docker run --rm -p 9443:9443 --name manager -e API_URL='http://10.0.0.6:9000/computation_output'  \
 -e CMD='gramine-sgx,./app/fintech,/fhe-fintech/app/fintech_src.py' encryptdev/app:0.1.1
```

### Note
Please, open an Issue in the repository to ask for a new PPT integration. The TRUSTUP team will react promptly. 




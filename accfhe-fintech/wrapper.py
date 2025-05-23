import os, sys, requests, subprocess, functools, zipfile, time
# to flush the print in gramine (sometime it gets stucked)
print = functools.partial(print, flush=True)

# environment variable set in the 'docker run' command
cmd = os.getenv('CMD')                                          # Command that starts the computations
api_url = os.getenv('API_URL')                                  # For api URL to upload the computation result
computation_script_path = os.getenv('COMPUTATION_SCRIPT_PATH')  # For the script that start the computation 
# env variables created in the Dockerfile
result_file = os.getenv('RESULT_FILE')                          # File where we save the compuation result
computation_ID_file = os.getenv('COMPUTATION_ID_FILE')          # File where we save the compuation ID
src_dir = os.getenv('SRC_DIR')                                  # Path to the folder that contains the computation script and resources
ppt = os.getenv('PPT')                                          # For the privacy-preserving technology to be used
computation_id= os.getenv('COMPUTATION_ID')                     # For the privacy-preserving technology to be used
result_dir = os.getenv('RESULT_DIR')                            # For the PPT results directory
path_to_zip_file = os.path.join(src_dir,f'{computation_id}_result.zip')
# The server is the script that enables the data owner to upload their data
def start_server():
    script_path = '../server.py'
    # Execute the script
    print ('========Executing the server to upload the data!========\n')
    if ppt == 'tee':
        result = subprocess.run(['gramine-sgx', './server', script_path], stdout=subprocess.PIPE)
    else:
        result = subprocess.run(['/usr/bin/python3.9', script_path], stdout=subprocess.PIPE)
    # Extract stdout
    stdout = result.stdout.decode('utf-8')
    print("========Output of the script=========\n")
    print(stdout)
    print ('========The server stopped!==========\n')


# Start the computation
def start_computation():
    # Split the string 'cmd' into substrings using comma as separator
    # we do this to determine the command that needs to be launched in subprocess.run.
    commands = cmd.split(',')
    # Remove any whitespace
    commands = [command.strip() for command in commands]

    # Execute the script
    print ('========Executing the computation!========\n')
    result = subprocess.run(commands, stdout=subprocess.PIPE)
    # Extract stdout
    stdout = result.stdout.decode('utf-8')
    print("========Output of the script========\n")
    print(stdout)
    print ('========Computation complete!========\n')

def add_to_zip():
    print('======== Creating ZIP Archive! ========\n')    
    with zipfile.ZipFile(path_to_zip_file, 'w', zipfile.ZIP_DEFLATED) as z:
        for root, dirs, files in os.walk(result_dir):
            for file in files:
                if not file.startswith('.'):
                    archive_name = os.path.relpath(os.path.join(root, file), result_dir)
                    z.write(os.path.join(root, file), archive_name)
    z.close()
    print('======== ZIP archive created! ========\n')

# function that sends the result of the computation to the api ingress service
def send_to_api():
    # with open(computation_ID_file, "r") as f:
    #     computation_ID = f.read()
    try:
        with open(result_file, "r") as file:
            content = file.read()
    except:
        content = "result was encrypted, Download ZIP file to access computation output"
    data = {'computation_ID': computation_id, 'result': content}

    if os.path.exists(path_to_zip_file):
        with open(path_to_zip_file, 'rb') as file:
            result_zip = {'file': (f"{computation_id}_result.zip", file, 'application/zip')}
            response = requests.post(api_url, data=data, files=result_zip)
    else: 
        response = requests.post(api_url, json=data)

    if response.status_code == 200:
        print('POST request was successful!')
        print('Response content:')
        print(response.text)
    else:
        print(f'POST request failed with status code: {response.status_code}')
    sys.exit()    
    #os.kill(os.getpid(), signal.SIGINT)


if __name__ == '__main__':
    print('========Starting the manager========\n')
    start_server()
    start_computation()
    add_to_zip()
    send_to_api()

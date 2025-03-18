import socket
import json
import threading
import time
import hashlib
import os

SERVER_IP = '192.168.0.37'
JSON_PORT = 890
BINARY_PORT = 891
FILE_SAVE_PATH = "G:/work/test/sdk_api/"
#camera_id = "left"
camera_id = "right"

def send_json_request(host, port, request):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        print("Connecting to", host, port)
        sock.connect((host, port))
        print("Sending request:", request)
        sock.sendall(json.dumps(request).encode('utf-8'))
        response = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
        return json.loads(response.decode('utf-8'))

def send_binary_request(host, port, request):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((host, port))
        sock.sendall(json.dumps(request).encode('utf-8'))
        response = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
        return response

def send_binary_file(host, port, file_path):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((host, port))
        data_len = 0
        with open(file_path, "rb") as file:
            while chunk := file.read(4096):
                data_len += len(chunk)
                sock.sendall(chunk)
        print("File Sent:", data_len)
        response = sock.recv(4096).decode('utf-8')
        return json.loads(response)

def test_get_camera_param(host, port):
    request = {"cmd": "get_camera_param_req", "camera_id": camera_id}
    response = send_json_request(host, port, request)
    print("Get Camera Param Response:", response)

def test_get_dev_info(host, port):
    request = {"cmd": "get_dev_info_req"}
    response = send_json_request(host, port, request)
    print("Get Device Info Response:", response)

def test_set_camera_param(host, port):
    request = {
        "cmd": "set_camera_param_req",
        "camera_id": camera_id,
        "gain": 10,
        "expose": 20,
        "gamma": 30,
        "framerate": 25,
        "ae_mode": "manual",
        "awb_mode": "manual"
    }
    response = send_json_request(host, port, request)
    print("Set Camera Param Response:", response)

def test_get_dnn_counting(host, port):
    request = {"cmd": "get_dnn_counting_req", "camera_id": camera_id}
    response = send_json_request(host, port, request)
    print("Get DNN Counting Response:", response)
    if response.get("cmd") == "get_dnn_counting_rsp" and response.get("ret_code") == 0:
        incar = response.get("incar")
        inbus = response.get("inbus")
        inped = response.get("inped")
        incycle = response.get("incycle")
        intruck = response.get("intruck")
        outcar = response.get("outcar")
        outbus = response.get("outbus")
        outped = response.get("outped")
        outcycle = response.get("outcycle")
        outtruck = response.get("outtruck")
        print(f"Incar: {incar}, Inbus: {inbus}, Inped: {inped}, Incycle: {incycle}, Intruck: {intruck}")
        print(f"Outcar: {outcar}, Outbus: {outbus}, Outped: {outped}, Outcycle: {outcycle}, Outtruck: {outtruck}")

def test_get_model_info(host, port):
    request = {"cmd": "get_model_info_req","camera_id": camera_id}
    response = send_json_request(host, port, request)
    print("Get Model Info Response:", response)

file_size = 0
file_md5 = ""
file_format = ""

def test_download_file_info(host, port):
    global file_size, file_md5, file_format
    request = {"cmd": "download_file_req", "camera_id": camera_id, "type": "image"}
    print("Download File Info Request:", request)
    response = send_json_request(host, port, request)
    print("Download File Info Response:", response)
    if response.get("cmd") == "download_file_info_rsp" and response.get("ret_code") == 0:
        file_size = int(response.get("size"))  # Convert file_size to integer
        file_md5 = response.get("md5")
        file_format = response.get("format")

def get_file_info(file_path):
    size = os.path.getsize(file_path)
    md5_hash = hashlib.md5()
    with open(file_path, "rb") as file:
        for chunk in iter(lambda: file.read(4096), b""):
            md5_hash.update(chunk)
    md5 = md5_hash.hexdigest()
    return size, md5

def test_upload_file_info(host, port):
    file_path = FILE_SAVE_PATH + "model.zip"  # Replace with the actual path to your file
    size, md5 = get_file_info(file_path)
    request = {
        "cmd": "update_model_packet_req",
        "camera_id": camera_id,
        "size": size,
        "md5": md5,
        "format": "zip",
        "version": "1.0"
    }
    response = send_json_request(host, port, request)
    print("Upload File Info Response:", response)

def test_download_file(host, port):
    global file_size, file_md5, file_format
    global download_cnt

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((host, port))
        #file_path = FILE_SAVE_PATH + "cam2_down." + file_format
        file_path = FILE_SAVE_PATH + "download." + file_format
        received_size = 0

        with open(file_path, "wb") as file:
            while received_size < file_size:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                file.write(chunk)
                received_size += len(chunk)
  
        # Validate the file size
        if received_size != file_size:
            print(f"File size mismatch: expected {file_size}, got {received_size}")

        # Validate the file MD5
        received_md5 = hashlib.md5()
        with open(file_path, "rb") as file:
            for chunk in iter(lambda: file.read(4096), b""):
                received_md5.update(chunk)
        received_md5 = received_md5.hexdigest()
        if received_md5 != file_md5:
            print(f"File MD5 mismatch: expected {file_md5}, got {received_md5}")

def test_upload_file(host, port):
    file_path = FILE_SAVE_PATH + "/model.zip"  # Replace with the actual path to your file
    response = send_binary_file(host, port, file_path)
    print("Upload File Response:", response)

def test_cds_get_request_param(host, port, cmd):
    request = {"cmd": cmd, "camera_id": camera_id}
    response = send_json_request(host, port, request)
    print("CDS Response:", response)

def run_tests():
    host = SERVER_IP
    json_port = JSON_PORT
    binary_port = BINARY_PORT

    # Run tests
    test_get_dev_info(host, json_port)
    test_get_camera_param(host, json_port)
    test_set_camera_param(host, json_port)
    test_get_dnn_counting(host, json_port)
    test_get_model_info(host, json_port)
    
    test_download_file_info(host, json_port)
    test_download_file(host, binary_port)

    test_upload_file_info(host, json_port)
    test_upload_file(host, binary_port)
    
    # test_cds_get_request_param(host, json_port, cmd="cds_get_config_req")
    # test_cds_get_request_param(host, json_port, cmd="cds_get_frame_output_req")
    # test_cds_get_request_param(host, json_port, cmd="cds_get_event_req")

def stress_test(iterations):
    for i in range(iterations):
        print(f"Iteration: {i+1}")
        run_tests()
        time.sleep(1)  # Add a short delay between iterations if needed

def test_invalid_json(host, port):
    request = "Invalid JSON 123123 {}"
    try:
        response = send_json_request(host, port, request)
        print("Invalid JSON Response:", response)
    except json.JSONDecodeError:
        print("Caught JSONDecodeError as expected")

def test_missing_camera_id(host, port):
    request = {"cmd": "get_camera_param_req"}
    response = send_json_request(host, port, request)
    print("Missing Camera ID Response:", response)

def test_invalid_camera_id(host, port):
    request = {"cmd": "get_camera_param_req", "camera_id": "invalid"}
    response = send_json_request(host, port, request)
    print("Invalid Camera ID Response:", response)

def test_unknown_command(host, port):
    request = {"cmd": "unknown_command"}
    response = send_json_request(host, port, request)
    print("Unknown Command Response:", response)

def run_invalid_tests():
    host = SERVER_IP
    json_port = JSON_PORT

    test_invalid_json(host, json_port)
    # test_missing_camera_id(host, json_port)
    # test_invalid_camera_id(host, json_port)
    # test_unknown_command(host, json_port)

    test_get_dev_info(host, json_port)

if __name__ == "__main__":
    
    stress_test(1000)  # Run the stress test with 100 iterations
    #run_invalid_tests()  # Run the invalid tests to check server robustness

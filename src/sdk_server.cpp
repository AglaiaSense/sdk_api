#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <iomanip>  // Include this header for std::setw and std::setfill
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  // Include this header for inet_addr
#include <unistd.h>
#include <openssl/md5.h>
#include <filesystem>  // Include this header for filesystem operations
//#include <chrono>  // Include this header for sleep_for
//#include <thread>  // Include this header for sleep_for
#include <csignal>  // Include this header for signal handling
#include <fcntl.h>  // Include this header for fcntl
#include <mutex>

#include "sdk_pack_json.h"
#include "json.hpp"  // Include the JSON for Modern C++ header
#include "sdk_log.h"
#include "date_utils.h"  // Include the new header

using json = nlohmann::json;
namespace fs = std::filesystem;  // Use the correct namespace

#define CONFIG_FILE_PATH "/home/root/AglaiaSense/resource/share_config/gs501.json"
#define MODEL_FILE_PATH "/home/root/AglaiaSense/GS501/image/CustomNet/"

#define SDK_VERSION "v1.7"

#define BUFFER_SIZE 4096
#define MAIN_LOCAL_PORT 10808
#define WEB_LOCAL_PORT 11808

#define CMD_UNKNOWN "unknown"
#define CMD_GET_DEV_INFO_REQ "get_dev_info_req"
#define CMD_GET_CAMERA_PARAM_REQ "get_camera_param_req"
#define CMD_SET_CAMERA_PARAM_REQ "set_camera_param_req"
#define CMD_GET_DNN_COUNTING_REQ "get_dnn_counting_req"
#define CMD_GET_MODEL_INFO_REQ "get_model_info_req"
#define CMD_DOWNLOAD_FILE_REQ "download_file_req"
#define CMD_UPDATE_MODEL_POCKET_REQ "update_model_packet_req"
#define CMD_UPDATE_MODEL_POCKET_RSP "update_model_packet_rsp"
#define CMD_CDS_GET_CONFIG_REQ "cds_get_config_req"
#define CMD_CDS_SET_CONFIG_REQ "cds_set_config_req"
#define CMD_CDS_GET_FRAME_OUTPUT_REQ "cds_get_frame_output_req"
#define CMD_CDS_GET_EVENT_REQ "cds_get_event_req"

std::string commands_list[] = {CMD_GET_DEV_INFO_REQ, 
    CMD_GET_CAMERA_PARAM_REQ, CMD_SET_CAMERA_PARAM_REQ, 
    CMD_GET_DNN_COUNTING_REQ, CMD_GET_MODEL_INFO_REQ, 
    CMD_DOWNLOAD_FILE_REQ, CMD_UPDATE_MODEL_POCKET_REQ,
    CMD_CDS_GET_CONFIG_REQ, CMD_CDS_SET_CONFIG_REQ,
    CMD_CDS_GET_FRAME_OUTPUT_REQ, CMD_CDS_GET_EVENT_REQ};


enum{
    SET_GAIN = 0x10,
    GET_GAIN,
    SET_EXPOSURE,
    GET_EXPOSURE,
    SET_FRAMERATE,
    GET_FRAMERATE,
    GET_AE_MODE,
    SET_AE_MODE,
    GET_AWB_MODE,
    SET_AWB_MODE, 
    BMP_GET,
    RAW_GET,
    DNN_GET,
    GET_FIRMWARE,
    UPDATE_FIRMWARE,
    ROI_SET,
    ROI_GET,
    CAM_EN,
    ENERGENCY_MODE,
    WHOIAM,
    GET_STATUS,
    GET_VERSION
};

typedef struct {
    uint8_t cmd;   // CMD
    uint8_t val;   //
}Trans_Data_t;

typedef struct {
    uint8_t cmd;   // CMD
    uint32_t val;   //
}Trans_Mode_t;

typedef struct {
    int size;
    std::string md5;
    std::string format = "";
    std::string file_path;
    std::string camera_id;
    std::string mode;
}File_Info_t;

typedef struct {
    std::string camera_id;
    std::string product_model;
    std::string sdk_version;
    std::string sn;
    std::string device_id0;
    std::string device_id1;
    std::string dnn_model;
    std::string passwd;
}SVR_Config_t;


File_Info_t g_file_info;
std::mutex g_file_mtx;

std::string g_model_version = "v1.0";

SVR_Config_t g_svr_cfg;

void clear_file_info() {
    g_file_info.size = 0;
    g_file_info.md5 = "";
    g_file_info.format = "";
    g_file_info.file_path = "";
    g_file_info.camera_id = "";
    g_file_info.mode = "";
}

std::pair<std::string, std::string> get_file_info(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("File not found: " + file_path);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read file: " + file_path);
    }

    unsigned char md5_hash[MD5_DIGEST_LENGTH];
    MD5((unsigned char *)buffer.data(), buffer.size(), (unsigned char *)md5_hash);

    std::ostringstream md5_str;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        md5_str << std::hex << std::setw(2) << std::setfill('0') << (int)md5_hash[i];
    }

    return {std::to_string(size), md5_str.str()};
}

bool is_valid_json(const std::string& input) {
    if (!json::accept(input)) return false;  // 语法校验
    json data = json::parse(input);
    return data.is_structured();  // 类型校验
}

void send_comm_response(int client_socket, const std::string& cmd_string, int ret_code, const std::string& message) {
    json response_json;

    if (ret_code == 0) {
        response_json = {
            {"cmd", cmd_string},
            {"ret_code", ret_code}
        };
    }else {
        response_json = {
            {"cmd", cmd_string},
            {"ret_code", ret_code},
            {"message", message}
        };
    }

    std::string response_data = response_json.dump();
    send(client_socket, response_data.c_str(), response_data.size(), 0);
    log_message("Send response: " + response_data);
    close(client_socket);
}

void send_err_response(int client_socket, const std::string& rsp_cmd, const std::string& message) {
    json response_json = pack_comm_rsp_json(-1, rsp_cmd, message);
    std::string response_data = response_json.dump();
    send(client_socket, response_data.c_str(), response_data.size(), 0);
    log_message("Send Err response: " + response_data);
    close(client_socket);
}

bool get_cmd_from_json(const json& request_json, std::string& cmd_str) {
    std::string cmd = request_json.value("cmd", "");

    for (const auto& command : commands_list) {
        if (cmd == command) {
            cmd_str = cmd;
            return true;
        }
    }
    return false;
}

bool read_config() {
    json config;
    try {
        std::ifstream file(CONFIG_FILE_PATH);
        if (!file.is_open()) {
            log_message("Open gs501.json config file error ");
            throw std::runtime_error("Failed to open config file: ");
        }

        file >> config;

        g_svr_cfg.camera_id = config.value("SensorNum", "");
        g_svr_cfg.product_model = config.value("ModelNumber", "");
        g_svr_cfg.sdk_version = SDK_VERSION;
        g_svr_cfg.sn = config.value("SerialNumber", "");
        g_svr_cfg.dnn_model = config.value("DNNModel", "");
        g_svr_cfg.passwd = config.value("ROOT_PASSWORD", "");
        
        if(g_svr_cfg.camera_id == "dual") {
            g_svr_cfg.device_id0 = config.value("DevId0", "");
            g_svr_cfg.device_id1 = config.value("DevId1", "");
        }else if(g_svr_cfg.camera_id == "left") {
            g_svr_cfg.device_id0 = config.value("DevId0", "");
        }else if(g_svr_cfg.camera_id == "right") {
            g_svr_cfg.device_id1 = config.value("DevId1", "");
        }

        return true;
    } catch (const std::exception& e) {
        log_message(e.what());
        return false;
    }
}

bool get_camera_id_from_json(const json& request_json, const std::string& cmd, std::string& camera) {
    if (cmd == CMD_GET_DEV_INFO_REQ) {
        // No need to check camera_id for this command
        return true;
    }
    std::string camera_id = request_json.value("camera_id", "");

    if (g_svr_cfg.camera_id == "dual") {
        if (camera_id != "left" && camera_id != "right") {
            log_message("input camera_id is invalid," + camera_id);
            return false;
        }
    }else {
        if (g_svr_cfg.camera_id != camera_id) {
            log_message("input camera_id is invalid");
            return false;
        }
    }

    camera = camera_id;
    return true;
}

void parse_cds_set_config(const json& request_json) {
    #if 0
    if (request_json.contains("app_config")) {
        const auto& app_config = request_json["app_config"];

        if (app_config.contains("general_config")) {
            const auto& general_config = app_config["general_config"];
            std::string source_type = general_config.value("source_type", "");
            int camera_id = general_config.value("camera_id", 0);
            int input_tensor_height = general_config.value("input_tensor_height", 0);
            int input_tensor_width = general_config.value("input_tensor_width", 0);
            int output_tensor_height = general_config.value("output_tensor_height", 0);
            int output_tensor_width = general_config.value("output_tensor_width", 0);
            // Process general_config as needed
        }

        if (app_config.contains("counting_config")) {
            const auto& counting_config = app_config["counting_config"];
            std::string counting_type = counting_config.value("counting_type", "");
            const auto& counting_interval = counting_config["counting_interval"];
            std::string unit = counting_interval.value("unit", "");
            int interval = counting_interval.value("interval", 0);

            const auto& counting_boundary = counting_config["counting_boundary"];
            for (const auto& boundary : counting_boundary) {
                std::string boundary_id = boundary.value("boundary_id", "");
                const auto& geometry = boundary["geometry"];
                std::string type = geometry.value("type", "");
                const auto& coordinates = geometry["coordinates"];
                // Process counting_boundary as needed
            }
        }

        if (app_config.contains("curb_config")) {
            const auto& curb_config = app_config["curb_config"];

            if (curb_config.contains("zone")) {
                const auto& zones = curb_config["zone"];
                for (const auto& zone : zones) {
                    std::string curb_zone_id = zone.value("curb_zone_id", "");
                    const auto& geometry = zone["geometry"];
                    std::string type = geometry.value("type", "");
                    const auto& coordinates = geometry["coordinates"];
                    // Process zones as needed
                }
            }

            if (curb_config.contains("space")) {
                const auto& spaces = curb_config["space"];
                for (const auto& space : spaces) {
                    std::string curb_space_id = space.value("curb_space_id", "");
                    const auto& geometry = space["geometry"];
                    std::string type = geometry.value("type", "");
                    const auto& coordinates = space["coordinates"];
                    // Process spaces as needed
                }
            }

            if (curb_config.contains("object")) {
                const auto& objects = curb_config["object"];
                for (const auto& object : objects) {
                    std::string curb_object_id = object.value("curb_object_id", "");
                    const auto& geometry = object["geometry"];
                    std::string type = geometry.value("type", "");
                    const auto& coordinates = geometry["coordinates"];
                    std::string object_type = object.value("object_type", "");
                    // Process objects as needed
                }
            }
        }

        if (app_config.contains("asset_config")) {
            const auto& asset_config = app_config["asset_config"];
            int dwell_time_threshold = asset_config.value("dwell_time_threshold", 0);
            // Process asset_config as needed
        }

        if (app_config.contains("ped_config")) {
            const auto& ped_config = app_config["ped_config"];
            const auto& zones = ped_config["zone"];
            for (const auto& zone : zones) {
                std::string ped_monitor_zone_id = zone.value("ped_monitor_zone_id", "");
                const auto& geometry = zone["geometry"];
                std::string type = geometry.value("type", "");
                const auto& coordinates = zone["coordinates"];
                // Process ped_config zones as needed
            }
        }
    }
    #endif
}


std::string get_latest_jpg_file(const std::string& directory_path) {
    std::string latest_file;
    std::time_t latest_time = 0;

    for (const auto& entry : fs::directory_iterator(directory_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".jpg") {
            auto file_time = fs::last_write_time(entry);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(file_time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            auto cftime = std::chrono::system_clock::to_time_t(sctp);

            if (cftime > latest_time) {
                latest_time = cftime;
                latest_file = entry.path().string();
            }
        }
    }

    return latest_file;
}
bool connect_to_local_socket_server(int port, void *data, int size, char *recv_buf = NULL) {
    std::string host = "127.0.0.1";

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        log_message("local socket: Failed to create socket");
        return false;
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());
    server_addr.sin_port = htons(port);

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_message("local socket: Failed to connect to server, host: " + host + ", port: " + std::to_string(port));
        close(client_socket);
        return false;
    }

    send(client_socket, (uint8_t *)data, size, 0);

    if(recv_buf == nullptr) {
        close(client_socket);
        return true;
    }

    int bytes_received = recv(client_socket, recv_buf, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        log_message("local socket: Failed to receive data ");
        close(client_socket);
        return false;
    } 
    close(client_socket);
    return true;
}
 
bool send_request_to_main(Trans_Data_t &data, std::string &camera_id, char *recv_buf = NULL, int sleep_time = 0) {
    int port = MAIN_LOCAL_PORT;

    if(camera_id == "right") {
        port += 1;
    }

    if (!connect_to_local_socket_server(port, &data, sizeof(data), recv_buf)) {
        return false;
    }

    // Add a delay of 100ms
    if (sleep_time > 0) {
        usleep(sleep_time * 1000);
    }
    return true;
}

bool send_request_to_web(Trans_Data_t &data, std::string &camera_id, SDK_Dnn_Counting_t &dnn_counting) {
    int port = WEB_LOCAL_PORT;
    char recv_buf[1024] = {0};

    if(camera_id == "right") {
        port += 1;
    }

    if (!connect_to_local_socket_server(port, &data, sizeof(data), recv_buf)) {
        log_message("Failed to connect to web server");
        return false;
    }

    std::string response_data(recv_buf);
    json response_json = json::parse(response_data);

    dnn_counting.in_count[0] = response_json.value("incar", 0);
    dnn_counting.in_count[1] = response_json.value("inbus", 0);
    dnn_counting.in_count[2] = response_json.value("inped", 0);
    dnn_counting.in_count[3] = response_json.value("incycle", 0);
    dnn_counting.in_count[4] = response_json.value("intruck", 0);
    
    dnn_counting.out_count[0] = response_json.value("outcar", 0);
    dnn_counting.out_count[1] = response_json.value("outbus", 0);
    dnn_counting.out_count[2] = response_json.value("outped", 0);
    dnn_counting.out_count[3] = response_json.value("outcycle", 0);
    dnn_counting.out_count[4] = response_json.value("outtruck", 0);

    return true;
}

char g_recv_buf[1024];
char g_recv_json_buf[BUFFER_SIZE];

void handle_client_connection(int client_socket) {
    static int read_cfg_flag = 0;

    if (read_cfg_flag == 0) {
        if (read_config()) {
            read_cfg_flag = 1;
            log_message("Read config file successfully");
        }        
    }

    try {
        std::string cmd;
        std::string camera_id;
        std::string rsp_cmd;
        std::string rsp_msg;
        int ret_code = 0;
        json request_json;
        json response_json;
        json config;

        int bytes_received = recv(client_socket, g_recv_json_buf, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            log_message("Failed to receive data");
            close(client_socket);
            return;
        }

        g_recv_json_buf[bytes_received] = '\0';
        std::string request_data(g_recv_json_buf);

        if (!is_valid_json(request_data)) {
            send_comm_response(client_socket, CMD_UNKNOWN, -1, "Invalid JSON format");
            log_message("Invalid JSON format");
            return;
        }

        log_message("Recv Cmd Json: " + request_data);
        request_json = json::parse(request_data);

        // Check if the command is valid
        if (!get_cmd_from_json(request_json, cmd)) {
            send_comm_response(client_socket, CMD_UNKNOWN, -1, "cmd is invalid");
            log_message("cmd is invalid");
            return;
        }
        rsp_cmd = cmd;

        // Check if the camera_id is valid
        if(!get_camera_id_from_json(request_json, cmd, camera_id)) {
            send_comm_response(client_socket, rsp_cmd, -1, "Invalid JSON format, camera_id must be 'left' or 'right'");
            log_message("Invalid JSON format, camera_id must be 'left' or 'right'");
            return;
        }

        //get response command
        size_t pos = rsp_cmd.rfind("req");
        if (pos != std::string::npos) {
            rsp_cmd.replace(pos, 3, "rsp");  // 替换长度为3的子串
        }

        try {
            if (cmd == CMD_GET_CAMERA_PARAM_REQ) {
                Trans_Data_t data;
                Trans_Data_t *p_recv_data = (Trans_Data_t *)g_recv_buf;
                SDK_Camera_Param_t camera_param = {0};

                data.cmd = GET_GAIN;
                if(!send_request_to_main(data, camera_id, (char *)&g_recv_buf, 0)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }
                camera_param.gain = p_recv_data->val;

                data.cmd = GET_EXPOSURE;
                if(!send_request_to_main(data, camera_id, (char *)&g_recv_buf, 0)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }
                camera_param.expose = p_recv_data->val;

                data.cmd = GET_FRAMERATE;
                if(!send_request_to_main(data, camera_id, (char *)&g_recv_buf, 0)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }
                camera_param.framerate = p_recv_data->val;

                data.cmd = GET_AE_MODE;
                if(!send_request_to_main(data, camera_id, (char *)&g_recv_buf, 0)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }

                Trans_Mode_t *p_recv_mode = (Trans_Mode_t *)g_recv_buf;
                camera_param.ae_mode = p_recv_mode->val;

                data.cmd = GET_AWB_MODE;
                if(!send_request_to_main(data, camera_id, (char *)&g_recv_buf, 0)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }
                camera_param.awb_mode = p_recv_mode->val;
                response_json = pack_get_camera_param_rsp_json(camera_param);
            } else if (cmd == CMD_GET_DEV_INFO_REQ) { 
                response_json = pack_get_device_info_rsp_json(g_svr_cfg.product_model, g_svr_cfg.sdk_version, g_svr_cfg.camera_id, g_svr_cfg.sn);
            } else if (cmd == CMD_SET_CAMERA_PARAM_REQ) {
                Trans_Data_t data;
                char tmp_val = 0;

                //set gain
                tmp_val = request_json.value("gain", 0);
                if(tmp_val < 0 || tmp_val > 60) {
                    send_err_response(client_socket, rsp_cmd, "gain value over the range 0-60");
                    return;
                }
                data.cmd = SET_GAIN;
                data.val = tmp_val;
                if(!send_request_to_main(data, camera_id)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }
                
                // set exposure
                tmp_val = request_json.value("expose", 0);
                if(tmp_val < 1 || tmp_val > 33) {
                    send_err_response(client_socket, rsp_cmd, "expose value over the range 1-33");
                    return;
                }

                data.cmd = SET_EXPOSURE;
                data.val = tmp_val;
                if(!send_request_to_main(data, camera_id)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }

                //set framerate
                tmp_val = request_json.value("framerate", 0);
                if(tmp_val < 1 || tmp_val > 30) {
                    send_err_response(client_socket, rsp_cmd, "framerate value over the range 1-30");
                    return;
                }

                data.cmd = SET_FRAMERATE;
                data.val = tmp_val;
                if(!send_request_to_main(data, camera_id)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }

                Trans_Mode_t mode_data;
                std::string tmp_mode;
                
                tmp_mode = request_json.value("ae_mode", "");
                if(tmp_mode != "auto" && tmp_mode != "manual") {
                    send_err_response(client_socket, rsp_cmd, "ae_mode value must be auto or manual");
                    return;
                }
                
                mode_data.cmd = SET_AE_MODE;
                if (tmp_mode == "auto") {
                    mode_data.val = 0;
                }else {
                    mode_data.val = 1;
                }

                if(!send_request_to_main(data, camera_id)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }

                //set awb_mode
                tmp_mode = request_json.value("awb_mode", "");
                if(tmp_mode != "auto" && tmp_mode != "manual") {
                    send_err_response(client_socket, rsp_cmd, "awb_mode value must be auto or manual");
                    return;
                }
                
                mode_data.cmd = SET_AWB_MODE;
                if (tmp_mode == "auto") {
                    mode_data.val = 0;
                }else {
                    mode_data.val = 1;
                }

                if(!send_request_to_main(data, camera_id)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }
                response_json = pack_comm_rsp_json(ret_code,rsp_cmd);
            } else if (cmd == CMD_GET_DNN_COUNTING_REQ) {
                Trans_Data_t data;
                SDK_Dnn_Counting_t dnn_counting = {0};
                data.cmd = DNN_GET;
                if(!send_request_to_web(data, camera_id, dnn_counting))
                {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to web");
                    return;
                }
                response_json = pack_dnn_counting_rsp_json(dnn_counting);
            } else if (cmd == CMD_GET_MODEL_INFO_REQ) {
                std::string device_id;
                if(camera_id == "left") {
                    device_id = g_svr_cfg.device_id0;//config["DevId0"];
                }else {
                    device_id = g_svr_cfg.device_id1;//config["DevId1"];
                }
                response_json = pack_get_model_info_rsp_json(device_id, g_svr_cfg.dnn_model);
            } else if (cmd == CMD_DOWNLOAD_FILE_REQ) {
                
                std::string type = request_json.value("type", "");
                if (type != "image") {
                    send_err_response(client_socket, rsp_cmd, "type must be image");
                    return;
                }
                Trans_Data_t data;
                
                data.cmd = ENERGENCY_MODE;
                data.val = 1;

                if(!send_request_to_main(data, camera_id, NULL, 100)) {
                    send_err_response(client_socket, rsp_cmd, "Failed to send request to main");
                    return;
                }

                //std::string directory_path = "/home/root/data/bmp/left/2025-03-12/";
                std::string directory_path = "/home/root/data/bmp/" + camera_id + "/" + get_current_date() + "/";
                std::string latest_jpg = get_latest_jpg_file(directory_path);
                if (latest_jpg.empty()) {
                    send_err_response(client_socket, rsp_cmd, "No JPG files found");
                    return;
                } 

                log_message("Latest JPG file: " + latest_jpg);
                
                auto [size, md5] = get_file_info(latest_jpg);

                g_file_mtx.lock();
                g_file_info.file_path = latest_jpg;
                g_file_info.size = std::stoi(size);
                g_file_info.md5 = md5;
                g_file_info.format = "jpg";
                g_file_info.mode = "send";
                g_file_mtx.unlock();

                response_json = pack_download_file_rsp_json(g_file_info.size, g_file_info.md5);
            } else if (cmd == CMD_UPDATE_MODEL_POCKET_REQ) {
                // Extract file information from the JSON request
                printf("%s(%d): run here \n", __FUNCTION__, __LINE__);
                g_file_mtx.lock();
                printf("%s(%d): run here \n", __FUNCTION__, __LINE__);
                g_file_info.size = request_json.value("size", 0);
                g_file_info.md5 = request_json.value("md5", "");
                g_file_info.format = request_json.value("format", "");
                g_file_info.camera_id = camera_id;
                g_file_info.mode = "receive";
                g_file_mtx.unlock();
                printf("%s(%d): run here \n", __FUNCTION__, __LINE__);

                g_model_version = request_json.value("version", "");
                
                response_json = pack_comm_rsp_json(0, rsp_cmd);
                log_message("Update model pocket request received");
            }
            else if(cmd == CMD_CDS_GET_CONFIG_REQ) {
                response_json = create_cds_get_config_response();
            }
            else if(cmd == CMD_CDS_SET_CONFIG_REQ) {
                parse_cds_set_config(request_json);
                response_json = {
                    {"cmd", "cds_set_config_rsp"},
                    {"ret_code", 0}
                };
            }
            else if (cmd == CMD_CDS_GET_FRAME_OUTPUT_REQ) {
                response_json = create_cds_get_frame_output_response();
            }
            else if (cmd == CMD_CDS_GET_EVENT_REQ) {
                response_json = create_cds_get_frame_output_response_with_events();
            }
            else {
                throw std::invalid_argument("Unknown command");
            }

            // send response
            std::string response_data = response_json.dump(4);  // Pretty print with 4 spaces
            send(client_socket, response_data.c_str(), response_data.size(), 0);
            log_message("Send response: " + response_data);
        } catch (const std::exception& e) {        
            send_comm_response(client_socket, rsp_cmd, -1, e.what());
            log_message(e.what());
            return;
        }

        close(client_socket);
    } catch (const std::bad_alloc& e) {
        log_message("Memory allocation failed: " + std::string(e.what()));
        send_comm_response(client_socket, CMD_UNKNOWN, -1, "Memory allocation failed");
        close(client_socket);
    } catch (const std::exception& e) {
        log_message("Exception: " + std::string(e.what()));
        send_comm_response(client_socket, CMD_UNKNOWN, -1, "Internal server error");
        close(client_socket);
    }
}

void handle_binary_client_connection(int client_socket) {
                printf("%s(%d): run here \n", __FUNCTION__, __LINE__);
    g_file_mtx.lock();
                printf("%s(%d): run here \n", __FUNCTION__, __LINE__);

    if (g_file_info.mode == "send") {
        std::ifstream file(g_file_info.file_path, std::ios::binary);
        if (!file.is_open()) {
            log_message("Failed to open file: " + g_file_info.file_path);
            close(client_socket);
            g_file_mtx.unlock();
            return;
        }

        char buffer[BUFFER_SIZE];
        while (file.read(buffer, BUFFER_SIZE)) {
            send(client_socket, buffer, file.gcount(), 0);
        }

        send(client_socket, buffer, file.gcount(), 0);
        clear_file_info();
        g_file_mtx.unlock();
        
        std::string rsp_msg = "Send binary file data of size: " + std::to_string(g_file_info.size) + " bytes";
        log_message(rsp_msg);
        close(client_socket);
    } else if (g_file_info.mode == "receive") {
        std::string file_path = MODEL_FILE_PATH;
        int ret_code = 0;
        std::string rsp_msg;
        int file_size = 0;
                printf("%s(%d): run here \n", __FUNCTION__, __LINE__);

        if (g_file_info.camera_id == "left") {
            file_path += g_svr_cfg.device_id0 + "/model.zip";
        }else {
            file_path += g_svr_cfg.device_id1 + "/model.zip";
        }
        log_message("save model file to : "+ file_path);

        std::ofstream file(file_path, std::ios::binary);
                printf("%s(%d): run here \n", __FUNCTION__, __LINE__);
        if (!file.is_open()) {
            log_message("Failed to open file: " + file_path);
            g_file_mtx.unlock();
            send_comm_response(client_socket, CMD_UPDATE_MODEL_POCKET_RSP, -1, "Failed to open file");
            return;
        }
                printf("%s(%d): run here \n", __FUNCTION__, __LINE__);

        char buffer[BUFFER_SIZE];
        int received_size = 0;
        while (received_size < g_file_info.size) {
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                int error = errno;
                if(error == ECONNRESET) {
                    log_message("client connection closed");
                    close(client_socket);
                }else {
                    log_message("Failed to receive data");
                    send_comm_response(client_socket, CMD_UPDATE_MODEL_POCKET_RSP, -1, "recv data error");
                }

                g_file_mtx.unlock();
                return ;
            }
            file.write(buffer, bytes_received);
            received_size += bytes_received;
        }
        file_size = g_file_info.size;
        clear_file_info();
        g_file_mtx.unlock();
                printf("%s(%d): run here \n", __FUNCTION__, __LINE__);

        try {
            auto [size, md5] = get_file_info(file_path);
            printf("%s(%d): run here \n", __FUNCTION__, __LINE__);
        }catch (const std::exception& e) {        
            send_comm_response(client_socket, CMD_UPDATE_MODEL_POCKET_RSP, -1, e.what());
            log_message(e.what());
            return;
        }
        // Validate the received file
        json response_json;
        //if (std::stoi(size) == g_file_info.size && md5 == g_file_info.md5) {
        if (received_size != file_size) {
            rsp_msg = "File validation failed";
            ret_code = -1;
        }
                printf("%s(%d): run here \n", __FUNCTION__, __LINE__);

        send_comm_response(client_socket, CMD_UPDATE_MODEL_POCKET_RSP, ret_code, rsp_msg);
                        printf("%s(%d): run here \n", __FUNCTION__, __LINE__);

    }

    
}

bool json_thread_running = true;
bool data_thread_running = true;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        json_thread_running = false;
        data_thread_running = false;
    }
}

void start_server(const std::string& host, int json_port, int binary_port) {
    signal(SIGINT, signal_handler);  // Register the signal handler

    int json_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int binary_server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Set sockets to non-blocking mode
    fcntl(json_server_socket, F_SETFL, O_NONBLOCK);
    fcntl(binary_server_socket, F_SETFL, O_NONBLOCK);

    sockaddr_in json_server_addr = {};
    json_server_addr.sin_family = AF_INET;
    json_server_addr.sin_addr.s_addr = inet_addr(host.c_str());
    json_server_addr.sin_port = htons(json_port);

    sockaddr_in binary_server_addr = {};
    binary_server_addr.sin_family = AF_INET;
    binary_server_addr.sin_addr.s_addr = inet_addr(host.c_str());
    binary_server_addr.sin_port = htons(binary_port);

    bind(json_server_socket, (sockaddr*)&json_server_addr, sizeof(json_server_addr));
    bind(binary_server_socket, (sockaddr*)&binary_server_addr, sizeof(binary_server_addr));

    listen(json_server_socket, 1);
    listen(binary_server_socket, 1);

    log_message("Cmd Server listening on " + host + ":" + std::to_string(json_port));
    log_message("Data Server listening on " + host + ":" + std::to_string(binary_port));
    
    std::thread json_thread([&]() {
        while (json_thread_running) {
            int client_socket = accept(json_server_socket, nullptr, nullptr);
            if (client_socket >= 0) {
                log_message("Accepted Cmd connection");
                std::thread(handle_client_connection, client_socket).detach();
            } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
                log_message("Error on accept for Cmd Server");
            }
            usleep(10);
        }
        close(json_server_socket);
    });

    std::thread binary_thread([&]() {
        while (data_thread_running) {
            int client_socket = accept(binary_server_socket, nullptr, nullptr);
            if (client_socket >= 0) {
                log_message("Accepted Data connection");
                std::thread(handle_binary_client_connection, client_socket).detach();
            } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
                log_message("Error on accept for Data Server");
            }
            usleep(10);
        }
        close(binary_server_socket);
    });

    json_thread.join();
    binary_thread.join();
}

int main() {
    start_server("0.0.0.0", 890, 891);
    
    return 0;
}
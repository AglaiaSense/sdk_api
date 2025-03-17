
#ifndef SDK_PACK_JSON_H
#define SDK_PACK_JSON_H

#include <string>

#include "json.hpp"  // Include the JSON for Modern C++ header

using json = nlohmann::json;

typedef struct {
    int gain;
    int expose;
    int gamma;
    int framerate;
    int ae_mode;
    int awb_mode;
}SDK_Camera_Param_t;

typedef struct {
    // bus, car, cycle, ped, truck
    int in_count[5];    
    int out_count[5];
}SDK_Dnn_Counting_t;

json pack_comm_rsp_json(int ret_code, const std::string& cmd_string, const std::string& message="");
json pack_get_camera_param_rsp_json(SDK_Camera_Param_t& param);
json pack_get_model_info_rsp_json(std::string& device_id, std::string& dnn_model);
json pack_get_device_info_rsp_json(std::string& product_model, std::string& sdk_version, std::string& camera_num, std::string& sn);
json pack_download_file_rsp_json(int file_size, std::string& file_md5);
json pack_dnn_counting_rsp_json(SDK_Dnn_Counting_t& counting);

json create_cds_get_config_response();
json create_cds_get_frame_output_response();
json create_cds_get_frame_output_response_with_events();

#endif // SDK_PACK_JSON_H
#include "sdk_pack_json.h"

#define MODE_AUTO   "auto"
#define MODE_MANUAL "manual"

json pack_comm_rsp_json(int ret_code, const std::string& cmd_string, const std::string& message) {
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
    return response_json;
}

json pack_get_camera_param_rsp_json(SDK_Camera_Param_t& param) {
    std::string ae_mode;
    std::string awb_mode;
    
    if (0 == param.ae_mode) {
        ae_mode = MODE_AUTO;
    }else {
        ae_mode = MODE_MANUAL;
    }

    if (0 == param.awb_mode) {
        awb_mode = MODE_AUTO;
    }else {
        awb_mode = MODE_MANUAL;
    }
    json response_json = {
        {"cmd", "get_camera_param_rsp"},
        {"ret_code", 0},
        {"gain", param.gain},
        {"gain_range", {0, 60}},
        {"expose", param.expose},
        {"expose_range", {1, 33}},
        {"gamma", param.gamma},
        {"gamma_range", {0, 60}},
        {"framerate", param.framerate},
        {"framerate_range", {1, 30}},
        {"ae_mode", ae_mode},
        {"awb_mode", awb_mode}
    };
    return response_json;
}

json pack_get_model_info_rsp_json(std::string& device_id, std::string& dnn_model) {
    json response_json = {
        {"cmd", "get_model_info_rsp"},
        {"ret_code", 0},
        {"device_id", device_id},
        //{"model_version", "999996"},
        //{"postproc_version", "v1.0"},
        {"model_type", dnn_model}
    };
    return response_json;
}

json pack_get_device_info_rsp_json(std::string& product_model, std::string& sdk_version, std::string& camera_num, std::string& sn) {
    json response_json = {
        {"cmd", "get_dev_info_rsp"},
        {"ret_code", 0},
        {"product_model", product_model},
        {"sdk_version", sdk_version},
        {"camera_num", camera_num},
        {"sn", sn}
    };
    return response_json;
}

json pack_download_file_rsp_json(int file_size, std::string& file_md5) {
    json response_json = {
        {"cmd", "download_file_info_rsp"},
        {"ret_code", 0},
        {"size", file_size},
        {"md5", file_md5},
        {"format", "jpg"}
    };
    return response_json;
}

json pack_dnn_counting_rsp_json(SDK_Dnn_Counting_t& counting) {
    json response_json = {
        {"cmd", "get_dnn_counting_rsp"},
        {"ret_code", 0},
        {"incar", counting.in_count[0]},
        {"inbus", counting.in_count[1]},
        {"inped", counting.in_count[2]},
        {"incycle", counting.in_count[3]},
        {"intruck", counting.in_count[4]},
        {"outcar", counting.out_count[0]},
        {"outbus", counting.out_count[1]},
        {"outped", counting.out_count[2]},
        {"outcycle", counting.out_count[3]},
        {"outtruck", counting.out_count[4]}
    };
    return response_json;
}

json create_detection_results() {
    return {
        {"object_id", {0, 1, 2}},
        {"object_bbox", {
            {0.3419082760810852, 0.05486205220222473, 0.3804876208305359, 0.0761398896574974},
            {0.5049829483032227, 0.03614787384867668, 0.5245765447616577, 0.06246821954846382},
            {0.5960353016853333, 0.028746748343110085, 0.6099321842193604, 0.04440883919596672},
            {0.3697490930557251, 0.048026323318481445, 0.3932635307312012, 0.06829333305358887},
            {0.380582857131958, 0.041543416182200116, 0.4052117824554443, 0.05972048044204712}
        }},
        {"object_class", {"car", "car", "car"}},
        {"object_score", {0.5, 0.7, 0.7}}
    };
}

json create_parking_results() {
    return {
        {"parking_space_0", {
            {"occupied", true},
            {"parking_dwell_time", 5}
        }},
        {"parking_space_1", {
            {"occupied", true},
            {"parking_dwell_time", 10},
            {"double_parking_status", true},
            {"double_parking_dwell_time", 0},
            {"double_parking_location", {{-85.7629808, 38.257341}}},
            {"vehicle_blocked_lane_types", "parking"}
        }},
        {"parking_space_2", {
            {"occupied", false}
        }},
        {"parking_space_3", {
            {"occupied", false}
        }},
        {"parking_space_4", {
            {"occupied", false}
        }}
    };
}

json create_counting_results() {
    return {
        {"traffic_boundary_nb", {
            {"car", 3},
            {"truck", 0}
        }},
        {"traffic_boundary_sb", {
            {"car", 3},
            {"truck", 0}
        }},
        {"bike_boundary_nb", {
            {"bicycle", 0}
        }},
        {"bike_boundary_sb", {
            {"bicycle", 0}
        }},
        {"pedestrian_boundary_nb", {
            {"pedestrian", 0}
        }},
        {"pedestrian_boundary_sb", {
            {"pedestrian", 0}
        }}
    };
}

json create_frame_output(int frame_id) {
    return {
        {"frame_id", frame_id},
        {"timestamp", "2023-10-01T12:00:00Z"},
        {"detection_results", create_detection_results()},
        {"counting_results", create_counting_results()},
        {"parking_results", create_parking_results()}
    };
}

json create_cds_get_frame_output_response() {
    json response_json = {
        {"cmd", "cds_get_frame_output_rsp"},
        {"ret_code", 0},
        {"outputs", json::array({
            create_frame_output(0),
            create_frame_output(1)
        })}
    };

    return response_json;
}

json create_event_location(double timestamp, double speed, const std::vector<double>& coordinates) {
    return {
        {"timestamp", timestamp},
        {"speed", speed},
        {"geometry", {
            {"type", "Point"},
            {"coordinates", coordinates}
        }}
    };
}

json create_curb_mgmt_event() {
    return {
        {"event_id", 0},
        {"event_type", "park_start"},
        {"event_purpose", "parcel_delivery"},
        {"event_location", create_event_location(1696161600000, 1.21, {-85.7629808, 38.257341})},
        {"event_time", "1696161600000"},
        {"event_publication_time", "1696161600000"},
        {"curb_area_ids", {"s_4th_st"}},
        {"curb_zone_id", "parking_zone_1"},
        {"curb_space_id", "parking_space_3"},
        {"object_id", 0},
        {"vehicle_length", 670},
        {"vehicle_type", "truck"},
        {"vehicle_blocked_lane_types", {"parking"}}
    };
}

json create_counting_event() {
    return {
        {"event_id", 2},
        {"event_type", "counting"},
        {"event_purpose", "through"},
        {"event_time", "1696161600000"},
        {"event_publication_time", "1696161600000"},
        {"curb_area_ids", {"s_4th_st"}},
        {"curb_zone_id", "traffic_lane_0"},
        {"boundary_id", "traffic_NB"},
        {"vehicle_type", "car"},
        {"count", 37},
        {"speed", 1.21}
    };
}

json create_asset_event() {
    return {
        {"event_id", 4},
        {"curb_object_id", "asset_0"},
        {"curb_object_type", "signal_cabinet"},
        {"event_location", {
            {"timestamp", 1696161600000},
            {"dwell_time", 300},
            {"geometry", {
                {"type", "Point"},
                {"coordinates", {-85.7629808, 38.257341}}
            }}
        }},
        {"status", "on"},
        {"curb_area_ids", {"s_4th_st"}},
        {"curb_zone_id", "sidewalk_0"},
        {"intruder_ids", {10}}
    };
}

json create_ped_event() {
    return {
        {"event_id", 5},
        {"event_type", "enter_area"},
        {"ped_monitor_zone_id", "ped_zone_0"},
        {"pedestrian_ids", {10}},
        {"event_location", {
            {"timestamp", 1696161600000},
            {"geometry", {
                {"type", "Point"},
                {"coordinates", {-85.7629808, 38.257341}}
            }}
        }},
        {"alarm_status", "on"}
    };
}

json create_frame_output_with_events(int frame_id, const std::string& event_type) {
    json frame_output = {
        {"frame_id", frame_id},
        {"timestamp", "2023-10-01T12:00:00Z"}
    };

    if (event_type == "curb_mgmt_event") {
        frame_output["curb_mgmt_event"] = create_curb_mgmt_event();
    } else if (event_type == "counting_event") {
        frame_output["counting_event"] = create_counting_event();
    } else if (event_type == "asset_event") {
        frame_output["asset_event"] = create_asset_event();
    } else if (event_type == "ped_event") {
        frame_output["ped_event"] = create_ped_event();
    }

    return frame_output;
}

json create_cds_get_frame_output_response_with_events() {
    json response_json = {
        {"cmd", "cds_get_frame_output_rsp"},
        {"ret_code", 0},
        {"outputs", json::array({
            create_frame_output_with_events(50, "curb_mgmt_event"),
            create_frame_output_with_events(60, "counting_event"),
            create_frame_output_with_events(70, "asset_event"),
            create_frame_output_with_events(75, "ped_event")
        })}
    };

    return response_json;
}
json create_cds_get_config_response() {
    json response_json = {
        {"cmd", "cds_get_config_rsp"},
        {"ret_code", 0},
        {"app_config", {
            {"general_config", {
                {"source_type", "camera"},
                {"camera_id", 1},
                {"input_tensor_height", 480},
                {"input_tensor_width", 640},
                {"output_tensor_height", 480},
                {"output_tensor_width", 640}
            }},
            {"counting_config", {
                {"counting_type", "total"},
                {"counting_interval", {
                    {"unit", "minute"},
                    {"interval", 5}
                }},
                {"counting_boundary", json::array({
                    {
                        {"boundary_id", "traffic_NB"},
                        {"geometry", {
                            {"type", "LineString"},
                            {"coordinates", {{{100.0, 0.0}, {101.0, 1.0}}}}
                        }}
                    },
                    {
                        {"boundary_id", "traffic_SB"},
                        {"geometry", {
                            {"type", "LineString"},
                            {"coordinates", {{{100.0, 0.0}, {101.0, 1.0}}}}
                        }}
                    },
                    {
                        {"boundary_id", "bike_NB"},
                        {"geometry", {
                            {"type", "LineString"},
                            {"coordinates", {{{100.0, 0.0}, {101.0, 1.0}}}}
                        }}
                    },
                    {
                        {"boundary_id", "bike_SB"},
                        {"geometry", {
                            {"type", "LineString"},
                            {"coordinates", {{{100.0, 0.0}, {101.0, 1.0}}}}
                        }}
                    },
                    {
                        {"boundary_id", "pedestrian_NB"},
                        {"geometry", {
                            {"type", "LineString"},
                            {"coordinates", {{{100.0, 0.0}, {101.0, 1.0}}}}
                        }}
                    },
                    {
                        {"boundary_id", "pedestrian_SB"},
                        {"geometry", {
                            {"type", "LineString"},
                            {"coordinates", {{{100.0, 0.0}, {101.0, 1.0}}}}
                        }}
                    }
                })}
            }},
            {"curb_config", {
                {"zone", json::array({
                    {
                        {"curb_zone_id", "parking_zone_0"},
                        {"geometry", {
                            {"type", "Polygon"},
                            {"coordinates", {{{-73.982105, 40.767932}, {-73.973694, 40.764551}, {-73.949318, 40.796918}, {-73.958416, 40.800686}, {-73.982105, 40.767932}}}}
                        }},
                        {"curb_space_ids", {"parking_space_0", "parking_space_1"}}
                    },
                    {
                        {"curb_zone_id", "loading_zone_0"},
                        {"geometry", {
                            {"type", "Polygon"},
                            {"coordinates", {{{-73.982105, 40.767932}, {-73.973694, 40.764551}, {-73.949318, 40.796918}, {-73.958416, 40.800686}, {-73.982105, 40.767932}}}}
                        }}
                    },
                    {
                        {"curb_zone_id", "bike_lane_0"},
                        {"geometry", {
                            {"type", "Polygon"},
                            {"coordinates", {{{-73.982105, 40.767932}, {-73.973694, 40.764551}, {-73.949318, 40.796918}, {-73.958416, 40.800686}, {-73.982105, 40.767932}}}}
                        }}
                    },
                    {
                        {"curb_zone_id", "traffic_lane_0"},
                        {"geometry", {
                            {"type", "Polygon"},
                            {"coordinates", {{{-73.982105, 40.767932}, {-73.973694, 40.764551}, {-73.949318, 40.796918}, {-73.958416, 40.800686}, {-73.982105, 40.767932}}}}
                        }}
                    },
                    {
                        {"curb_zone_id", "sidewalk_0"},
                        {"geometry", {
                            {"type", "Polygon"},
                            {"coordinates", {{{-73.982105, 40.767932}, {-73.973694, 40.764551}, {-73.949318, 40.796918}, {-73.958416, 40.800686}, {-73.982105, 40.767932}}}}
                        }},
                        {"curb_object_ids", {"asset_0"}}
                    }
                })},
                {"space", json::array({
                    {
                        {"curb_space_id", "parking_space_0"},
                        {"geometry", {
                            {"type", "Polygon"},
                            {"coordinates", {{{-73.982105, 40.767932}, {-73.973694, 40.764551}, {-73.949318, 40.796918}, {-73.958416, 40.800686}, {-73.982105, 40.767932}}}}
                        }},
                        {"curb_zone_id", "parking_zone_0"}
                    },
                    {
                        {"curb_space_id", "parking_space_1"},
                        {"geometry", {
                            {"type", "Polygon"},
                            {"coordinates", {{{-73.982105, 40.767932}, {-73.973694, 40.764551}, {-73.949318, 40.796918}, {-73.958416, 40.800686}, {-73.982105, 40.767932}}}}
                        }},
                        {"curb_zone_id", "parking_zone_0"}
                    }
                })},
                {"object", json::array({
                    {
                        {"curb_object_id", "asset_0"},
                        {"geometry", {
                            {"type", "Point"},
                            {"coordinates", {-73.982105, 40.767932}}
                        }},
                        {"object_type", "signal_cabinet"},
                        {"curb_zone_id", "sidewalk_0"}
                    }
                })}
            }},
            {"asset_config", {
                {"dwell_time_threshold", 240}
            }},
            {"ped_config", {
                {"zone", json::array({
                    {
                        {"ped_monitor_zone_id", "ped_zone_0"},
                        {"geometry", {
                            {"type", "Polygon"},
                            {"coordinates", {{{-73.982105, 40.767932}, {-73.973694, 40.764551}, {-73.949318, 40.796918}, {-73.958416, 40.800686}, {-73.982105, 40.767932}}}}
                        }}
                    }
                })}
            }}
        }}
    };

    return response_json;  // Pretty print with 4 spaces
}

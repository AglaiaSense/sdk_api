#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>

// Define DBG_EN to enable debug mode
// #define DBG_EN

#define MAX_LOG_SIZE (100 * 1024 * 1024)    // 100MB

std::mutex log_mutex;

void check_log_file_size(const std::string& log_file) {
    std::ifstream file(log_file, std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        std::streamsize size = file.tellg();
        file.close();
        //std::cout << "log size = " << size << std::endl;
        if (size > MAX_LOG_SIZE) { 
            std::ofstream ofs(log_file, std::ios::trunc);
            ofs.close();
        }
    }
}

void log_message(const std::string& message) {
    std::lock_guard<std::mutex> guard(log_mutex);

#ifdef DBG_EN
    // Print log message to console
    std::cout << message << std::endl;
#else
    // Write log message to file
    const std::string log_file = "server.log";
    check_log_file_size(log_file); // Check log file size before writing

    std::ofstream log_stream(log_file, std::ios::app);
    if (!log_stream.is_open()) {
        std::cerr << "Failed to open log file: " << log_file << std::endl;
        return;
    }

    std::time_t now = std::time(nullptr);
    char time_str[100];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    log_stream << "[" << time_str << "] " << message << std::endl;
    log_stream.close();
#endif
}
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>

// Define DBG_EN to enable debug mode
// #define DBG_EN

std::mutex log_mutex;

void log_message(const std::string& message) {
    std::lock_guard<std::mutex> guard(log_mutex);

#ifdef DBG_EN
    // Print log message to console
    std::cout << message << std::endl;
#else
    // Write log message to file
    const std::string log_file = "server.log";
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
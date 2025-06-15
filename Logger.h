#ifndef PROJEKT3_LOGGER_H
#define PROJEKT3_LOGGER_H

#include <fstream>
#include <iostream>
#include <ctime>

class Logger {
private:
    mutable std::ofstream file;
public:
    Logger(const std::string& filename) {
        file.open(filename, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Failed to open log file: " << filename << std::endl;
        }
    }
    ~Logger() { if (file.is_open()) file.close(); }

    enum LogLevel { INFO, WARN, ERROR, SUCCESS };

    void log(const std::string& message, LogLevel level) const {
        if (level == INFO && message.find("getPiece") != std::string::npos) return; // Skip getPiece logs
        std::string levelStr;
        std::string colorCode;
        switch (level) {
            case INFO:    levelStr = "INFO";    colorCode = "\033[36m"; break;
            case WARN:    levelStr = "WARN";    colorCode = "\033[33m"; break;
            case ERROR:   levelStr = "ERROR";   colorCode = "\033[31m"; break;
            case SUCCESS: levelStr = "SUCCESS"; colorCode = "\033[32m"; break;
        }

        auto now = std::time(nullptr);
        char timeStr[26];
        ctime_r(&now, timeStr);
        timeStr[24] = '\0';

        std::string logMessage = "[" + std::string(timeStr) + "] [" + levelStr + "] " + message;

        std::cout << colorCode << logMessage << "\033[0m" << std::endl;

        if (file.is_open()) {
            file << logMessage << std::endl;
            file.flush();
        }
    }
};

#endif //PROJEKT3_LOGGER_H

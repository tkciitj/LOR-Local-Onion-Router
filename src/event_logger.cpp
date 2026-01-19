#include "../include/event_logger.h"
#include <fstream>
#include <ctime>

void log_event(
    const std::string& node,
    const std::string& event,
    const std::string& details
) {
    std::ofstream out("visualization/events.json", std::ios::app);

    std::time_t t = std::time(nullptr);

    out << "{"
        << "\"node\":\"" << node << "\","
        << "\"event\":\"" << event << "\","
        << "\"details\":\"" << details << "\","
        << "\"timestamp\":" << t
        << "},\n";
}

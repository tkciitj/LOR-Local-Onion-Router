#pragma once
#include <string>

void log_event(
    const std::string& node,
    const std::string& event,
    const std::string& details = ""
);

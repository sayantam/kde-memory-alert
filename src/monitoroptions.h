#pragma once

#include <chrono>

struct MonitorOptions {
    double thresholdPercent = 90.0;
    std::chrono::milliseconds pollInterval = std::chrono::seconds(10);
};

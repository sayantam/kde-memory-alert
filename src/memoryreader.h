#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

struct MemorySample {
    std::optional<double> rawUsagePercent;
    QString error;
};

class MemoryReader {
public:
    static MemorySample read();
    static MemorySample parse(const QByteArray &contents);
};

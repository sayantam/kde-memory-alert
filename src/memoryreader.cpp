#include "memoryreader.h"

#include <QFile>

MemorySample MemoryReader::read()
{
    QFile file(QStringLiteral("/proc/meminfo"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {std::nullopt, QStringLiteral("Cannot open /proc/meminfo: %1").arg(file.errorString())};
    }
    const auto contents = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        return {std::nullopt, QStringLiteral("Cannot read /proc/meminfo: %1").arg(file.errorString())};
    }
    return parse(contents);
}

MemorySample MemoryReader::parse(const QByteArray &contents)
{
    std::optional<quint64> total;
    std::optional<quint64> available;
    for (const auto &line : contents.split('\n')) {
        const auto fields = line.simplified().split(' ');
        if (fields.isEmpty() || (fields[0] != "MemTotal:" && fields[0] != "MemAvailable:")) {
            continue;
        }
        auto &destination = fields[0] == "MemTotal:" ? total : available;
        if (destination || fields.size() != 3 || fields[2] != "kB") {
            return {std::nullopt, QStringLiteral("Invalid or duplicate memory field")};
        }
        for (const char character : fields[1]) {
            if (character < '0' || character > '9') {
                return {std::nullopt, QStringLiteral("Memory field is not an unsigned integer")};
            }
        }
        bool valid = false;
        const auto value = fields[1].toULongLong(&valid);
        if (!valid) {
            return {std::nullopt, QStringLiteral("Memory field is out of range")};
        }
        destination = value;
    }
    if (!total || !available || *total == 0 || *available > *total) {
        return {std::nullopt, QStringLiteral("Missing or inconsistent MemTotal/MemAvailable")};
    }
    // Subtract before conversion; both fields have the same units.
    return {100.0 * static_cast<double>(*total - *available) / static_cast<double>(*total), {}};
}

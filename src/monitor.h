#pragma once

#include "memoryreader.h"
#include "monitoroptions.h"

#include <QObject>
#include <QTimer>
#include <functional>

struct MonitorState {
    std::optional<double> usagePercent;
    double thresholdPercent;
    bool alerting = false;
    bool stale = false;
};
Q_DECLARE_METATYPE(MonitorState)

class Monitor : public QObject {
    Q_OBJECT
public:
    using SampleProvider = std::function<MemorySample()>;
    explicit Monitor(MonitorOptions options, SampleProvider provider = MemoryReader::read,
                     QObject *parent = nullptr);
    void start();
    void sampleNow();
    const MonitorState &state() const { return m_state; }

Q_SIGNALS:
    void stateChanged(const MonitorState &state);

private:
    SampleProvider m_provider;
    QTimer m_timer;
    MonitorState m_state;
    QString m_lastError;
};

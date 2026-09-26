#include "monitor.h"

#include <QDebug>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

Monitor::Monitor(MonitorOptions options, SampleProvider provider, QObject *parent)
    : QObject(parent), m_provider(std::move(provider)), m_state{std::nullopt, options.thresholdPercent}
{
    if (!m_provider || !std::isfinite(options.thresholdPercent) || options.thresholdPercent < 0
        || options.thresholdPercent > 100 || options.pollInterval.count() <= 0
        || options.pollInterval.count() > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Invalid monitor options or sample provider");
    }
    m_timer.setInterval(options.pollInterval);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &Monitor::sampleNow);
}

void Monitor::start()
{
    if (m_timer.isActive()) {
        return;
    }
    sampleNow();
    m_timer.start();
}

void Monitor::sampleNow()
{
    const auto sample = m_provider();
    if (!sample.rawUsagePercent || !std::isfinite(*sample.rawUsagePercent)
        || *sample.rawUsagePercent < 0 || *sample.rawUsagePercent > 100) {
        const auto error = sample.error.isEmpty() ? QStringLiteral("Invalid memory sample") : sample.error;
        if (error != m_lastError) {
            qWarning().noquote() << "Memory sample failed:" << error;
            m_lastError = error;
        }
        m_state.stale = true;
    } else {
        m_state.usagePercent = std::round(*sample.rawUsagePercent * 10.0) / 10.0;
        m_state.alerting = *m_state.usagePercent > m_state.thresholdPercent;
        m_state.stale = false;
        m_lastError.clear();
    }
    Q_EMIT stateChanged(m_state);
}

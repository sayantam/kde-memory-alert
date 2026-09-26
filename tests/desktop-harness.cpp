#include "desktopui.h"
#include "monitor.h"

#include <KDBusService>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("memory-alert-harness"));
    app.setOrganizationDomain(QStringLiteral("sayantam.github.io"));
    app.setQuitOnLastWindowClosed(false);
    KLocalizedString::setApplicationDomain("kde-memory-alert");
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({QStringLiteral("sample-file"), QStringLiteral("Read a raw percentage or 'error' from a file every second; hide controls."), QStringLiteral("path")});
    parser.process(app);
    KDBusService service(KDBusService::Unique);

    MemorySample sample{50.0, {}};
    const auto path = parser.value(QStringLiteral("sample-file"));
    MonitorOptions options;
    options.pollInterval = std::chrono::seconds(1);
    Monitor monitor(options, [&sample, path] {
        if (path.isEmpty()) {
            return sample;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return MemorySample{std::nullopt, QStringLiteral("Harness file unavailable")};
        }
        bool valid = false;
        const auto value = file.readAll().trimmed().toDouble(&valid);
        return valid ? MemorySample{value, {}} : MemorySample{std::nullopt, QStringLiteral("Injected read failure")};
    });
    DesktopUi desktop;
    QObject::connect(&monitor, &Monitor::stateChanged, &desktop, &DesktopUi::update);
    QObject::connect(&desktop, &DesktopUi::quitRequested, &app, &QApplication::quit);

    QWidget controls;
    controls.setWindowTitle(QStringLiteral("Memory Alert — test controls"));
    auto *layout = new QVBoxLayout(&controls);
    auto *status = new QLabel(QStringLiteral("Synthetic memory only. Choose a sample, then switch to another app."));
    layout->addWidget(status);
    const auto addSample = [&](const QString &label, MemorySample next) {
        auto *button = new QPushButton(label);
        layout->addWidget(button);
        QObject::connect(button, &QPushButton::clicked, &controls, [&, next] {
            status->setText(QStringLiteral("Sample will change in 5 seconds; switch to your typing window."));
            QTimer::singleShot(5000, &controls, [&, next] {
                sample = next;
                status->setText(QStringLiteral("Sample changed. Monitoring continues every second."));
            });
        });
    };
    addSample(QStringLiteral("Normal (50%)"), {50.0, {}});
    addSample(QStringLiteral("At threshold after rounding (90.04%)"), {90.04, {}});
    addSample(QStringLiteral("Above threshold after rounding (90.06%)"), {90.06, {}});
    addSample(QStringLiteral("High (95%)"), {95.0, {}});
    addSample(QStringLiteral("Read failure"), {std::nullopt, QStringLiteral("Injected read failure")});
    auto *quit = new QPushButton(QStringLiteral("Quit test harness"));
    layout->addWidget(quit);
    QObject::connect(quit, &QPushButton::clicked, &desktop, &DesktopUi::requestQuit);
    if (path.isEmpty()) {
        controls.show();
    }
    monitor.start();
    return app.exec();
}

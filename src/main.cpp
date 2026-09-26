#include "desktopui.h"
#include "monitor.h"

#include <KDBusService>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kde-memory-alert"));
    app.setOrganizationDomain(QStringLiteral("sayantam.github.io"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));
    app.setDesktopFileName(QStringLiteral("io.github.sayantam.kde-memory-alert"));
    app.setApplicationDisplayName(QStringLiteral("KDE Memory Alert"));
    app.setQuitOnLastWindowClosed(false);
    KLocalizedString::setApplicationDomain("kde-memory-alert");

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("Warn when system RAM usage exceeds the threshold."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    KDBusService service(KDBusService::Unique);
    // Intentionally do not handle activateRequested: relaunches stay in the background.
    DesktopUi desktop;
    Monitor monitor(MonitorOptions{});
    QObject::connect(&monitor, &Monitor::stateChanged, &desktop, &DesktopUi::update);
    QObject::connect(&desktop, &DesktopUi::quitRequested, &app, &QApplication::quit);
    monitor.start();
    return app.exec();
}

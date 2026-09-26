#include "desktopui.h"
#include "memoryreader.h"
#include "monitor.h"
#include "trayplacement.h"

#include <KLocalizedString>
#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QMenu>
#include <QSignalSpy>
#include <QTest>
#include <limits>

class MemoryTests : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void trayPlacement_data()
    {
        QTest::addColumn<QString>("edge");
        QTest::addColumn<QPoint>("expected");
        // A tray centered at 800px along a full-length 1000px panel.
        QTest::newRow("left") << QStringLiteral("left") << QPoint(60, 750);
        QTest::newRow("right") << QStringLiteral("right") << QPoint(540, 750);
        QTest::newRow("top") << QStringLiteral("top") << QPoint(588, 60);
        QTest::newRow("bottom") << QStringLiteral("bottom") << QPoint(588, 840);
    }

    void trayPlacement()
    {
        QFETCH(QString, edge);
        QFETCH(QPoint, expected);
        const bool horizontal = edge == "top" || edge == "bottom";
        const QJsonObject panel{
            {"screenGeometry", QJsonObject{{"x", 300}, {"y", 1200}, {"width", 1000}, {"height", 1000}}},
            {"geometry", QJsonObject{{"x", horizontal ? 750 : 0}, {"y", horizontal ? 0 : 750},
                                     {"width", horizontal ? 100 : 48}, {"height", horizontal ? 48 : 100}}},
            {"location", edge}, {"length", 1000}, {"height", 48}, {"alignment", "left"}, {"offset", 0}};
        const auto placement = ::trayPlacement(QJsonDocument(QJsonArray{panel}).toJson(), QSize(400, 100), QPoint(500, 1500));
        QVERIFY(placement);
        QCOMPARE(placement->screen, QRect(300, 1200, 1000, 1000));
        QCOMPARE(placement->position, expected);
    }

    void laptopTrayPlacement()
    {
        // Actual Plasma layout: a centered left panel on a non-origin screen.
        const QByteArray layout = R"([{"screenGeometry":{"x":341,"y":1440,"width":1920,"height":1080},
            "location":"left","height":48,"alignment":"center","offset":0,"length":1052,
            "geometry":{"x":8,"y":688,"width":32,"height":296}}])";
        const auto placement = ::trayPlacement(layout, QSize(504, 88), QPoint(0, 0));
        QVERIFY(placement); // Use the tray's screen even when another screen is active.
        QCOMPARE(placement->position, QPoint(60, 806));
        QVERIFY(!::trayPlacement("[]", QSize(504, 88), QPoint()));
        QVERIFY(!::trayPlacement("invalid", QSize(504, 88), QPoint()));
        QVERIFY(!::trayPlacement("[{}]", QSize(504, 88), QPoint()));
    }

    void initTestCase()
    {
        QLocale::setDefault(QLocale::c());
        KLocalizedString::setApplicationDomain("kde-memory-alert");
        QApplication::setQuitOnLastWindowClosed(false);
    }

    void parse_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<double>("expected");
        QTest::newRow("ordinary") << QByteArray("MemTotal: 1000 kB\nMemAvailable: 200 kB\n") << 80.0;
        QTest::newRow("cache-is-available") << QByteArray("MemTotal: 1000 kB\nMemFree: 0 kB\nCached: 500 kB\nMemAvailable: 500 kB\n") << 50.0;
        QTest::newRow("all-available") << QByteArray("MemAvailable:\t1000 kB\nMemTotal: 1000 kB") << 0.0;
        QTest::newRow("none-available") << QByteArray("MemTotal: 1000 kB\nMemAvailable: 0 kB") << 100.0;
        QTest::newRow("fractional") << QByteArray("MemTotal: 10000 kB\nMemAvailable: 996 kB") << 90.04;
        QTest::newRow("large") << QByteArray("MemTotal: 10000000000 kB\nMemAvailable: 1000000000 kB") << 90.0;
    }

    void parse()
    {
        QFETCH(QByteArray, input);
        QFETCH(double, expected);
        const auto sample = MemoryReader::parse(input);
        QVERIFY(sample.rawUsagePercent.has_value());
        QCOMPARE(*sample.rawUsagePercent, expected);
        QVERIFY(sample.error.isEmpty());
    }

    void invalidInput_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("missing-available") << QByteArray("MemTotal: 100 kB");
        QTest::newRow("missing-total") << QByteArray("MemAvailable: 10 kB");
        QTest::newRow("zero-total") << QByteArray("MemTotal: 0 kB\nMemAvailable: 0 kB");
        QTest::newRow("too-much-available") << QByteArray("MemTotal: 100 kB\nMemAvailable: 101 kB");
        QTest::newRow("negative") << QByteArray("MemTotal: 100 kB\nMemAvailable: -1 kB");
        QTest::newRow("junk") << QByteArray("MemTotal: 100 kB\nMemAvailable: ten kB");
        QTest::newRow("wrong-units") << QByteArray("MemTotal: 100 kB\nMemAvailable: 10 MB");
        QTest::newRow("missing-value") << QByteArray("MemTotal: kB\nMemAvailable: 10 kB");
        QTest::newRow("overflow") << QByteArray("MemTotal: 999999999999999999999999 kB\nMemAvailable: 10 kB");
        QTest::newRow("duplicate") << QByteArray("MemTotal: 100 kB\nMemTotal: 100 kB\nMemAvailable: 10 kB");
    }

    void invalidInput()
    {
        QFETCH(QByteArray, input);
        const auto sample = MemoryReader::parse(input);
        QVERIFY(!sample.rawUsagePercent.has_value());
        QVERIFY(!sample.error.isEmpty());
    }

    void rounding_data()
    {
        QTest::addColumn<double>("raw");
        QTest::addColumn<double>("rounded");
        QTest::addColumn<bool>("alerting");
        QTest::newRow("below") << 89.94 << 89.9 << false;
        QTest::newRow("equality") << 90.0 << 90.0 << false;
        QTest::newRow("above-rounds-equal") << 90.04 << 90.0 << false;
        QTest::newRow("halfway") << 90.05 << 90.1 << true;
        QTest::newRow("above") << 90.06 << 90.1 << true;
        QTest::newRow("full") << 100.0 << 100.0 << true;
    }

    void rounding()
    {
        QFETCH(double, raw);
        QFETCH(double, rounded);
        QFETCH(bool, alerting);
        Monitor monitor(MonitorOptions{}, [raw] { return MemorySample{raw, {}}; });
        monitor.sampleNow();
        QCOMPARE(*monitor.state().usagePercent, rounded);
        QCOMPARE(monitor.state().alerting, alerting);
    }

    void immediateAndTimedSampling()
    {
        int calls = 0;
        MonitorOptions options;
        options.pollInterval = std::chrono::milliseconds(20);
        Monitor monitor(options, [&calls] { ++calls; return MemorySample{50.0, {}}; });
        QSignalSpy changes(&monitor, &Monitor::stateChanged);
        monitor.start();
        QCOMPARE(calls, 1);
        QCOMPARE(changes.count(), 1);
        monitor.start();
        QCOMPARE(calls, 1); // Starting twice must not sample twice or create another timer.
        QTRY_VERIFY_WITH_TIMEOUT(calls >= 2, 500);
    }

    void transitionsAndPresentation()
    {
        MemorySample sample{89.0, {}};
        Monitor monitor(MonitorOptions{}, [&sample] { return sample; });
        DesktopUi desktop;
        connect(&monitor, &Monitor::stateChanged, &desktop, &DesktopUi::update);
        const QList<double> sequence{89.0, 90.0, 91.0, 92.0, 90.0, 90.04, 90.06, 90.04};
        const QList<bool> expected{false, false, true, true, false, false, true, false};
        for (int i = 0; i < sequence.size(); ++i) {
            sample = {sequence[i], {}};
            monitor.sampleNow();
            QCOMPARE(desktop.window().isVisible(), expected[i]);
            QCOMPARE(desktop.tray().status(), expected[i] ? KStatusNotifierItem::Active : KStatusNotifierItem::Passive);
            QCOMPARE(desktop.window().message(), desktop.tray().toolTipSubTitle());
        }
        sample = {90.06, {}};
        monitor.sampleNow();
        QCOMPARE(desktop.window().message(), QStringLiteral("Warning! Memory usage at 90.1% — exceeds 90% threshold"));
        QVERIFY(desktop.window().testAttribute(Qt::WA_ShowWithoutActivating));
        QVERIFY(desktop.window().windowFlags().testFlag(Qt::WindowDoesNotAcceptFocus));
        QVERIFY(desktop.window().windowFlags().testFlag(Qt::WindowStaysOnTopHint));
        QVERIFY(!desktop.window().windowFlags().testFlag(Qt::WindowMinimizeButtonHint));
        QVERIFY(!desktop.window().windowFlags().testFlag(Qt::WindowCloseButtonHint));
        auto &window = const_cast<AlertWindow &>(desktop.window());
        QVERIFY(!window.close());
        QVERIFY(window.isVisible());
        window.showMinimized();
        QTRY_VERIFY(!window.isMinimized());
        auto &tray = const_cast<KStatusNotifierItem &>(desktop.tray());
        QVERIFY(tray.associatedWindow() == nullptr);
        tray.activate();
        QVERIFY(window.isVisible());
        QVERIFY(tray.isMenu());
        QCOMPARE(tray.contextMenu()->actions().size(), 1);
        QCOMPARE(tray.contextMenu()->actions()[0]->text(), QStringLiteral("Quit"));
        QSignalSpy quit(&desktop, &DesktopUi::quitRequested);
        tray.contextMenu()->actions()[0]->trigger();
        QCOMPARE(quit.count(), 1);
    }

    void failuresAndRecovery()
    {
        MemorySample sample{std::nullopt, QStringLiteral("Test read failure")};
        Monitor monitor(MonitorOptions{}, [&sample] { return sample; });
        DesktopUi desktop;
        connect(&monitor, &Monitor::stateChanged, &desktop, &DesktopUi::update);
        monitor.sampleNow();
        QVERIFY(!monitor.state().usagePercent);
        QVERIFY(!desktop.window().isVisible());
        QVERIFY(desktop.tray().toolTipSubTitle().contains(QStringLiteral("unavailable")));
        sample = {95.0, {}};
        monitor.sampleNow();
        QVERIFY(desktop.window().isVisible());
        sample = {std::nullopt, QStringLiteral("Test read failure")};
        monitor.sampleNow();
        QVERIFY(monitor.state().stale);
        QCOMPARE(*monitor.state().usagePercent, 95.0);
        QVERIFY(desktop.window().isVisible());
        QCOMPARE(desktop.tray().status(), KStatusNotifierItem::Active);
        QVERIFY(desktop.window().message().contains(QStringLiteral("Reading stale")));
        sample = {80.0, {}};
        monitor.sampleNow();
        QVERIFY(!monitor.state().stale);
        QVERIFY(!desktop.window().isVisible());
        QVERIFY(!desktop.tray().toolTipSubTitle().contains(QStringLiteral("stale")));
        sample = {std::numeric_limits<double>::quiet_NaN(), {}};
        monitor.sampleNow();
        QVERIFY(monitor.state().stale);
        QVERIFY(!desktop.window().isVisible());
        QCOMPARE(*monitor.state().usagePercent, 80.0);
        QCOMPARE(desktop.tray().status(), KStatusNotifierItem::Passive);
    }

    void alternativeThreshold()
    {
        MonitorOptions options;
        options.thresholdPercent = 75.5;
        MemorySample sample{75.54, {}};
        Monitor monitor(options, [&sample] { return sample; });
        DesktopUi desktop;
        connect(&monitor, &Monitor::stateChanged, &desktop, &DesktopUi::update);
        monitor.sampleNow();
        QVERIFY(!monitor.state().alerting);
        sample = {75.56, {}};
        monitor.sampleNow();
        QVERIFY(monitor.state().alerting);
        QCOMPARE(desktop.window().message(), QStringLiteral("Warning! Memory usage at 75.6% — exceeds 75.5% threshold"));
    }
};

QTEST_MAIN(MemoryTests)
#include "memory-tests.moc"

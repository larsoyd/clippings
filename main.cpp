// theme and config arrive as the json file in DESKTOP_SHELL_SETTINGS

#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>

#include <unistd.h>

#include <cstdio>

#include "application.hpp"
#include "capture-overlays.hpp"
#include "capture-session.hpp"
#include "document-canvas.hpp"
#include "instance-door.hpp"
#include "live-settings.hpp"
#include "screen-grabber.hpp"
#include "wayfire-client.hpp"

// an installed copy keeps its qml beside the binary, a build from the tree uses the tree's
static QString qml_directory()
{
    const QString installed = QCoreApplication::applicationDirPath() + QStringLiteral("/../share/clippings/qml");
    return QFile::exists(installed + QStringLiteral("/Main.qml")) ? QDir::cleanPath(installed)
                                                                  : QStringLiteral(SOURCE_DIRECTORY "/qml");
}

static QJsonObject command_from_arguments(const QStringList& arguments)
{
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({QStringLiteral("capture"), QStringLiteral("Start a capture: selection, window, screen or all."), QStringLiteral("mode")});
    parser.addPositionalArgument(QStringLiteral("files"), QStringLiteral("Images to open."), QStringLiteral("[files...]"));
    parser.process(arguments);

    QJsonObject command;
    if (parser.isSet(QStringLiteral("capture")))
    {
        command.insert(QStringLiteral("capture"), parser.value(QStringLiteral("capture")));
    }
    else if (!parser.positionalArguments().isEmpty())
    {
        QJsonArray files;
        for (const QString& name : parser.positionalArguments())
        {
            files.append(QUrl::fromUserInput(name, QDir::currentPath(), QUrl::AssumeLocalFile).toString());
        }
        command.insert(QStringLiteral("open"), files);
    }
    return command;
}

int main(int argc, char **argv)
{
    // the session's file dialogs come through the portal, qt only asks it when told to
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORMTHEME"))
    {
        qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
    }

    QGuiApplication application(argc, argv);
    QCoreApplication::setApplicationVersion(QStringLiteral(CLIPPINGS_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("clippings"));
    QGuiApplication::setQuitOnLastWindowClosed(false);

    const QJsonObject command = command_from_arguments(QCoreApplication::arguments());
    const QString door_path = InstanceDoor::default_path();
    StartLock start_lock(door_path + QStringLiteral(".lock"));
    if (InstanceDoor::hand_over(door_path, command))
    {
        return 0;
    }

    const QString settings_path = qEnvironmentVariable("DESKTOP_SHELL_SETTINGS");
    LiveSettings settings(settings_path);
    if (settings_path.isEmpty() || !settings.load())
    {
        std::fprintf(stderr, "[clippings] no settings path=%s, start it from a Mavernix session\n", qPrintable(settings_path));
        return 1;
    }

    const QString socket = qEnvironmentVariable("WAYFIRE_SOCKET");
    if (socket.isEmpty())
    {
        std::fprintf(stderr, "[clippings] refused: WAYFIRE_SOCKET is not set\n");
        return 1;
    }

    QTemporaryDir scratch;
    WayfireClient compositor(socket);
    ScreenGrabber grabber;
    CaptureSession session(compositor, grabber, QCoreApplication::applicationPid());
    session.set_scratch_folder(scratch.path());
    Application app(session, compositor);

    qmlRegisterType<DocumentCanvas>("Clippings", 1, 0, "DocumentCanvas");
    qmlRegisterUncreatableType<Document>("Clippings", 1, 0, "Document", QStringLiteral("made by the program"));
    qmlRegisterUncreatableType<DocumentList>("Clippings", 1, 0, "DocumentList", QStringLiteral("made by the program"));
    qmlRegisterUncreatableType<CaptureSession>("Clippings", 1, 0, "CaptureSession", QStringLiteral("made by the program"));

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("frozen"), new FrozenImages(session));
    engine.rootContext()->setContextProperty(QStringLiteral("settings"), settings.value());
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &app);
    engine.rootContext()->setContextProperty(QStringLiteral("session"), &session);
    QObject::connect(&settings, &LiveSettings::changed, &engine, [&] {
        engine.rootContext()->setContextProperty(QStringLiteral("settings"), settings.value());
    });

    CaptureOverlays overlays(engine, session, qml_directory());
    session.set_overlays(&overlays);
    QObject::connect(&application, &QGuiApplication::screenRemoved, &session, [&] { session.displays_changed(); });
    QObject::connect(&application, &QGuiApplication::screenAdded, &session, [&] { session.displays_changed(); });

    engine.load(qml_directory() + QStringLiteral("/Main.qml"));
    if (engine.rootObjects().isEmpty())
    {
        std::fprintf(stderr, "[clippings] could not load Main.qml dir=%s\n", qPrintable(qml_directory()));
        return 1;
    }

    InstanceDoor door;
    QObject::connect(&door, &InstanceDoor::commandReceived, &app, &Application::run_command);
    if (!door.listen(door_path))
    {
        std::fprintf(stderr, "[instance] could not listen path=%s\n", qPrintable(door_path));
    }
    start_lock.release();

    app.run_command(command);
    return application.exec();
}

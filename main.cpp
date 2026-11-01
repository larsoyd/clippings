#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <cstdio>
#include <cstdlib>

static QVariantMap read_settings()
{
    const QString path = qEnvironmentVariable("DESKTOP_SHELL_SETTINGS");
    QFile file(path);
    if (path.isEmpty() || !file.open(QIODevice::ReadOnly))
    {
        std::fprintf(stderr, "[clippings] no settings path=%s, start it from a Mavernix session\n", qPrintable(path));
        std::exit(1);
    }
    return QJsonDocument::fromJson(file.readAll()).object().toVariantMap();
}

// an installed copy keeps its qml beside the binary, a build from the tree uses the tree's
static QString qml_directory()
{
    const QString installed = QCoreApplication::applicationDirPath() + QStringLiteral("/../share/clippings/qml");
    return QFile::exists(installed + QStringLiteral("/Main.qml")) ? QDir::cleanPath(installed)
                                                                  : QStringLiteral(SOURCE_DIRECTORY "/qml");
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    // the dock and panel match windows to apps by this, it has to be the desktop entry's name
    QGuiApplication::setDesktopFileName(QStringLiteral("clippings"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("settings"), read_settings());
    engine.load(qml_directory() + QStringLiteral("/Main.qml"));
    if (engine.rootObjects().isEmpty())
    {
        std::fprintf(stderr, "[clippings] could not load Main.qml dir=%s\n", qPrintable(qml_directory()));
        return 1;
    }
    return application.exec();
}

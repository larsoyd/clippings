#include "live-settings.hpp"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include <cstdio>

static const int REREAD_DELAY_MS = 150;

LiveSettings::LiveSettings(const QString& file_path, QObject *parent) : QObject(parent), path(file_path)
{
    soon.setSingleShot(true);
    soon.setInterval(REREAD_DELAY_MS);
    // the folder too, an atomic replace ends the watch on the file itself
    connect(&watcher, &QFileSystemWatcher::fileChanged, &soon, qOverload<>(&QTimer::start));
    connect(&watcher, &QFileSystemWatcher::directoryChanged, &soon, qOverload<>(&QTimer::start));
    connect(&soon, &QTimer::timeout, this, [this] {
        watch();
        if (load())
        {
            emit changed();
        }
    });
}

bool LiveSettings::load()
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        std::fprintf(stderr, "[settings] unreadable path=%s\n", qPrintable(path));
        return false;
    }

    QJsonParseError problem{};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &problem);
    if (problem.error != QJsonParseError::NoError)
    {
        std::fprintf(stderr, "[settings] unparsable path=%s error=%s\n", qPrintable(path), qPrintable(problem.errorString()));
        return false;
    }

    const QVariantMap fresh = document.object().toVariantMap();
    const bool first = current.isEmpty();
    if (fresh == current)
    {
        return false;
    }

    current = fresh;
    if (first)
    {
        watch();
    }
    return true;
}

const QVariantMap& LiveSettings::value() const
{
    return current;
}

void LiveSettings::watch()
{
    watcher.addPath(QFileInfo(path).absolutePath());
    watcher.addPath(path);
}

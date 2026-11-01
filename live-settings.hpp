#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>

// the desktop's settings file, read again whenever the session rewrites it
class LiveSettings : public QObject
{
    Q_OBJECT

  public:
    explicit LiveSettings(const QString& path, QObject *parent = nullptr);

    // false when the file can't be read or parsed
    bool load();
    const QVariantMap& value() const;

  signals:
    void changed();

  private:
    void watch();

    QString path;
    QVariantMap current;
    QFileSystemWatcher watcher;
    QTimer soon;
};

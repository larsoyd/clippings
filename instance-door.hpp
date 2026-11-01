#pragma once

#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QString>

// held from the start of the program until it listens, so of several starts one becomes the program
class StartLock
{
  public:
    explicit StartLock(const QString& path, int wait_ms = 10000);
    ~StartLock();
    StartLock(const StartLock&) = delete;
    StartLock& operator=(const StartLock&) = delete;

    bool held() const;
    void release();

  private:
    int descriptor = -1;
    bool is_held = false;
};

// how a later start hands its command to the running program
class InstanceDoor : public QObject
{
    Q_OBJECT

  public:
    using QObject::QObject;

    static QString default_path();
    // true only once the running program answered
    static bool hand_over(const QString& path, const QJsonObject& command);
    // false when another program holds the path
    bool listen(const QString& path);

  signals:
    void commandReceived(const QJsonObject& command);

  private:
    void accept_connections();
    void read_command(QLocalSocket *socket);

    QLocalServer server;
};

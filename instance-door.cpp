#include "instance-door.hpp"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QThread>

#include <cstdio>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

static const int CONNECT_TIMEOUT_MS = 500;
static const int ANSWER_TIMEOUT_MS = 3000;
static const int POLL_MS = 10;

StartLock::StartLock(const QString& path, int wait_ms)
{
    descriptor = ::open(QFile::encodeName(path).constData(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (descriptor < 0)
    {
        std::fprintf(stderr, "[instance] no lock file path=%s\n", qPrintable(path));
        return;
    }

    const qint64 ends = QDateTime::currentMSecsSinceEpoch() + wait_ms;
    while (::flock(descriptor, LOCK_EX | LOCK_NB) != 0)
    {
        if (QDateTime::currentMSecsSinceEpoch() >= ends)
        {
            std::fprintf(stderr, "[instance] start lock timed out wait_ms=%d\n", wait_ms);
            return;
        }
        QThread::msleep(POLL_MS);
    }
    is_held = true;
}

StartLock::~StartLock()
{
    release();
}

bool StartLock::held() const
{
    return is_held;
}

void StartLock::release()
{
    if (descriptor >= 0)
    {
        ::close(descriptor);
        descriptor = -1;
    }
    is_held = false;
}

QString InstanceDoor::default_path()
{
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    return (folder.isEmpty() ? QStringLiteral("/tmp") : folder) + QStringLiteral("/clippings.socket");
}

bool InstanceDoor::hand_over(const QString& path, const QJsonObject& command)
{
    QLocalSocket socket;
    socket.connectToServer(path);
    if (!socket.waitForConnected(CONNECT_TIMEOUT_MS))
    {
        return false;
    }

    socket.write(QJsonDocument(command).toJson(QJsonDocument::Compact) + '\n');
    socket.flush();
    if (socket.bytesToWrite() > 0 && !socket.waitForBytesWritten(ANSWER_TIMEOUT_MS))
    {
        return false;
    }
    return socket.waitForReadyRead(ANSWER_TIMEOUT_MS) && socket.readAll().startsWith("ok");
}

bool InstanceDoor::listen(const QString& path)
{
    connect(&server, &QLocalServer::newConnection, this, &InstanceDoor::accept_connections, Qt::UniqueConnection);
    if (server.listen(path))
    {
        return true;
    }

    // a socket left by a crash answers nobody
    QLocalSocket probe;
    probe.connectToServer(path);
    if (probe.waitForConnected(CONNECT_TIMEOUT_MS))
    {
        return false;
    }
    QLocalServer::removeServer(path);
    return server.listen(path);
}

void InstanceDoor::accept_connections()
{
    while (QLocalSocket *socket = server.nextPendingConnection())
    {
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] { read_command(socket); });
        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void InstanceDoor::read_command(QLocalSocket *socket)
{
    if (!socket->canReadLine())
    {
        return;
    }

    const QJsonObject command = QJsonDocument::fromJson(socket->readLine()).object();
    socket->write("ok\n");
    socket->flush();
    emit commandReceived(command);
}

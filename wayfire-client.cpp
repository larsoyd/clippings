#include "wayfire-client.hpp"

#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QtEndian>

#include <algorithm>
#include <cstdio>

static const int SOCKET_TIMEOUT_MS = 2000;
static const int LARGEST_PICTURE_SIDE = 16384;
static const QByteArray PICTURE_WORD = "WINDOWPICTURE";

static QRect rect_from(const QJsonObject& geometry)
{
    return QRect(geometry.value(QLatin1String("x")).toInt(), geometry.value(QLatin1String("y")).toInt(),
                 geometry.value(QLatin1String("width")).toInt(), geometry.value(QLatin1String("height")).toInt());
}

QList<DisplayInfo> displays_from(const QJsonArray& reply)
{
    QList<DisplayInfo> displays;
    for (const QJsonValue& value : reply)
    {
        const QJsonObject output = value.toObject();
        displays.append({output.value(QLatin1String("name")).toString(),
                         rect_from(output.value(QLatin1String("geometry")).toObject())});
    }
    return displays;
}

QList<WindowInfo> windows_from(const QJsonArray& reply, const QList<DisplayInfo>& displays)
{
    QList<WindowInfo> windows;
    for (const QJsonValue& value : reply)
    {
        const QJsonObject view = value.toObject();
        const bool shown = view.value(QLatin1String("mapped")).toBool() && !view.value(QLatin1String("minimized")).toBool();
        // a toplevel that never had focus is a hidden helper surface
        const qint64 focus_time = qint64(view.value(QLatin1String("last-focus-timestamp")).toDouble());
        if (view.value(QLatin1String("role")).toString() != QLatin1String("toplevel") || !shown || focus_time <= 0)
        {
            continue;
        }

        const QString output = view.value(QLatin1String("output-name")).toString();
        const auto display = std::find_if(displays.begin(), displays.end(),
                                          [&](const DisplayInfo& candidate) { return candidate.name == output; });
        if (display == displays.end())
        {
            continue;
        }

        const QRect local = rect_from(view.value(QLatin1String("geometry")).toObject());
        // geometry beyond the display belongs to a workspace not on screen
        if (!QRect(QPoint(0, 0), display->logical.size()).intersects(local))
        {
            continue;
        }

        WindowInfo window;
        window.id = view.value(QLatin1String("id")).toInteger();
        window.pid = view.value(QLatin1String("pid")).toInteger();
        window.title = view.value(QLatin1String("title")).toString();
        window.app_id = view.value(QLatin1String("app-id")).toString();
        window.output = output;
        window.geometry = local.translated(display->logical.topLeft());
        window.focus_time = focus_time;
        window.on_top = view.value(QLatin1String("always-on-top")).toBool();
        windows.append(window);
    }

    std::stable_sort(windows.begin(), windows.end(), [](const WindowInfo& one, const WindowInfo& other) {
        return one.on_top != other.on_top ? one.on_top : one.focus_time > other.focus_time;
    });
    return windows;
}

const WindowInfo *window_at(const QList<WindowInfo>& windows, const QPoint& point)
{
    for (const WindowInfo& window : windows)
    {
        if (window.geometry.contains(point))
        {
            return &window;
        }
    }
    return nullptr;
}

WayfireClient::WayfireClient(const QString& socket_path) : path(socket_path)
{
}

QByteArray WayfireClient::exchange(const QString& method, const QJsonObject& data) const
{
    QLocalSocket socket;
    socket.connectToServer(path);
    if (!socket.waitForConnected(SOCKET_TIMEOUT_MS))
    {
        std::fprintf(stderr, "[wayfire] no connection method=%s socket=%s\n", qPrintable(method), qPrintable(path));
        return {};
    }

    const QByteArray body = QJsonDocument(QJsonObject{{QStringLiteral("method"), method}, {QStringLiteral("data"), data}})
                                .toJson(QJsonDocument::Compact);
    const quint32 length = qToLittleEndian<quint32>(quint32(body.size()));
    socket.write(reinterpret_cast<const char *>(&length), sizeof(length));
    socket.write(body);
    socket.flush();

    const auto read_exactly = [&socket](qint64 wanted) {
        QByteArray bytes;
        while (bytes.size() < wanted)
        {
            if (socket.bytesAvailable() == 0 && !socket.waitForReadyRead(SOCKET_TIMEOUT_MS))
            {
                return QByteArray();
            }
            bytes += socket.read(wanted - bytes.size());
        }
        return bytes;
    };

    const QByteArray header = read_exactly(sizeof(quint32));
    if (header.isEmpty())
    {
        std::fprintf(stderr, "[wayfire] no reply method=%s\n", qPrintable(method));
        return {};
    }
    return read_exactly(qFromLittleEndian<quint32>(header.constData()));
}

QJsonObject WayfireClient::call(const QString& method, const QJsonObject& data) const
{
    return QJsonDocument::fromJson(exchange(method, data)).object();
}

QJsonArray WayfireClient::call_list(const QString& method) const
{
    return QJsonDocument::fromJson(exchange(method, {})).array();
}

QList<DisplayInfo> WayfireClient::displays() const
{
    return displays_from(call_list(QStringLiteral("window-rules/list-outputs")));
}

QList<WindowInfo> WayfireClient::windows() const
{
    return windows_from(call_list(QStringLiteral("window-rules/list-views")), displays());
}

qint64 WayfireClient::focused() const
{
    const QJsonObject info = call(QStringLiteral("window-rules/get-focused-view")).value(QLatin1String("info")).toObject();
    return info.value(QLatin1String("role")).toString() == QLatin1String("toplevel")
        ? info.value(QLatin1String("id")).toInteger(-1)
        : -1;
}

QPoint WayfireClient::pointer() const
{
    const QJsonObject position = call(QStringLiteral("window-rules/get_cursor_position")).value(QLatin1String("pos")).toObject();
    return QPoint(position.value(QLatin1String("x")).toInt(), position.value(QLatin1String("y")).toInt());
}

bool WayfireClient::focus(qint64 id) const
{
    const QJsonObject reply = call(QStringLiteral("window-rules/focus-view"), {{QStringLiteral("id"), id}});
    return reply.value(QLatin1String("result")).toString() == QLatin1String("ok");
}

QImage WayfireClient::window_picture(qint64 id, const QString& file_path) const
{
    const QJsonObject reply = call(QStringLiteral("window-pictures/write"),
        {{QStringLiteral("id"), id}, {QStringLiteral("path"), file_path}, {QStringLiteral("largest_side"), LARGEST_PICTURE_SIDE}});
    if (reply.contains(QLatin1String("error")) || reply.isEmpty())
    {
        std::fprintf(stderr, "[wayfire] no window picture id=%lld reply=%s\n", qlonglong(id),
                     QJsonDocument(reply).toJson(QJsonDocument::Compact).constData());
        return {};
    }

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }

    const QList<QByteArray> header = file.readLine().trimmed().split(' ');
    if (header.size() != 3 || header[0] != PICTURE_WORD)
    {
        return {};
    }

    const int width = header[1].toInt();
    const int height = header[2].toInt();
    const QByteArray pixels = file.readAll();
    if (width <= 0 || height <= 0 || pixels.size() < qsizetype(width) * height * 4)
    {
        return {};
    }

    return QImage(reinterpret_cast<const uchar *>(pixels.constData()), width, height, width * 4, QImage::Format_RGBA8888)
        .convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

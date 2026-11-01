#pragma once

#include <QJsonArray>
#include <QList>
#include <QRect>
#include <QString>

struct DisplayInfo
{
    QString name;
    QRect logical;
};

struct WindowInfo
{
    qint64 id = -1;
    qint64 pid = -1;
    QString title;
    QString app_id;
    QString output;
    // in the layout of all displays, without the shadow
    QRect geometry;
    qint64 focus_time = 0;
    bool on_top = false;
};

QList<DisplayInfo> displays_from(const QJsonArray& reply);
// what can be seen right now, topmost first
QList<WindowInfo> windows_from(const QJsonArray& reply, const QList<DisplayInfo>& displays);
const WindowInfo *window_at(const QList<WindowInfo>& windows, const QPoint& point);

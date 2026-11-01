#pragma once

#include <QImage>
#include <QList>
#include <QString>

#include "wayfire-data.hpp"

// what capture needs from the compositor
class Compositor
{
  public:
    virtual ~Compositor() = default;
    virtual QList<DisplayInfo> displays() const = 0;
    virtual QList<WindowInfo> windows() const = 0;
    virtual qint64 focused() const = 0;
    virtual bool focus(qint64 id) const = 0;
    virtual QImage window_picture(qint64 id, const QString& file) const = 0;
};

// the windows and panels of a capture
class OverlayHost
{
  public:
    virtual ~OverlayHost() = default;
    virtual void show_selection() = 0;
    virtual void hide_selection() = 0;
    virtual void show_countdown(const QPoint& near) = 0;
    virtual void hide_countdown() = 0;
};

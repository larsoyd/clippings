#pragma once

#include <QJsonArray>
#include <QJsonObject>

#include "compositor.hpp"

// the compositor's socket, every call bounded so a dead compositor can't hang the program
class WayfireClient : public Compositor
{
  public:
    explicit WayfireClient(const QString& path);

    QJsonObject call(const QString& method, const QJsonObject& data = {}) const;
    QJsonArray call_list(const QString& method) const;

    QList<DisplayInfo> displays() const override;
    QList<WindowInfo> windows() const override;
    qint64 focused() const override;
    QPoint pointer() const override;
    bool focus(qint64 id) const override;
    QImage window_picture(qint64 id, const QString& file, bool with_shadow) const override;

  private:
    QByteArray exchange(const QString& method, const QJsonObject& data) const;

    QString path;
};

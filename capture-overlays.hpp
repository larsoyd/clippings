#pragma once

#include <QList>
#include <QPointer>
#include <QQuickImageProvider>
#include <QQuickView>

#include "compositor.hpp"

class CaptureSession;

// hands the frozen pictures to qml as image://frozen/GENERATION/OUTPUT
class FrozenImages : public QQuickImageProvider
{
  public:
    explicit FrozenImages(const CaptureSession& session);
    QImage requestImage(const QString& id, QSize *size, const QSize& wanted) override;

  private:
    const CaptureSession& session;
};

// one full screen layer surface per display, above panels and windows
class CaptureOverlays : public OverlayHost
{
  public:
    CaptureOverlays(QQmlEngine& engine, const CaptureSession& session, const QString& qml_directory, const QString& art_folder);
    ~CaptureOverlays() override;

    void show_selection() override;
    void hide_selection() override;
    void show_countdown(const QPoint& near) override;
    void hide_countdown() override;

  private:
    QQuickView *make_view(const QString& file, const QVariantMap& properties);
    void follow_pointer();
    void apply_cursor(QQuickView *view);

    QQmlEngine& engine;
    const CaptureSession& session;
    QString qml_directory;
    QString art_folder;
    QList<QQuickView *> views;
    QString keyboard_output;
    QQuickView *countdown = nullptr;
};

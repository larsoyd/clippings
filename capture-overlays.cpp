#include "capture-overlays.hpp"

#include "capture-session.hpp"

#include <LayerShellQt/Window>
#include <QCursor>
#include <QGuiApplication>
#include <QPixmap>
#include <QQmlEngine>
#include <QQuickItem>
#include <QScreen>

#include <cstdio>

static const QSize COUNTDOWN_SIZE(391, 144);

FrozenImages::FrozenImages(const CaptureSession& capture)
    : QQuickImageProvider(QQuickImageProvider::Image), session(capture)
{
}

QImage FrozenImages::requestImage(const QString& id, QSize *size, const QSize&)
{
    const FrozenOutput *output = session.desktop().output(id.section(QLatin1Char('/'), 1));
    if (!output)
    {
        return {};
    }

    if (size)
    {
        *size = output->image.size();
    }
    return output->image;
}

CaptureOverlays::CaptureOverlays(QQmlEngine& qml_engine, const CaptureSession& capture, const QString& directory,
                                 const QString& art)
    : engine(qml_engine), session(capture), qml_directory(directory), art_folder(art)
{
}

CaptureOverlays::~CaptureOverlays()
{
    hide_selection();
    hide_countdown();
}

QQuickView *CaptureOverlays::make_view(const QString& file, const QVariantMap& properties)
{
    auto *view = new QQuickView(&engine, nullptr);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->setInitialProperties(properties);
    view->setSource(QUrl::fromLocalFile(qml_directory + QLatin1Char('/') + file));
    for (const QQmlError& error : view->errors())
    {
        std::fprintf(stderr, "[overlay] qml error=%s\n", qPrintable(error.toString()));
    }
    return view;
}

void CaptureOverlays::show_selection()
{
    for (QScreen *screen : QGuiApplication::screens())
    {
        const FrozenOutput *output = session.desktop().output(screen->name());
        if (!output)
        {
            std::fprintf(stderr, "[overlay] no picture for screen=%s\n", qPrintable(screen->name()));
            continue;
        }

        QQuickView *view = make_view(QStringLiteral("Overlay.qml"),
            {{QStringLiteral("outputName"), output->name},
             {QStringLiteral("outputX"), output->logical.x()},
             {QStringLiteral("outputY"), output->logical.y()},
             {QStringLiteral("picture"), QStringLiteral("image://frozen/%1/%2").arg(session.generation()).arg(output->name)}});
        view->setScreen(screen);

        auto *layer = LayerShellQt::Window::get(view);
        layer->setScope(QStringLiteral("clippings-selection"));
        layer->setLayer(LayerShellQt::Window::LayerOverlay);
        layer->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorBottom
                                                        | LayerShellQt::Window::AnchorLeft | LayerShellQt::Window::AnchorRight));
        layer->setExclusiveZone(-1);
        layer->setKeyboardInteractivity(screen->name() == session.pointer_output()
                                            ? LayerShellQt::Window::KeyboardInteractivityExclusive
                                            : LayerShellQt::Window::KeyboardInteractivityNone);
        layer->setWantsToBeOnActiveScreen(false);
        layer->setScreen(screen);
        view->show();
        views.append(view);
        apply_cursor(view);
    }

    QObject::connect(&session, &CaptureSession::modeChanged, views.first(), [this] {
        for (QQuickView *view : std::as_const(views))
        {
            apply_cursor(view);
        }
    });
    keyboard_output = session.pointer_output();
    QObject::connect(&session, &CaptureSession::selectionChanged, views.first(), [this] { follow_pointer(); });
}

// only the surface under the pointer may hold the keyboard, its pop-ups need the keys
void CaptureOverlays::follow_pointer()
{
    const QString output = session.pointer_output();
    if (output.isEmpty() || output == keyboard_output)
    {
        return;
    }

    keyboard_output = output;
    for (QQuickView *view : std::as_const(views))
    {
        LayerShellQt::Window::get(view)->setKeyboardInteractivity(view->screen()->name() == output
                                                                      ? LayerShellQt::Window::KeyboardInteractivityExclusive
                                                                      : LayerShellQt::Window::KeyboardInteractivityNone);
    }
}

void CaptureOverlays::apply_cursor(QQuickView *view)
{
    auto *area = view->rootObject()->findChild<QQuickItem *>(QStringLiteral("pointerArea"));
    if (!area)
    {
        return;
    }

    if (session.mode() != CaptureSession::Selection)
    {
        area->setCursor(Qt::PointingHandCursor);
        return;
    }

    const double density = view->devicePixelRatio();
    QPixmap picture(art_folder + (density > 1 ? QStringLiteral("/crosshair@2x.png") : QStringLiteral("/crosshair.png")));
    if (picture.isNull())
    {
        area->setCursor(Qt::CrossCursor);
        return;
    }

    picture.setDevicePixelRatio(density > 1 ? 2 : 1);
    area->setCursor(QCursor(picture, 16, 16));
}

void CaptureOverlays::hide_selection()
{
    for (QQuickView *view : views)
    {
        view->close();
        view->deleteLater();
    }
    views.clear();
}

void CaptureOverlays::show_countdown(const QPoint& near)
{
    QScreen *screen = QGuiApplication::screenAt(near);
    if (!screen)
    {
        screen = QGuiApplication::primaryScreen();
    }

    countdown = make_view(QStringLiteral("Countdown.qml"), {});
    countdown->setColor(Qt::transparent);
    countdown->resize(COUNTDOWN_SIZE);
    countdown->setScreen(screen);
    auto *layer = LayerShellQt::Window::get(countdown);
    layer->setScope(QStringLiteral("clippings-countdown"));
    layer->setLayer(LayerShellQt::Window::LayerTop);
    layer->setAnchors(LayerShellQt::Window::Anchors());
    layer->setDesiredSize(COUNTDOWN_SIZE);
    layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
    layer->setWantsToBeOnActiveScreen(false);
    layer->setScreen(screen);
    countdown->show();
}

void CaptureOverlays::hide_countdown()
{
    if (countdown)
    {
        countdown->close();
        countdown->deleteLater();
        countdown = nullptr;
    }
}

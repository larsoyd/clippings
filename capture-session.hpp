#pragma once

#include <QHash>
#include <QImage>
#include <QObject>
#include <QRect>
#include <QString>
#include <QTimer>

#include "compositor.hpp"
#include "frozen-desktop.hpp"
#include "screen-grabber.hpp"

// one capture from the key press to the finished image, and only one at a time
class CaptureSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int mode READ mode WRITE set_mode NOTIFY modeChanged)
    Q_PROPERTY(int delay READ delay WRITE set_delay NOTIFY settingsChanged)
    Q_PROPERTY(bool pointer READ pointer WRITE set_pointer NOTIFY settingsChanged)
    Q_PROPERTY(bool running READ running NOTIFY stateChanged)
    Q_PROPERTY(bool selecting READ selecting NOTIFY stateChanged)
    Q_PROPERTY(QRect selection READ selection NOTIFY selectionChanged)
    Q_PROPERTY(QRect target READ target NOTIFY selectionChanged)
    Q_PROPERTY(QString targetTitle READ target_title NOTIFY selectionChanged)
    Q_PROPERTY(QSize pixelSize READ pixel_size NOTIFY selectionChanged)
    Q_PROPERTY(int remaining READ remaining NOTIFY remainingChanged)

  public:
    enum Mode
    {
        Selection,
        Window,
        Screen,
        AllScreens,
    };
    Q_ENUM(Mode)

    enum class State
    {
        Idle,
        Hiding,
        Grabbing,
        Selecting,
        Counting,
        Regrabbing,
    };

    CaptureSession(Compositor& compositor, ScreenGrabber& grabber, qint64 own_pid, QObject *parent = nullptr);
    void set_overlays(OverlayHost *host);
    void set_scratch_folder(const QString& folder);

    int mode() const;
    void set_mode(int mode);
    int delay() const;
    void set_delay(int seconds);
    bool pointer() const;
    void set_pointer(bool with_pointer);
    bool running() const;
    bool selecting() const;
    State state() const;
    QRect selection() const;
    QRect target() const;
    QString target_title() const;
    QSize pixel_size() const;
    int remaining() const;
    const FrozenDesktop& desktop() const;
    // grows on every freeze, so image urls never serve an older picture
    int generation() const;

    // false when a capture is already running
    bool begin(int mode, bool leave_out_own_windows);
    void displays_changed();

    Q_INVOKABLE void pointer_moved(int x, int y);
    Q_INVOKABLE void pressed(int x, int y);
    Q_INVOKABLE void released(int x, int y);
    Q_INVOKABLE void nudge(int x, int y, bool resize);
    Q_INVOKABLE void cycle(int step);
    Q_INVOKABLE void accept();
    Q_INVOKABLE void cancel();

  signals:
    void modeChanged();
    void settingsChanged();
    void stateChanged();
    void selectionChanged();
    void remainingChanged();
    void finished(const QImage& image, double density);
    void cancelled();
    void failed(const QString& reason);
    // the program's own windows go away before a capture, and come back after
    void hideOwnWindows();
    void showOwnWindows();

  private:
    enum class Drag
    {
        None,
        New,
        Move,
        Resize,
    };

    void set_state(State next);
    void freeze();
    void pictures_grabbed(const QHash<QString, QImage>& pictures);
    void take_window_pictures();
    void wait_for_own_windows(const QRect& area);
    void poll_own_windows();
    void area_settled();
    void tick();
    void deliver(const FrozenDesktop& from);
    void finish_with_failure(const QString& reason);
    void end();
    void set_selection(const QRect& area);
    int handle_at(const QPoint& point) const;
    const WindowInfo *hovered_window() const;
    QString scratch_file(qint64 id) const;

    Compositor& compositor;
    ScreenGrabber& grabber;
    OverlayHost *overlays = nullptr;
    qint64 own_pid;
    QString scratch_folder;

    State current = State::Idle;
    Mode current_mode = Selection;
    int delay_seconds = 0;
    bool with_pointer = false;
    bool own_windows_hidden = false;
    qint64 focused_before = -1;
    int freeze_count = 0;

    QList<DisplayInfo> displays;
    QList<WindowInfo> windows;
    FrozenDesktop frozen;
    QHash<qint64, QImage> window_pictures;

    QPoint pointer_at;
    QRect chosen;
    int cycled_window = -1;
    Drag drag = Drag::None;
    int drag_handle = -1;
    QPoint drag_start;
    QRect drag_chosen;

    int seconds_left = 0;
    QTimer second;
    QTimer own_windows_poll;
    QRect hidden_area;
    int polls_left = 0;
};

#include "capture-session.hpp"

#include <QFile>

#include <algorithm>
#include <cstdio>

static const int HANDLE_REACH = 6;
static const int SMALLEST_DRAG = 3;
static const int SETTLE_DEADLINE_MS = 1500;
static const int OWN_WINDOW_POLL_MS = 10;
static const int OWN_WINDOW_POLLS = 150;

// edges rather than pixels, a drag of 300 points selects 300 points
static QRect spanned(const QPoint& one, const QPoint& other)
{
    return QRect(QPoint(std::min(one.x(), other.x()), std::min(one.y(), other.y())),
                 QSize(std::abs(one.x() - other.x()), std::abs(one.y() - other.y())));
}

CaptureSession::CaptureSession(Compositor& wayfire, ScreenGrabber& screens, qint64 pid, QObject *parent)
    : QObject(parent), compositor(wayfire), grabber(screens), own_pid(pid)
{
    connect(&grabber, &ScreenGrabber::grabbed, this, &CaptureSession::pictures_grabbed);
    connect(&grabber, &ScreenGrabber::failed, this, &CaptureSession::finish_with_failure);
    connect(&grabber, &ScreenGrabber::settled, this, &CaptureSession::area_settled);
    second.setInterval(1000);
    connect(&second, &QTimer::timeout, this, &CaptureSession::tick);
    own_windows_poll.setInterval(OWN_WINDOW_POLL_MS);
    connect(&own_windows_poll, &QTimer::timeout, this, &CaptureSession::poll_own_windows);
}

void CaptureSession::set_overlays(OverlayHost *host)
{
    overlays = host;
}

void CaptureSession::set_scratch_folder(const QString& folder)
{
    scratch_folder = folder;
}

int CaptureSession::mode() const
{
    return current_mode;
}

void CaptureSession::set_mode(int mode)
{
    if (mode == current_mode)
    {
        return;
    }

    current_mode = Mode(mode);
    cycled_window = -1;
    emit modeChanged();
    emit selectionChanged();
}

int CaptureSession::delay() const
{
    return delay_seconds;
}

void CaptureSession::set_delay(int seconds)
{
    delay_seconds = std::max(0, seconds);
    emit settingsChanged();
}

bool CaptureSession::pointer() const
{
    return with_pointer;
}

void CaptureSession::set_pointer(bool value)
{
    with_pointer = value;
    emit settingsChanged();
}

bool CaptureSession::running() const
{
    return current != State::Idle;
}

bool CaptureSession::selecting() const
{
    return current == State::Selecting;
}

CaptureSession::State CaptureSession::state() const
{
    return current;
}

QRect CaptureSession::selection() const
{
    return chosen;
}

const WindowInfo *CaptureSession::hovered_window() const
{
    if (cycled_window >= 0 && cycled_window < windows.size())
    {
        return &windows[cycled_window];
    }
    return window_at(windows, pointer_at);
}

QRect CaptureSession::target() const
{
    switch (current_mode)
    {
    case Selection:
        return chosen;
    case Window:
    {
        const WindowInfo *window = hovered_window();
        return window ? window->geometry : QRect();
    }
    case Screen:
    {
        const FrozenOutput *output = frozen.output_at(pointer_at);
        return output ? output->logical : QRect();
    }
    case AllScreens:
        return frozen.bounds();
    }
    return {};
}

QString CaptureSession::target_title() const
{
    const WindowInfo *window = current_mode == Window ? hovered_window() : nullptr;
    return window ? window->title : QString();
}

QSize CaptureSession::pixel_size() const
{
    const QRect area = target();
    const double density = frozen.density(area);
    return area.isEmpty() ? QSize() : QSize(int(area.width() * density), int(area.height() * density));
}

int CaptureSession::remaining() const
{
    return seconds_left;
}

QPoint CaptureSession::pointer_position() const
{
    return pointer_at;
}

QString CaptureSession::pointer_output() const
{
    const FrozenOutput *output = frozen.output_at(pointer_at);
    return output ? output->name : QString();
}

const FrozenDesktop& CaptureSession::desktop() const
{
    return frozen;
}

int CaptureSession::generation() const
{
    return freeze_count;
}

void CaptureSession::set_state(State next)
{
    current = next;
    emit stateChanged();
}

bool CaptureSession::begin(int mode, bool leave_out_own_windows)
{
    if (current != State::Idle)
    {
        std::fprintf(stderr, "[capture] refused: a capture is running\n");
        return false;
    }

    displays = compositor.displays();
    if (displays.isEmpty())
    {
        emit failed(QStringLiteral("The compositor reported no displays."));
        return false;
    }

    set_mode(mode);
    chosen = QRect();
    cycled_window = -1;
    focused_before = compositor.focused();
    windows = compositor.windows();
    pointer_at = compositor.pointer();

    QRect own_area;
    for (const WindowInfo& window : windows)
    {
        if (window.pid == own_pid)
        {
            own_area |= window.geometry;
        }
    }

    if (leave_out_own_windows && !own_area.isEmpty())
    {
        set_state(State::Hiding);
        own_windows_hidden = true;
        emit hideOwnWindows();
        wait_for_own_windows(own_area);
        return true;
    }

    freeze();
    return true;
}

void CaptureSession::wait_for_own_windows(const QRect& area)
{
    hidden_area = area;
    polls_left = OWN_WINDOW_POLLS;
    own_windows_poll.start();
}

void CaptureSession::poll_own_windows()
{
    const QList<WindowInfo> now = compositor.windows();
    const bool gone = std::none_of(now.begin(), now.end(), [this](const WindowInfo& window) { return window.pid == own_pid; });
    if (!gone && --polls_left > 0)
    {
        return;
    }

    own_windows_poll.stop();
    if (!gone)
    {
        std::fprintf(stderr, "[capture] own windows still mapped after hiding polls=%d\n", OWN_WINDOW_POLLS);
    }
    grabber.settle(hidden_area, SETTLE_DEADLINE_MS);
}

void CaptureSession::area_settled()
{
    if (current == State::Hiding)
    {
        windows = compositor.windows();
        freeze();
    }
    else if (current == State::Regrabbing)
    {
        QStringList names;
        for (const DisplayInfo& display : displays)
        {
            names.append(display.name);
        }
        grabber.grab(names, with_pointer);
    }
}

void CaptureSession::freeze()
{
    set_state(State::Grabbing);
    QStringList names;
    for (const DisplayInfo& display : displays)
    {
        names.append(display.name);
    }
    grabber.grab(names, with_pointer);
}

void CaptureSession::pictures_grabbed(const QHash<QString, QImage>& pictures)
{
    if (current != State::Grabbing && current != State::Regrabbing)
    {
        return;
    }

    FrozenDesktop desktop;
    for (const DisplayInfo& display : displays)
    {
        if (!pictures.contains(display.name))
        {
            finish_with_failure(QStringLiteral("no picture of display %1").arg(display.name));
            return;
        }
        desktop.add({display.name, display.logical, pictures.value(display.name)});
    }

    if (current == State::Regrabbing)
    {
        if (current_mode == Window)
        {
            take_window_pictures();
        }
        deliver(desktop);
        return;
    }

    frozen = desktop;
    freeze_count++;
    take_window_pictures();
    if (overlays)
    {
        overlays->show_selection();
    }
    set_state(State::Selecting);
    emit selectionChanged();
}

QString CaptureSession::scratch_file(qint64 id) const
{
    return QStringLiteral("%1/window-%2.rgba").arg(scratch_folder).arg(id);
}

void CaptureSession::take_window_pictures()
{
    window_pictures.clear();
    for (const WindowInfo& window : std::as_const(windows))
    {
        if (window.pid == own_pid && own_windows_hidden)
        {
            continue;
        }

        const QString file = scratch_file(window.id);
        const QImage picture = compositor.window_picture(window.id, file);
        QFile::remove(file);
        if (!picture.isNull())
        {
            window_pictures.insert(window.id, picture);
        }
    }
}

void CaptureSession::pointer_moved(int x, int y)
{
    pointer_at = QPoint(x, y);
    cycled_window = -1;

    const QPoint at = pointer_at;
    if (drag == Drag::New)
    {
        set_selection(spanned(drag_start, at));
        return;
    }
    if (drag == Drag::Move)
    {
        const QRect all = frozen.bounds();
        QRect moved = drag_chosen.translated(at - drag_start);
        moved.moveLeft(std::clamp(moved.left(), all.left(), std::max(all.left(), all.right() + 1 - moved.width())));
        moved.moveTop(std::clamp(moved.top(), all.top(), std::max(all.top(), all.bottom() + 1 - moved.height())));
        set_selection(moved);
        return;
    }
    if (drag == Drag::Resize)
    {
        // handles count clockwise from the top left, odd ones sit on an edge's middle
        QPoint first = drag_chosen.topLeft();
        QPoint second = drag_chosen.topLeft() + QPoint(drag_chosen.width(), drag_chosen.height());
        const int handle = drag_handle;
        if (handle == 7 || handle == 0 || handle == 6)
        {
            first.setX(at.x());
        }
        if (handle == 2 || handle == 3 || handle == 4)
        {
            second.setX(at.x());
        }
        if (handle == 0 || handle == 1 || handle == 2)
        {
            first.setY(at.y());
        }
        if (handle == 4 || handle == 5 || handle == 6)
        {
            second.setY(at.y());
        }
        set_selection(spanned(first, second));
        return;
    }
    emit selectionChanged();
}

int CaptureSession::handle_at(const QPoint& point) const
{
    if (chosen.isEmpty())
    {
        return -1;
    }

    const int left = chosen.left();
    const int top = chosen.top();
    const int right = left + chosen.width();
    const int bottom = top + chosen.height();
    const int middle_x = left + chosen.width() / 2;
    const int middle_y = top + chosen.height() / 2;
    const QPoint places[] = {{left, top}, {middle_x, top}, {right, top}, {right, middle_y},
                             {right, bottom}, {middle_x, bottom}, {left, bottom}, {left, middle_y}};
    for (int index = 0; index < 8; index++)
    {
        if ((places[index] - point).manhattanLength() <= HANDLE_REACH * 2)
        {
            return index;
        }
    }
    return -1;
}

void CaptureSession::pressed(int x, int y)
{
    if (current != State::Selecting)
    {
        return;
    }

    pointer_at = QPoint(x, y);
    if (current_mode != Selection)
    {
        return;
    }

    drag_start = pointer_at;
    drag_chosen = chosen;
    drag_handle = handle_at(pointer_at);
    drag = drag_handle >= 0 ? Drag::Resize : chosen.contains(pointer_at) ? Drag::Move : Drag::New;
    if (drag == Drag::New)
    {
        set_selection(QRect());
    }
}

void CaptureSession::released(int x, int y)
{
    if (current != State::Selecting)
    {
        return;
    }

    pointer_at = QPoint(x, y);
    if (current_mode != Selection)
    {
        accept();
        return;
    }

    const QRect dragged = spanned(drag_start, pointer_at);
    if (drag == Drag::New && (dragged.width() < SMALLEST_DRAG || dragged.height() < SMALLEST_DRAG))
    {
        set_selection(QRect());
    }
    drag = Drag::None;
    drag_handle = -1;
}

void CaptureSession::nudge(int x, int y, bool resize)
{
    if (current != State::Selecting)
    {
        return;
    }

    if (current_mode != Selection)
    {
        cycle(x + y);
        return;
    }

    if (chosen.isEmpty())
    {
        const FrozenOutput *output = frozen.output_at(pointer_at);
        const QRect place = output ? output->logical : frozen.bounds();
        set_selection(QRect(place.center() - QPoint(100, 75), QSize(200, 150)));
        return;
    }

    const QRect all = frozen.bounds();
    if (resize)
    {
        QRect grown = chosen;
        grown.setWidth(std::max(1, grown.width() + x));
        grown.setHeight(std::max(1, grown.height() + y));
        set_selection(grown & all);
        return;
    }

    QRect moved = chosen.translated(x, y);
    moved.moveLeft(std::clamp(moved.left(), all.left(), std::max(all.left(), all.right() + 1 - moved.width())));
    moved.moveTop(std::clamp(moved.top(), all.top(), std::max(all.top(), all.bottom() + 1 - moved.height())));
    set_selection(moved);
}

void CaptureSession::cycle(int step)
{
    if (current != State::Selecting || windows.isEmpty() || current_mode != Window)
    {
        return;
    }

    const int size = int(windows.size());
    const int from = cycled_window >= 0 ? cycled_window : 0;
    cycled_window = (from + step % size + size) % size;
    emit selectionChanged();
}

void CaptureSession::set_selection(const QRect& area)
{
    chosen = area;
    emit selectionChanged();
}

void CaptureSession::accept()
{
    if (current != State::Selecting)
    {
        return;
    }

    if (target().isEmpty())
    {
        std::fprintf(stderr, "[capture] nothing chosen mode=%d\n", int(current_mode));
        return;
    }

    std::fprintf(stderr, "[capture] accepted mode=%d delay=%d area=%d,%d %dx%d\n", int(current_mode), delay_seconds,
                 target().x(), target().y(), target().width(), target().height());
    if (overlays)
    {
        overlays->hide_selection();
    }

    if (delay_seconds > 0)
    {
        // the target is fixed now, the pointer is free to set the scene up
        chosen = target();
        seconds_left = delay_seconds;
        emit remainingChanged();
        set_state(State::Counting);
        if (overlays)
        {
            overlays->show_countdown(chosen.center());
        }
        second.start();
        return;
    }

    deliver(frozen);
}

void CaptureSession::tick()
{
    seconds_left--;
    emit remainingChanged();
    if (seconds_left > 0)
    {
        return;
    }

    second.stop();
    if (overlays)
    {
        overlays->hide_countdown();
    }
    set_state(State::Regrabbing);
    grabber.settle(frozen.bounds().intersected(chosen.adjusted(-400, -200, 400, 200)), SETTLE_DEADLINE_MS);
}

void CaptureSession::deliver(const FrozenDesktop& from)
{
    const QRect area = current == State::Regrabbing ? chosen : target();
    QImage image;
    double density = from.density(area);

    const WindowInfo *window = nullptr;
    for (const WindowInfo& candidate : std::as_const(windows))
    {
        if (current_mode == Window && candidate.geometry == area)
        {
            window = &candidate;
            break;
        }
    }

    if (window && window_pictures.contains(window->id))
    {
        image = window_pictures.value(window->id);
        density = double(image.width()) / std::max(1, area.width());
    }
    else
    {
        image = from.compose(area);
    }

    if (image.isNull())
    {
        finish_with_failure(QStringLiteral("nothing could be composed for %1,%2 %3x%4")
                                .arg(area.x()).arg(area.y()).arg(area.width()).arg(area.height()));
        return;
    }

    end();
    std::fprintf(stderr, "[capture] finished size=%dx%d density=%g\n", image.width(), image.height(), density);
    emit finished(image, density);
}

void CaptureSession::cancel()
{
    if (current == State::Idle)
    {
        return;
    }

    const qint64 back_to = focused_before;
    end();
    if (back_to >= 0)
    {
        compositor.focus(back_to);
    }
    emit cancelled();
}

void CaptureSession::finish_with_failure(const QString& reason)
{
    if (current == State::Idle)
    {
        return;
    }

    end();
    emit failed(reason);
}

void CaptureSession::displays_changed()
{
    if (current != State::Idle)
    {
        finish_with_failure(QStringLiteral("The displays changed during the capture."));
    }
}

void CaptureSession::end()
{
    grabber.stop();
    second.stop();
    own_windows_poll.stop();
    if (overlays)
    {
        overlays->hide_selection();
        overlays->hide_countdown();
    }

    frozen.clear();
    window_pictures.clear();
    drag = Drag::None;
    chosen = QRect();
    const bool bring_back = own_windows_hidden;
    own_windows_hidden = false;
    set_state(State::Idle);
    if (bring_back)
    {
        emit showOwnWindows();
    }
}

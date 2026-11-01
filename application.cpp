#include "application.hpp"

#include "export.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDrag>
#include <QJsonArray>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeData>
#include <QProcess>
#include <QStandardPaths>
#include <QWindow>

#include <cstdio>

static const int THUMBNAIL_SIDE = 96;

Application::Application(CaptureSession& capture, WayfireClient& wayfire, QObject *parent)
    : QObject(parent), session(capture), compositor(wayfire), preferences(QStringLiteral("clippings"), QStringLiteral("clippings"))
{
    session.set_delay(preferences.value(QStringLiteral("delay"), 0).toInt());
    session.set_pointer(preferences.value(QStringLiteral("pointer"), false).toBool());
    connect(&session, &CaptureSession::settingsChanged, this, [this] {
        preferences.setValue(QStringLiteral("delay"), session.delay());
        preferences.setValue(QStringLiteral("pointer"), session.pointer());
    });
    connect(&session, &CaptureSession::finished, this, &Application::add_capture);
    connect(&session, &CaptureSession::failed, this, &Application::capture_failed);
    connect(&session, &CaptureSession::cancelled, this, &Application::quit_if_idle);
    connect(&session, &CaptureSession::hideOwnWindows, this, [this] {
        windows_hidden = true;
        emit ownWindowsChanged();
    });
    connect(&session, &CaptureSession::showOwnWindows, this, [this] {
        windows_hidden = false;
        emit ownWindowsChanged();
    });
}

QVariantList Application::documents() const
{
    QVariantList list;
    for (Document *document : open_documents)
    {
        list.append(QVariant::fromValue(static_cast<QObject *>(document)));
    }
    return list;
}

bool Application::launcher_shown() const
{
    return launcher;
}

void Application::set_launcher_shown(bool shown)
{
    if (shown != launcher)
    {
        launcher = shown;
        emit launcherChanged();
    }
}

bool Application::own_windows_hidden() const
{
    return windows_hidden;
}

bool Application::play_sound() const
{
    return preferences.value(QStringLiteral("sound"), true).toBool();
}

void Application::set_play_sound(bool play)
{
    preferences.setValue(QStringLiteral("sound"), play);
    emit preferencesChanged();
}

QString Application::save_folder() const
{
    return preferences.value(QStringLiteral("folder"), QDir::homePath() + QStringLiteral("/Desktop")).toString();
}

void Application::set_save_folder(const QString& folder)
{
    preferences.setValue(QStringLiteral("folder"), folder);
    emit preferencesChanged();
}

QString Application::art_folder() const
{
    const QString data = qEnvironmentVariable("XDG_DATA_HOME", QDir::homePath() + QStringLiteral("/.local/share"));
    return data + QStringLiteral("/clippings/art");
}

QString Application::icon_source() const
{
    const QString icon = art_folder() + QStringLiteral("/clippings-256.png");
    return QFileInfo::exists(icon) ? QUrl::fromLocalFile(icon).toString() : QString();
}

QString Application::version() const
{
    return QCoreApplication::applicationVersion();
}

void Application::run_command(const QJsonObject& command)
{
    const QString capture_word = command.value(QLatin1String("capture")).toString();
    if (!capture_word.isEmpty())
    {
        static const QStringList modes{QStringLiteral("selection"), QStringLiteral("window"), QStringLiteral("screen"), QStringLiteral("all")};
        capture(int(std::max<qsizetype>(0, modes.indexOf(capture_word))), false);
        return;
    }

    const QJsonArray files = command.value(QLatin1String("open")).toArray();
    if (!files.isEmpty())
    {
        QList<QUrl> urls;
        for (const QJsonValue& file : files)
        {
            urls.append(QUrl::fromUserInput(file.toString(), QDir::currentPath(), QUrl::AssumeLocalFile));
        }
        open_urls(urls);
        return;
    }

    if (open_documents.isEmpty())
    {
        set_launcher_shown(true);
    }
    bring_forward();
}

void Application::capture(int mode, bool leave_out_own_windows)
{
    session.begin(mode, leave_out_own_windows);
}

void Application::capture_failed(const QString& reason)
{
    std::fprintf(stderr, "[capture] failed reason=%s\n", qPrintable(reason));
    emit alert(QStringLiteral("The screen could not be captured."), reason);
    if (open_documents.isEmpty())
    {
        set_launcher_shown(true);
    }
}

void Application::add_capture(const QImage& image, double density)
{
    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd 'at' HH.mm.ss"));
    add_document(new Document(image, QStringLiteral("Screen Shot %1").arg(now), density, this));
    set_launcher_shown(false);
    if (play_sound())
    {
        QProcess::startDetached(QStringLiteral("mavernix-sound"), {QStringLiteral("screen-capture")});
    }
}

void Application::add_document(Document *document)
{
    open_documents.append(document);
    emit documentsChanged();
}

void Application::open_urls(const QList<QUrl>& urls)
{
    for (const QUrl& url : urls)
    {
        const QString path = url.toLocalFile();
        QImageReader reader(path);
        reader.setAutoTransform(true);
        const QImage image = reader.read();
        if (image.isNull())
        {
            emit alert(QStringLiteral("The document “%1” could not be opened.").arg(QFileInfo(path).fileName()), reader.errorString());
            continue;
        }

        auto *document = new Document(image, QFileInfo(path).fileName(), 1, this);
        document->opened_from(path);
        add_document(document);
    }

    if (!open_documents.isEmpty())
    {
        set_launcher_shown(false);
    }
    else if (!launcher)
    {
        set_launcher_shown(true);
    }
}

void Application::close_document(QObject *object)
{
    auto *document = qobject_cast<Document *>(object);
    if (!document || !open_documents.removeOne(document))
    {
        return;
    }

    emit documentsChanged();
    document->deleteLater();
    quit_if_idle();
}

void Application::closed_launcher()
{
    set_launcher_shown(false);
    quit_if_idle();
}

void Application::quit_if_idle()
{
    if (open_documents.isEmpty() && !launcher && !session.running())
    {
        std::fprintf(stderr, "[application] quitting, nothing left open\n");
        QCoreApplication::quit();
    }
}

void Application::quit_now()
{
    session.cancel();
    QCoreApplication::quit();
}

QString Application::default_save_url(QObject *object) const
{
    auto *document = qobject_cast<Document *>(object);
    if (!document)
    {
        return {};
    }

    const QString known = document->path();
    if (!known.isEmpty())
    {
        return QUrl::fromLocalFile(known).toString();
    }
    return QUrl::fromLocalFile(save_folder() + QLatin1Char('/') + document->title() + QStringLiteral(".png")).toString();
}

QString Application::file_of(const QUrl& url) const
{
    return url.toLocalFile();
}

QString Application::copy(QObject *object)
{
    auto *document = qobject_cast<Document *>(object);
    return document ? copy_image(document->flattened()) : QStringLiteral("There is no document to copy.");
}

void Application::drag_out(QObject *object, QQuickItem *from)
{
    auto *document = qobject_cast<Document *>(object);
    if (!document || !from)
    {
        return;
    }

    // a drag always carries the flattened edits, so an unsaved or changed document gets a file of its own
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/drags");
    QDir().mkpath(folder);
    const QString name = QFileInfo(document->title()).completeBaseName().isEmpty() ? QStringLiteral("Clipping") : document->title();
    const QString file = folder + QLatin1Char('/') + (name.endsWith(QLatin1String(".png")) ? name : name + QStringLiteral(".png"));
    const QString problem = write_image(document->flattened(), file);
    if (!problem.isEmpty())
    {
        emit alert(QStringLiteral("The picture could not be dragged."), problem);
        return;
    }

    auto *mime = new QMimeData;
    mime->setUrls({QUrl::fromLocalFile(file)});
    mime->setImageData(document->flattened());

    auto *drag = new QDrag(from);
    drag->setMimeData(mime);
    drag->setPixmap(QPixmap::fromImage(document->flattened().scaled(THUMBNAIL_SIDE, THUMBNAIL_SIDE, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    drag->exec(Qt::CopyAction);
}

void Application::bring_forward()
{
    QList<qint64> own;
    for (const WindowInfo& window : compositor.windows())
    {
        if (window.pid == QCoreApplication::applicationPid())
        {
            own.append(window.id);
        }
    }
    if (!own.isEmpty())
    {
        compositor.focus(own.first());
    }
}

void Application::minimize_window(QObject *window)
{
    if (auto *quick = qobject_cast<QWindow *>(window))
    {
        quick->showMinimized();
    }
}

#pragma once

#include <QImage>
#include <QJsonObject>
#include <QObject>
#include <QQuickItem>
#include <QSettings>
#include <QString>
#include <QUrl>
#include <QVariantList>

#include "capture-session.hpp"
#include "document.hpp"
#include "wayfire-client.hpp"

// the running program: its documents, what starts a capture, and when it may quit
class Application : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList documents READ documents NOTIFY documentsChanged)
    Q_PROPERTY(bool launcherShown READ launcher_shown WRITE set_launcher_shown NOTIFY launcherChanged)
    Q_PROPERTY(bool ownWindowsHidden READ own_windows_hidden NOTIFY ownWindowsChanged)
    Q_PROPERTY(bool playSound READ play_sound WRITE set_play_sound NOTIFY preferencesChanged)
    Q_PROPERTY(QString saveFolder READ save_folder WRITE set_save_folder NOTIFY preferencesChanged)
    Q_PROPERTY(QString iconSource READ icon_source CONSTANT)
    Q_PROPERTY(QString artFolder READ art_folder CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)

  public:
    Application(CaptureSession& session, WayfireClient& compositor, QObject *parent = nullptr);

    QVariantList documents() const;
    bool launcher_shown() const;
    void set_launcher_shown(bool shown);
    bool own_windows_hidden() const;
    bool play_sound() const;
    void set_play_sound(bool play);
    QString save_folder() const;
    void set_save_folder(const QString& folder);
    QString icon_source() const;
    QString art_folder() const;
    QString version() const;

    void run_command(const QJsonObject& command);

    Q_INVOKABLE void capture(int mode, bool leave_out_own_windows);
    Q_INVOKABLE void open_urls(const QList<QUrl>& urls);
    Q_INVOKABLE void close_document(QObject *document);
    Q_INVOKABLE void closed_launcher();
    Q_INVOKABLE void quit_now();
    Q_INVOKABLE QString default_save_url(QObject *document) const;
    Q_INVOKABLE QString file_of(const QUrl& url) const;
    Q_INVOKABLE QString copy(QObject *document);
    Q_INVOKABLE void drag_out(QObject *document, QQuickItem *from);
    Q_INVOKABLE void bring_forward();
    Q_INVOKABLE void minimize_window(QObject *window);

  signals:
    void documentsChanged();
    void launcherChanged();
    void ownWindowsChanged();
    void preferencesChanged();
    void alert(const QString& message, const QString& detail);
    void quitRequested();

  private:
    void add_capture(const QImage& image, double density);
    void add_document(Document *document);
    void quit_if_idle();
    void capture_failed(const QString& reason);

    CaptureSession& session;
    WayfireClient& compositor;
    QList<Document *> open_documents;
    QSettings preferences;
    bool launcher = false;
    bool windows_hidden = false;
    int untitled = 0;
};

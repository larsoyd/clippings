#pragma once

#include <QIcon>
#include <QQuickImageProvider>

// image://themed/NAME, the desktop's icon of that name, so tool bars match the other apps
class ThemedIcons : public QQuickImageProvider
{
public:
    ThemedIcons() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

    QPixmap requestPixmap(const QString& name, QSize *size, const QSize& requested) override
    {
        const int side = requested.width() > 0 ? requested.width() : 32;
        const QPixmap picture = QIcon::fromTheme(name).pixmap(side, side);
        if (size)
        {
            *size = picture.size();
        }
        return picture;
    }
};

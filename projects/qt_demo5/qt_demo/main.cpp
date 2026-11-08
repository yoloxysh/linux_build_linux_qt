#include "qt_demo.h"

#include <QtGui/QFont>
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
#ifdef Q_OS_WIN
    QFont font(QStringLiteral("Microsoft YaHei"));
#else
    QFont font(QStringLiteral("WenQuanYi Zen Hei"));
#endif
    font.setPixelSize(14);
    app.setFont(font);

    qt_demo window;
    window.resize(800, 480);
#ifdef Q_OS_WIN
    window.show();
#else
    window.showFullScreen();
#endif
    return app.exec();
}

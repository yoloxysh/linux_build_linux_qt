#include "qt_demo.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qt_demo"));
    qt_demo window;
    window.setWindowTitle(QStringLiteral("HMI 冲击试验台"));
    window.resize(800, 480);
    window.showFullScreen();
    return app.exec();
}

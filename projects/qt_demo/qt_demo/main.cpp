#include "qt_demo.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qt_demo window;
    window.showFullScreen();
    return app.exec();
}

#include <QApplication>
#include "html5applicationviewer.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Html5ApplicationViewer viewer("open/");
    viewer.setOrientation(Html5ApplicationViewer::ScreenOrientationAuto);
    viewer.showExpanded();
    viewer.setMinimumSize(500,250);
    viewer.resize(500,400);
    viewer.setMouseTracking(true);
    return app.exec();
}

#ifndef HTML5APPLICATIONVIEWER_H
#define HTML5APPLICATIONVIEWER_H

#include <QWidget>
#include <QUrl>
#include <QPushButton>

#include "logger.h"

class QGraphicsWebView;

class Html5ApplicationViewer : public QWidget
{
    Q_OBJECT
    QString Patch;
    QStringList current_wheel_angle;
    QStringList current_wheel_power;
    QStringList line_position;
    QStringList file_names;
    QPushButton *buttonWA;
    QPushButton *buttonWP;
    QPushButton *buttonLP;

public:
    enum ScreenOrientation {
        ScreenOrientationLockPortrait,
        ScreenOrientationLockLandscape,
        ScreenOrientationAuto
    };

    explicit Html5ApplicationViewer(QString patch="", QWidget *parent = 0);
    virtual ~Html5ApplicationViewer();

    void loadFile(const QString &fileName);
    void loadUrl(const QUrl &url);

    void setOrientation(ScreenOrientation orientation);

    void showExpanded();

    QGraphicsWebView *webView() const;

private:
    class Html5ApplicationViewerPrivate *m_d;
public slots:
    void loadWheelAngle();
    void loadWheelPower();
    void loadLinePosition();
    void reload();
};

#endif

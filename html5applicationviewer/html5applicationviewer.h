#ifndef HTML5APPLICATIONVIEWER_H
#define HTML5APPLICATIONVIEWER_H

#include <QWidget>
#include <QUrl>
#include <QPushButton>
#include <QColumnView>
#include <QSplitter>
#include <QListWidget>
#include <QFileDialog>

#include "logger.h"
#include "graphdata.h"

class QGraphicsWebView;

class Html5ApplicationViewer : public QWidget
{
    Q_OBJECT
    GraphData data;//обьект для хранения данных графиков
    QListWidget *listOfOpenedFiles;//список открытых файлов
    QListWidget *listOfGraphs;//список типов графиков для отображения
    QString lastPatch="";//путь последнегоудачного открытия файла
    QFrame *frameWithGraphs;//фрейм в котором будут отображатся графики
    QStringList listOfGraphNames;
    QPushButton *button_Save;
    void addFileToList(QString fileName);//добавление файлов в
public:
    enum ScreenOrientation {
        ScreenOrientationLockPortrait
    };
    explicit Html5ApplicationViewer(QWidget *parent = 0);
    virtual ~Html5ApplicationViewer();

    void load(int index,const QString &stringFileSrc);//загрузка html-страницы в indexй view
    void showExpanded();
    void redivisionGraph(int count);//деление области для графиков(frameWithGraphs) на count графиков
    QGraphicsWebView *webView(int index) const;//возвращает indexй view
private:
    class Html5ApplicationViewerPrivate **view;//область для отображения html-страницы
public slots:
    void deleteItem();//удаление из списка
    void openFolder();//открытие папки
    void openFile();//открытие файла
    void selectItem(QListWidgetItem* listWidgetItem);//установка галочек выбора
    void selectItemToShow(int i);//Загрузка/удаление данных из файла в программе
    void show1();//перерисовка графика
    void potomNazovuFunc();//функция для обработки выбора типа графика
    void saveImages();
};

#endif



#ifndef EXTENDEDLISTITEM_H
#define EXTENDEDLISTITEM_H

#include <QWidget>
#include <QPushButton>
#include <QListWidget>
#include <QCheckBox>


#include <QtDebug>


class ExtendedListItem : public QWidget
{
    Q_OBJECT
        QColor colorDef;
        QCheckBox *checkBox;
        QListWidgetItem *listWItemParent;
        QString stringSrc;
        QPushButton *button;
signals:
    void buttonClicked();
    void checkBoxChanged(int);
public:
        ExtendedListItem(QString stringLabel, bool addButton = true, QWidget *parent = 0);
        ExtendedListItem(QListWidget* listWidget, QString stringLabel, bool addButton = true);
        ~ExtendedListItem();
        QString getLabelText();
        QString getFileSrc();
        void addTo(QListWidget* listWidget);
        void Check();
        bool isChecked();        
        void setColorOfCheckBox(QString color);
        void setClearColorOfCheckBox();
        QString getColor();
        QListWidgetItem* parent1();
};

#endif // EXTENDEDLISTITEM_H

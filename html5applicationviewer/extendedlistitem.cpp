#include "extendedlistitem.h"
#include <QGridLayout>

ExtendedListItem::ExtendedListItem(QString str_label, bool addButton, QWidget *parent)
    : QWidget(parent)
{
  listWItemParent = new QListWidgetItem;
  QGridLayout *layout = new QGridLayout;
  stringSrc=str_label;
  if(str_label.lastIndexOf('/')!=-1)
      str_label=str_label.mid(str_label.lastIndexOf('/')+1,str_label.length()-1);
  checkBox =new QCheckBox(str_label);
  checkBox->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
  connect(checkBox,SIGNAL(stateChanged(int)),SIGNAL(checkBoxChanged(int)));
  if(addButton){
    button = new QPushButton("x");
    button->setMaximumSize(30,30);
    connect(button,SIGNAL(clicked()),SIGNAL(buttonClicked()));
    layout->addWidget(button,0,1,Qt::AlignRight);
    layout->setColumnStretch(1,0);
  }
  else
    button=0;
  layout->setColumnStretch(0,1);
  layout->addWidget(checkBox,0,0);
  layout->setMargin(0);
  setLayout(layout);
      checkBox->setStyleSheet("background:#fff");
  listWItemParent->setSizeHint(QSize(this->sizeHint().width(),40));
}
void ExtendedListItem::setColorOfCheckBox(QString color)
{
    listWItemParent->setBackgroundColor(QColor(color));
    checkBox->setStyleSheet("QCheckBox{color:white;background:"+color+"}");
}
void ExtendedListItem::setClearColorOfCheckBox()
{

    listWItemParent->setBackgroundColor(QColor(255,255,255));
    checkBox->setStyleSheet("background:#fff");
}

QListWidgetItem* ExtendedListItem::parent1()
{
    return listWItemParent;
}

ExtendedListItem::ExtendedListItem(QListWidget* ListWidget, QString str_label, bool addButton): ExtendedListItem(str_label,addButton,ListWidget)
{
  this->addTo(ListWidget);
}

void ExtendedListItem::addTo(QListWidget *ListWidget)
{
  ListWidget->addItem(listWItemParent);
  ListWidget->setItemWidget(listWItemParent, this);
}
bool ExtendedListItem::isChecked(){
    return checkBox->isChecked();
}

void ExtendedListItem::Check()
{
  checkBox->setChecked(!checkBox->isChecked());
}
QString ExtendedListItem::getLabelText()
{
  return checkBox->text();
}
QString ExtendedListItem::getFileSrc()
{
  return stringSrc;
}
ExtendedListItem::~ExtendedListItem()
{
  delete listWItemParent;
  delete checkBox;
  delete button;
}


#ifndef MYQLISTWIDGETITEM_H
#define MYQLISTWIDGETITEM_H

#include <QWidget>
#include <QListWidget>

class MyQListWidgetItem : public QListWidgetItem
{
public:
    MyQListWidgetItem();
    MyQListWidgetItem(QString file);
    QString filepath;//记录文件的绝对地址，在列表中显示时只显示文件名
    void addPlayingMark();
    void addPauseMark();
    void removePlayingMark();
    bool playingMark;
private:
    const int ITEM_HEIGHT=40;//Item的高度
};

#endif // MYQLISTWIDGETITEM_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include"myqlistwidgetitem.h"
#include "player.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    //实现无标题栏之后自定义标题栏的拖动功能
    QPoint move_point; //移动的距离
    bool mouse_press; //鼠标按下
    //鼠标按下事件
    void mousePressEvent(QMouseEvent *event);
   //鼠标释放事件
    void mouseReleaseEvent(QMouseEvent *event);
   //鼠标移动事件
    void mouseMoveEvent(QMouseEvent *event);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private slots:
     void minButtonClicked();//处理最小化按钮单击
     void closeButtonClicked();//处理关闭按钮单击
     void myTimeout();//处理QTimer的超时函数，即计时器运行时，每隔一秒进行响应
     void on_loadFileButton_clicked();//处理导入文件按钮单击
     void on_playButton_clicked();//处理播放按钮单击
     void on_preButton_clicked();//处理上一曲按钮单击
     void on_nextButton_clicked();//处理下一曲按钮单击
     void on_volumnSlider_volumnChanged(int);//实时处理音量滑动条改变的音量调节
     void on_listWidget_itemDoubleClicked(QListWidgetItem *item);//处理列表Item按钮单击
     void on_modeBotton_clicked();//处理音乐播放模式按钮单击
     void updateItem(QAction*);//处理音乐播放模式选择后更改播放模式

     void on_volumnButton_clicked();//处理音量按钮单击
     void on_upButton_clicked();
     void on_downButton_clicked();

public slots:
     void on_stopButton_clicked();

private:
     //设置自定义标题栏的高度
     //这涉及到鼠标拖动时的响应区域
     //标题栏色块的区域以及最小化和关闭按钮的区域
     const int TITLE_HEIGHT=40;
     //底部的工具栏的背景色
     const int BOTTOM_HEIGHT=100;

     player Player;

     QTimer *timer;//计时器
     QSlider *volumnSlider;//音量滑动条
     QMenu *modeMenu;//播放模式菜单


     bool playing=false;//设置初始播放状态为假

     MyQListWidgetItem * currentPlayingItem;

     pthread_t play_thread;

     double speedmode;//记录当前播放模式
     double current_second = 0;

};

void* song_play_wrapper(void* arg);
#endif // MAINWINDOW_H

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTextCodec>
#include <QPropertyAnimation>
#include <QTimer>
#include <QMouseEvent>
#include <QMenu>
#include <QWidgetAction>
#include <QFileDialog>
#include <QMessageBox>
#include "myqlistwidgetitem.h"
#include<QDebug>

struct ThreadArguments {
    player* Player;
    QString filepath;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
    ui->setupUi(this);

    //设置白色背景
    QLabel *background = new QLabel(this);
    background->setStyleSheet("QLabel{border-image: url(:/images/background.jpg)}");
    background->setGeometry(0, 40, this->width(), this->height()-140);
    //设置指定颜色的标题栏
    QLabel *title = new QLabel(this);
    title->setStyleSheet("QLabel{background-color:rgb(242,242,242)}");
    title->setGeometry(0, 0, this->width(), TITLE_HEIGHT);
    //设置下方控件区域的蓝色背景
    QLabel *bottom = new QLabel(this);
    bottom->setStyleSheet("background-color: rgb(13,127,217)");
    bottom->setGeometry(0,this->height()-BOTTOM_HEIGHT , this->width(), BOTTOM_HEIGHT);

    ui->setupUi(this);//再设置一次UI

    //设置窗体标题栏隐藏并设置位于顶层
    setWindowFlags(Qt::FramelessWindowHint);
    //可获取鼠标跟踪效果
    setMouseTracking(true);

    //计时器
    timer=new QTimer();

    //音量调节滑动条
    volumnSlider=new QSlider();

    volumnSlider->setMinimum(0);
    volumnSlider->setMaximum(100);
    volumnSlider->setValue(this->Player.volume);

    //音乐播放模式的菜单
    modeMenu=new QMenu();
    modeMenu->setStyleSheet("QMenu{ padding:5px; background:white; border:1px solid gray; } QMenu::item{ padding:0px 0px 0px 20px; height:25px; } QMenu::item:selected:enabled{ background:lightgray; color:white; } QMenu::item:selected:!enabled{ background:transparent; } QMenu::separator{ height:1px; background:lightgray; margin:5px 0px 5px 0px; }");
    //定义一个action组
    QActionGroup *group = new QActionGroup(this);
    group->setExclusive(true);
    group->setEnabled(true);
    // 增加Action
    QAction *action1 = new QAction(tr("0.5 speed"), group);
    action1->setCheckable(true);
    modeMenu->addAction(action1);
    QAction *action2 = new QAction(tr("1.0 speed"), group);
    action2->setCheckable(true);
    action2->setChecked(true);
    modeMenu->addAction(action2);
    QAction *action3 = new QAction(tr("1.5 speed"), group);
    action3->setCheckable(true);
    modeMenu->addAction(action3);
    QAction *action4 = new QAction(tr("2.0 speed"), group);
    action4->setCheckable(true);
    modeMenu->addAction(action4);
    // 设置播放模式菜单的信号和槽函数
    connect(group, SIGNAL(triggered(QAction*)), SLOT(updateItem(QAction*)));

    //初始模式为顺序播放(自动循环)
    speedmode=1;
    ui->modeBotton->setStyleSheet("QToolButton{border-image: url(:/images/one.png) } QToolButton:hover{border-image: url(:/images/one_hover.png)} ");

    QToolButton *minButton = new QToolButton(this);
    QToolButton *closeButton= new QToolButton(this);
    minButton->setStyleSheet("QToolButton{border-image: url(:/images/min.png) } QToolButton:hover{border-image: url(:/images/min_hover.png)} ");
    closeButton->setStyleSheet("QToolButton{border-image: url(:/images/close.png) } QToolButton:hover{border-image: url(:/images/close_hover.png)} ");
    minButton->setGeometry(this->width()-TITLE_HEIGHT*2,  0,  TITLE_HEIGHT,  TITLE_HEIGHT);
    closeButton->setGeometry(this->width()-TITLE_HEIGHT,  0,  TITLE_HEIGHT,  TITLE_HEIGHT);

    ui->listWidget->setFocusPolicy(Qt::NoFocus);//去除项目被选中时的虚线边框
    ui->listWidget->setAlternatingRowColors(true);//设置列表中想邻条目背景色
    ui->listWidget->setStyleSheet("QListWidget{background-color:rgba(255, 255, 255, 0.5)} QListWidget::item{color:black; background-color:rgba(255, 255, 255, 0.6); } QListWidget::item:alternate{background:rgba(242,242,242,0.6);} QListWidget::item:selected:enabled{ color:white;  background:rgb(0,120,215)} QListWidget::item:hover{background:rgb(233,244,255)}");

    ui->playButton->setStyleSheet("QToolButton{border-image: url(:/images/play.png) } QToolButton:hover{border-image: url(:/images/play_clicked.png)} QToolButton:pressed{border-image: url(:/images/play_hover.png)}");
    ui->nextButton->setStyleSheet("QToolButton{border-image: url(:/images/next.png) } QToolButton:hover{border-image: url(:/images/next_hover.png)} QToolButton:pressed{border-image: url(:/images/next_clicked.png)}");
    ui->preButton->setStyleSheet("QToolButton{border-image: url(:/images/pre.png) } QToolButton:hover{border-image: url(:/images/pre_hover.png)} QToolButton:pressed{border-image: url(:/images/pre_clicked.png)}");

    ui->progresslSlider->setStyleSheet("QSlider::groove:horizontal{ border:0px; height:4px; } QSlider::sub-page:horizontal{ background:white; } QSlider::add-page:horizontal{ background:lightgray; } QSlider::handle:horizontal{ background:white; width:10px; border-radius:5px; margin:-3px 0px -3px 0px; }");
    ui->progresslSlider->setEnabled(false);
    ui->volumnButton->setStyleSheet("QToolButton{border-image: url(:/images/volumn.png) } QToolButton:hover{border-image: url(:/images/volumn_hover.png)}");
    ui->upButton->setStyleSheet("QToolButton{border-image: url(:/images/up.png) } QToolButton:hover{border-image: url(:/images/up_hover.png)}");
    ui->downButton->setStyleSheet("QToolButton{border-image: url(:/images/down.png) } QToolButton:hover{border-image: url(:/images/down_hover.png)}");
    ui->stopButton->setStyleSheet("QToolButton{border-image: url(:/images/stop.png) } QToolButton:hover{border-image: url(:/images/stop_hover.png)}");

    QPixmap pix_circle,pix_singer,pix_title,pix_singlist;
    pix_circle.load(":/images/cp.png");
    pix_singer.load(":/images/singer.png");
    pix_title.load(":/images/title.png");
    pix_singlist.load(":/images/singlist.png");
    ui->label_circle->setPixmap(pix_circle);
    ui->label_singer->setPixmap(pix_singer);
    ui->label_title->setPixmap(pix_title);
    ui->label_singlist->setPixmap(pix_singlist);

    volumnSlider->setStyleSheet("QSlider::groove{ border:0px; width:4px; } QSlider::add-page{ background:gray; } QSlider::sub-page{ background:lightgray; } QSlider::handle{ background:rgb(13,127,217); height:10px; border-radius:5px; margin:0px -3px 0px -3px; }");

    //设置最小化、关闭按钮的信号和槽函数
    connect(minButton, SIGNAL(clicked()), this, SLOT(minButtonClicked()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(closeButtonClicked()));

    //设置计时器超时的信号和槽函数
    connect(timer, SIGNAL(timeout()), this, SLOT(myTimeout()));
    //设置导入文件按钮的信号和槽函数
    connect(ui->loadFileButton, SIGNAL(clicked()), this, SLOT(on_loadFileButton_clicked()));
    //设置播放暂停按钮的信号和槽函数
    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(on_playButton_clicked()));
    //设置上一曲按钮的信号和槽函数
    connect(ui->preButton, SIGNAL(clicked()), this, SLOT(on_preButton_clicked()));
    //设置下一曲按钮的信号和槽函数
    connect(ui->nextButton, SIGNAL(clicked()), this, SLOT(on_nextButton_clicked()));

    //实时处理音量滑动条改变的音量调节
    connect(volumnSlider, SIGNAL(valueChanged(int)), SLOT(on_volumnSlider_volumnChanged(int)));
    //设置列表中项目双击的信号和槽函数
    connect(ui->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(on_listWidget_itemDoubleClicked(QListWidgetItem *)));
    //设置模式选择按钮的信号和槽函数
    connect(ui->modeBotton, SIGNAL(clicked()), this, SLOT(on_modeBotton_clicked()));

    //设置音量 按钮的信号和槽函数
    connect(ui->volumnButton, SIGNAL(clicked()), this, SLOT(on_volumnButton_clicked()));
    connect(ui->upButton, SIGNAL(clicked()), this, SLOT(on_upButton_clicked()));
    connect(ui->downButton, SIGNAL(clicked()), this, SLOT(on_downButton_clicked()));
    connect(ui->stopButton, SIGNAL(clicked()), this, SLOT(on_stopButton_clicked()));

    //出现的渐变动画
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(400);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
 if(event->button() == Qt::LeftButton){
     if(event->pos().y()>TITLE_HEIGHT)//设置只能拖动“标题栏”来拖动窗口
         return;
      mouse_press = true;
      //鼠标相对于窗体的位置（或者使用event->globalPos() - this->pos()）
      move_point = event->pos();
 }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(mouse_press){
        QPoint move_pos = event->globalPos();
       this->move(move_pos - move_point);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    mouse_press = false;
}

void MainWindow::minButtonClicked()
{
    showMinimized();
}

void MainWindow::closeButtonClicked()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setDuration(300);
    animation->setStartValue(1);
    animation->setEndValue(0);
    animation->start();
    connect(animation, SIGNAL(finished()), this, SLOT(close()));
}

void MainWindow::myTimeout()
{
    if(Player.second == 0){
        ui->currentTimeLabel->setText("00:00");
        ui->totalTImeLabel->setText("00:00");
    }
    else if(Player.second <= current_second){
        on_stopButton_clicked();
    }
    else{
        int totalSeconds = Player.second;

        current_second += speedmode;
        int current_second_int = int(current_second);
        //设置当前时间
        // 计算总时间的分钟和秒钟
        int totalMinutes = totalSeconds / 60;
        int totalSecondsRemainder = totalSeconds % 60;

        // 设置进度条的值
        int progress = (current_second_int * 100) / totalSeconds;
        ui->progresslSlider->setValue(progress);

        // 计算当前时间的分钟和秒钟
        int currentMinutes = current_second_int / 60;
        int currentSeconds = current_second_int % 60;

        // 格式化当前时间
        QString currentTimeText = QString("%1:%2")
            .arg(currentMinutes, 2, 10, QLatin1Char('0'))
            .arg(currentSeconds, 2, 10, QLatin1Char('0'));
        ui->currentTimeLabel->setText(currentTimeText);

        // 格式化音乐总时间
        QString totalTimeText = QString("%1:%2")
            .arg(totalMinutes, 2, 10, QLatin1Char('0'))
            .arg(totalSecondsRemainder, 2, 10, QLatin1Char('0'));
        ui->totalTImeLabel->setText(totalTimeText);
    }
}

void MainWindow::on_loadFileButton_clicked()
{
    const QStringList filelist = QFileDialog::getOpenFileNames(this, tr("打开文件"), 0, tr("音乐文件(*.mp3 *.wav *.wma *.aac *.au *.aif);;All files (*.*)"));
    if(!filelist.empty()){
        for(int i=0;i<filelist.length();++i){
            QString file=filelist[i];
            MyQListWidgetItem * item=new  MyQListWidgetItem(file);
            ui->listWidget->addItem(item);
        }
    }
    else{
        QMessageBox message(QMessageBox::Warning, "No music", "Please select a music file.", QMessageBox::Yes);
        message.exec();
    }
}

void MainWindow::on_playButton_clicked()
{
    int index = ui->listWidget->currentIndex().row();
    if(index==-1&&playing==false)
    {
        QMessageBox message(QMessageBox::Warning, "No music selected", "Please select a music in the list.", QMessageBox::Yes);
        message.exec();
        return;
    }
    if(playing == false){

        if((ui->progresslSlider->value())!=0){ // pause before
            currentPlayingItem->removePlayingMark();
            currentPlayingItem->addPlayingMark();
            Player.song_continue();
            ui->playButton->setStyleSheet("QToolButton{border-image: url(:/images/pause.png) } QToolButton:hover{border-image: url(:/images/pause_clicked.png)} QToolButton:pressed{border-image: url(:/images/pause_hover.png)}");
            playing=true;
            timer->setInterval(1000);
            timer->start();
        }
        else{ // play a new song
            ui->playButton->setStyleSheet("QToolButton{border-image: url(:/images/pause.png) } QToolButton:hover{border-image: url(:/images/pause_clicked.png)} QToolButton:pressed{border-image: url(:/images/pause_hover.png)}");
            playing=true;
            MyQListWidgetItem* item=dynamic_cast<MyQListWidgetItem*>(ui->listWidget->currentItem());
            currentPlayingItem=item;
            for(int i=0;i<(ui->listWidget->count());++i)
            {
                MyQListWidgetItem * temp=dynamic_cast<MyQListWidgetItem *>(ui->listWidget->item(i));
                temp->removePlayingMark();
            }
            item->addPlayingMark();
            Player.song_stop();
            Player.second = 0;
            ui->currentTimeLabel->setText("00:00");
            ui->totalTImeLabel->setText("00:00");
            ThreadArguments* threadArgs = new ThreadArguments;
            threadArgs->Player = &Player;
            threadArgs->filepath = item->filepath;
            pthread_create(&play_thread, NULL, song_play_wrapper, threadArgs);
            current_second = 0;
            ui->progresslSlider->setValue(0);
            timer->setInterval(1000);
            timer->start();
        }
    }
    else{
        playing = false;
        Player.song_pause();
        timer->stop();
        currentPlayingItem->addPauseMark();
        ui->playButton->setStyleSheet("QToolButton{border-image: url(:/images/play.png) } QToolButton:hover{border-image: url(:/images/play_clicked.png)} QToolButton:pressed{border-image: url(:/images/play_hover.png)}");
    }
}

void* song_play_wrapper(void* arg) {
    ThreadArguments* args = static_cast<ThreadArguments*>(arg);
    player* Player = args->Player;
    QString filepath = args->filepath;
    Player->song_play(filepath);
    delete args;  // 注意要释放内存
    return NULL;
}

void MainWindow::on_preButton_clicked()
{
    Player.play_back();
    if(current_second<=10){
        current_second = 0;
    }
    else{
        current_second -= 11;
    }
}

void MainWindow::on_nextButton_clicked()
{
    Player.play_front();
    if(current_second >= Player.second-10){
        current_second = Player.second;
    }
    else{
        current_second += 9;
    }
}

void MainWindow::on_volumnSlider_volumnChanged(int)
{
    Player.set_volume(volumnSlider->value());
}

void MainWindow::on_listWidget_itemDoubleClicked(QListWidgetItem *importeditem)
{
    playing=false;
    int row=ui->listWidget->currentRow();
    ui->listWidget->setCurrentRow(row);
    ui->progresslSlider->setValue(0);
    on_playButton_clicked();
}

void MainWindow::on_modeBotton_clicked()
{
    QPoint position;
    position.setX(715-30);
    position.setY(375-92);
    modeMenu->exec(mapToGlobal(position));
}

void MainWindow::updateItem(QAction *action)
{
    if (action->text() == tr("0.5 speed")) {
        speedmode=0.5;
        ui->modeBotton->setStyleSheet("QToolButton{border-image: url(:/images/zeroandfive.png) } QToolButton:hover{border-image: url(:/images/zeroandfive_hover.png)} ");
    }
    else if (action->text() == tr("1.0 speed")) {
        speedmode=1;
        ui->modeBotton->setStyleSheet("QToolButton{border-image: url(:/images/one.png) } QToolButton:hover{border-image: url(:/images/one_hover.png)} ");
    }
    else if (action->text() == tr("1.5 speed")) {
        speedmode=1.5;
        ui->modeBotton->setStyleSheet("QToolButton{border-image: url(:/images/oneandfive.png) } QToolButton:hover{border-image: url(:/images/oneandfive_hover.png)} ");
    }
    else if (action->text() == tr("2.0 speed")) {
        speedmode=2;
        ui->modeBotton->setStyleSheet("QToolButton{border-image: url(:/images/two.png) } QToolButton:hover{border-image: url(:/images/two_hover.png)} ");
    }
    Player.set_speed(speedmode);
}

void MainWindow::on_volumnButton_clicked()
{
    QMenu * vol=new QMenu(this);
    vol->setStyleSheet("QMenu{ padding:5px; background:white; border:1px solid gray; } QMenu::item{ padding:0px 30px 0px 20px; height:25px; } QMenu::item:selected:enabled{ background:lightgray; color:white; } QMenu::item:selected:!enabled{ background:transparent; } QMenu::separator{ height:1px; background:lightgray; margin:5px 0px 5px 0px; }");
    QWidgetAction * waction=new QWidgetAction(this);
     waction->setDefaultWidget(volumnSlider);
     vol->addAction(waction);
     QPoint position;
     position.setX(710-43);
     position.setY(405-92-10);
     vol->exec(mapToGlobal(position));
}

void MainWindow::on_upButton_clicked()
{
    int index=ui->listWidget->currentRow();
    int count=ui->listWidget->count();
    if(index==0)
        index=count-1;
    else
         index=index-1;
    ui->listWidget->setCurrentRow(index);
    ui->progresslSlider->setValue(0);
    playing=false;
    on_playButton_clicked();
}

void MainWindow::on_downButton_clicked()
{
    int index=ui->listWidget->currentRow();
    int count=ui->listWidget->count();
    index=(index+1)%count;
    ui->listWidget->setCurrentRow(index);
    ui->progresslSlider->setValue(0);
    playing=false;
    on_playButton_clicked();
}

void MainWindow::on_stopButton_clicked()
{
    timer->stop();
    playing=false;
    Player.song_stop();
    ui->playButton->setStyleSheet("QToolButton{border-image: url(:/images/play.png) } QToolButton:hover{border-image: url(:/images/play_clicked.png)} QToolButton:pressed{border-image: url(:/images/play_hover.png)}");
    ui->progresslSlider->setValue(0);
    ui->currentTimeLabel->setText("00:00");
    ui->totalTImeLabel->setText("00:00");
    for(int i=0;i<(ui->listWidget->count());++i)
    {
        MyQListWidgetItem * temp=dynamic_cast<MyQListWidgetItem *>(ui->listWidget->item(i));//row=index+1???????
        temp->removePlayingMark();
    }
}



#include <QtGui>
#include "simwidget.h"
#include "Platform.h"

int UpdateLCD(u8* bits);
extern int _mpress;
extern int _mx;
extern int _my;

QElapsedTimer _systimer;

QWidget* _dst = 0;
int _scroll = 0;
QImage _lcd(240,320,QImage::Format_ARGB32);

void Hardware_::SetBacklight(u8,int)
{
}


u32 Hardware_::GetTicks()
{
    return _systimer.elapsed();
}

class MyThread : public QThread
 {
 public:
     void run()
     {
         _systimer.start();
         UpdateLCD(_lcd.bits());
         Hardware.Init();
         Shell_Init();

         while (_dst)
         {
             Shell_Loop();
             int scroll = UpdateLCD(_lcd.bits());
             if (scroll != -1)
             {
                 _scroll = scroll;
                 _dst->update();
             }
             fflush(stdout);
             this->usleep(10*1000);
         }
     }
};


MyThread _thread;

SimWidget::SimWidget(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
}

class SleeperThread : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};

SimWidget::~SimWidget()
{
    _dst = 0;
    while (_thread.isRunning())
        SleeperThread::msleep(1);
}

void SimWidget::paintEvent(QPaintEvent *)
{
    if (_dst == 0)
    {
        _dst = this;
        _thread.start();
    }
    while (_scroll >= 320)
        _scroll -= 320;
    while (_scroll < 0)
        _scroll += 320;

    int a = 320-_scroll;
    int b = _scroll;

    QRectF srca(0,b,240,a);
    QRectF srcb(0,0,240,b);

    QRectF dsta(0,0,240,a);
    QRectF dstb(0,a,240,b);

    QPainter painter(this);

    painter.drawImage(dsta,_lcd,srca);
    painter.drawImage(dstb,_lcd,srcb);
}

void SimWidget::mouseMoveEvent ( QMouseEvent * event )
{
    if (_mpress)
    {
        _mx = event->x();
        _my = event->y();
    }
}

void SimWidget::mousePressEvent ( QMouseEvent * event )
{
    _mpress = 128;
    _mx = event->x();
    _my = event->y();
}

void SimWidget::mouseReleaseEvent ( QMouseEvent * event )
{
    _mpress = 0;
    _mx = event->x();
    _my = event->y();
}

void SimWidget::timer()
{
}

#ifndef SIMWIDGET_H
#define SIMWIDGET_H

#include <QWidget>

class SimWidget : public QWidget
{
    Q_OBJECT
    bool _tracking;
public:
    explicit SimWidget(QWidget *parent = 0);
    ~SimWidget();

    void paintEvent(QPaintEvent *);
    virtual void	mouseMoveEvent ( QMouseEvent * event );
    virtual void	mousePressEvent ( QMouseEvent * event );
    virtual void	mouseReleaseEvent ( QMouseEvent * event );
    signals:

public slots:
    void timer();
};

#endif // SIMWIDGET_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "common.h"
#include <QMainWindow>
#include <QXmlStreamReader>

namespace Ui {
    class MainWindow;
}

class EpubGrinder;
class SimWidget;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void grind(const string& path);

public slots:
    void open();
    void fonts();
    void bookshelf();

private:
    SimWidget* _sim;
    EpubGrinder* _grinder;
    Ui::MainWindow *ui;
    QXmlStreamReader xml;
};

#endif // MAINWINDOW_H

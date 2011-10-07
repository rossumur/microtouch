#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui>

#include <stdlib.h>
#include <stdio.h>

#include "epubgrinder.h"
#include "simwidget.h"
#include "fonteditor.h"
#include "blobfile.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(open()));
    connect(ui->actionFont,SIGNAL(triggered()),this,SLOT(fonts()));
    connect(ui->actionBundle_files_into_bookshelf_bks_file,SIGNAL(triggered()),this,SLOT(bookshelf()));
    _grinder = 0;

    _sim = new SimWidget(0);
    setCentralWidget(_sim);

  //  QTimer *timer = new QTimer(this);
  //  connect(timer, SIGNAL(timeout()), _sim, SLOT(timer()));
  //  timer->start(100);
}

MainWindow::~MainWindow()
{
    delete _grinder;
    delete ui;
}

// TODO: 8.3 filenames may no be unique - files may alias on top of one another..
void to8(string& name)
{
    if (name.size() > 8)
        name.resize(8);
    for (int i = 0; i < name[i]; i++)
    {
        name[i] = toupper(name[i]);
        if (name[i] == ' ')
            name[i] = '_';
    }
}

void MainWindow::grind(const string& path)
{
    int slash = path.find_last_of('/');
    if (slash == -1)
        slash = path.find_last_of('\\');
    string name = path.substr(slash+1);
    name = name.substr(0,name.find_last_of('.'));
    to8(name);

    string out = name + ".EPB";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save epb book as"),
                                QString(out.c_str()),
                                tr("Book (*.EPB)"));
    if (!fileName.length())
        return;

    EpubGrinder grinder;
    grinder.grind(path,fileName.toStdString());
}

void MainWindow::fonts()
{
    fonteditor* f = new fonteditor();
    f->open();
}

void MainWindow::open()
{
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("Open File"),
                                                     "",
                                                     tr("EPUB (*.epub)"));
    for (int i = 0; i < files.size(); i++)
        grind(files[i].toStdString());
}

void MainWindow::bookshelf()
{
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("Open files to bundle into bookshelf"),
                                                     "",
                                                     tr("EPB (*.epb)"));
    if (files.empty())
        return;

    QString name = "BOOKS.BKS";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save bookshelf as"),
                                name,
                                tr("Bookshelf files (*.BKS)"));
    if (!fileName.length())
        return;

    BlobFile bookshelf;
    BlobFile data;
    BlobFile names;
    for (int i = 0; i < files.size(); i++)
    {
        ByteArray file;
        string s = files[i].toStdString();
        file.load(s.c_str());
        data.add(file);

        int n = s.find_last_of('/');
        if (n == -1)
            n = 0;
        else
            n++;
        ByteArray name;
        for (int j = n; j < s.length(); j++)
            name.write(s[j]);
        names.add(name);
    }
    bookshelf.add(data);
    bookshelf.add(names);
    bookshelf.toFile(fileName.toStdString().c_str(),'I',2001);
}

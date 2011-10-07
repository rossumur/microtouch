#ifndef FONTEDITOR_H
#define FONTEDITOR_H

#include <QDialog>
#include <vector>
#include <map>
using namespace std;

namespace Ui {
    class fonteditor;
}

class BPix
{
    int width;
    int height;
    vector<unsigned short> pix; // 16 bit RGB
};

class BChar
{
public:
    int code;
    int top;
    int left;
    int bottom;
    int right;
    BPix pix;
    QImage img;
};

class BFont {
public:
    QString name;
    int ascent;
    int descent;
    int lineHeight;
    vector<BChar> chars;
};

class fonteditor : public QDialog
{
    Q_OBJECT
    QFont _font;
    BFont _bfont;

public:
    explicit fonteditor(QWidget *parent = 0);
    ~fonteditor();

    void refresh();

public slots:
    void getfont();

private:
    Ui::fonteditor *ui;
};

#endif // FONTEDITOR_H

#include "fonteditor.h"
#include "ui_fonteditor.h"
#include <QtGui>

fonteditor::fonteditor(QWidget *parent) :
    QDialog(parent),
    _font("Helvetica",10),
    ui(new Ui::fonteditor)
{
    ui->setupUi(this);
    connect(ui->pushButton,SIGNAL(clicked()),this,SLOT(getfont()));
}

fonteditor::~fonteditor()
{
    delete ui;
}

void fonteditor::getfont()
{
    bool ok;
    QFont font = QFontDialog::getFont(
        &ok, _font, this);
    if (ok) {
        _font = font;
        refresh();
    }
}

typedef unsigned short u16;
typedef unsigned char u8;

//  RGB to 4 bit color index
//  Turns out 4 bit luminance is your friend...
u8 toColor(const u8* p)
{
    int r = p[2];
    int g = p[1];
    int b = p[0];
    int y = 0.29*r + 0.59*g + (1-0.29-0.59)*b;
    return y >> 4;
}

bool isBlankLine(QImage& img, int y)
{
    const uchar* bits = img.bits();
    bits += y*img.bytesPerLine();
    for (int i =0; i < img.width(); i++)
    {
        if (toColor(bits + (i << 2)) != 0xF)
            return false;
    }
    return true;
}

bool isBlankCol(QImage& img, int x)
{
    const uchar* bits = img.bits();
    bits += x*4;
    for (int i =0; i < img.height(); i++)
    {
        if (toColor(bits) != 0xF)
            return false;
        bits += img.bytesPerLine();
    }
    return true;
}

void Shrink(QImage& img, int& top, int& bottom, int& right)
{
    for (;top < bottom; top++)
        if (!isBlankLine(img,top))
            break;
    for (; bottom > top; bottom--)
        if(!isBlankLine(img,bottom-1))
            break;
    for (; right > 0; right--)
        if(!isBlankCol(img,right-1))
            break;
}

#define TOCOLOR(_r,_g,_b) ((((_r) & 0xF8) << 8) | (((_g) & 0xFC) << 3) | (((_b) & 0xF8) >> 3))

//  Generate a Font File...
//  Fonts are 3:3:2 rgb
class FontBuilder
{
    vector<u8> _data;

    //  Font format
    typedef struct
    {
        u8 sig;
        u8 firstChar;
        u8 lastChar;
        u8 height;
        u8 ascent;
        u8 descent;
        u8 flags;
        u8 r0;
    } BFontHdr;

    typedef struct
    {
        u8 width;
        u8 topOffset;
        u16 pos;
    } BCharRec;

    map<int,int> _used;
public:
    int write(vector<BChar>& chars, int ascent, int descent, int lineHeight)
    {
        BFontHdr hdr;
        hdr.sig = 'F';
        hdr.firstChar = chars[0].code;
        hdr.lastChar = chars[chars.size()-1].code;
        hdr.ascent = ascent;
        hdr.descent = descent;
        hdr.height = lineHeight;
        hdr.flags = 0;
        hdr.r0 = 0;

        int hdrSize = 8 + 4*(chars.size()+ 1);
        _data.resize(hdrSize);
        memcpy(&_data[0],&hdr,8);

        BCharRec* cr;
        for (int i = 0; i < chars.size(); i++)
        {
            cr = (BCharRec*)(&_data[8] + i*4);
            cr->pos = _data.size();
            cr->width = chars[i].right-chars[i].left;
            u8 odd = ((chars[i].bottom - chars[i].top)*cr->width) & 1;
            cr->topOffset = (chars[i].top << 1) | odd;   // Odd number of pixels in char
            writeChar(chars[i]);
        }
        cr = (BCharRec*)(&_data[8] + chars.size()*4);
        cr->pos = _data.size();
        cr->width = 0;
        cr->topOffset = 0;

        return _data.size();
    }

    void writeChar(BChar& c)
    {
        vector<u8> pix;
        const u8* bits = c.img.bits();
        int rowbytes = c.img.bytesPerLine();
        for (int y = c.top; y < c.bottom; y++)
        {
            const uchar* b = bits + y*rowbytes;
            for (int i = c.left; i < c.right; i++)
                pix.push_back(toColor(b + (i << 2)));
        }

        // Add Compression here....

        // Write completed char
        if (pix.size() & 1)
            pix.push_back(0xF);  // align to 2, this pixel won't get shown....

        // Need to rethink clipping
        for (int i = 0; i < pix.size(); i += 2)
        {
            int p = (pix[i] << 4) | pix[i+1];
            _data.push_back(p);
        }
    }

    static int blackPalette(int c)
    {
        c = (c << 4) | c;
        return TOCOLOR(c,c,c);
    }

    static int greenPalette(int c)
    {
        c = (c << 4) | c;
        return TOCOLOR(c,0xFF,c);
    }

    static int redPalette(int c)
    {
        c = (c << 4) | c;
        return TOCOLOR(0xFF,c,c);
    }

    static int bluePalette(int c)
    {
        c = (c << 4) | c;
        return TOCOLOR(c,c,0xFF);
    }

    static int bluePaletteInverted(int c)
    {
        c = (c << 4) | c;
        c = 0xFF-c;
        return TOCOLOR(c,c,0xFF);
    }

    static int greyPalette(int c)
    {
        c = (c << 4) | c;
        c = 0x80 + c/2;
        return TOCOLOR(c,c,c);
    }

    void toFile(const char* path)
    {
        FILE* f = fopen(path,"wb");
        u8* d= &_data[0];
        fwrite(d,_data.size(),1,f);
        fclose(f);

        f = fopen((QString(path) + ".h").toUtf8(),"wb");
        dump(f,"defaultFont",&_data[0],_data.size());
/*
        dumpClut(f,"blackPalette",blackPalette);
        dumpClut(f,"redPalette",redPalette);
        dumpClut(f,"greenPalette",greenPalette);
        dumpClut(f,"bluePalette",bluePalette);
        dumpClut(f,"bluePaletteInverted",bluePaletteInverted);
        dumpClut(f,"greyPalette",greyPalette);
*/
        fclose(f);
    }

    void dumpClut(FILE* f,const char* name, int (*ToColor)(int index))
    {
        vector<u8> clut;
        for (int c = 0; c < 16; c++)
        {
            u16 color = ToColor(c);
            clut.push_back(color >> 8);
            clut.push_back(color);
        }
        dump(f,name,&clut[0],clut.size());
    }

    const u8* data()
    {
        return &_data[0];
    }

    void dump(FILE* f,const string& name, const uchar* data, int len)
    {
        fprintf(f,"extern const u8 %s[%d] PROGMEM;\n",name.c_str(),len);
        fprintf(f,"const u8 %s[%d] PROGMEM = \n{",name.c_str(),len);
        for (int i = 0; i < len; i++)
        {
            if  ((i & 0xF) == 0)
                fprintf(f,"\n");
            fprintf(f,"0x%02X,",data[i]);
        }
        fprintf(f,"\n};\n");
    }
};

u8 M(const u8* f)
{
    return f[0];
}

u16 MW(const u8* f)
{
    return ((u16*)f)[0];
}

static u8 DrawChar(const u8* font, char c, short dx, short dy, QPainter& painter)
{
    u8 width,height,top;
    short src,end,i;

    height = M(font+3);
    i = c - M(font+1);
    i = (i + 2) << 2;
    width = M(font+i);
    top = M(font+i+1);
    src = MW(font+i+2); // SRC and END <<1 on Disk based fonts...
    end = MW(font+i+6);

    u8 odd = top & 1;
    top >>= 1;
    dy += top;
    height -= top;

    if (height == 0 || end <= src || dx+width > 240 || dy+height > 320)
        return width;

    int x = 0;
    int y = 0;
    int n  = end-src;
    const u8 *p = font + src;

    u8 flag = width*height & 1;
    while (n--)
    {
        int r,g,b;
        u8 c2 = *p++;
        u8 c = c2 >> 4;

        r = g = b = (c << 4) | c;
        painter.fillRect(dx+x,dy+y,1,1,QColor(r,g,b));
        x++;
        if (x == width)
        {
            x = 0;
            y++;
        }

        if (n == 0 && flag)
            break;
        c = c2 & 0x0F;
        r = g = b = (c << 4) | c;
        painter.fillRect(dx+x,dy+y,1,1,QColor(r,g,b));
        x++;
        if (x == width)
        {
            x = 0;
            y++;
        }
    }
    return width;
}

static int DrawString(const u8* font,const char* s, int x, int y,QPainter& painter)
{
    while (*s)
        x += DrawChar(font,*s++,x,y,painter);
    return x;
}

void fonteditor::refresh()
{
    QFontMetrics fm(_font);

    // Render
    vector<BChar> chars;
    QPainter cpaint;
    QPixmap cpix(64,64);
    cpaint.begin(&cpix);
    cpaint.setFont(_font);
    cpaint.setPen(Qt::black);
    int ascent = fm.ascent();
    int lineHeight = fm.lineSpacing()+1;
    int spacewidth = fm.boundingRect(QChar('i')).right();    // SPACE

    for (int c = 32; c < 128; c++)
    {
        QRect r = fm.boundingRect(QChar(c));
#ifdef __WIN32
        r.adjust(0,0,4,0);  // sigh. font metrics in windows dont work.
#endif
        cpaint.fillRect(0,0,64,64,Qt::white);
        cpaint.drawText(0,ascent,QString(QChar(c)));

        BChar b;
        b.code = c;
        b.top = 0;
        b.bottom = lineHeight;
        b.left = 0;
        b.right = r.right();
        b.img = cpix.toImage();
        Shrink(b.img,b.top,b.bottom,b.right);
        if (c == 32)
            b.right = spacewidth;    // SPACE
        chars.push_back(b);
    }
    cpaint.end();

    //
    FontBuilder builder;
    int len = builder.write(chars,ascent,fm.descent(),lineHeight);
    ui->label_2->setText("Font is " + QString::number(len) + " bytes.");

    //  Display?
    vector<QString> test;
    test.push_back("The quick brown fox jumps over the lazy dog");
    test.push_back("THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG");
#ifdef __WIN32
    test.push_back("Fonts will look better...");
    test.push_back("..if you make them in linux");
#endif
    test.push_back("Made from " + _font.toString());

    QPainter paint;
    QPixmap pic(512,120);
    paint.begin(&pic);
    paint.fillRect(0,0,512,120,Qt::white);

    int y = 10;
    for (int i = 0; i < test.size(); i++)
    {
        DrawString(builder.data(),test[i].toUtf8(),10,y,paint);
        y += lineHeight;
    }
    paint.end();
    ui->label->setPixmap(pic);


    // Save
    QStringList s = _font.toString().split(',');
    QString name = s[0]+s[1]+".ctf";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Font"),
                                name,
                                tr("Fonts (*.ctf)"));
    if (fileName.length())
        builder.toFile(fileName.toUtf8());
}

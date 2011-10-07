#ifndef EPUBGRINDER_H
#define EPUBGRINDER_H

#include "common.h"
#include "zipfile.h"

string toString(wstring w);
wstring toWString(string s);
string toString(int i);

class Sax
{
public:
    virtual ~Sax();
    virtual void startDocument();
    virtual bool endDocument();
    virtual void start(const string& tag, std::map<string,string>& attr);
    virtual void end(const string& tag);
    virtual void text(const wstring& text);
    virtual void whitespace(const wstring& w);
};

class EpubGrinder
{
    FILE* _file;
    vector<ZipFileInfo> _files;

public:
    EpubGrinder();
    virtual ~EpubGrinder();
    void grind(const string& path, const string& out);

private:
    u8* readAll(const string& name, int& len);
    bool loadXml(const string& path, Sax& b);
    bool parseChapter(int index);
};

bool applyXml(const char* data, int len, Sax& b);
bool applyXml(const wstring& xml, Sax &b);

#endif // EPUBGRINDER_H

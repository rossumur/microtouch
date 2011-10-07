#ifndef BUILDER_H
#define BUILDER_H

#include "epubgrinder.h"
#include "blobfile.h"
#include "layout.h"

#include <QtGui>

class ManifestItem
{
public:
    string href;
    string mediaType;
};

class SpineItem
{
public:
    string idref;
    string linear;
};

class NavpointItem
{
public:
    wstring text;
    string src;
    int depth;
};

class Builder : public Sax
{
    BlobFile _root;
    BlobFile _navPoints;
    Layout _layout;

public:
    string Path;
    map<string,NavpointItem> Navpoints;
    map<string,ManifestItem> Manifest;
    vector<SpineItem> Spine;
    map<string,wstring> Metadata;

    Builder();
    string pathById(const string& id);

    void writeBadge();
    void writeToc();
    void writeGlyphs();
    void writeMeta();

    void startBook();
    void endBook(const string& dst);
    void startNavpoint(const NavpointItem& item);

    void addImage(const string& path, const u8* data, int len);

    virtual void startDocument();
    virtual bool endDocument();
    virtual void start(const string& tag, map<string,string>& attr);
    virtual void end(const string& tag);
    virtual void text(const wstring& text);
    virtual void whitespace(const wstring& w);
};

#endif // BUILDER_H

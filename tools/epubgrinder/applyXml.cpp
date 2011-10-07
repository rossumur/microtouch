
#include <QtGui>
#include "epubgrinder.h"

wstring toUtf8(const QStringRef& s)
{
    return s.toString().toStdWString();
}

bool applyXml(const wstring &xml, Sax &b)
{
    string utf8 = toString(xml);    // TOOD
    const char* data = utf8.c_str();
    int len = utf8.size();
    applyXml(data,len,b);
}

bool applyXml(const char* data, int len, Sax& b)
{
    QXmlStreamReader xml(QByteArray(data,len));
    b.startDocument();
    while (!xml.atEnd())
    {
        xml.readNext();
        if (xml.isStartElement())
        {
            map<string,string> attr;
            for (int i = 0; i < xml.attributes().count(); i++)
            {
                QXmlStreamAttribute a = xml.attributes().at(i);
                attr[a.qualifiedName().toString().toStdString()] = a.value().toString().toStdString();
            }
            b.start(xml.qualifiedName().toString().toStdString(),attr);
        }
        else if (xml.isEndElement())
        {
            b.end(xml.qualifiedName().toString().toStdString());
        }
        else if (xml.isCharacters())
        {
            b.text(toUtf8(xml.text()));
        }
        else if (xml.isWhitespace())
        {
            b.whitespace(toUtf8(xml.text()));
        }
        else if (xml.isEntityReference())
        {
            QString name = xml.name().toString();
            wstring e;
            if (name == "nbsp")
                e.push_back(0xA0);
            else
                printf("Unhandled entity\n");
            b.text(e);
        }
    }

    if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError)
    {
        qWarning() << "XML ERROR: line " << xml.lineNumber() << "col" <<  xml.columnNumber() << " " << xml.errorString();
    }
    return b.endDocument();
}


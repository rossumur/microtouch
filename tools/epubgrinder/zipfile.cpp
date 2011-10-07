#include "zipfile.h"
#include <zlib.h>

bool ZipInfo(FILE* file, vector<ZipFileInfo>& files)
{
    files.clear();
    fseek(file,-sizeof(DirectoryHeader),SEEK_END);
    u32 mark = ftell(file);
    DirectoryHeader header;
    fread(&header,sizeof(DirectoryHeader),1,file);
    if (header.sig != DIRECTORY_HEADER && header.directorySize >= mark)
        return false;

    fseek(file,mark-header.directorySize,SEEK_SET);

    char name[1024];
    for (int i = 0; i < header.nDirectoryEntries; i++)
    {
        ZipFileInfo f;
        fread(&f.info,sizeof(DirectoryFileHeader),1,file);
        if (f.info.sig != DIRECTORY_FILE_HEADER)
            return false;
        int n = sizeof(name)-1;
        if (n > f.info.fileNameLen)
            n = f.info.fileNameLen;
        fread(name,n,1,file);
        name[n] = 0;
        f.name = name;
        if (n < f.info.fileNameLen)
        {
            // BAD JUJU truncating name TODO
            fseek(file,f.info.fileNameLen-n,SEEK_CUR);
        }
        printf("%s\n",name);
        fseek(file,f.info.extraLen + f.info.commentLen,SEEK_CUR);
        files.push_back(f);
    }
    return true;
}

u8* ZipDecompress(FILE* file, const ZipFileInfo& to)
{
    LocalFileHeader fh;
    fseek(file,to.info.headerOffset,SEEK_SET);
    fread(&fh,sizeof(LocalFileHeader),1,file);
    if (fh.sig != LOCAL_FILE_HEADER)
        return 0;
    fseek(file,fh.fileNameLen + fh.extraLen,SEEK_CUR);

    u8* src = new u8[fh.compressedSize];
    fread(src,fh.compressedSize,1,file);

    if (to.info.compression == 0)
        return src;

    u8* dst = new u8[fh.uncompressedSize];

    z_stream stream = {0};
    stream.next_in = src;
    stream.avail_in = fh.compressedSize;
    stream.next_out = dst;
    stream.avail_out = fh.uncompressedSize;

    int err = inflateInit2(&stream, -MAX_WBITS);
    if (err == Z_OK)
    {
        err = inflate(&stream, Z_FINISH);
        inflateEnd(&stream);
        if (err == Z_STREAM_END)
            err = Z_OK;
        inflateEnd(&stream);
    }
    if (err != Z_OK)
    {
        delete [] dst;
        dst = 0;
        printf("Error inflating %s\n",to.name.c_str());
    }
    delete [] src;
    return dst;
}

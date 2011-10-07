#ifndef BLOBFILE_H
#define BLOBFILE_H

#include "common.h"

class ByteArray : public vector<u8>
{
public:
    void write(u8 b);
    void write32(u32 n);
    void write(const ByteArray& data);
    bool load(const char* path);
    void set(void* d, int len);
};

class BlobFile
{
    vector<ByteArray> _blobs;
public:
    BlobFile();

    int count();
    void clear();
    int payloadSize();
    void add(BlobFile& blob);
    void add(ByteArray& data);
    void toArray(ByteArray& dst, u8 = 'I', u32 id = 0);
    void toFile(const char* path, u8 = 'I', u32 id = 0);
};

#endif // BLOBFILE_H

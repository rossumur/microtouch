#include "blobfile.h"
#include "string.h"

void ByteArray::write(u8 b)
{
    push_back(b);
}

void ByteArray::write32(u32 n)
{
    for (int i = 0; i < 4; i++)
    {
        write(n);
        n >>= 8;
    }
}

void ByteArray::write(const ByteArray &data)
{
    for (int i = 0; i < data.size(); i++)
        push_back(data[i]);
}

void ByteArray::set(void* d, int len)
{
    resize(len);
    vector<u8>& v = *this;
    memcpy(&v[0],d,len);
}

bool ByteArray::load(const char* path)
{
    clear();
    FILE* f = fopen(path,"rb");
    if (!f)
    {
        printf("ByteArray::load failed for %s\n",path);
        return false;
    }
    fseek(f,0,SEEK_END);
    int len = ftell(f);
    fseek(f,0,SEEK_SET);
    resize(len);
    vector<u8>& v = *this;
    fread(&v[0],len,1,f);
    fclose(f);
    return true;
}

//  General format of a Blob file
//  3 - 'BLB'
//  1 - Type ('S' - strings, 'K' - key/value, 'I' - raw data)
//  4 - ID
//  4 - Count
//  (count+1)*4 - Offsets of child blobs
//  Data
//  Blobs are padded to 4 byte boundaries

BlobFile::BlobFile()
{
}

int BlobFile::count()
{
    return _blobs.size();
}

int BlobFile::payloadSize()
{
    u32 offset = 4 + 4 + 4 + (_blobs.size() + 1)* 4;
    for (int i = 0; i < _blobs.size(); i++)
        offset += _blobs[i].size();
    return offset;
}

void BlobFile::clear()
{
    _blobs.clear();
}

void BlobFile::add(ByteArray& data)
{
    _blobs.push_back(data);
}

void BlobFile::add(BlobFile &blob)
{
    ByteArray data;
    blob.toArray(data);
    add(data);
}

void BlobFile::toArray(ByteArray &dst, u8 type, u32 id)
{
    dst.write('B');
    dst.write('L');
    dst.write('B');
    dst.write(type);
    dst.write32(id);
    dst.write32(_blobs.size());

    u32 offset = 4 + 4 + 4 + (_blobs.size() + 1)* 4;   // Start of data
    for (int i = 0; i < _blobs.size(); i++)
    {
        dst.write32(offset);
        offset += _blobs[i].size();
    }
    dst.write32(offset);
    for (int i = 0; i < _blobs.size(); i++)
        dst.write(_blobs[i]);
}

void BlobFile::toFile(const char* path, u8 type, u32 id)
{
    ByteArray dst;
    dst.write('B');
    dst.write('L');
    dst.write('B');
    dst.write(type);
    dst.write32(id);
    dst.write32(_blobs.size());

    u32 offset = 4 + 4 + 4 + (_blobs.size() + 1)* 4;   // Start of data
    for (int i = 0; i < _blobs.size(); i++)
    {
        dst.write32(offset);
        offset += _blobs[i].size();
    }
    dst.write32(offset);

    FILE* f = fopen(path,"wb");
    fwrite(&dst[0],dst.size(),1,f);
    for (int i = 0; i < _blobs.size(); i++)
    {
        ByteArray b = _blobs[i];
        fwrite(&b[0],b.size(),1,f);
    }
    fclose(f);
}

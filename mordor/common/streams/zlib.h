#ifndef __ZLIB_STREAM_H__
#define __ZLIB_STREAM_H__
// Copyright (c) 2009 - Decho Corp.

#include <zlib.h>

#include "filter.h"

struct ZlibException : virtual ExceptionBase
{
public:
    ZlibException() : m_rc(0) {}
    ZlibException(int rc) :
      m_rc(rc)
    {}
    
    int rc() const { return m_rc; }

private:
    int m_rc;
};

struct NeedPresetDictionaryException : virtual ZlibException
{
public:
    NeedPresetDictionaryException() : ZlibException(Z_NEED_DICT)
    {}
};

struct CorruptedZlibStreamException : public ZlibException
{
public:
    CorruptedZlibStreamException() : ZlibException(Z_DATA_ERROR)
    {}
};

class ZlibStream : public MutatingFilterStream
{
public:
    enum Strategy {
        DEFAULT = Z_DEFAULT_STRATEGY,
        FILTERED = Z_FILTERED,
        HUFFMAN = Z_HUFFMAN_ONLY,
        FIXED = Z_FIXED,
        RLE = Z_RLE
    };

protected:
    enum Type {
        ZLIB,
        DEFLATE,
        GZIP
    };

    ZlibStream(Stream::ptr parent, bool own, Type type, int level = Z_DEFAULT_COMPRESSION,
        int windowBits = 15, int memlevel = 8, Strategy strategy = DEFAULT);

private:
    void init(Type type, int level = Z_DEFAULT_COMPRESSION, int windowBits = 15,
        int memlevel = 8, Strategy strategy = DEFAULT);

public:
    ZlibStream(Stream::ptr parent, int level, int windowBits, int memlevel, Strategy strategy,
        bool own = true);
    ZlibStream(Stream::ptr parent, bool own = true);
    ~ZlibStream();

    bool supportsSeek() const { return false; }
    bool supportsSize() const { return false; }
    bool supportsTruncate() const { return false; }

    void close(CloseType type = BOTH);
    size_t read(Buffer &b, size_t len);
    size_t write(const Buffer &b, size_t len);
    void flush();

private:
    void flush(int flush);
    void flushBuffer();

private:
    static const size_t m_bufferSize = 64 * 1024;
    Buffer m_inBuffer, m_outBuffer;
    z_stream m_strm;
    bool m_closed;
};

#endif

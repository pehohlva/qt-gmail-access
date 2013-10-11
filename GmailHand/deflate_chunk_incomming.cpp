/* 
 * File:   deflate_chunk_incomming.cpp
 * Author: pro
 * 
 * Created on 30. September 2013, 10:29
 */

#include "deflate_chunk_incomming.h"

namespace ChunkDeflate {

#ifdef DEFLATE_PLAY_YES

    Rfc1951Compressor::Rfc1951Compressor(int chunkSize) {
        _chunkSize = chunkSize;
        _buffer = new char[chunkSize];

        /* allocate deflate state */
        _zStream.zalloc = Z_NULL;
        _zStream.zfree = Z_NULL;
        _zStream.opaque = Z_NULL;

        bool ok(deflateInit2(&_zStream,
                Z_DEFAULT_COMPRESSION,
                Z_DEFLATED,
                -(MAX_WBITS - 2), // 32KB // MAX_WBITS == 15 (zconf.h) MEM128KB
                MAX_MEM_LEVEL - 2, // 64KB // MAX_MEM_LEVEL = 9 (zconf.h) MEM256KB
                Z_DEFAULT_STRATEGY) == Z_OK);
        Q_ASSERT(ok);
    }

    Rfc1951Compressor::~Rfc1951Compressor() {
        delete[] _buffer;
        deflateEnd(&_zStream);
    }

    bool Rfc1951Compressor::write(QIODevice *out, QByteArray *in) {
        _zStream.next_in = reinterpret_cast<Bytef*> (in->data());
        _zStream.avail_in = in->size();

        do {
            _zStream.next_out = reinterpret_cast<Bytef*> (_buffer);
            _zStream.avail_out = _chunkSize;
            int result = deflate(&_zStream, Z_SYNC_FLUSH);
            if (result != Z_OK &&
                    result != Z_STREAM_END &&
                    result != Z_BUF_ERROR) {
                return false;
            }
            out->write(_buffer, _chunkSize - _zStream.avail_out);
        } while (!_zStream.avail_out);
        return true;
    }

    Rfc1951Decompressor::Rfc1951Decompressor(int chunkSize) {
        _chunkSize = chunkSize;
        _stagingBuffer = new char[_chunkSize];

        /* allocate inflate state */
        _zStream.zalloc = Z_NULL;
        _zStream.zfree = Z_NULL;
        _zStream.opaque = Z_NULL;
        _zStream.avail_in = 0;
        _zStream.next_in = Z_NULL;
        bool ok(inflateInit2(&_zStream, -MAX_WBITS) == Z_OK);
        Q_ASSERT(ok);
    }

    Rfc1951Decompressor::~Rfc1951Decompressor() {
        inflateEnd(&_zStream);
        delete[] _stagingBuffer;
    }

    bool Rfc1951Decompressor::consume(QIODevice *in) {
        while (in->bytesAvailable()) {
            _inBuffer = in->read(_chunkSize);
            _zStream.next_in = reinterpret_cast<Bytef*> (_inBuffer.data());
            _zStream.avail_in = _inBuffer.size();
            do {
                _zStream.next_out = reinterpret_cast<Bytef *> (_stagingBuffer);
                _zStream.avail_out = _chunkSize;
                int result = inflate(&_zStream, Z_SYNC_FLUSH);
                if (result != Z_OK &&
                        result != Z_STREAM_END &&
                        result != Z_BUF_ERROR) {
                    return false;
                }
                _output.append(_stagingBuffer, _chunkSize - _zStream.avail_out);
            } while (_zStream.avail_out == 0);
        }
        return true;
    }

    bool Rfc1951Decompressor::canReadLine() const {
        return _output.contains('\n');
    }

    QByteArray Rfc1951Decompressor::readLine() {
        int eolPos = _output.indexOf('\n');
        if (eolPos == -1) {
            return QByteArray();
        }

        QByteArray result = _output.left(eolPos + 1);
        _output = _output.mid(eolPos + 1);
        return result;
    }

    QByteArray Rfc1951Decompressor::read(qint64 maxSize) {
        QByteArray res = _output.left(maxSize);
        _output = _output.mid(maxSize);
        return res;
    }

#endif

}



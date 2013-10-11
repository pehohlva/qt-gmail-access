/* 
 * File:   deflate_chunk_incomming.h
 * Author: pro
 *
 * Created on 30. September 2013, 10:29
 */

#ifndef DEFLATE_CHUNK_INCOMMING_H
#define	DEFLATE_CHUNK_INCOMMING_H

#include <QIODevice>

#include <zlib.h>

namespace ChunkDeflate {

    /* From RFC4978 The IMAP COMPRESS:   
       "When using the zlib library (see [RFC1951]), the functions
       deflateInit2(), deflate(), inflateInit2(), and inflate() suffice to
       implement this extension.  The windowBits value must be in the range
       -8 to -15, or else deflateInit2() uses the wrong format.
       deflateParams() can be used to improve compression rate and resource
       use.  The Z_FULL_FLUSH argument to deflate() can be used to clear the
       dictionary (the receiving peer does not need to do anything)."
   
       Total zlib mem use is 176KB plus a 'few kilobytes' per connection that uses COMPRESS:
       96KB for deflate, 24KB for 3x8KB buffers, 32KB plus a 'few' kilobytes for inflate.
     */

#ifdef DEFLATE_PLAY_YES

    class Rfc1951Compressor {
    public:
        explicit Rfc1951Compressor(int chunkSize = 8192);
        ~Rfc1951Compressor();

        bool write(QIODevice *out, QByteArray *in);

    private:
        int _chunkSize;
        z_stream _zStream;
        char *_buffer;
    };

    class Rfc1951Decompressor {
    public:
        explicit Rfc1951Decompressor(int chunkSize = 8192);
        ~Rfc1951Decompressor();

        bool consume(QIODevice *in);
        bool canReadLine() const;
        QByteArray readLine();
        QByteArray read(qint64 maxSize);

    private:
        int _chunkSize;
        z_stream _zStream;
        QByteArray _inBuffer;
        char *_stagingBuffer;
        QByteArray _output;
    };

#endif

}




#endif	/* DEFLATE_CHUNK_INCOMMING_H */


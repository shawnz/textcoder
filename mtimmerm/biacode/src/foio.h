//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  For commercial purposes, mail me at matt@timmermans.org, and we'll talk.
//
//  foio.h
//
//  These classes implement a bijection between byte streams and "finitely odd"
//  bit streams, where a finitely odd bit stream is of infinite length, but
//  the position of the rightmost 1 bit is finite, i.e., there is a final
//  1 bit somewhere in the stream, which is followed by an infinite number
//  of zero bits
//
//  This is implemented as a pair of STL streambufs that wrap character
//  streams
//===========================================================================

#ifndef tpvlegql4ggmbytk1a2rabtafjkb4z41
#define tpvlegql4ggmbytk1a2rabtafjkb4z41

#include <ostream>
#include <istream>
#include <streambuf>

typedef unsigned char BYTE;


//===========================================================================
//  BytesAsFoBitsOutBif: wrap an output stream
//===========================================================================

class BytesAsFOBitsOutBuf : public std::streambuf
  {
  public:
  BytesAsFOBitsOutBuf(std::ostream &bytestream,int bytesperblock=1);
  ~BytesAsFOBitsOutBuf()
    {End();}
  void End();
  private:
  std::ostream &base;
  long segsize; //for output: number of zero bytes at the end so far
  int blocksize,blockleft;
  char buf[256];
  char segfirst;
  bool reserve0;
  virtual int overflow(int c);
  virtual int sync();
  };


//===========================================================================
//  BytesAsFoBitsInBuf: wrap an input stream
//===========================================================================

class BytesAsFOBitsInBuf : public std::streambuf
  {
  public:
  BytesAsFOBitsInBuf(std::istream &bytestream,int bytesperblock=1);
  private:
  std::istream &base;
  int blocksize,blockleft;
  bool in_done;   //input from base is finished
  bool reserve0;
  char buf[256];
  virtual int underflow();
  };


//===========================================================================
//  Streams to use the streambufs
//===========================================================================

//this is needed to avoid a bug in my compiler
typedef std::ostream stdostream;
typedef std::istream stdistream;

class FOBitOStream : public stdostream
  {
  public:
  FOBitOStream(std::ostream &base,int blocksize=1)
    : stdostream(&buffer),buffer(base,blocksize)
    {}
  void End()
    {buffer.End();}
  private:
  BytesAsFOBitsOutBuf buffer;
  };

class FOBitIStream : public stdistream
  {
  public:
  FOBitIStream(std::istream &base,int blocksize=1)
    : stdistream(&buffer),buffer(base,blocksize)
    {}
  private:
  BytesAsFOBitsInBuf buffer;
  };


#endif


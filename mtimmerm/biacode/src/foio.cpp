//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  For commercial purposes, mail me at matt@timmermans.org, and we'll talk.
//===========================================================================

//NOTE:  When we write files out, we XOR each byte with 55.  This is theoretically
//unnecessary, but it helps when you're playing with repeated decompression -- it
//tends to stop files from getting incredibly large very quickly.

#include "foio.h"
#include <assert.h>
//===========================================================================
//  BytesAsFoBitsOutBif: wrap an output stream
//===========================================================================

BytesAsFOBitsOutBuf::BytesAsFOBitsOutBuf(std::ostream &bytestream,int bytesperblock)
  : base(bytestream), segsize(0), reserve0(false), segfirst(0)
  {
  blocksize=(bytesperblock>0?bytesperblock:1);
  blockleft=0;
  }

int BytesAsFOBitsOutBuf::overflow(int c)
  {
  char *s,*e;
  for (s=pbase(),e=pptr();s!=e;++s)
    {
    if (!segsize)
      {
      segfirst=*s;
      ++segsize;
      }
    else if (!*s)
      ++segsize;
    else
      {
      if (!blockleft)
        {
        if (reserve0)
          reserve0=!(segfirst&127);
        else
          reserve0=!segfirst;
        blockleft=blocksize-1;
        }
      else
        {
        reserve0=reserve0&&!segfirst;
        --blockleft;
        }
      base.put(segfirst^55);

      for (--segsize;segsize;--segsize)
        {
        if (!blockleft)
          {
          reserve0=true;
          blockleft=blocksize-1;
          }
        else
          --blockleft;
        base.put(55);
        }
      segfirst=*s;
      ++segsize;
      }
    }
  //reset the buffer and store the overflow char
  buf[0]=(char)c;
  setp(buf,buf+256);
  if (c>=0)
    pbump(1);
  return(c&255);
  }


int BytesAsFOBitsOutBuf::sync()
  {
  overflow(-1);
  return(0);
  }

void BytesAsFOBitsOutBuf::End()
  {
  sync();
  if (!segsize)
    segfirst=0;
  TOP:
  for(;blockleft;--blockleft)
    {
    reserve0=reserve0&&!segfirst;
    base.put(segfirst^55);
    segfirst=0;
    }
  if (reserve0)
    {
    assert(segfirst!=0);
    if (segfirst!=(char)128)  //no end here
      {
      reserve0=false;
      blockleft=blocksize;
      goto TOP;
      }
    }
  else if (segfirst)  //no end here
    {
    blockleft=blocksize;
    goto TOP;
    }
  segsize=0;
  reserve0=false;
  blockleft=0;
  }

//===========================================================================
//  BytesAsFoBitsOutBif: wrap an input stream
//===========================================================================


BytesAsFOBitsInBuf::BytesAsFOBitsInBuf(std::istream &bytestream,int bytesperblock)
  : base(bytestream), blockleft(0), in_done(false), reserve0(false)
  {
  blocksize=(bytesperblock>0?bytesperblock:1);
  blockleft=0;
  }


int BytesAsFOBitsInBuf::underflow()
  {
  char *s,*e;
  int inbyte;
  for (s=buf,e=buf+256;(s!=e);++s)
    {
    if (in_done)
      inbyte=0;
    else
      {
      inbyte=base.get();
      if (inbyte<0)
        {
        in_done=true;
        inbyte=0;
        }
      else
        inbyte^=55;
      }
    if (blockleft)
      {
      reserve0=reserve0&&!inbyte;
      *s=(char)inbyte;
      --blockleft;
      }
    else if (in_done) //we've read the last block
      {
      if (reserve0) //128 then end
        {
        *s=(char)128;
        reserve0=false;
        }
      else
        break;
      }
    else  //another block
      {
      if (reserve0)
        reserve0=!(inbyte&127);
      else
        reserve0=!inbyte;
      blockleft=blocksize-1;
      *s=(char)inbyte;
      }
    }
  if (s>buf)
    {
    setg(buf,buf,s);
    return((unsigned char)buf[0]);
    }
  else
    {
    setg(0,0,0);
    return(-1);
    }
  }



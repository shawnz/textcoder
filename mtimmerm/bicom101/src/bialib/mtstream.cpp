//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//===========================================================================


#include "mtstream.h"
#include <assert.h>


//===========================================================================
//  IOByteQ:  A FIFO for bytes that connects sources to streams
//===========================================================================

void IOByteQ::Realloc(unsigned len)
  {
  DATUM *dest;
  unsigned bufused=(unsigned)(m_pput-m_pget);
  unsigned allocsize=(unsigned)(m_bufend-m_buf);
  unsigned bufread=(unsigned)(m_pget-m_buf);
  if
    (
    ((allocsize-bufused)>=len) && //if moving provides enough space and
    (bufread>=bufused) //destination doesn't overlap
    )
    {
    //then move data to the start of the buffer
    for (dest=m_buf;m_pget<m_pput;*dest++=*m_pget++);
    m_pget=m_buf;
    m_pput=dest;
    }
  else  //allocate new
    {
    DATUM *oldbuf=m_buf;
    allocsize=(m_pput-m_pget)+len;
    allocsize+=(allocsize>>4);
    if (allocsize<32)
      allocsize=32;
    m_buf=new DATUM[allocsize];
    m_bufend=m_buf+allocsize;
    //move data to the start of new buffer
    for (dest=m_buf;m_pget<m_pput;*dest++=*m_pget++);
    m_pget=m_buf;
    m_pput=dest;
    if (oldbuf)
      delete [] oldbuf; //delete old buffer
    }
  }


//===========================================================================
//  IORunQ:  A FIFO for IOByteRuns
//    This class is the same as IOByteQ, with the DATUM type redefined
//    I could have used templates, but didn't want to worry
//    possible incompatabilities
//===========================================================================

void IORunQ::Realloc(unsigned len)
  {
  DATUM *dest;
  unsigned bufused=(unsigned)(m_pput-m_pget);
  unsigned allocsize=(unsigned)(m_bufend-m_buf);
  unsigned bufread=(unsigned)(m_pget-m_buf);
  if
    (
    ((allocsize-bufused)>=len) && //if moving provides enough space and
    (bufread>=bufused) //destination doesn't overlap
    )
    {
    //then move data to the start of the buffer
    for (dest=m_buf;m_pget<m_pput;*dest++=*m_pget++);
    m_pget=m_buf;
    m_pput=dest;
    }
  else  //allocate new
    {
    DATUM *oldbuf=m_buf;
    allocsize=(m_pput-m_pget)+len;
    allocsize+=(allocsize>>4);
    if (allocsize<32)
      allocsize=32;
    m_buf=new DATUM[allocsize];
    m_bufend=m_buf+allocsize;
    //move data to the start of new buffer
    for (dest=m_buf;m_pget<m_pput;*dest++=*m_pget++);
    m_pget=m_buf;
    m_pput=dest;
    delete [] oldbuf; //delete old buffer
    }
  }


//===========================================================================
//  IOByteSource:  A callback to write data into a FIFO
//===========================================================================

IOByteSource::~IOByteSource() {}

//===========================================================================
//  MemoryByteSource:  Byte source from memory
//===========================================================================

bool MemoryByteSource::FillQ(IOByteQ *dest)
  {
  BYTE *d;
  unsigned putlen;
  if (m_len)
    {
    putlen=(m_len<1024?m_len:1024);
    d=dest->Put(putlen);
    m_len-=putlen;
    while(putlen--)
      *d++=*m_src++;
    }
  return !!m_len;
  }

//===========================================================================
//  Byte stream input from source
//===========================================================================

unsigned ByteStream::Read(BYTE *dest,unsigned len)
  {
  unsigned request=len;
  unsigned avail;
  BYTE *src;
  while(len&&!AtEnd())
    {
    avail=m_queue.DataLeft();
    if (avail>len)
      avail=len;

    src=m_queue.Get(avail);
    len-=avail;
    while(avail--)
      *dest++=*src++;
    }
  return(request-len);
  }


//===========================================================================
//  FOStream: Byte source as an FO stream
//===========================================================================

void FOStream::Read(BYTE *dest,unsigned len)
  {
  unsigned avail;
  BYTE *src;
  while(len&&!InTail())
    {
    if (m_prezeros)
      {
      if (len<m_prezeros)
        avail=len;
      else
        avail=m_prezeros;
      m_prezeros-=avail;
      len-=avail;
      while(avail--)
        *dest++=0;
      }
    else
      {
      avail=m_queue.DataLeft();

      if (avail>len)
        avail=len;

      src=m_queue.Get(avail);
      len-=avail;
      while(avail--)
        *dest++=*src++;
      }
    }
  while(len--)
    *dest++=0;
  }

//returns InTail()
bool FOStream::DoFill()
  {
  IOByteQ::MARK mark;
  BYTE *s,*e;
  unsigned prelen,postlen;
  while(m_queue.IsEmpty())
    {
    m_prezeros+=m_postzeros;
    m_postzeros=0;
    if (!m_src)
      return true;
    mark=m_queue.GetMark();
    if (!m_src->FillQ(&m_queue))
      {
      if (m_autodel)
        delete m_src;
      m_src=0;
      }
    m_queue.DataSinceMark(mark,&s,&e);
    for (prelen=0;(s!=e)&&!*s;++prelen,++s);
    for (postlen=0;(s!=e)&&!e[-1];++postlen,--e);
    m_queue.Unput(postlen);
    m_queue.Get(prelen);
    m_prezeros+=prelen;
    m_postzeros+=postlen;
    }
  return(false);
  }

//===========================================================================
//  Filter to XOR all bytes in a stream with 0x55
//===========================================================================

bool XOR55ByteSource::FillQ(IOByteQ *dest)
  {
  if (!m_src)
    return(false);
  
  bool ret;
  IOByteQ::MARK mark;
  BYTE *s,*e;

  mark=dest->GetMark();
  ret=m_src->FillQ(dest);
  dest->DataSinceMark(mark,&s,&e);
  for (;s!=e;++s)
    {
    *s ^= (BYTE)0x55;
    }
  return ret;
  }


//===========================================================================
//  ByteFileSource:  A byte source that reads from files
//===========================================================================

bool ByteFileSource::Open(const char *name)
  {
  Close();
  m_f=fopen(name,"rb");
  m_doclose=!!m_f;
  return(!!m_f);
  }

void ByteFileSource::UseFile(FILE *infile,bool autoclose)
  {
  Close();
  m_f=infile;
  m_doclose=autoclose;
  }

void ByteFileSource::Close()
  {
  if (m_doclose)
    fclose(m_f);
  m_f=NULL;
  m_doclose=false;
  }

bool ByteFileSource::FillQ(IOByteQ *dest)
  {
  if (m_f)
    {
    unsigned notread=((unsigned)fread(dest->Put(1024),1,1024,m_f));
    notread=1024-notread;
    if (notread)
      {
      dest->Unput(notread);
      Close();
      }
    }
  return(!!m_f);
  }




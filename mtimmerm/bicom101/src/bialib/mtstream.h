//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//
//  mtstream.h
//
//  These are base classes used to implement streams of bytes that connect
//  processing stages.
//
//===========================================================================

#ifndef tpvlegql4ggmbytk1a2rabtafjkb4z41
#define tpvlegql4ggmbytk1a2rabtafjkb4z41

#include <stdio.h>

typedef unsigned char BYTE;


//===========================================================================
//  IOByteQ:  A FIFO for bytes that connects sources to streams
//===========================================================================

class IOByteQ
  {
  public:
  typedef BYTE DATUM;

  public:
  IOByteQ()
    {
    m_buf=m_pget=m_pput=m_bufend=0;
    }
  ~IOByteQ()
    {
    delete [] m_buf;
    m_buf=m_pget=m_pput=m_bufend=0;
    }
  void Clear()
    {
    m_pget=m_pput=m_bufend=m_buf;
    }

  public: //read methods
  unsigned DataLeft()
    {return((unsigned)(m_pput-m_pget));}
  bool IsEmpty()
    {return(m_pput<=m_pget);}
  DATUM &Get()
    {return(*m_pget++);}
  DATUM *Get(unsigned len)
    {
    DATUM *ret=m_pget;
    m_pget+=len;
    return ret;
    }

  public: //write methods
  //To write data into the Queue call Put(number of bytes)
  //and write data at the returned pointer.
  DATUM *Put(unsigned len)
    {
    DATUM *ret;
    if ((unsigned)(m_bufend-m_pput)<len)
      Realloc(len);
    ret=m_pput;
    m_pput+=len;
    return ret;
    }
  //you can unput data right after you put it
  void Unput(unsigned len)
    {m_pput-=len;}

  public: //for in-place edits
  typedef unsigned MARK;
  MARK GetMark()
    {return ((unsigned)(m_pput-m_pget));}
  void DataSinceMark(MARK mark,DATUM **pstart,DATUM **pend)
    {*pstart=m_pget+mark;*pend=m_pput;}

  private:
  void Realloc(unsigned len);
  DATUM *m_buf;
  DATUM *m_pget;
  DATUM *m_pput;
  DATUM *m_bufend;
  };


//===========================================================================
//  IORunQ:  A FIFO for IOByteRuns
//    This class is the same as IOByteQ, with the DATUM type redefined
//    I could have used templates, but didn't want to worry
//    possible incompatabilities
//===========================================================================

struct IOByteRun
  {
  BYTE c;
  unsigned long len;
  };

class IORunQ
  {
  public:
  typedef IOByteRun DATUM;

  public:
  IORunQ()
    {
    m_buf=m_pget=m_pput=m_bufend=0;
    }
  ~IORunQ()
    {
    delete [] m_buf;
    m_buf=m_pget=m_pput=m_bufend=0;
    }
  void Clear()
    {
    m_pget=m_pput=m_bufend=m_buf;
    }

  public: //read methods
  unsigned DataLeft()
    {return((unsigned)(m_pput-m_pget));}
  bool IsEmpty()
    {return(m_pput<=m_pget);}
  DATUM &Get()
    {return(*m_pget++);}
  DATUM *Get(unsigned len)
    {
    DATUM *ret=m_pget;
    m_pget+=len;
    return ret;
    }

  public: //write methods
  //To write data into the Queue call Put(number of bytes)
  //and write data at the returned pointer.
  DATUM *Put(unsigned len)
    {
    DATUM *ret;
    if ((unsigned)(m_bufend-m_pput)<len)
      Realloc(len);
    ret=m_pput;
    m_pput+=len;
    return ret;
    }
  //you can unput data right after you put it
  void Unput(unsigned len)
    {m_pput-=len;}

  public: //for in-place edits
  typedef unsigned MARK;
  MARK GetMark()
    {return ((unsigned)(m_pput-m_pget));}
  void DataSinceMark(MARK mark,DATUM **pstart,DATUM **pend)
    {*pstart=m_pget+mark;*pend=m_pput;}

  private:
  void Realloc(unsigned len);
  DATUM *m_buf;
  DATUM *m_pget;
  DATUM *m_pput;
  DATUM *m_bufend;
  };


//===========================================================================
//  IOByteSource:  A callback to write data into a FIFO
//===========================================================================

class IOByteSource
  {
  public:
  virtual ~IOByteSource();
  virtual bool FillQ(IOByteQ *dest)=0;
  };

//===========================================================================
//  MemoryByteSource:  Byte source from memory
//===========================================================================

class MemoryByteSource : public IOByteSource
  {
  public:
  MemoryByteSource(BYTE *src,unsigned len)
    {m_src=src;m_len=len;}
  bool FillQ(IOByteQ *dest);
  private:
  BYTE *m_src;
  unsigned m_len;
  };

//===========================================================================
//  ByteStream: Streamed byte source
//===========================================================================

class ByteStream
  {
  public:
  ByteStream()
    {m_src=0;m_autodel=false;}
  ByteStream(IOByteSource *source,bool autodelete=false)
    {
    m_src=0;m_autodel=false;
    SetSource(source,autodelete);
    }
  ~ByteStream()
    {Clear();}

  void SetSource(IOByteSource *source,bool autodelete=false)
    {Clear();m_src=source;m_autodel=autodelete;}

  void Clear()
    {
    m_queue.Clear();
    if (m_src&&m_autodel)
      delete m_src;
    m_src=0;
    }

  bool AtEnd()
    {
    while(m_queue.IsEmpty())
      {
      if (!m_src)
        return true;
      if (!m_src->FillQ(&m_queue))
        {
        if (m_autodel)
          delete m_src;
        m_src=0;
        }
      }
    return false;
    }

  //you MUST call AtEnd() before calling Get() or Peek()
  BYTE Get()
    {return (m_queue.Get());}
  BYTE Peek()
    {return(*(m_queue.Get(0)));}

  unsigned Read(BYTE *dest,unsigned len);
  
  private:
  IOByteQ m_queue;
  IOByteSource *m_src;
  bool m_autodel;
  };


//===========================================================================
//  FOStream: Byte source as an FO stream
//===========================================================================

class FOStream
  {
  public:
  FOStream()
    {m_src=0;m_autodel=false;m_prezeros=m_postzeros=0;}
  FOStream(IOByteSource *source,bool autodelete=false)
    {
    m_src=0;m_autodel=false;m_prezeros=m_postzeros=0;
    SetSource(source,autodelete);
    }
  ~FOStream()
    {Clear();}

  void SetSource(IOByteSource *source,bool autodelete=false)
    {Clear();m_src=source;m_autodel=autodelete;}

  void Clear()
    {
    m_queue.Clear();
    if (m_src&&m_autodel)
      delete m_src;
    m_src=0;m_prezeros=m_postzeros=0;
    }

  bool InTail()
    {
    return(m_queue.IsEmpty()?DoFill():false);
    }

  BYTE Get()
    {
    if (InTail())
      return 0;
    else if (m_prezeros)
      {--m_prezeros;return 0;}
    else
      return (m_queue.Get());
    }
  BYTE Peek()
    {
    if (InTail()||m_prezeros)
      return 0;
    else
      return (*(m_queue.Get(0)));
    }

  void Read(BYTE *dest,unsigned len);
  
  private:
  bool DoFill();
  IOByteQ m_queue;
  IOByteSource *m_src;
  bool m_autodel;
  unsigned long m_prezeros,m_postzeros;
  };



//===========================================================================
//  Byte stream filter to XOR all bytes in a with 0x55
//===========================================================================

class XOR55ByteSource : public IOByteSource
  {
  public:
  XOR55ByteSource()
    {m_src=0;m_autodel=false;}
  XOR55ByteSource(IOByteSource *source,bool autodelete=false)
    {
    m_src=0;m_autodel=false;
    SetSource(source,autodelete);
    }
  ~XOR55ByteSource()
    {Clear();}

  void SetSource(IOByteSource *source,bool autodelete=false)
    {Clear();m_src=source;m_autodel=autodelete;}

  void Clear()
    {
    if (m_src&&m_autodel)
      delete m_src;
    m_src=0;
    }
  
  public: //from IOByteSource
  virtual bool FillQ(IOByteQ *dest);

  private:
  IOByteSource *m_src;
  bool m_autodel;
  };

//===========================================================================
//  ByteFileSource:  A byte source that reads from files
//===========================================================================

class ByteFileSource : public IOByteSource
  {
  public:
  ByteFileSource()
    {m_f=0;m_doclose=false;}
  ~ByteFileSource()
    {Close();}

  bool Open(const char *name);
  void UseFile(FILE *infile,bool autoclose=false);
  void Close();

  public: //from IOByteSource
  virtual bool FillQ(IOByteQ *dest);

  FILE *m_f;
  bool m_doclose;
  };


#endif


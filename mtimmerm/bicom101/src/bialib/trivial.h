//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//
//  trivial.h
//
//  These classes implement a simple bijection between byte streams and
//  "finitely odd" bit streams, where a finitely odd bit stream is of infinite
//  length, but the position of the rightmost 1 bit is finite, i.e., there
//  is a final 1 bit somewhere in the stream, which is followed by an infinite
//  number of zero bits
//
//===========================================================================

#ifndef wddstx1afsvbsxtpezmxgvut0q4ehncy
#define wddstx1afsvbsxtpezmxgvut0q4ehncy

#include "mtstream.h"

//===========================================================================
//  TrivialFOEncoder: encode bytes to FO.  The byte stream must be an even
//    number of blocklen sized blocks
//===========================================================================

class TrivialFOEncoder : public IOByteSource
{
public:
    TrivialFOEncoder(unsigned blocklen = 1)
    {
        m_blocklen = (blocklen ? blocklen : 1);
        m_reserved0 = m_done = false;
    }
    TrivialFOEncoder(IOByteSource *source, bool autodelete = false, unsigned blocklen = 1)
    {
        m_blocklen = (blocklen ? blocklen : 1);
        m_reserved0 = m_done = false;
        SetSource(source, autodelete);
    }
    ~TrivialFOEncoder()
    {
        Clear();
    }

    void SetSource(IOByteSource *source, bool autodelete = false)
    {
        Clear();
        m_in.SetSource(source, autodelete);
    }

    void Clear()
    {
        m_in.Clear();
        m_reserved0 = m_done = false;
    }

public: // from IOByteSource
    virtual bool FillQ(IOByteQ *dest);

private:
    ByteStream m_in;
    unsigned m_blocklen, m_taillen;
    BYTE m_tail[16];
    bool m_reserved0, m_done;
};

//===========================================================================
//  TrivialFODecoder: Decode FO into byte blocks
//===========================================================================

class TrivialFODecoder : public IOByteSource
{
public:
    TrivialFODecoder(unsigned blocklen = 1)
    {
        m_blocklen = (blocklen ? blocklen : 1);
        m_reserved0 = m_done = false;
    }
    TrivialFODecoder(IOByteSource *source, bool autodelete = false, unsigned blocklen = 1)
    {
        m_blocklen = (blocklen ? blocklen : 1);
        m_reserved0 = m_done = false;
        SetSource(source, autodelete);
    }
    ~TrivialFODecoder()
    {
        Clear();
    }

    void SetSource(IOByteSource *source, bool autodelete = false)
    {
        Clear();
        m_in.SetSource(source, autodelete);
    }

    void Clear()
    {
        m_in.Clear();
        m_reserved0 = m_done = false;
    }

public: // from IOByteSource
    virtual bool FillQ(IOByteQ *dest);

private:
    FOStream m_in;
    unsigned m_blocklen;
    bool m_reserved0, m_done;
};

#endif

//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//
//  trivial.cpp
//
//  These classes implement a simple bijection between byte streams and
//  "finitely odd" bit streams, where a finitely odd bit stream is of infinite
//  length, but the position of the rightmost 1 bit is finite, i.e., there
//  is a final 1 bit somewhere in the stream, which is followed by an infinite
//  number of zero bits
//
//===========================================================================

#include "trivial.h"

bool TrivialFOEncoder::FillQ(IOByteQ *dest)
{
    unsigned outlen = 0;
    unsigned blocktail = m_blocklen - 1;
    unsigned i;
    BYTE c;

    while ((outlen < 256) && (!m_done))
    {
        outlen += m_blocklen;
        // do a block or end
        if (m_in.AtEnd()) // ending
        {
            if (m_reserved0)
                *(dest->Put(1)) = (BYTE)128;
            m_done = true;
            break;
        }
        // output a block
        c = m_in.Get();
        *(dest->Put(1)) = c;
        if (m_reserved0)
            m_reserved0 = !(c & 127);
        else
            m_reserved0 = !c;

        for (i = blocktail; i; --i)
        {
            c = (m_in.AtEnd() ? 0 : m_in.Get());
            *(dest->Put(1)) = c;
            if (c)
                m_reserved0 = false;
        }
    }
    return !m_done;
}

bool TrivialFODecoder::FillQ(IOByteQ *dest)
{
    unsigned outlen = 0;
    unsigned blocktail = m_blocklen - 1;
    unsigned i;
    BYTE c;

    while ((outlen < 256) && (!m_done))
    {
        outlen += m_blocklen;
        // do a block or end
        c = m_in.Get();

        if (m_reserved0)
        {
            if ((c == (BYTE)128) && m_in.InTail())
            {
                m_done = true;
                break;
            }
            *(dest->Put(1)) = c;
            m_reserved0 = !(c & 127);
        }
        else
        {
            if ((c == 0) && m_in.InTail())
            {
                m_done = true;
                break;
            }
            *(dest->Put(1)) = c;
            m_reserved0 = !c;
        }

        for (i = blocktail; i; --i)
        {
            c = m_in.Get();
            *(dest->Put(1)) = c;
            if (c)
                m_reserved0 = false;
        }
    }
    return !m_done;
}

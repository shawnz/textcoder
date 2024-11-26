//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  For commercial purposes, mail me at matt@timmermans.org, and we'll talk.
//===========================================================================

#include <assert.h>
#include "arithmetic.h"

//===========================================================================
//  ArithmeticEncoder
//===========================================================================

ArithmeticEncoder::ArithmeticEncoder(std::ostream &outstream)
    : bytesout(outstream)
{
    // current range [0,65536)
    low = 0;
    range = BIT16;
    intervalbits = 16;
    // no ends reserved yet
    freeendeven = MASK16;
    nextfreeend = 0;
    // no bytes buffered for carry propogation
    carrybyte = 0;
    carrybuf = 0;
}

void ArithmeticEncoder::Encode(
    const ArithmeticModel *model, int symbol, bool could_have_ended)
{
    U32 newh, newl;

    // nextfreeend is known to be in the range here.

    if (could_have_ended)
    {
        // we could have ended here, so we need to reserve
        // the next end value.
        if (nextfreeend)
            nextfreeend += (freeendeven + 1) << 1;
        else
            nextfreeend = freeendeven + 1;
    }

    // narrow the interval to encode the symbol
    model->GetSymRange(symbol, &newl, &newh);
    newl = newl * range / model->ProbOne();
    newh = newh * range / model->ProbOne();
    range = newh - newl;
    low += newl;

    // make sure nextfreeend is at least low
    if (nextfreeend < low)
        nextfreeend = ((low + freeendeven) & ~freeendeven) | (freeendeven + 1);

    // adjust range
    if (range <= (BIT16 >> 1))
    {
        // scale range once
        low += low;
        range += range;
        nextfreeend += nextfreeend;
        freeendeven += freeendeven + 1;

        // ensure that nextfreeend is in the range
        while (nextfreeend - low >= range)
        {
            freeendeven >>= 1;
            // smallest number of the required oddness >= low
            nextfreeend = ((low + freeendeven) & ~freeendeven) | (freeendeven + 1);
        }

        for (;;)
        {
            if (++intervalbits == 24)
            {
                // need to output a byte
                // adjust and output
                newl = low & ~MASK16;
                low -= newl;
                nextfreeend -= newl;
                // there can only be one number this even in the range.
                // nextfreeend is in the range
                // if nextfreeend is this even, next step must reduce evenness
                freeendeven &= MASK16;

                ByteWithCarry(newl >> 16);
                intervalbits -= 8;
            }

            if (range > (BIT16 >> 1))
                break; // finished scaling range

            // scale again
            low += low;
            range += range;
            nextfreeend += nextfreeend;
            freeendeven += freeendeven + 1;
        }
        while (range <= (BIT16 >> 1))
            ;
    }
    else
    {
        // ensure that nextfreeend is in the range
        while (nextfreeend - low >= range)
        {
            freeendeven >>= 1;
            // smallest number of the required oddness >= low
            nextfreeend = ((low + freeendeven) & ~freeendeven) | (freeendeven + 1);
        }
    }
}

void ArithmeticEncoder::End()
{
    // output the next free end code
    // bytes after EOF are assumed all zero
    nextfreeend <<= (24 - intervalbits);

    while (nextfreeend)
    {
        ByteWithCarry(nextfreeend >> 16);
        nextfreeend = (nextfreeend & MASK16) << 8;
    }
    if (carrybuf) // flush the carry buffer
        ByteWithCarry(0);

    // reset everything for next stream, if any
    low = 0;
    range = BIT16;
    intervalbits = 16;
    freeendeven = MASK16;
    nextfreeend = 0;
    carrybyte = 0;
    carrybuf = 0;
}

inline void ArithmeticEncoder::ByteWithCarry(U32 outbyte)
{
    if (carrybuf)
    {
        if (outbyte >= 256)
        {
            bytesout.put((char)(carrybyte + 1));
            while (--carrybuf) // write carrybuf-1 bytes
                bytesout.put(0);
            carrybyte = (BYTE)outbyte;
        }
        else if (outbyte < 255)
        {
            bytesout.put((char)carrybyte);
            while (--carrybuf) // write carrybuf-1 bytes
                bytesout.put((char)255);
            carrybyte = (BYTE)outbyte;
        }
    }
    else
        carrybyte = (BYTE)outbyte;
    ++carrybuf;
}

//===========================================================================
//  ArithmeticDecoder
//===========================================================================

ArithmeticDecoder::ArithmeticDecoder(std::istream &instream)
    : bytesin(instream)
{
    low = 0;
    range = BIT16;
    intervalbits = 16;
    freeendeven = MASK16;
    nextfreeend = 0;
    value = 0;
    // this is -24 instead of -16 because we preload a zero into the follow
    // buffer
    valueshift = -24;
    followbyte = 0;
    followbuf = 1;
}

int ArithmeticDecoder::Decode(
    const ArithmeticModel *model,
    // set true if is a valid place to end
    bool can_end)
{
    int ret;
    U32 newh, newl;

    // read input until we have enough significant bits
    while (valueshift <= 0)
    {
        // need new bytes
        value <<= 8;
        valueshift += 8;
        if (!--followbuf)
        {
            value |= followbyte;
            // refill followbuf
            int cin;
            do
            {
                cin = bytesin.get();
                if (cin < 0)
                {
                    followbuf = -1;
                    break;
                }
                ++followbuf;
                followbyte = (BYTE)cin;
            } while (!followbyte);
        }
    }

    // check for end
    if (can_end)
    {
        // check for end
        if ((followbuf < 0) && (((nextfreeend - low) << valueshift) == value))
            return (-1); // EOF

        if (nextfreeend)
            nextfreeend += (freeendeven + 1) << 1;
        else
            nextfreeend = freeendeven + 1;
    }

    // decode the symbol and narrow the interval
    newl = ((value >> valueshift) * model->ProbOne() + model->ProbOne() - 1) / range;
    ret = model->GetSymbol(newl, &newl, &newh);
    newl = newl * range / model->ProbOne();
    newh = newh * range / model->ProbOne();

    range = newh - newl;
    value -= (newl << valueshift);
    low += newl;

    // make sure nextfreeend>=low
    if (nextfreeend < low)
        nextfreeend = ((low + freeendeven) & ~freeendeven) | (freeendeven + 1);

    // adjust range
    if (range <= (BIT16 >> 1))
    {
        // scale range once
        low += low;
        range += range;
        nextfreeend += nextfreeend;
        freeendeven += freeendeven + 1;
        --valueshift;

        // ensure that nextfreeend is in the range
        while (nextfreeend - low >= range)
        {
            freeendeven >>= 1;
            // smallest number of the required oddness >= low
            nextfreeend = ((low + freeendeven) & ~freeendeven) | (freeendeven + 1);
        }

        for (;;)
        {
            if (++intervalbits == 24)
            {
                // need to drop a byte
                newl = low & ~MASK16;
                low -= newl;
                nextfreeend -= newl;
                // there can only be one number this even in the range.
                // nextfreeend is in the range
                // if nextfreeend is this even, next step must reduce evenness
                freeendeven &= MASK16;
                intervalbits -= 8;
            }

            if (range > (BIT16 >> 1))
                break; // finished scaling range

            // scale again
            low += low;
            range += range;
            nextfreeend += nextfreeend;
            freeendeven += freeendeven + 1;
            --valueshift;
        }
        while (range <= (BIT16 >> 1))
            ;
    }
    else
    {
        // ensure that nextfreeend is in the range
        while (nextfreeend - low >= range)
        {
            freeendeven >>= 1;
            // smallest number of the required oddness >= low
            nextfreeend = ((low + freeendeven) & ~freeendeven) | (freeendeven + 1);
        }
    }

    return (ret);
}

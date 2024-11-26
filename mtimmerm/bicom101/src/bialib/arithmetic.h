//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//
//  arithmetic.h
//  Defines arithmetic encoder/decoder classes, and the interface for
//  probability models for use with these
//===========================================================================

#ifndef hpb0crrbky1yqkano50anynlk1t5m5rs
#define hpb0crrbky1yqkano50anynlk1t5m5rs

typedef unsigned long U32;
typedef unsigned char BYTE;

#include "mtstream.h"

static const U32 MAXP1 = ((U32)1) << 21;

class ArithmeticEncoder;
class ArithmeticDecoder;

class ArithmeticModel
{
public:
    virtual void Encode(BYTE symbol, ArithmeticEncoder *dest) = 0;
    virtual BYTE Decode(ArithmeticDecoder *src) = 0;
};

static const U32 BIT16 = 0x10000L;
static const U32 MASK16 = 0x0FFFFL;

static const int MAXRANGEBITS = 23;
static const U32 MAXRANGE = ((U32)1) << MAXRANGEBITS;
static const U32 MAXRANGEMASK = MAXRANGE - 1;

//===========================================================================
//  MulDiv24:  We need this little helper to prevent overflow in
//             probability scaling
//===========================================================================
// multiply two 32-bit numbers, add a 32-bit number, and divide by a 24-bit
// divisor, when the result fits into 32 bits.
// we make real good use of those extra 8 bits at the top of the divisor

#ifdef _MSC_VER
#if _MSC_VER >= 1200
#define U64_TYPE unsigned __int64
#endif
#endif

#ifdef U64_TYPE // quick version if compiler supports 64-bit ints

inline U32 MulAddDiv24(U32 a, U32 b, U32 accum, U32 divisor)
{
    return ((U32)((((U64_TYPE)a) * b + accum) / divisor));
}

#else // portable version

inline U32 MulAddDiv24(U32 a, U32 b, U32 accum, U32 divisor)
{
    U32 ret = 0;

    if (a >= divisor)
    {
        ret += b * (a / divisor);
        a %= divisor;
    }

    while (a) // at most 3 times
    {
        // Q=ret+(a*b+accum)/divisor
        if (b >= (U32)0x01000000L)
        {
            //  (a*b+accum)/divisor
            //= (accum+a*((b/divisor)*divisor+b%divisor))/divisor
            //= (accum + a*b%divisor +a*(b/divisor*divisor))/divisor
            //= (accum + a*b%divisor)/divisor + a*(b/divisor)
            ret += a * (b / divisor);
            b %= divisor;
        }
        if (accum >= (U32)0x01000000L)
        {
            ret += accum / divisor;
            accum %= divisor;
        }
        accum += (a & 255) * b;
        a >>= 8;
        b <<= 8;
    }
    // Q=ret+(a*b+accum)/divisor
    // Q=ret+(0*b+accum)/divisor
    return (ret + (accum / divisor));
}

#endif

//===========================================================================
//  ArithmeticEncoder:  Encodes a byte stream to an FO stream according to a
//    model
//===========================================================================

class ArithmeticEncoder : public IOByteSource
{
public:
    ArithmeticEncoder(ArithmeticModel *modelin, IOByteSource *src, bool autodel = false);

    // callback for model
    void Encode(
        U32 p1, // probability 1 ( 0 - 16384 )
        U32 psymlow,
        U32 psymhigh);

private:
    // stream could have ended here, but doesn't
    void NotEnd();
    // finish encoding
    void End();

    // produce a byte, managing carry propogation
    void ByteWithCarry(U32 byte);

    // our current interval is [low,low+range)
    U32 low, range;
    // the number of bits we have in low.  When this gets to 24, we output
    // a byte, reducing it to 16
    int intervalbits;
    // a low-order mask.  All the free ends have these bits 0
    U32 freeendeven;
    // the next free end in the current range.  This number
    // is either 0 (the most even number possible, or
    //(freeendeven+1)*(2x+1)
    U32 nextfreeend;

private: // input processing
    ArithmeticModel *model;
    ByteStream m_in;

private: // output processing
    // from IOByteSource
    bool FillQ(IOByteQ *dest);

    // we delay output of strings matching [\x00-\xfe] \xff*, because
    // we might have to do carry propogation
    BYTE carrybyte;
    unsigned long carrybuf;

    // we store runs here until we can output them
    IORunQ runqout;
    IOByteRun currentrun;
    bool m_encodingdone;
};

//===========================================================================
//  ArithmeticDecoder:  Decodes an FO stream to a byte stream according to a
//    model
//===========================================================================

class ArithmeticDecoder : public IOByteSource
{
public:
    ArithmeticDecoder(ArithmeticModel *modelin, IOByteSource *src, bool autodel = false);

    // callbacks for model
    U32 GetP(U32 p1)
    {
        return (MulAddDiv24(value >> valueshift, p1, p1 - 1, range));
    }

    void Narrow // narrow the interval after decoding
        (
            U32 p1, // probability 1 ( 0 - 16384 )
            U32 psymlow,
            U32 psymhigh);

private:
    // need to call this everywhere the encoder calls NotEnd() or End()
    bool AtEnd();

    // our current interval is [low,low+range)
    U32 low, range;
    // the number of bits we have in low.  When this gets to 24, we output
    // drop the top 8, reducing it to 16.  This matches the encoder behaviour
    int intervalbits;
    // a low-order mask.  All the free ends have these bits 0
    U32 freeendeven;
    // the next free end in the current range.  This number
    // is either 0 (the most even number possible, or
    //(freeendeven+1)*(2x+1)
    U32 nextfreeend;

    // the current value is low+(value>>valueshift)
    // when valueshift is <0, we shift it left by 8 and read
    // a byte into the low-order bits
    U32 value;
    int valueshift;

private: // input processing
    ArithmeticModel *model;
    FOStream m_in;

private: // output processing
    // from IOByteSource
    bool FillQ(IOByteQ *dest);
};

#endif

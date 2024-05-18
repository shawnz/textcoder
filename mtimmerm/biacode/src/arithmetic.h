//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  For commercial purposes, mail me at matt@timmermans.org, and we'll talk.
//
//  arithmetic.h
//  Defines arithmetic encoder/decoder classes, and the interface for
//  probability models for use with these
//===========================================================================

#ifndef hpb0crrbky1yqkano50anynlk1t5m5rs
#define hpb0crrbky1yqkano50anynlk1t5m5rs

typedef unsigned long U32;
typedef unsigned char BYTE;

#include <ostream>
#include <istream>

static const U32 MAXP1=0x08000L;

class ArithmeticModel
  {
  public:

  //current total cumulative probability
  //always 0 < ProbOne() <= MAXP1
  U32 ProbOne() const
    {return(prob1);}

  //will set newlow,newhigh | 0 <= newlow < newhigh <= ProbOne()
  virtual void GetSymRange(int symbol,U32 *newlow,U32 *newhigh) const = 0;
  //must call with 0 <= p <= ProbOne()
  //will set newlow,newhigh | 0 <= newlow < newhigh <= ProbOne()
  virtual int GetSymbol(U32 p,U32 *newlow,U32 *newhigh) const = 0;
  protected:
  U32 prob1;
  };


static const U32 BIT16=0x10000L;
static const U32 MASK16=0x0FFFFL;

class ArithmeticEncoder
  {
  public:
  ArithmeticEncoder(std::ostream &outstream);

  //encode a symbol
  void Encode
    (
    const ArithmeticModel *model,
    int symbol,
    // set true if this was a valid place to end
    bool could_have_ended
    );

  //finish encoding
  void End();

  private:

  //produce a byte, managing carry propogation
  ByteWithCarry(U32 byte);

  //we write the finitely odd output number here
  std::ostream &bytesout;
  //our current interval is [low,low+range)
  U32 low,range;
  //the number of bits we have in low.  When this gets to 24, we output
  //a byte, reducing it to 16
  int intervalbits;
  //a low-order mask.  All the free ends have these bits 0
  U32 freeendeven;
  //the next free end in the current range.  This number
  //is either 0 (the most even number possible, or
  //(freeendeven+1)*(2x+1)
  U32 nextfreeend;

  //we delay output of strings matching [\x00-\xfe] \xff*, because
  //we might have to do carry propogation
  BYTE carrybyte;
  unsigned long carrybuf;
  };



class ArithmeticDecoder
  {
  public:
  ArithmeticDecoder(std::istream &instream);
  int Decode
    (
    const ArithmeticModel *model,
    // set true if is a valid place to end
    bool can_end
    );

  private:

  std::istream &bytesin;

  //our current interval is [low,low+range)
  U32 low,range;
  //the number of bits we have in low.  When this gets to 24, we output
  //drop the top 8, reducing it to 16.  This matches the encoder behaviour
  int intervalbits;
  //a low-order mask.  All the free ends have these bits 0
  U32 freeendeven;
  //the next free end in the current range.  This number
  //is either 0 (the most even number possible, or
  //(freeendeven+1)*(2x+1)
  U32 nextfreeend;

  //the current value is low+(value>>valueshift)
  //when valueshift is <0, we shift it left by 8 and read
  //a byte into the low-order bits
  U32 value;
  int valueshift;

  //we read ahead when we encounter a zero to see if we are in the
  //infinite zero tail of the finitely odd number we're reading
  //followbuf gets set < 0 when this happens.
  BYTE followbyte;
  long followbuf;
  };

#endif

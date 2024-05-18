//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//===========================================================================

#include <assert.h>
#include "arithmetic.h"

#ifdef _DEBUG
#include <assert.h>
#define _ASSERT(x) assert(x)
#else
#define _ASSERT(x)
#endif


//===========================================================================
//  ArithmeticEncoder
//===========================================================================

ArithmeticEncoder::ArithmeticEncoder
  (ArithmeticModel *mod,IOByteSource *src,bool autodel)
  :m_in(src,autodel)
  {
  model=mod;
  //current range [0,65536)
  low=0;
  range=MAXRANGE;
  intervalbits=MAXRANGEBITS;
  //no ends reserved yet
  freeendeven=MAXRANGEMASK;
  nextfreeend=0;
  //no bytes buffered for carry propogation
  carrybyte=0;
  carrybuf=0;
  //no buffered bytes to output
  currentrun.c=0;
  currentrun.len=0;
  //still processing
  m_encodingdone=false;
  }


void ArithmeticEncoder::Encode
  (
  U32 p1,U32 psymlow,U32 psymhigh
  )
  {
  U32 newh,newl;

  //nextfreeend is known to be in the range here.

  //narrow the interval to encode the symbol

  // I would write this:
  // newl=psymlow*range/p1;
  // newh=psymhigh*range/p1;
  // but that could overflow, so we do it this way

  newl=MulAddDiv24(psymlow,range,0,p1);
  newh=MulAddDiv24(psymhigh,range,0,p1);

  //adjust range
  range=newh-newl;
  low+=newl;

  //make sure nextfreeend is at least low
  if (nextfreeend<low)
    nextfreeend=((low+freeendeven)&~freeendeven)|(freeendeven+1);

  //adjust range
  if (range<=(MAXRANGE>>1))
    {
    //scale range once
    _ASSERT(low<=0x7FFFFFFFL);
    _ASSERT(range<=0x7FFFFFFFL);
    low+=low;
    range+=range;
    nextfreeend+=nextfreeend;
    freeendeven+=freeendeven+1;

    //ensure that nextfreeend is in the range
    while(nextfreeend-low>=range)
      {
      freeendeven>>=1;
      //smallest number of the required oddness >= low
      nextfreeend=((low+freeendeven)&~freeendeven)|(freeendeven+1);
      }

    for(;;)
      {
      if (++intervalbits==(MAXRANGEBITS+8))
        {
        //need to output a byte
        //adjust and output
        newl=low&~MAXRANGEMASK;
        low-=newl;
        nextfreeend-=newl;
        //there can only be one number this even in the range.
        //nextfreeend is in the range
        //if nextfreeend is this even, next step must reduce evenness
        freeendeven&=MAXRANGEMASK;

        ByteWithCarry(newl>>MAXRANGEBITS);
        intervalbits-=8;
        }

      if (range>(MAXRANGE>>1))
        break;  //finished scaling range

      //scale again
      _ASSERT(low<=0x7FFFFFFFL);
      _ASSERT(range<=0x7FFFFFFFL);
      low+=low;
      range+=range;
      nextfreeend+=nextfreeend;
      freeendeven+=freeendeven+1;
      }
    }
  else
    {
      //ensure that nextfreeend is in the range
    while(nextfreeend-low>=range)
      {
      freeendeven>>=1;
      //smallest number of the required oddness >= low
      nextfreeend=((low+freeendeven)&~freeendeven)|(freeendeven+1);
      }
    }
  } 



inline void ArithmeticEncoder::NotEnd()
  {
  //we could have ended here, so we need to reserve
  //the next end value.
  if (nextfreeend)
    nextfreeend+=(freeendeven+1)<<1;
  else
    nextfreeend=freeendeven+1;
  }



void ArithmeticEncoder::End()
  {
  //output the next free end code
  //bytes after EOF are assumed all zero
  nextfreeend<<=(MAXRANGEBITS+8-intervalbits);
    
  while(nextfreeend)
    {
    ByteWithCarry(nextfreeend>>MAXRANGEBITS);
    nextfreeend=(nextfreeend&MAXRANGEMASK)<<8;
    }
  if (carrybuf) //flush the carry buffer
    ByteWithCarry(0);

  //we're done
  m_encodingdone=true;
  }


inline void ArithmeticEncoder::ByteWithCarry(U32 outbyte)
  {
  IOByteRun *d;
  if (carrybuf)
    {
    if (outbyte>=256)
      {
      //write carrybuf bytes
      d=runqout.Put(2);
      d->c=carrybyte+1;
      d->len=1;
      ++d;
      d->c=0;
      d->len=carrybuf-1;
      carrybuf=1;
      carrybyte=(BYTE)outbyte;
      }
    else if (outbyte<255)
      {
      //write carrybuf bytes
      d=runqout.Put(2);
      d->c=carrybyte;
      d->len=1;
      ++d;
      d->c=(BYTE)255;
      d->len=carrybuf-1;
      carrybuf=1;
      carrybyte=(BYTE)outbyte;
      }
    else  //add the 0xFF to carry buf
      ++carrybuf;
    }
  else
    {
    carrybyte=(BYTE)outbyte;
    carrybuf=1;
    }
  }


bool ArithmeticEncoder::FillQ(IOByteQ *dest)
  {
  BYTE *d=dest->Put(256);
  BYTE *e=d+256;

  while(d!=e)
    {
    while(!currentrun.len)
      {
      while(runqout.IsEmpty())
        {
        if (m_encodingdone)
          {
          dest->Unput(e-d);
          return(false);
          }
        if (m_in.AtEnd())
          End();
        else
          {
          NotEnd();
          model->Encode(m_in.Get(),this);
          }
        }
      currentrun=runqout.Get();
      }
    *d++=currentrun.c;
    --currentrun.len;
    }
  return(true);
  }

//===========================================================================
//  ArithmeticDecoder
//===========================================================================

ArithmeticDecoder::ArithmeticDecoder
  (ArithmeticModel *modelin,IOByteSource *src,bool autodel)
  :m_in(src,autodel)
  {
  model=modelin;
  low=0;
  range=MAXRANGE;
  intervalbits=MAXRANGEBITS;
  freeendeven=MAXRANGEMASK;
  nextfreeend=0;
  value=0;
  valueshift=-MAXRANGEBITS;
  }

bool ArithmeticDecoder::AtEnd()
  {
  //read input until we have enough significant bits
  while(valueshift<=0)
    {
    //need new bytes
    valueshift+=8;
    value=(value<<8)|m_in.Get();
    }

  //check for end
  if (m_in.InTail()&&(((nextfreeend-low)<<valueshift)==value))
    return true;

  //reserve this possible end
  if (nextfreeend)
    nextfreeend+=(freeendeven+1)<<1;
  else
    nextfreeend=freeendeven+1;

  return false;
  }


void ArithmeticDecoder::Narrow
  (
  U32 p1,U32 psymlow,U32 psymhigh
  )
  {
  U32 newh,newl;

  //newl=psymlow*range/p1;
  //newh=psymhigh*range/p1;
  newl=MulAddDiv24(psymlow,range,0,p1);
  newh=MulAddDiv24(psymhigh,range,0,p1);

  range=newh-newl;
  value-=(newl<<valueshift);
  low+=newl;

  //make sure nextfreeend>=low
  if (nextfreeend<low)
    nextfreeend=((low+freeendeven)&~freeendeven)|(freeendeven+1);


  //adjust range
  if (range<=(MAXRANGE>>1))
    {
    //scale range once
    low+=low;
    range+=range;
    nextfreeend+=nextfreeend;
    freeendeven+=freeendeven+1;
    --valueshift;

    //ensure that nextfreeend is in the range
    while(nextfreeend-low>=range)
      {
      freeendeven>>=1;
      //smallest number of the required oddness >= low
      nextfreeend=((low+freeendeven)&~freeendeven)|(freeendeven+1);
      }

    for(;;)
      {
      if (++intervalbits==(MAXRANGEBITS+8))
        {
        //need to drop a byte
        newl=low&~MAXRANGEMASK;
        low-=newl;
        nextfreeend-=newl;
        //there can only be one number this even in the range.
        //nextfreeend is in the range
        //if nextfreeend is this even, next step must reduce evenness
        freeendeven&=MAXRANGEMASK;
        intervalbits-=8;
        }

      if (range>(MAXRANGE>>1))
        break;  //finished scaling range

      //scale again
      low+=low;
      range+=range;
      nextfreeend+=nextfreeend;
      freeendeven+=freeendeven+1;
      --valueshift;
      }
    }
  else
    {
      //ensure that nextfreeend is in the range
    while(nextfreeend-low>=range)
      {
      freeendeven>>=1;
      //smallest number of the required oddness >= low
      nextfreeend=((low+freeendeven)&~freeendeven)|(freeendeven+1);
      }
    }

  //read input until we have enough significant bits for next operation
  while(valueshift<=0)
    {
    //need new bytes
    valueshift+=8;
    value=(value<<8)|m_in.Get();
    }

  return;
  } 

bool ArithmeticDecoder::FillQ(IOByteQ *dest)
  {
  BYTE *d=dest->Put(256);
  BYTE *e=d+256;

  while(d<e)
    {
    if (AtEnd())
      {
      dest->Unput(e-d);
      return false;
      }
    *d++=model->Decode(this);
    }
  return true;
  }


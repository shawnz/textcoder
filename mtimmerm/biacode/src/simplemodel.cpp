//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  For commercial purposes, mail me at matt@timmermans.org, and we'll talk.
//===========================================================================

#include "simplemodel.h"

//===========================================================================
//  SimpleAdaptiveModel: a demonstration model (not too useful) to use with
//    ArithmeticEn/Decoder
//===========================================================================


SimpleAdaptiveModel::SimpleAdaptiveModel(int numsymbols)
  {
  int i;
  //round numsymbols to a power of two
  for (symzeroindex=1;symzeroindex<numsymbols;symzeroindex+=symzeroindex);
  //probability heap is twice this big
  probheap=new U32[symzeroindex<<1];
  for (i=symzeroindex<<1;i--;)
    probheap[i]=0;
  prob1=0;
  //initial probabilities
  for (i=numsymbols;i--;)
    AddP(i,1);

  //set up window
  for (i=4096;i--;)
    window[i]=-1; //no symbol
  w0=window;
  w1=w0+1024;
  w2=w0+2048;
  w3=w0+3072;
  };

SimpleAdaptiveModel::~SimpleAdaptiveModel()
  {
  delete [] probheap;
  }

void SimpleAdaptiveModel::Update(int symbol)
  {
  w1=((w1==window)?w1+4095:w1-1);
  if (*w1>=0) //6
    SubP(*w1,2);
  w2=((w2==window)?w2+4095:w2-1);
  if (*w2>=0) //4
    SubP(*w2,1);
  w3=((w3==window)?w3+4095:w3-1);
  if (*w3>=0) //3
    SubP(*w3,1);
  w0=((w0==window)?w0+4095:w0-1);
  if (*w0>=0) //2
    SubP(*w0,2);
  *w0=symbol;
  AddP(symbol,6);
  }


void SimpleAdaptiveModel::Reset()
  {
  int *w,*lim;
  lim=window+4095;

  for (w=w0;w!=w1;w=(w==lim?window:w+1))
    {
    if (*w<0)
      goto DONE;
    SubP(*w,6);*w=-1;
    }
  for (w=w1;w!=w2;w=(w==lim?window:w+1))
    {
    if (*w<0)
      goto DONE;
    SubP(*w,4);*w=-1;
    }
  for (w=w2;w!=w3;w=(w==lim?window:w+1))
    {
    if (*w<0)
      goto DONE;
    SubP(*w,3);*w=-1;
    }
  for (w=w3;w!=w0;w=(w==lim?window:w+1))
    {
    if (*w<0)
      goto DONE;
    SubP(*w,2);*w=-1;
    }
 
  DONE:
  return;
  }


void SimpleAdaptiveModel::GetSymRange(int symbol,U32 *newlow,U32 *newhigh) const
  {
  int i,bit=symzeroindex;
  U32 low=0;

  for (i=1;i<symzeroindex;)
    {
    bit>>=1;
    i+=i;
    if (symbol&bit)
      low+=probheap[i++];
    }
  *newlow=low;
  *newhigh=low+probheap[i];
  }

int SimpleAdaptiveModel::GetSymbol(U32 p,U32 *newlow,U32 *newhigh) const
  {
  int i;
  U32 low=0;

  for (i=1;i<symzeroindex;)
    {
    i+=i;
    if ((p-low)>=probheap[i])
      low+=probheap[i++];
    }
  *newlow=low;
  *newhigh=low+probheap[i];
  return(i-symzeroindex);
  }


void SimpleAdaptiveModel::AddP(int sym,U32 n)
  {
  for (sym+=symzeroindex;sym;sym>>=1)
    probheap[sym]+=n;
  prob1=probheap[1];
  }

void SimpleAdaptiveModel::SubP(int sym,U32 n)
  {
  for (sym+=symzeroindex;sym;sym>>=1)
    probheap[sym]-=n;
  prob1=probheap[1];
  }


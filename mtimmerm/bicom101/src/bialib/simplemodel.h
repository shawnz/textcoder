//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//
//  simplemodel.h
//
//  SimpleAdaptiveModel: a demonstration model to use with ArithmeticEn/Decoder
//  This model isn't really too useful in real life.
//===========================================================================

#ifndef o4tv2mxugi3nv22ufqerzstu334qfamu
#define o4tv2mxugi3nv22ufqerzstu334qfamu

#include "arithmetic.h"
#include "exclude.h"

//===========================================================================
//  ProbHeapModelBase: a base class for order0 models that use a probability
//    heap 
//===========================================================================

class ProbHeapModelBase : public ArithmeticModel
  {
  public:
  ProbHeapModelBase();
  ~ProbHeapModelBase();

  public: //from ArithmeticModel
  virtual void Encode(BYTE symbol,ArithmeticEncoder *dest);
  virtual BYTE Decode(ArithmeticDecoder *src);

  public:
  //for incorporation into other models -- these ones don't update!
  void Encode(BYTE symbol,ArithmeticEncoder *dest,ExclusionSet &excl);
  BYTE Decode(ArithmeticDecoder *src,ExclusionSet &excl);

  virtual void Update(int symbol) = 0;

  protected:
  void AddP(int symbol,unsigned n);
  void SubP(int symbol,unsigned n);
  unsigned SymP(int symbol)
    {return(probheap[symzeroindex+symbol]);}
  unsigned P1()
    {return(probheap[1]);}
  unsigned *probheap;
  int symzeroindex;
  };


//===========================================================================
//  SimpleAdaptiveModel : An adaptive order0 model that only remembers the
//    last 4096 symbols.  More recent symbols are given higher weights
//===========================================================================

class SimpleAdaptiveModel : public ProbHeapModelBase
  {
  public:
  SimpleAdaptiveModel();

  void Reset();

  public: //from ProbHeapModelBase
  void Update(int symbol);

  private:
  int window[4096],*w0,*w1,*w2,*w3;
  };


//===========================================================================
//  SimpleDecayModel : An adaptive order0 model that decays old counts
//    gradually
//===========================================================================

class SimpleDecayModel : public ProbHeapModelBase
  {
  public:
  SimpleDecayModel(unsigned inicounts=1,unsigned decayshift=8);
  public: //from ProbHeapModelBase
  void Update(int symbol);
  unsigned m_decayshift;
  };

#endif

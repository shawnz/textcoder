//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  For commercial purposes, mail me at matt@timmermans.org, and we'll talk.
//
//  simplemodel.h
//
//  SimpleAdaptiveModel: a demonstration model to use with ArithmeticEn/Decoder
//  This model isn't really too useful in real life.
//===========================================================================

#ifndef o4tv2mxugi3nv22ufqerzstu334qfamu
#define o4tv2mxugi3nv22ufqerzstu334qfamu

#include "arithmetic.h"


class SimpleAdaptiveModel : public ArithmeticModel
  {
  public:
  SimpleAdaptiveModel(int numsymbols);
  ~SimpleAdaptiveModel();

  void Update(int symbol);
  void Reset();

  public: //from ArithmeticModel
  //will newlow,newhigh | 0 <= newlow < newhigh <= ProbOne()
  virtual void GetSymRange(int symbol,U32 *newlow,U32 *newhigh) const;
  //must call with 0 <= p <= ProbOne()
  //will newlow,newhigh | 0 <= newlow < newhigh <= ProbOne()
  virtual int GetSymbol(U32 p,U32 *newlow,U32 *newhigh) const;
  private:
  void AddP(int symbol,U32 n);
  void SubP(int symbol,U32 n);
  U32 *probheap;
  int symzeroindex;
  int window[4096],*w0,*w1,*w2,*w3;
  };

#endif

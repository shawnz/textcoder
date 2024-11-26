//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//===========================================================================

#include "simplemodel.h"

//===========================================================================
//  ProbHeapModelBase: a base class for order0 models that use a probability
//    heap
//===========================================================================

ProbHeapModelBase::ProbHeapModelBase()
{
    int i;
    symzeroindex = 256; // number of symbols
    // probability heap is twice this big
    probheap = new unsigned[symzeroindex << 1];
    for (i = symzeroindex << 1; i--;)
        probheap[i] = 0;
    // initial probabilities
    for (i = 256; i--;)
        AddP(i, 1);
};

ProbHeapModelBase::~ProbHeapModelBase()
{
    delete[] probheap;
}

void ProbHeapModelBase::Encode(BYTE sym, ArithmeticEncoder *dest)
{
    int symbol = sym;
    int i, bit = symzeroindex;
    unsigned low = 0;

    for (i = 1; i < symzeroindex;)
    {
        bit >>= 1;
        i += i;
        if (symbol & bit)
            low += probheap[i++];
    }
    dest->Encode(probheap[1], low, low + probheap[i]);
    Update(symbol);
}

void ProbHeapModelBase::Encode(BYTE sym, ArithmeticEncoder *dest, ExclusionSet &ex)
{
    int symbol = sym;
    int i, bit = symzeroindex;
    unsigned low = 0;
    unsigned expl, exph;
    const BYTE *exs, *exe;

    for (i = 1; i < symzeroindex;)
    {
        bit >>= 1;
        i += i;
        if (symbol & bit)
            low += probheap[i++];
    }
    for (
        exs = ex.Sort(), exe = exs + ex.NumExcls(), expl = exph = 0;
        exs != exe;
        ++exs)
    {
        if (*exs > sym)
            exph += SymP(*exs);
        else
            expl += SymP(*exs);
    }
    low -= expl;
    dest->Encode(probheap[1] - (expl + exph), low, low + probheap[i]);
}

BYTE ProbHeapModelBase::Decode(ArithmeticDecoder *src)
{
    int i;
    BYTE ret;
    unsigned low = 0;
    unsigned p = src->GetP(probheap[1]);

    for (i = 1; i < symzeroindex;)
    {
        i += i;
        if ((p - low) >= probheap[i])
            low += probheap[i++];
    }
    ret = (BYTE)(i - symzeroindex);
    src->Narrow(probheap[1], low, low + probheap[i]);
    Update(ret);
    return (ret);
}

BYTE ProbHeapModelBase::Decode(ArithmeticDecoder *src, ExclusionSet &ex)
{
    int i;
    BYTE ret;
    unsigned low = 0, tp;
    unsigned p1 = probheap[1], p;
    const BYTE *exs = ex.Sort();
    const BYTE *exe = exs + ex.NumExcls();

    while (exe != exs)
    {
        --exe;
        p1 -= SymP(*exe);
    }
    exe = exs + ex.NumExcls();
    p = src->GetP(p1);

    for (i = 1; i < symzeroindex;)
    {
        i += i;
        if ((p - low) >= probheap[i])
            low += probheap[i++];
    }
    p -= low;
    while (
        (exs != exe) &&
        (*exs <= (i - symzeroindex)))
    {
        tp = SymP(*exs);
        p += tp;
        low -= tp;
        ++exs;
        while (p >= (tp = probheap[i]))
        {
            p -= tp;
            low += tp;
            ++i;
        }
    }
    ret = (BYTE)(i - symzeroindex);
    src->Narrow(p1, low, low + probheap[i]);
    return (ret);
}

void ProbHeapModelBase::AddP(int sym, unsigned n)
{
    for (sym += symzeroindex; sym; sym >>= 1)
        probheap[sym] += n;
}

void ProbHeapModelBase::SubP(int sym, unsigned n)
{
    for (sym += symzeroindex; sym; sym >>= 1)
        probheap[sym] -= n;
}

//===========================================================================
//  SimpleAdaptiveModel : An adaptive order0 model that only remembers the
//    last 4096 symbols.  More recent symbols are given higher weights
//===========================================================================

SimpleAdaptiveModel::SimpleAdaptiveModel()
{
    int i;
    symzeroindex = 256; // number of symbols
    // probability heap is twice this big
    probheap = new unsigned[symzeroindex << 1];
    for (i = symzeroindex << 1; i--;)
        probheap[i] = 0;
    // initial probabilities
    for (i = 256; i--;)
        AddP(i, 1);

    // set up window
    for (i = 4096; i--;)
        window[i] = -1; // no symbol
    w0 = window;
    w1 = w0 + 1024;
    w2 = w0 + 2048;
    w3 = w0 + 3072;
}

void SimpleAdaptiveModel::Reset()
{
    int *w, *lim;
    lim = window + 4095;

    for (w = w0; w != w1; w = (w == lim ? window : w + 1))
    {
        if (*w < 0)
            goto DONE;
        SubP(*w, 6);
        *w = -1;
    }
    for (w = w1; w != w2; w = (w == lim ? window : w + 1))
    {
        if (*w < 0)
            goto DONE;
        SubP(*w, 4);
        *w = -1;
    }
    for (w = w2; w != w3; w = (w == lim ? window : w + 1))
    {
        if (*w < 0)
            goto DONE;
        SubP(*w, 3);
        *w = -1;
    }
    for (w = w3; w != w0; w = (w == lim ? window : w + 1))
    {
        if (*w < 0)
            goto DONE;
        SubP(*w, 2);
        *w = -1;
    }

DONE:
    return;
}

void SimpleAdaptiveModel::Update(int symbol)
{
    w1 = ((w1 == window) ? w1 + 4095 : w1 - 1);
    if (*w1 >= 0) // 6
        SubP(*w1, 2);
    w2 = ((w2 == window) ? w2 + 4095 : w2 - 1);
    if (*w2 >= 0) // 4
        SubP(*w2, 1);
    w3 = ((w3 == window) ? w3 + 4095 : w3 - 1);
    if (*w3 >= 0) // 3
        SubP(*w3, 1);
    w0 = ((w0 == window) ? w0 + 4095 : w0 - 1);
    if (*w0 >= 0) // 2
        SubP(*w0, 2);
    *w0 = symbol;
    AddP(symbol, 6);
}

//===========================================================================
//  SimpleDecayModel : An adaptive order0 model that decays old counts
//    gradually
//===========================================================================

SimpleDecayModel::SimpleDecayModel(unsigned inicounts, unsigned decayshift)
{
    m_decayshift = decayshift;
    int i;
    for (i = symzeroindex + 255; i >= symzeroindex; --i)
    {
        probheap[i] = inicounts;
    }
    for (; i; --i)
    {
        probheap[i] = probheap[i << 1] + probheap[(i << 1) + 1];
    }
}

void SimpleDecayModel::Update(int symbol)
{
    AddP(symbol, (P1() >> m_decayshift) + 1);
    if (P1() > MAXP1)
    {
        int i;
        for (i = symzeroindex + 255; i >= symzeroindex; --i)
        {
            probheap[i] = (probheap[i] + 1) >> 1;
        }
        for (; i; --i)
        {
            probheap[i] = probheap[i << 1] + probheap[(i << 1) + 1];
        }
    }
}

//===========================================================================
// Copyright (C) 2000 Matt Timmermans
// Free for non-commercial purposes as long as this notice remains intact.
// To discuss commercial use, mail me at matt@timmermans.org
//
// sufftree.h
//
// This file started life as Jesper Larsson's excellent sliding window
// suffix tree.  Now it implements a modified PPM* model for the bicom
// bijective compressor.  Much of the original elegance of Jesper's code
// has been destroyed with the addition of new functionality.  In particular,
// the clever hashing scheme for children was ditched, because I had to keep
// linked lists anyway.
//
// This model does a few things differently:
//
// Contexts are unbounded.  We start at the longest context and escape
// through shorter ones.  The first non-deterministic context we look at is
// the one with the highest P(MPS), i.e., we use Charle's blooks Local Order
// Estimation.
// To avoid being too slow, we look at onlythe longest determinstic context,
// and at most SuffixTreeModel::SuffixTreeModel::MAXWALK non-deterministic
// contexts.  We will only escape through SuffixTreeModel::MAXNONDET
// non-deterministic contexts.
// We also stop escaping when we hit a context that could result in more than
// MAXEXCLS exclusions.  These don't tend to be too useful anyway.
//
// In non-deterministic contexts, escape probabilities are estimated
// originally using PPM method C.  In deterministic contexts, the initial
// estimate is based on the context length.
//
// In both cases, this estimate is used only as an index into a table of 
// adaptive binary contexts that produce the probability we actually use.
//
// The result is like Charles Bloom's SEE, but not as good -- might change
// that later.
//
// This is Jesper's original copyright notice:
//
//============================================================================
// Copyright 1999, N. Jesper Larsson, all rights reserved.
// 
// This file contains an implementation of a sliding window index using a
// suffix tree. It corresponds to the algorithms and representation presented
// in the Ph.D. thesis "Structures of String Matching and Data Compression",
// Lund University, 1999.
//
// This software may be used freely for any purpose. However, when distributed,
// the original source must be clearly stated, and, when the source code is
// distributed, the copyright notice must be retained and any alterations in
// the code must be clearly marked. No warranty is given regarding the quality
// of this software.
//===========================================================================
// (note: ALL of Jesper's code has been altered, so consider this a clear
// marking of that fact ;-)

#ifndef n43f4esskjqa2rfs4ejr1oxj4w5slu4h
#define n43f4esskjqa2rfs4ejr1oxj4w5slu4h

#include "arithmetic.h"
#include "simplemodel.h"
#include "exclude.h"
#include "stdio.h"

static const int SIGNBIT=(int)(((int)-1)^(((unsigned)(int)(-1))>>1));
typedef unsigned long U32;

class SuffixTreeModel : public ArithmeticModel
  {
  public:
  SuffixTreeModel(int maxsize,FILE *statfileout=0);
  ~SuffixTreeModel();

  void Reset();

  void Encode(BYTE symbol,ArithmeticEncoder *dest);
  BYTE Decode(ArithmeticDecoder *src);

  void Update(BYTE c)
    {
    if (m_currentSize==m_maxSize)
      AdvanceTail();
    else
      ++m_currentSize;
    m_leafbuf[front].bufsym=c;
    AdvanceFront();
    }
  enum {MAXEXCLS=192,MAXWALK=70,MAXNONDET=40,ESCADJUST=128,ESCORDERS=7,DETLEAVES=6};
  private:

  struct INode;
  struct LNode;

  class Point
    {
    public:
    bool InEdge()
      {return(r>0);}
    //After Canonize()ing a point, r >0 <=> proj!=0, i.e., point
    //is inside an edge
    INode *ins;  //parent of point
    LNode *r;    //if !=0, then child(ins,a)
    int proj; //length of point - string(ins)
    BYTE sym;
    };

  struct LNode
    {
    U32 count;       //PPM count in context of parent
    LNode *next;     //next child of the same parent
    INode *parent;
    BYTE childcount; //number of children minus one, except
                     //for the root which always has
                     //childcount==1.
                     //leaves always have childcount==0 
    BYTE sym;        //first symbol on the edge to this node
    BYTE isleaf;     //this node is a leaf?1:0
    union
      {
      BYTE cred;     //for INodes, Fiala's credits
      BYTE bufsym;   //for LNodes, buffer symbol.  We only put this
                     //in the LNode structure because we have a byte
                     //to spare here.
      };
    };

  struct INode : public LNode
    {
    int pos;          // start of edge label in m_leafbuf[..].bufsym
    int depth;        // string depth at this node
    INode *suffix;    // suffix link
    U32 contextP1;    // sum of ->count on all children
    LNode *firstchild;// linked list of children, MPS first
    };
  
  //we use these to manage allocation of internal nodes
  struct INodeBlock
    {
    enum{SIZE=4000};
    INodeBlock *next;
    INode data[4000];
    };


  class BitPred
    {
    public:
    void Adjust(bool hit,U32 div)
      {
      div=((hitcount+misscount)/div)+1;
      if (hit)
        hitcount+=div;
      else
        misscount+=div;
      if (hitcount+misscount>=4096)
        {
        hitcount=(hitcount+1)>>1;
        misscount=(misscount+1)>>1;
        }
      }
    U32 hitcount,misscount;
    };


  enum  //statfile vars
    {
    SV_CHILDREN =0x01,
    SV_PREVEXCLS=0x02,
    SV_MCPMISSES=0x04,
    SV_CLEN     =0x08,
    SV_MCPP     =0x10,
    SV_MCPMISSP =0x20,
    SV_HIT      =0x80
    };
  class StatVec
    {
    public:
    BYTE children;  //children-1
    BYTE prevexcls;
    BYTE mcpmisses;
    U32 clen;
    U32 mcpp;
    U32 mcpmissp;

    void Clear()
      {children=prevexcls=mcpmisses=0;clen=mcpp=mcpmissp=0;}
    void Write(FILE *f,bool hit);
    };

  INode *NewIBlock();

  INode *NewI(int depth,int pos)
    {
    INode *ret;
    if (ret=m_ifreelist)
      m_ifreelist=(INode *)ret->next;
    else
      ret=NewIBlock();
    ret->count=0;
    ret->next=0;
    ret->parent=0;
    ret->pos=pos;
    ret->depth=depth;
    ret->suffix=0;
    ret->childcount=(BYTE)-1;
    ret->contextP1=0;
    ret->firstchild=0;
    ret->isleaf=0;
    ret->cred=1;
    return(ret);
    }

  void DelI(LNode *n)
    {
    n->next=m_ifreelist;
    m_ifreelist=(INode *)n;
    }

  LNode *GetChild(INode *parent,BYTE c);

  void CreateEdge(INode *parent,LNode *child,BYTE c,U32 inicounts);
  U32 DeleteEdge(INode *parent,LNode *child);
  U32 IncMPC(INode *parent,LNode *child);
    

  void Canonize(SuffixTreeModel::Point *p);
  void UpdateCredits(INode *v,int i);
  void AdvanceFront();
  BYTE Walk(BYTE c,ArithmeticEncoder *dest,ArithmeticDecoder *src);
  void AdvanceTail();

  int m_maxSize;
  int m_currentSize;

  Point ap; //active point
  int front, tail;         /* limits of window.*/
  ExclusionSet m_excl;
  SimpleAdaptiveModel order0;
  FILE *m_statfile;
  StatVec sv;
  BitPred escapepredictors[63][ESCORDERS],esc2pred[63][ESCORDERS],detpred[20][DETLEAVES];
  INodeBlock *m_iblocks;
  INode *m_ifreelist;
  INode *m_rootnode;
  LNode *m_leafbuf;  //leaves and buffer
  LNode *m_extracountnode;
  };


#endif


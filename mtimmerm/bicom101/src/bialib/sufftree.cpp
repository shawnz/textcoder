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

#include "sufftree.h"

#include <stdlib.h>
#include <limits.h>
#include <time.h>

#ifdef _DEBUG
#include <assert.h>
#define _ASSERT(x) assert(x)
#else
#define _ASSERT(x)
#endif

#define RANDOMIZE_NODE_ORDER 1
#define K (UCHAR_MAX+1)

/* INode numbering:
  
   INode 0 is nil.
   INode 1 is root.
   Nodes 2...m_maxSize-1 are non-root internal nodes.
   Nodes m_maxSize...2*m_maxSize-1 are leaves.*/



/* Macros used to keep indices inside the circular buffer (avoiding
   modulo operations for speed). M0 is for subtractions to stay
   nonnegative, MM for additions to stay below m_maxSize.*/
#define M0(i)           ((i)<0 ? (i)+m_maxSize : (i))
#define MM(i)           ((i)<m_maxSize ? (i) : (i)-m_maxSize)

//these constants were gathered by examining the contexts after processing
//book2 -- they don't really make too much difference, as long as they're
//not silly.  The rows are log(# of leaves).  It's interesting how little
//difference the # of leaves in a deterministic context makes.
static int dini[20*8]=
  {
  25,29,30,33,37,42,44,42,45,47,45,48,49,50,53,59,60,61,61,61,
  23,31,26,31,38,45,46,42,46,50,47,49,52,49,53,59,60,61,61,61,
  23,31,31,39,44,48,52,50,54,52,54,55,57,55,57,62,61,62,61,61,
  23,28,33,46,51,54,54,52,54,56,57,56,56,58,58,62,61,63,63,59,
  16,27,32,47,52,53,53,56,56,58,58,53,57,60,61,62,63,63,63,57,
  25,33,38,56,52,53,56,55,58,54,57,56,55,61,62,63,63,62,63,61,
  25,33,38,56,52,53,56,55,58,54,57,56,55,61,62,63,63,62,63,61,
  25,33,38,56,52,53,56,55,58,54,57,56,55,61,62,63,63,62,63,61
  };

SuffixTreeModel::SuffixTreeModel(int windowSize,FILE *statfileout)
  {
  m_iblocks=0;
  m_ifreelist=0;
  m_rootnode=0;

  m_leafbuf=new LNode[windowSize];
  m_maxSize=windowSize;

  if (m_statfile=statfileout)
    {
    //write number of short vars
    putc(3,m_statfile);
    }

  Reset();
  }


void SuffixTreeModel::Reset()
  {
  int i,j;

    {
    //free internal nodes
    LNode *n=m_rootnode;
    INode *in;
    while(n)
      {
      if (n->isleaf)
        {n=n->next;continue;}
      in=(INode *)n;
      while(n=in->firstchild)
        {
        in->firstchild=n->next;
        n->next=in->next;
        in->next=n;
        }
      n=in->next;
      DelI(in);
      }
    }
  m_rootnode=NewI(0,0);
  m_rootnode->childcount=1;  //stays 1 forever

  m_currentSize=0;

  //initialize active point.
  ap.ins=m_rootnode;
  
  ap.proj=0;
  ap.r=0;

  //initialize window limits.
  tail=front=0;
  
  m_extracountnode=0;

  //init escape predictors
  for(i=0;i<20;++i)
    {
    for(j=0;j<DETLEAVES;j++)
      {
      detpred[i][j].hitcount=dini[i+j*20];
      detpred[i][j].misscount=64-dini[i+j*20];
      }
    }

  for (i=0;i<63;++i)
    {
    escapepredictors[i][0].hitcount=(63-i);
    escapepredictors[i][0].misscount=(i+1);
    esc2pred[i][0]=escapepredictors[i][0];
    for (j=1;j<ESCORDERS;j++)
      esc2pred[i][j]=escapepredictors[i][j]=esc2pred[i][0];
    }
  }


SuffixTreeModel::~SuffixTreeModel()
  {
#ifdef _DEBUG
    {
    long a,b;
    unsigned i,j;
    for (j=0;j<DETLEAVES;++j)
      {
      for (i=0;i<20;i++)
        {
        a=detpred[i][j].hitcount;
        b=detpred[i][j].misscount+a;
        a=(a*64+(b>>1))/b;
        if (a>63)
          a=63;
        if (a<1)
          a=1;
        printf("%2ld,",a);
        }
      printf("\n");
      }
    }
#endif
  INodeBlock *iblock;
  while(iblock=m_iblocks)
    {
    m_iblocks=iblock->next;
    delete iblock;
    }
  delete [] m_leafbuf;
  }

SuffixTreeModel::INode *SuffixTreeModel::NewIBlock()
  {
  INodeBlock *block=new INodeBlock;
  INode *s,*e;
  block->next=m_iblocks;
  m_iblocks=block;
  s=block->data;
  e=s+INodeBlock::SIZE-1;
  while(e!=s)
    {
    e->next=m_ifreelist;
    m_ifreelist=e;
    --e;
    }
  return s;
  }


SuffixTreeModel::LNode *SuffixTreeModel::GetChild(SuffixTreeModel::INode *parent,BYTE c)
  {
  LNode **pos=&(parent->firstchild),**ipos,*ret;
  if (!*pos)
    return 0;
  if ((*pos)->sym==c)
    return(*pos);
  pos=&((*pos)->next);
  if (!*pos)
    return 0;
  if ((*pos)->sym==c)
    return(*pos);

  ipos=pos;ret=0;
  for (pos=&((*pos)->next);*pos;pos=&((*pos)->next))
    {
    if ((*pos)->sym==c)
      {
      ret=*pos;
      *pos=ret->next;
      ret->next=*ipos;
      *ipos=ret;
      break;
      }
    }
  return ret;
  }


/* canonize subroutine:

   r is return value. To avoid unnecessary access to the child list, r is
   preserved between calls. If r is not 0 it is assumed that
   r==child(ins, a), and (ins, r) is the edge of the insertion point.*/
void SuffixTreeModel::Canonize(SuffixTreeModel::Point *p)
  {
  int depthdiff;
  if (p->proj && p->ins==0)
    {
    //the root is the child of nil for all symbols
    p->ins=m_rootnode; --(p->proj); p->r=0;
    }
  while (p->proj)
    {
    if (!p->r)
      {
      p->sym=m_leafbuf[M0(front-p->proj)].bufsym;
      p->r=GetChild(p->ins, p->sym);
      }
    if (p->r->isleaf) //r is leaf
      break;
    depthdiff=((INode *)(p->r))->depth-p->ins->depth;
    if (p->proj<depthdiff)
      {
      // insertion point doesn't extend all the way to r
      break;
      }
    p->proj-=depthdiff; p->ins=(INode *)p->r; p->r=0;
    }
  }

void SuffixTreeModel::CreateEdge
  (
  SuffixTreeModel::INode *parent,
  SuffixTreeModel::LNode *child,
  BYTE c,U32 inicounts
  )
  {
  LNode **pos;
  child->parent=parent;
  child->count=inicounts;
  child->sym=c;
  parent->contextP1+=inicounts;
  parent->childcount++;
  pos=&(parent->firstchild);
  if (*pos&&((*pos)->count>inicounts))
    pos=&((*pos)->next);
  child->next=*pos;
  *pos=child;
  }


U32 SuffixTreeModel::DeleteEdge
  (
  SuffixTreeModel::INode *parent,
  SuffixTreeModel::LNode *child
  )
  {
  U32 occurs=child->count;
  LNode **pos=&(parent->firstchild);
  if (*pos==child)  //removing MPS
    {
    parent->childcount--;
    parent->contextP1-=occurs;    
    if (*pos=child->next)
      {
      LNode **bestpos=pos;
      U32 bestcount=(*pos)->count;
      for(pos=&((*pos)->next);*pos;pos=&((*pos)->next))
        {
        if ((*pos)->count>bestcount)
          {
          bestcount=(*pos)->count;
          bestpos=pos;
          }
        }
      child=*bestpos;
      *bestpos=child->next;
      child->next=parent->firstchild;
      parent->firstchild=child;
      }
    }
  else  //removing other child
    {
    for(pos=&((*pos)->next);*pos&&(*pos!=child);pos=&((*pos)->next));
    _ASSERT(*pos);
    if (*pos == child)
      {
      parent->childcount--;
      parent->contextP1-=occurs;    
      *pos=child->next;
      }
    }
  return occurs;
  }


U32 SuffixTreeModel::IncMPC
  (
  SuffixTreeModel::INode *parent,
  SuffixTreeModel::LNode *child
  )
  {
  child->count++;
  parent->contextP1++;
  if ((parent->firstchild!=child)&&(child->count>parent->firstchild->count))
    {
    //new MPS
    LNode *n;
    while((n=parent->firstchild)!=child)
      {
      parent->firstchild=n->next;
      n->next=child->next;
      child->next=n;
      }
    }
  return(child->count);
  }

/* Macro for Update subroutine:

   Send credits up the tree, updating pos values, until a nonzero credit
   is found. Sign bit of suf links is used as credit bit.*/
void SuffixTreeModel::UpdateCredits(SuffixTreeModel::INode *v,int i)
  {
  INode *u;
  int d;
  int j, ii=M0(i-tail), jj;
  while (v!=m_rootnode)
    {
    u=v->parent;
    d=u->depth;
    j=M0(v->pos-d);
    jj=M0(j-tail);
    if (ii>jj)
      {
      v->pos=MM(i+d);
      }
    else
      {
      i=j; ii=jj;
      }
    if (!v->cred)
      {
      v->cred=1;
      break;
      }
    v->cred=0;
    v=u;
    }
  }


void SuffixTreeModel::Encode(BYTE symbol,ArithmeticEncoder *dest)
  {
  Walk(symbol,dest,0);
  Update(symbol);
  }

BYTE SuffixTreeModel::Decode(ArithmeticDecoder *src)
  {
  BYTE ret=Walk(0,0,src);
  Update(ret);
  return ret;
  }

static U32 iroot(U32 n)
  {
  //the easy-but-not-quite-optimal way
  U32 ret=0,bit;

  for (bit=1;bit&&(bit*bit<=n);bit<<=1)
    {ret=bit;}
  for (bit=ret>>1;bit;bit>>=1)
    {
    ret+=bit;
    if (ret*ret>n)
      ret-=bit;
    }
  return ret;
  }


inline void put32(U32 i,FILE *f)
  {
  putc((int)(i&255),f);
  putc((int)((i>>8)&255),f);
  putc((int)((i>>16)&255),f);
  putc((int)((i>>24)&255),f);
  }


BYTE SuffixTreeModel::Walk(BYTE c,ArithmeticEncoder *dest,ArithmeticDecoder *src)
  {
  INode *insnode;
  LNode *edge;
  BitPred *epred;
  U32 cesc,chit,p1,clow,nl;
  bool escaped=false;
  Point context;
  double bestmpsp;
  INode *cnode[MAXNONDET];
  unsigned i,numcontexts,cnum;
  unsigned exclsleft;
  unsigned numnew;
  LNode *newedges[MAXEXCLS];
  BYTE prediction;
  
  m_excl.Clear();
  Canonize(&ap);
  context=ap;

  if (m_statfile)
    sv.Clear();

  if (context.InEdge())  //deterministic context
    {
    int j;
    U32 clen;
    _ASSERT(!!context.proj);

    insnode=context.ins;
    if (context.r->isleaf)
      {
      j=MM((context.r-m_leafbuf)+insnode->depth);
      }
    else
      {
      j=((INode *)context.r)->pos;
      }
    prediction=m_leafbuf[MM(j+context.proj)].bufsym;
    clen=insnode->depth+context.proj;
    if (clen<1)
      clen=1;

    if (!insnode->firstchild)
      goto DETMISS;

    nl=insnode->firstchild->count;

    if (m_statfile)
      {
      sv.clen=clen;
      sv.mcpp=nl;
      }

    cesc=clen;
    if (cesc)
      --cesc;
    if (cesc>=8)
      {
      cesc=((cesc-8)>>1)+8;
      chit=12;
      while(cesc>=chit)
        {
        cesc=((cesc-chit)>>1)+chit;
        chit+=2;
        }
      }
    if (cesc>=20)
      cesc=19;

    if (insnode==m_extracountnode)
      --nl;

    if (nl)
      --nl;
    if (nl>=2)
      {
      for(chit=(nl-2),nl=2;chit;chit>>=2,++nl);
      if (nl>=DETLEAVES)
        nl=DETLEAVES-1;
      }
    epred=&(detpred[cesc][nl]);
    cesc=epred->misscount;
    chit=epred->hitcount;

    p1=cesc+chit;
    if (src)
      {
      escaped=src->GetP(p1)>=chit;
      if (escaped)
        src->Narrow(p1,chit,p1);
      else
        {
        src->Narrow(p1,0,chit);
        c=prediction;
        }
      }
    else
      escaped=(c!=prediction);

    epred->Adjust(!escaped,ESCADJUST);

    if (dest)
      {
      if (escaped)
        dest->Encode(p1,chit,p1);
      else
        dest->Encode(p1,0,chit);
      }

    if (m_statfile)
      sv.Write(m_statfile,!escaped);

    if (!escaped)
      return c;

    m_excl.Exclude(prediction);

    DETMISS:
    //shorten context until non-deterministic
    for (;;)
      {
      if (insnode->depth<1)
        goto ORDER0;
      context.ins=insnode->suffix;
      context.r=0;
      Canonize(&context);
      if (!context.InEdge())
        break;
      insnode=context.ins;
      }
    }
  
  //we are not in a deterministic context
  //make the list of contexts to escape through

  numcontexts=0;
  double mpsp;
  for (i=0;i<MAXWALK;++i)
    {
    if ((!context.ins)||(context.ins==m_rootnode))
      break;
    insnode=context.ins;
    if (insnode->firstchild)
      {
      mpsp=(double)insnode->firstchild->count;
      mpsp/=(double)insnode->contextP1;
      if (!numcontexts)
        bestmpsp=mpsp;
      else if(mpsp>bestmpsp)
        {
        numcontexts=0;
        bestmpsp=mpsp;
        }
      if (numcontexts>=MAXNONDET)
        break;
      cnode[numcontexts]=context.ins;
      ++numcontexts;
      }
    context.ins=insnode->suffix;
    Canonize(&context);
    }

  //walk the contexts to encode the symbol

  exclsleft=MAXEXCLS-m_excl.NumExcls();
  
  for(cnum=0;(cnum<numcontexts)&&exclsleft;++cnum)
    {
    escaped=true;
    insnode=cnode[cnum];
    cesc=((unsigned)insnode->childcount)+1;
    chit=0;
    numnew=0;
    if (m_statfile)
      {
      sv.Clear();
      sv.clen=insnode->depth;
      sv.prevexcls=(BYTE)m_excl.NumExcls();
      sv.children=insnode->childcount;
      }
    for (edge=insnode->firstchild;edge&&exclsleft;edge=edge->next)
      {
      prediction=edge->sym;
      if (!m_excl.Exclude(prediction))
        {
        //was already excluded
        continue;
        }
      --exclsleft;
      newedges[numnew++]=edge;
      chit+=edge->count;
      if (prediction==c)
        escaped=false;
      }
    if (!numnew)  //no new symbols
      continue;
    if (edge)
      {m_excl.Backup(numnew);break;}

    p1=chit;

    //get escape estimator
      {
      int orderindex=insnode->depth;
      if (orderindex>=ESCORDERS)
        orderindex=ESCORDERS-1;
      else if(orderindex)
        --orderindex;
      cesc=(cesc<<6)/(cesc+chit);
      if (cesc>0)
        --cesc;
      if (cesc>=63)
        cesc=62;
      if (!cnum)
        epred=&(escapepredictors[cesc][orderindex]);
      else
        epred=&(esc2pred[cesc][orderindex]);
      cesc=epred->misscount;
      chit=epred->hitcount;
      }

    if (src)  //decode escape
      escaped=(src->GetP(cesc+chit)>=chit);

    epred->Adjust(!escaped,ESCADJUST);

    if (m_statfile)
      {
      sv.clen=insnode->depth;
      sv.children=insnode->childcount;
      _ASSERT(((unsigned)sv.children)+1>=m_excl.NumExcls());
      sv.mcpmisses=(BYTE)((((unsigned)sv.children)+1)-m_excl.NumExcls());
      for (i=0;i<numnew;i++)
        sv.mcpp+=newedges[i]->count;
      _ASSERT(insnode->contextP1>=sv.mcpp);
      sv.mcpmissp=insnode->contextP1-sv.mcpp;
      sv.Write(m_statfile,!escaped);
      }

    if (escaped)
      {
      if (src)
        src->Narrow(cesc+chit,chit,cesc+chit);
      if (dest)
        dest->Encode(cesc+chit,chit,cesc+chit);
      continue;
      }
    if (src)
      src->Narrow(cesc+chit,0,chit);
    if (dest)
      dest->Encode(cesc+chit,0,chit);

    //yay! no escape!
    nl=newedges[0]->count>>2; //I have no idea why this is good!!!
    newedges[0]->count+=nl;
    p1+=nl;
    if (src)
      {
      //decode char
      chit=src->GetP(p1);
      clow=0;
      for (i=0;i<numnew;i++)
        {
        edge=newedges[i];
        if (chit<edge->count)
          {
          c=edge->sym;
          chit=edge->count;
          break;
          }
        clow+=edge->count;
        chit-=edge->count;
        }
      _ASSERT(i<numnew);
      }
    else
      {
      clow=0;
      for (i=0;i<numnew;i++)
        {
        edge=newedges[i];
        prediction=edge->sym;
        if (c==prediction)
          {
          chit=edge->count;
          break;
          }
        clow+=edge->count;
        }
      _ASSERT(i<numnew);
      }

 
    _ASSERT(p1<MAXP1);


    if (src)
      src->Narrow(p1,clow,clow+chit);
    if (dest)
      dest->Encode(p1,clow,clow+chit);
    _ASSERT(clow+chit<=p1);

    newedges[0]->count-=nl;
    p1-=nl;
    return(c);
    }

  //sigh -- didn't find a match anywhere
  ORDER0:
  if (src)
    c=order0.Decode(src,m_excl);
  else if (dest)
    order0.Encode(c,dest,m_excl);
  order0.Update(c);
  return(c);
  }



/* Function advancefront:

   Moves front, the right endpoint of the window, increasing its size.
*/

void SuffixTreeModel::AdvanceFront()
  {
  INode *u, *v;
  LNode *s;
  int j;
  BYTE b, c;

  v=0;
  c=m_leafbuf[front].bufsym;
  while (1)
    {
    Canonize(&ap);
    if (!ap.r)
      {
      //active point at node.
      m_extracountnode=0;
      if (!ap.ins)
        {
        ap.r=m_rootnode; //root is child of ins for any c
        break;           //endpoint found.
        }
      ap.r=GetChild(ap.ins, c);
      if (ap.r)
        {          
        //ins has a child for c.
        ap.sym=c; //a is first symbol in (ins, r) label
        IncMPC(ap.ins,ap.r);
        //if we split this node, we want to remove the count we just added
        //from the new child
        m_extracountnode=ap.r;
        break;           //endpoint found.
        }
      else
        {
        u=ap.ins; //will add child below u.
        }
      }
    else
      {
      // active point on edge.
      j=(ap.r->isleaf ? MM((ap.r-m_leafbuf)+ap.ins->depth) : ((INode *)ap.r)->pos);
      b=m_leafbuf[MM(j+ap.proj)].bufsym;    // next symbol in (ins, r) label.
      if (c==b)           /* if same as front symbol.*/
        break;           /* endpoint found.*/
      else
        {              /* edge must be split.*/
        u=NewI(ap.ins->depth+ap.proj,M0(front-ap.proj));
        U32 rcounts=DeleteEdge(ap.ins, ap.r);
        CreateEdge(ap.ins, u, ap.sym,rcounts);
        //we added a count when we came into this edge.  That coun't
        //isn't valid for the part that's getting split off, so
        //remove it
        if (m_extracountnode==ap.r)
          --rcounts;
        _ASSERT(rcounts>0);
        //m_extracountnode will get cleared below
        CreateEdge(u, ap.r, b,rcounts);
        if (!ap.r->isleaf)
          ((INode *)ap.r)->pos=MM(j+ap.proj);
        }
      }
    m_extracountnode=0;
    s=m_leafbuf+M0(front-u->depth);
    s->childcount=0;
    s->count=0;
    s->isleaf=1;
    s->next=0;
    s->parent=0;
    s->sym=0;
    CreateEdge(u, s, c,1);   /* add new leaf s.*/
    m_rootnode->childcount=1;  // don't count children of root
    if (u==ap.ins)            /* skip update if new node.*/
      UpdateCredits(u, M0(front-u->depth));
    if (v)
      v->suffix=u;
    v=u;
    ap.ins=ap.ins->suffix;
    ap.r=0;                   /* force getting new r in canonize.*/
    }
  if (v)
    v->suffix=ap.ins;
  ++ap.proj;                   /* move active point down.*/
  front=MM(front+1);
  }


/* Function advancetail:

   Moves tail, the left endpoint of the window, forward by positions
   positions, decreasing its size.*/
void SuffixTreeModel::AdvanceTail()
  {
  INode *u, *w;              /* nodes.*/
  LNode *v,*s;
  BYTE c;
  int i, d;
  U32 oldcounts;

  Canonize(&ap);
  v=m_leafbuf+tail; // the leaf to delete.
  u=v->parent;
  oldcounts=DeleteEdge(u, v);
  if (v==ap.r)
    {
    //replace instead of delete

    i=M0(front-(ap.ins->depth+ap.proj));
    if (m_extracountnode==v)
      {
      m_extracountnode=m_leafbuf+i;
      ++oldcounts;
      }
    CreateEdge(ap.ins, m_leafbuf+i, v->sym,oldcounts);
    UpdateCredits(ap.ins, i);
    ap.ins=ap.ins->suffix;
    ap.r=0;                   /* force getting new r in canonize.*/
    }
  else if (u!=m_rootnode && !u->childcount)
    {
    // u has only one child left, delete it.
    _ASSERT(u->firstchild&&!u->firstchild->next);
    c=u->sym;
    _ASSERT(c==m_leafbuf[u->pos].bufsym);
    w=u->parent;
    d=u->depth-w->depth;
    s=u->firstchild; // the remaining child of u.
    _ASSERT(s->sym==m_leafbuf[MM(u->pos+d)].bufsym);
    if (u==ap.ins)
      {
      ap.ins=w;
      ap.proj+=d;
      ap.sym=c;             // new first symbol of (ins, r) label
      m_extracountnode=0;
      }
    else if (u==ap.r)
      {
      if (m_extracountnode==u)
        m_extracountnode=s;
      ap.r=s;             // new child(ins, a).
      }
    if (u->cred)
      UpdateCredits(w, M0(u->pos-w->depth));
    DeleteEdge(u, s);
    oldcounts=DeleteEdge(w, u);
    CreateEdge(w, s, c,oldcounts);
    if (!s->isleaf)
      ((INode *)s)->pos=M0(((INode *)s)->pos-d);
    DelI(u);
    }
  tail=MM(tail+1);
  }




void SuffixTreeModel::StatVec::Write(FILE *f,bool hit)
  {
  unsigned n=0,i;
  U32 vec[8];
  BYTE code=hit?(BYTE)128:0;
  BYTE bit;

  vec[n++]=children;
  vec[n++]=prevexcls;
  vec[n++]=mcpmisses;
  vec[n++]=clen;
  vec[n++]=mcpp;
  vec[n++]=mcpmissp;

  for (bit=1,i=0;i<n;i++,bit<<=1)
    {
    if (vec[i])
      code|=bit;
    }
  putc(code,f);
  for (bit=1,i=0;i<n;i++,bit<<=1)
    {
    if (vec[i])
      {
      if (i<3)
        putc((BYTE)vec[i],f);
      else
        put32(vec[i],f);
      }
    }

  Clear();
  }



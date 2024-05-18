//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//
//  exclude.h
//
//  This is a little class for remembering exclusions in multi-order models
//
//===========================================================================

#ifndef ce1aclnxqkpuetpge0v4a12v1d0ymaj4
#define ce1aclnxqkpuetpge0v4a12v1d0ymaj4

typedef unsigned char BYTE;

class ExclusionSet
  {
  public:
  ExclusionSet();

  //add an exclusion if it doesn't already exist
  bool Exclude(BYTE sym)  //returns true iff exclusion is new
    {
    if (m_mask[sym])
      return false;
    else
      {
      m_mask[m_excls[m_count++]=sym]=1;
      return true;
      }
    }

  //check if a symbol is excluded
  bool IsExcluded(BYTE sym)
    {return(!!(m_mask[sym]));}

  //clear the exclusion set
  void Clear()
    {
    while(m_count)
      {m_mask[m_excls[--m_count]]=0;}
    }

  //Undo the last n exclusions -- unpredictable if you have
  //called Sort() since you last called Clear()
  void Backup(unsigned n)
    {
    while(n--)
      {m_mask[m_excls[--m_count]]=0;}
    }

  //Count the exclusions
  unsigned NumExcls()
    {return m_count;}

  //Sort the excluded symbols and get a pointer to the array
  const BYTE *Sort()
    {
    QSB(m_excls,m_excls+m_count);
    return(m_excls);
    }

  private:
  static void QSB(BYTE *start,BYTE *end);
  BYTE m_mask[256];
  BYTE m_excls[256];
  unsigned m_count;
  };

#endif

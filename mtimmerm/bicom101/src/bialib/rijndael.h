//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//
//  rijndael.h
//
//  This is a Rijndael encryptor/decryptor, adapted from the AES reference source
//  128-bit (16 byte)blocks.  This can be combined with the trivial encoder/
//  decoder in mtstream.h to create a bijective encryption between byte
//  strings
//
//===========================================================================

#include "mtstream.h"
#include "string.h"


//===========================================================================
//  Rijndael: Block encrypt/decrypt.  Adapted from AES reference source
//===========================================================================

class Rijndael
  {
  public:
  Rijndael()
    {
    SetKey(0,0,true);  //default key
    }

  Rijndael
    (
    const BYTE *key,unsigned keylen /*in bytes*/,bool dopasshash=false
    )
    {
    SetKey(key,keylen,dopasshash);
    }

  void SetKey
    (
    const BYTE *key,unsigned keylen /*in bytes*/,bool dopasshash=false
    );

  //dest and src can be the same in these calls
  void EncryptBlock(BYTE *dest,const BYTE *src,BYTE *cbcbuf=0); //16 bytes!!!
  void DecryptBlock(BYTE *dest,const BYTE *src,BYTE *cbcbuf=0); //16 bytes!!!

  private:
  int ROUNDS;
  enum{MAXKC=256/32,MAXKEYBYTES=MAXKC*4,MAXROUNDS=14};
  int m_keybits;
  bool m_isencryptkey;
  BYTE m_key[MAXKEYBYTES];
  BYTE m_roundkeys[MAXROUNDS+1][4][4];

  void KeySched();
  void EncToDecKey();
  void DecToEncKey()
    {KeySched();} //probably don't need to, but I didn't check
  };


//===========================================================================
//  RijndaelFOEncrypt:  Bijectively encrypt an FO stream
//===========================================================================

class RijndaelFOEncrypt : public IOByteSource
  {
  public:
  RijndaelFOEncrypt
    (
    const BYTE *key,unsigned keylen /*in bytes*/,bool dopasshash=false
    )
    :cipher(key,keylen,dopasshash)
    {
    Clear();
    }
  ~RijndaelFOEncrypt()
    {Clear();}

  void SetSource(IOByteSource *source,bool autodelete=false)
    {Clear();m_in.SetSource(source,autodelete);}

  void Clear()
    {
    m_in.Clear();
    m_start=true;
    m_done=false;
    m_partiallen=0;
    }

  public: //from IOByteSource

  virtual bool FillQ(IOByteQ *dest);

  private:
  Rijndael cipher;
  FOStream m_in;
  unsigned m_partiallen;
  bool m_start,m_done;
  BYTE m_ivbuf[16],m_ivbuf2[16];
  BYTE m_partial[32];
  };

//===========================================================================
//  RijndaelFODecrypt:  Bijectively decrypt an FO stream
//===========================================================================

class RijndaelFODecrypt : public IOByteSource
  {
  public:
  RijndaelFODecrypt
    (
    const BYTE *key,unsigned keylen /*in bytes*/,bool dopasshash=false
    )
    :cipher(key,keylen,dopasshash)
    {
    Clear();
    }
  ~RijndaelFODecrypt()
    {Clear();}

  void SetSource(IOByteSource *source,bool autodelete=false)
    {Clear();m_in.SetSource(source,autodelete);}

  void Clear()
    {
    m_in.Clear();
    m_start=true;
    m_done=false;
    m_partiallen=0;
    }

  public: //from IOByteSource

  virtual bool FillQ(IOByteQ *dest);

  private:
  Rijndael cipher;
  FOStream m_in;
  unsigned m_partiallen;
  bool m_start,m_done;
  BYTE m_ivbuf[16],m_ivbuf2[16];
  BYTE m_partial[32];
  };


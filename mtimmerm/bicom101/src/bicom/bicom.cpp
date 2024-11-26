//===========================================================================
//  Copyright (C) 2000 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  To discuss commercial use, mail me at matt@timmermans.org
//===========================================================================

#include "mtstream.h"
#include "arithmetic.h"
#include "sufftree.h"
#include "trivial.h"
#include "rijndael.h"

#include <stdlib.h>

static char *_callname;

int usage()
{
    char *s;
    for (s = _callname; *s; ++s)
        ;
    for (; (s != _callname) && (s[-1] != '\\') && (s[-1] != ':') && (s[-1] != '/'); --s)
        ;
    fprintf(stderr, "\nBijective compressor V1.01\n");
    fprintf(stderr, "Copyright (C) 2000, Matt Timmermans\n\n");
    fprintf(stderr, "USAGE: %s [-d] [-p passwd] <infile> <outfile>\n\n", s);
    fprintf(stderr, "  -d:  decompress (default is compress)\n");
    fprintf(stderr, "  -p passwd: encrpyt/decrypt(Rijndael) with passphrase\n");
    fprintf(stderr, "     (passwd is hex if it starts with 0x)\n\n");
    return (100);
}

static int Test(const BYTE *passwd, unsigned passwdlen, bool pwhash);

static int _unhex(char *dest, const char *src)
{
    int ret = 0;
    int nibble = 0;
    BYTE c;
    BYTE bits = 0;
    while (c = (BYTE)*src++)
    {
        if (c <= 32)
            continue; // whitespace
        if ((c >= (BYTE)'0') && (c <= (BYTE)'9'))
            c -= (BYTE)'0';
        else if ((c >= (BYTE)'A') && (c <= (BYTE)'F'))
            c -= (BYTE)('A' - 10);
        else if ((c >= (BYTE)'a') && (c <= (BYTE)'f'))
            c -= (BYTE)('a' - 10);
        else
            return (-1); // invalid hex char
        bits = (bits << 4) | c;
        if (++nibble == 2)
        {
            dest[ret++] = bits;
            bits = 0;
            nibble = 0;
        }
    }
    if (nibble)
        dest[ret++] = bits << 4;
    return ret;
}

int main(int argc, char **argv)
{
    char *s;
    bool decomp = false;
    bool test = false;
    char *statfilename = 0;
    char *passwd = 0;
    int passwdlen = 0;
    bool passwdhashalways = false;

    if (argc)
    {
        _callname = *argv++;
        --argc;
    }
    else
        _callname = "biacode";

    while (argc && (s = *argv) && (*s++ == '-'))
    {
        if (!*s)
            return (usage());
        --argc;
        ++argv;

        while (*s)
            switch (*s++)
            {
            case 'd':
            case 'D':
                decomp = true;
                break;

            case 'T':
                test = true;
                break;

            case 'S':
                if (!*s)
                {
                    if (!(argc && (s = *argv)))
                        return (usage());
                    --argc;
                    ++argv;
                }
                statfilename = s;
                for (; *s; s++)
                    ;
                break;

            case 'p':
                if (!*s)
                {
                    if (!(argc && (s = *argv)))
                        return (usage());
                    --argc;
                    ++argv;
                }
                passwd = s;
                for (; *s; s++)
                    ;
                if ((passwd[0] == '0') && ((passwd[1] == 'x') || (passwd[1] == 'X')))
                {
                    passwdlen = _unhex(passwd, passwd + 2);
                    if (passwdlen < 0)
                    {
                        fprintf(stderr, "Invalid character in hex passphrase.\n");
                        return 10;
                    }
                }
                else
                {
                    passwdlen = strlen(passwd);
                    passwdhashalways = true;
                }
                break;

            default:
                return (usage());
            }
    }

    if (test)
    {
        if (argc)
            return (usage());
        return (Test((BYTE *)passwd, passwdlen, passwdhashalways));
    }

    if (argc != 2)
        return (usage());

    {
        FILE *statfile = 0;
        ByteFileSource ifsource;
        if (!ifsource.Open(argv[0]))
        {
            fprintf(stderr, "Could not read file \"%s\"\n", argv[0]);
            return (10);
        }
        FILE *fout = fopen(argv[1], "wb");
        if (!fout)
        {
            fprintf(stderr, "Could not write file \"%s\"\n", argv[1]);
            return (10);
        }
        if (statfilename)
        {
            statfile = fopen(statfilename, "wb");
            if (!statfile)
            {
                fprintf(stderr, "Could not write stat file \"%s\"\n", statfilename);
                fclose(fout);
                return (10);
            }
        }

        SuffixTreeModel model(1 << 20, statfile);
        ByteStream outbytes;

        if (decomp)
        {
            XOR55ByteSource *cmpbytes = new XOR55ByteSource(&ifsource);
            IOByteSource *fo = new TrivialFOEncoder(cmpbytes, true);
            if (passwd)
            {
                RijndaelFODecrypt *rj = new RijndaelFODecrypt((BYTE *)passwd, passwdlen, passwdhashalways);
                rj->SetSource(fo, true);
                fo = rj;
            }
            ArithmeticDecoder *bytes = new ArithmeticDecoder(&model, fo, true);
            outbytes.SetSource(bytes, true);
        }
        else
        {
            IOByteSource *fo = new ArithmeticEncoder(&model, &ifsource);
            if (passwd)
            {
                RijndaelFOEncrypt *rj = new RijndaelFOEncrypt((BYTE *)passwd, passwdlen, passwdhashalways);
                rj->SetSource(fo, true);
                fo = rj;
            }
            TrivialFODecoder *bytes = new TrivialFODecoder(fo, true);
            XOR55ByteSource *bytes55 = new XOR55ByteSource(bytes, true);
            outbytes.SetSource(bytes55, true);
        }
        while (!outbytes.AtEnd())
            putc(outbytes.Get(), fout);

        if (statfile)
            fclose(statfile);
        fclose(fout);
    }

    return (0);
}

static bool testfail()
{
    return (false);
}

static int Test(const BYTE *pw, unsigned pwlen, bool pwhash)
{
    SuffixTreeModel modelin(4096), modelout(4096);
    if (!pw)
        pwlen = 0;
    RijndaelFOEncrypt encr(pw, pwlen, pwhash);
    RijndaelFODecrypt decr(pw, pwlen, pwhash);
    int bytelen, i, inpos;
    BYTE in[10];
    bool ok = true;
    int tick = 0;

    for (bytelen = 0; ok && bytelen < 5; bytelen++)
    {
        printf("Testing %d byte files...", bytelen);
        for (i = 0; i < bytelen; ++i)
            in[i] = 0;
        for (;;) // count through all the files of this length
        {
            if (++tick == 256)
            {
                printf(".");
                tick = 0;
            }

            {
                // compress and expand
                modelin.Reset();
                modelout.Reset();
                MemoryByteSource datain(in, bytelen);
                ArithmeticEncoder foin(&modelin, &datain);
                if (pw)
                    encr.SetSource(&foin);
                TrivialFODecoder mid(pw ? (IOByteSource *)&encr : (IOByteSource *)&foin);
                TrivialFOEncoder foout(&mid);
                if (pw)
                    decr.SetSource(&foout);
                ArithmeticDecoder dataout(&modelout, pw ? (IOByteSource *)&decr : (IOByteSource *)&foout);
                ByteStream test(&dataout);

                for (inpos = 0; inpos < bytelen; ++inpos)
                {
                    if (
                        test.AtEnd() ||
                        (test.Get() != in[inpos]))
                    {
                        ok = testfail();
                        goto DONELEN;
                    }
                }
                if (!test.AtEnd())
                {
                    ok = testfail();
                    goto DONELEN;
                }
            }

            {
                // expand and compress
                modelin.Reset();
                modelout.Reset();
                MemoryByteSource datain(in, bytelen);
                TrivialFOEncoder foin(&datain);
                if (pw)
                    decr.SetSource(&foin);
                ArithmeticDecoder mid(&modelin, pw ? (IOByteSource *)&decr : (IOByteSource *)&foin);
                ArithmeticEncoder foout(&modelout, &mid);
                if (pw)
                    encr.SetSource(&foout);
                TrivialFODecoder dataout(pw ? (IOByteSource *)&encr : (IOByteSource *)&foout);
                ByteStream test(&dataout);

                for (inpos = 0; inpos < bytelen; ++inpos)
                {
                    if (
                        test.AtEnd() ||
                        (test.Get() != in[inpos]))
                    {
                        ok = testfail();
                        goto DONELEN;
                    }
                }
                if (!test.AtEnd())
                {
                    ok = testfail();
                    goto DONELEN;
                }
            }
            // next length

            i = 0;
            for (;;)
            {
                if (i == bytelen)
                    goto DONELEN;
                if (++(in[i]))
                    break;
                ++i;
            }
        }
    DONELEN:
        printf(ok ? "OK\n" : "FAIL!\n");
    }
    return (true);
}

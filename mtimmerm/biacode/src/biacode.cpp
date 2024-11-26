//===========================================================================
//  Copyright (C) 1999 Matt Timmermans
//  Free for non-commercial purposes as long as this notice remains intact.
//  For commercial purposes, mail me at matt@timmermans.org, and we'll talk.
//===========================================================================

#include <iostream>
#include <fstream>
#include <strstream>
#include "arithmetic.h"
#include "simplemodel.h"
#include "foio.h"

static char *_callname;

using namespace std;

int usage()
{
    char *s;
    for (s = _callname; *s; ++s)
        ;
    for (; (s != _callname) && (s[-1] != '\\') && (s[-1] != ':') && (s[-1] != '/'); --s)
        ;
    cerr << endl
         << "Bijective arithmetic encoder V1.2" << endl
         << "Copyright (C) 1999, Matt Timmermans" << endl
         << endl;
    cerr << "USAGE: " << s << " [-d] [-b <blocksize>] <infile> <outfile>" << endl
         << endl;
    cerr << "  -d:  decompress (default is compress)" << endl;
    cerr << "  -b n: compressed blocksize to n bytes (default is 1)" << endl
         << endl;
    return (100);
}

static int Test();

int main(int argc, char **argv)
{
    char *s;
    bool decomp = false;
    bool test = false;
    int blocksize = 1;

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

            case 'b':
            case 'B':
                if (!*s)
                {
                    if (!(argc && (s = *argv)))
                        return (usage());
                    --argc;
                    ++argv;
                }
                blocksize = atoi(s);
                for (; *s; s++)
                    ;
                if (blocksize < 1)
                    return (usage());
                break;

            default:
                return (usage());
            }
    }

    if (test)
    {
        if (argc)
            return (usage());
        return (Test());
    }

    if (argc != 2)
        return (usage());

    {
        ifstream infile(argv[0], ios::in | ios::binary);
        if (infile.fail())
        {
            cerr << "Could not read file \"" << argv[0] << endl;
            return (10);
        }
        ofstream outfile(argv[1], ios::out | ios::binary);
        if (outfile.fail())
        {
            cerr << "Could not write file \"" << argv[0] << endl;
            return (10);
        }

        SimpleAdaptiveModel model(256);
        int sym;

        if (decomp)
        {
            FOBitIStream inbits(infile, blocksize);
            ArithmeticDecoder decoder(inbits);
            for (;;)
            {
                sym = decoder.Decode(&model, true);
                if (sym < 0)
                    break;
                outfile.put((char)(sym));
                model.Update(sym);
            }
        }
        else
        {
            FOBitOStream outbits(outfile, blocksize);
            ArithmeticEncoder encoder(outbits);
            for (;;)
            {
                sym = infile.get();
                if (sym < 0)
                    break;
                encoder.Encode(&model, sym, true);
                model.Update(sym);
            }
            encoder.End();
            outbits.End();
        }

        outfile.close();
        infile.close();
    }

    return (0);
}

static bool testfail()
{
    return (false);
}

static int Test()
{
    SimpleAdaptiveModel model(256);
    int bytelen, i, inpos, outpos, sym;
    char in[10], mid[200], out[200];
    bool ok = true;

    for (bytelen = 0; ok && bytelen < 5; bytelen++)
    {
        cout << "Testing " << bytelen << " byte files...";
        for (i = 0; i < bytelen; ++i)
            in[i] = 0;
        for (;;) // count through all the files of this length
        {
            {
                ostrstream outstr(mid, 200);
                {
                    // compress
                    FOBitOStream outbits(outstr);
                    ArithmeticEncoder encoder(outbits);
                    model.Reset();
                    for (inpos = 0; inpos < bytelen; ++inpos)
                    {
                        sym = (unsigned char)in[inpos];
                        encoder.Encode(&model, sym, true);
                        model.Update(sym);
                    }
                    encoder.End();
                    outbits.End();
                }
                mid[outstr.pcount()] = 0;
                istrstream instr(mid, static_cast<int>(outstr.pcount()));
                {
                    // decompress
                    FOBitIStream inbits(instr);
                    ArithmeticDecoder decoder(inbits);
                    model.Reset();
                    outpos = 0;
                    for (;;)
                    {
                        sym = decoder.Decode(&model, true);
                        if (sym < 0)
                            break;
                        if (
                            (outpos == inpos) ||
                            (((char)sym) != in[outpos]))
                        {
                            ok = testfail();
                            goto DONELEN;
                        }
                        model.Update(sym);
                        ++outpos;
                    }
                    if (inpos != outpos)
                    {
                        ok = testfail();
                        goto DONELEN;
                    }
                }
            }

            {
                in[bytelen] = 0;
                istrstream instr(in, bytelen);
                {
                    // decompress
                    FOBitIStream inbits(instr);
                    ArithmeticDecoder decoder(inbits);
                    model.Reset();
                    outpos = 0;
                    for (;;)
                    {
                        sym = decoder.Decode(&model, true);
                        if (sym < 0)
                            break;
                        mid[outpos++] = (char)sym;
                        model.Update(sym);
                    }
                }
                ostrstream outstr(out, 200);
                {
                    // compress
                    FOBitOStream outbits(outstr);
                    ArithmeticEncoder encoder(outbits);
                    model.Reset();
                    for (inpos = 0; inpos < outpos; ++inpos)
                    {
                        sym = (unsigned char)mid[inpos];
                        encoder.Encode(&model, sym, true);
                        model.Update(sym);
                    }
                    encoder.End();
                    outbits.End();
                }
                if (outstr.pcount() != bytelen)
                {
                    ok = testfail();
                    goto DONELEN;
                }
                for (i = 0; i < bytelen; ++i)
                {
                    if (in[i] != out[i])
                    {
                        ok = testfail();
                        goto DONELEN;
                    }
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
        cout << (ok ? "OK" : "FAIL!") << endl;
    }
    return (true);
}

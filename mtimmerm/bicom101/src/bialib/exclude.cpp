#include "exclude.h"

ExclusionSet::ExclusionSet()
{
    unsigned i;
    m_count = 0;
    for (i = 256; i--;)
        m_mask[i] = 0;
}

// quicksort the exclusions
void ExclusionSet::QSB(BYTE *start, BYTE *end)
{
    BYTE t, cpiv, *l, *h;
    unsigned n;

TOP:
    n = end - start;
    if (n < 3)
    {
        if (n == 2)
        {
            --end;
            if (*end < *start)
            {
                t = *start;
                *start = *end;
                *end = t;
            }
        }
        return;
    }
    h = end - 1;
    l = start + (n >> 1);
    if (*l < *start)
    {
        t = *start;
        *start = *l;
        *l = t;
    }
    if (*h < *start)
    {
        t = *start;
        *start = *h;
        *h = t;
    }
    if (*h < *l)
    {
        t = *l;
        *l = *h;
        *h = t;
    }
    cpiv = *l;
    for (l = start + 1; l != h;)
    {
        if (*l > cpiv)
        {
            --h;
            t = *l;
            *l = *h;
            *h = t;
        }
        else
            ++l;
    }
    if ((end - l) > (l - start))
    {
        QSB(start, l);
        start = l;
        goto TOP;
    }
    else
    {
        QSB(l, end);
        end = l;
        goto TOP;
    }
}
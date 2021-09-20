 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Common code needed by all the various graphics systems.
  *
  * (c) 1996 Bernd Schmidt, Ed Hanway, Samuel Devulder
  */

#include "sysconfig.h"
#include "sysdeps.h"
#include "custom.h"
#include "xwin.h"

#define	RED 0
#define	GRN	1
#define	BLU	2

// cmf: deleted a lot of unneeded code (+146k of memory)

uae_u32 doMask (int p, int bits, int shift)
{
    /* scale to 0..255, shift to align msb with mask, and apply mask */
    uae_u32 val = p << 24;
    if (!bits)
	return 0;
    val >>= (32 - bits);
    val <<= shift;

    return val;
}

static unsigned int doAlpha (int alpha, int bits, int shift)
{
    return (alpha & ((1 << bits) - 1)) << shift;
}

void alloc_colors64k (int rw, int gw, int bw, int rs, int gs, int bs, int aw, int as, int alpha, int byte_swap)
{
    register unsigned int i;
    unsigned int bpp = rw + gw + bw + aw;

    for (i = 0; i < 4096; i++) {
	int r = ((i >> 8) << 4) | (i >> 8);
	int g = (((i >> 4) & 0xf) << 4) | ((i >> 4) & 0x0f);
	int b = ((i & 0xf) << 4) | (i & 0x0f);

	xcolors[i] = doMask(r, rw, rs) | doMask(g, gw, gs) | doMask(b, bw, bs) | doAlpha (alpha, aw, as);

        if (byte_swap) {
	    if (bpp <= 16)
		xcolors[i] = bswap_16 (xcolors[i]);
	    else
		xcolors[i] = bswap_32 (xcolors[i]);
	}

        if (bpp <= 16) {
	    /* Fill upper 16 bits of each colour value
	     * with a copy of the colour. */
	    xcolors[i] = xcolors[i] * 0x00010001;
	}
    }

#ifdef AGA
    /* create AGA color tables */
    for (i = 0; i < 256; i++) {
	xredcolors  [i] = doColor (i, rw, rs) | doAlpha (alpha, aw, as);
	xgreencolors[i] = doColor (i, gw, gs) | doAlpha (alpha, aw, as);
	xbluecolors [i] = doColor (i, bw, bs) | doAlpha (alpha, aw, as);

        if (byte_swap) {
	    if (bpp <= 16) {
		xredcolors  [i] = bswap_16 (xredcolors[i]);
		xgreencolors[i] = bswap_16 (xgreencolors[i]);
		xbluecolors [i] = bswap_16 (xbluecolors[i]);
	    } else {
		xredcolors  [i] = bswap_32 (xredcolors[i]);
		xgreencolors[i] = bswap_32 (xgreencolors[i]);
		xbluecolors [i] = bswap_32 (xbluecolors[i]);
	    }
	}

        if (bpp <= 16) {
	    /* Fill upper 16 bits of each colour value with
	     * a copy of the colour. */
	    xredcolors  [i] = xredcolors  [i] * 0x00010001;
	    xgreencolors[i] = xgreencolors[i] * 0x00010001;
	    xbluecolors [i] = xbluecolors [i] * 0x00010001;
	}
    }
#endif
}

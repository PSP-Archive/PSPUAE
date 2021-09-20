
#include "sysconfig.h"
#include "sysdeps.h"

#include "crc32.h"

static uae_u32 crc_table[256];

static void make_crc_table (void)
{
    uae_u32 c;
    unsigned int n, k;
    for (n = 0; n < 256; n++) {
	c = (unsigned long)n;
	for (k = 0; k < 8; k++)
	    c = (c >> 1) ^ (c & 1 ? 0xedb88320 : 0);
	crc_table[n] = c;
    }
}

uae_u32 get_crc32 (const uae_u8 *buf, unsigned int len)
{
    uae_u32 crc;

    if (!crc_table[1])
	make_crc_table();

    crc = 0xffffffff;
    while (len-- > 0)
	crc = crc_table[(crc ^ (*buf++)) & 0xff] ^ (crc >> 8);

    return crc ^ 0xffffffff;
}

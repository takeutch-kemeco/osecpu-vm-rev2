#include "osecpu-vm.h"

void hh4Init(HH4Reader *hh4r, void *p, int half, void *p1)
{
	hh4r->p = p;
	hh4r->half = half;
	hh4r->p1 = p1;
	hh4r->errorCode = 0;
	return;
}

int hh4Get4bit(HH4Reader *hh4r)
{
	int value = *hh4r->p;
	if (hh4r->half == 0) {
		value >>= 4;
		hh4r->half = 1;
	} else {
		value &= 0xf;
		hh4r->p++;
		hh4r->half = 0;
	}
	return value;
}

Int32 hh4GetUnsigned(HH4Reader *hh4r)
{
	int i = hh4Get4bit(hh4r), len = 3;
	if (i <= 0x6)
		;	// 0xxxŒ^
	else if (i == 0x7) {
		len = hh4GetUnsigned(hh4r) * 4;
		if (len > 32) {
			hh4r->errorCode = 1;
			len = 0;
		}
		int j;
		i = 0;
		for (j = len; j > 0; j -= 4) {
			i <<= 4;
			i |= hh4Get4bit(hh4r);
		}
	} else if (i <= 0xb) {	// 10xx_xxxxŒ^
		i = i << 4 | hh4Get4bit(hh4r);
		len = 6;
		i &= 0x3f;
	} else if (i <= 0xd) {	// 110x_xxxx_xxxxŒ^
		i = i << 8 | hh4Get4bit(hh4r) << 4;
		i |= hh4Get4bit(hh4r);
		len = 9;
		i &= 0x1ff;
	} else if (i == 0xe) {	// 1110_xxxx_xxxx_xxxxŒ^
		i = hh4Get4bit(hh4r) << 8;
		i |= hh4Get4bit(hh4r) << 4;
		i |= hh4Get4bit(hh4r);
		len = 12;
	} else { // 0x0f‚Í“Ç‚Ý”ò‚Î‚·.
		i = hh4GetUnsigned(hh4r);
		len = hh4r->len;
	}
	hh4r->len = len;
	return i;
}

Int32 hh4GetSigned(HH4Reader *hh4r)
{
	Int32 i = hh4GetUnsigned(hh4r);
	int len = hh4r->len;
	if (0 < len && len <= 31 && i >= (1 << (len - 1)))
		i -= 1 << len; // MSB‚ª1‚È‚çˆø‚«ŽZ‚µ‚Ä•‰”‚É‚·‚é.
	return i;
}

Int32 *hh4Decode(Jitc *jitc)
{
	HH4Reader *hh4r = jitc->hh4r;
	Int32 *dst = jitc->hh4dst, *dst1 = jitc->hh4dst1; 

	jitc->errorCode = 0;
	for (;;) {
		if (hh4r->p >= hh4r->p1)
			break;
		if (hh4r->errorCode != 0)
			goto err1;
		Int32 opecode = hh4GetUnsigned(hh4r);
		int len = instrLengthSimple(opecode), i;
		if (len > 0 && opecode != 0x02) {
			if (dst + len > dst1)
				goto err;
			*dst = opecode;
			for (i = 1; i < len; i++)
				dst[i] = hh4GetUnsigned(hh4r);
			dst += len;
			continue;
		}
		if (opecode == 0x02) {
			if (dst + 4 > dst1)
				goto err;
			*dst = opecode;
			dst[1] = hh4GetSigned(hh4r);
			dst[2] = hh4GetUnsigned(hh4r);
			dst[3] = hh4GetUnsigned(hh4r);
			dst += 4;
			continue;
		}
		if (opecode == 0xfd) {
			if (dst + 3 > dst1)
				goto err;
			*dst = opecode;
			dst[1] = hh4GetUnsigned(hh4r);
			dst[2] = hh4GetUnsigned(hh4r);
			if (0 <= dst[2] && dst[2] <= 3)
				jitc->dr[dst[2]] = dst[1];
			dst += 3;
			continue;
		}
		jitc->hh4dst = dst;
		dst = ext_hh4Decode(jitc, opecode);
		if (dst != jitc->hh4dst)
			continue; // ‰½‚©ˆ—‚Å‚«‚½‚æ‚¤‚È‚Ì‚ÅŽŸ‚Ö.

		fprintf(stderr, "Error: hh4Decode: opecode=0x%02X\n", opecode); // “à•”ƒGƒ‰[.
		exit(1);
	}
	return dst;
err:
	jitc->errorCode = JITC_HH4_DST_OVERRUN;
	return dst;
err1:
	jitc->errorCode = JITC_HH4_BITLENGTH_OVER;
	return dst;
}

unsigned char *hh4StrToBin(unsigned char *src, unsigned char *src1, unsigned char *dst, unsigned char *dst1)
{
	int half = 0, c, d;
	for (;;) {
		if (src1 != NULL && src >= src1) break;
		if (*src == '\0') break;
		c = *src++;
		d = -1;
		if ('0' <= c && c <= '9')
			d = c - '0';
		if ('A' <= c && c <= 'F')
			d = c - ('A' - 10);
		if ('a' <= c && c <= 'f')
			d = c - ('a' - 10);
		if (d >= 0) {
			if (half == 0) {
				if (dst1 != NULL && dst >= dst1) {
					dst = NULL;
					break;
				}
				*dst = d << 4;
				half = 1;
			} else {
				*dst |= d;
				dst++;
				half = 0;
			}
		}
	}
	if (dst != NULL && half != 0)
		dst++;
	return dst;
}

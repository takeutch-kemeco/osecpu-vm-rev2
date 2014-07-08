#include "osecpu-vm.h"

#include "tek.c"

// typedef unsigned char UCHAR;

// upx関係.

typedef struct _DecodeUpxStr {
	const UCHAR *p;
	int bitBuf, bitBufLen;
	int tmp;
} DecodeUpxStr;

int decode_upx_getBit(DecodeUpxStr *s)
{
	if (s->bitBufLen == 0) {
		s->bitBuf = s->p[0] | s->p[1] << 8;
		s->p += 2;
		s->bitBufLen |= 16;
	}
	s->bitBufLen--;
	return (s->bitBuf >> s->bitBufLen) & 1;
}

int decode_upx_getTmpBit(DecodeUpxStr *s)
{
	s->tmp = (s->tmp << 1 | decode_upx_getBit(s)) & 0xffff;
	return decode_upx_getBit(s);
}

int decode_upx(const UCHAR *p, const UCHAR *p1, UCHAR *q, UCHAR *q1)
{
	DecodeUpxStr s;
	int i, dis;
	UCHAR *q0 = q;
	i = p1 - p;
	memmove(q1 - 8 - i, p, i);
	s.p = q1 - 8 - i;
	dis |= -1;
	s.bitBufLen &= 0;
	goto l1;
l0:
	if (s.p <= q) goto err;
	*q++ = *s.p++;
l1:
	i = decode_upx_getBit(&s);
	if (i != 0) goto l0;
	s.tmp = 1;
	do {
		i = decode_upx_getTmpBit(&s);
		if (s.tmp == 0) goto fin;
	} while (i == 0);
	if (s.tmp >= 3)
		dis = ~((s.tmp - 3) << 8 | *s.p++);
	s.tmp &= 0;
	i = decode_upx_getTmpBit(&s);
	s.tmp = s.tmp << 1 | i;
	if (s.tmp == 0) {
		s.tmp |= 1;
		do {
			i = decode_upx_getTmpBit(&s);
		} while (i == 0);
		s.tmp += 2;
	}
	s.tmp++;
	if (dis < -0xd00) s.tmp++;
	if (s.p <= q + s.tmp) goto err;
	for (i = 0; i < s.tmp; i++)
		q[i] = q[i + dis];
	q += s.tmp;
	goto l1;
err:
	q = q0 - 1;
fin:
	return q - q0;
}

// tek5関係.

int decode_tek5(const UCHAR *p, const UCHAR *p1, UCHAR *q, UCHAR *q1)
{
	return -1;
}

// フロントエンドコード関係.

typedef struct _DecodeForLoop {
	int r, bit, v1t, v1v, step, label;
} DecodeForLoop;

typedef struct _DecodeFcodeStr {
	Hh4Reader hh4r;
	unsigned char *q;
	char flag4, flagD, err;
	int rep[3][8], bitR[0x40];
	int getIntTyp, getIntBit, getIntOrg, lastLabel;
	DecodeForLoop floop[16];
	int floopDepth;
} DecodeFcodeStr;

void decode_fcodeStep(DecodeFcodeStr *s);
void fcode_updateRep(DecodeFcodeStr *s, int typ, int r);
int fcode_getSigned(DecodeFcodeStr *s);
int fcode_getReg(DecodeFcodeStr *s, int typ);
int fcode_getInteger(DecodeFcodeStr *s, const int *len3table);
void fcode_putOpecode1(DecodeFcodeStr *s, int i);
void fcode_putLb(DecodeFcodeStr *s, int opt, int i);
void fcode_putLimm(DecodeFcodeStr *s, int bit, int r, int i);
void fcode_putPlimm(DecodeFcodeStr *s, int p, int i);
void fcode_putCnd(DecodeFcodeStr *s, int r);
void fcode_putAlu(DecodeFcodeStr *s, int opecode, int bit, int r0, int r1, int r2);
void fcode_putCp(DecodeFcodeStr *s, int bit, int r0, int r1);
void fcode_putPcp(DecodeFcodeStr *s, int p0, int p1);
int fcode_putLimmOrCp(DecodeFcodeStr *s, int bit, int r);
void fcode_ope06(DecodeFcodeStr *s);
void fcode_ope07(DecodeFcodeStr *s);
void fcode_opeAlu(DecodeFcodeStr *s, int opecode);
void fcode_api0002(DecodeFcodeStr *s);
void fcode_api0003(DecodeFcodeStr *s);
void fcode_api0004(DecodeFcodeStr *s);
void fcode_api0005(DecodeFcodeStr *s);
void fcode_api0010(DecodeFcodeStr *s);

static int len3table0[7] = { -1, 0, 1, 2, 3, 4, -0x10 /* rep0 */ };

#define BIT_UNKNOWN		0x7fffffff	// とにかく大きな値にする.
#define MIN(a, b)		((a) < (b) ? (a) : (b))

int decode_fcode(const unsigned char *p, const unsigned char *p1, unsigned char *q, unsigned char *q1)
{
	DecodeFcodeStr s;
	int i, j;
	unsigned char *q0 = q;
	i = p1 - p;
	memmove(q1 - i, p, i);
	hh4ReaderInit(&s.hh4r, q1 - i, 0, q1, 0);
	s.q = q;
	s.err = 0;
	s.flag4 = 0;
	s.flagD = 0;
	s.lastLabel = -1;
	s.floopDepth = 0;
	for (i = 0; i < 0x40; i++)
		s.bitR[i] = BIT_UNKNOWN;
	for (j = 0; j < 3; j++) {
		for (i = 0; i < 8; i++)
			s.rep[j][i] = 0x30 + i;
	}
	*s.q++ = 0x00;
	while (s.err == 0) {
		decode_fcodeStep(&s);
		if (hh4ReaderEnd(&s.hh4r) != 0) break;
	}
	while (s.err == 0 && s.floopDepth > 0)
		fcode_ope07(&s);
	if (s.err != 0)
		s.q = q0 - 1;
	return s.q - q0;
}

int fcode_swapOpecode(int i)
// 0を14に代用可能にする.
// osecpu-110より試験的に導入.
// OSWP命令によりキャンセルされる（未実装）.
// コード末尾の0.5バイトを埋めるためには1を使う.
{
	if (i == 0x00) i = 0x14;
	return i;
}

void decode_fcodeStep(DecodeFcodeStr *s)
{
	int opecode, i;
	if (s->err == 0) {
		opecode = hh4ReaderGetUnsigned(&s->hh4r);
		if (opecode == 0 && hh4ReaderEnd(&s->hh4r) != 0)
			goto fin; // 末尾の0.5バイトを埋めていた0を発見. これは無視する.
		opecode = fcode_swapOpecode(opecode);
		if (opecode == 0x0) {
			if (s->flag4 == 0) {
				fcode_putOpecode1(s, 0xf0);
				goto fin;
			}
		}
		if (opecode == 0x4) {
			if (s->flag4 == 0) {
				s->flag4 = 1;
				goto fin;
			}
			s->flag4 = 0;
			fcode_putCnd(s, fcode_getReg(s, 0));
			goto fin;
		}
		if (opecode == 0x5) {
			i = fcode_getSigned(s);
			if (i == 0x0002) { fcode_api0002(s); goto fin; }
			if (i == 0x0003) { fcode_api0003(s); goto fin; }
			if (i == 0x0004) { fcode_api0004(s); goto fin; }
			if (i == 0x0005) { fcode_api0005(s); goto fin; }
			if (i == 0x0010) { fcode_api0010(s); goto fin; }
		}
		if (opecode == 0x6) {
			if (s->floopDepth >= 16) goto err;
			fcode_ope06(s);
			goto fin;
		}
		if (opecode == 0x07) {
			if (s->floopDepth <= 0) goto err;
			fcode_ope07(s);
			goto fin;
		}
		if (opecode == 0x0d) {
			if (s->flagD != 0) goto err;
			s->flagD = 1;
			goto fin;
		}
		if (0x10 <= opecode && opecode <= 0x1b && opecode != 0x17) {
			fcode_opeAlu(s, opecode);
			goto fin;
		}
err:
		s->err = 1;
	}
fin:
	return;
}

void fcode_updateRep(DecodeFcodeStr *s, int typ, int r)
{
	int tmp0 = r, tmp1, i;
	for (i = 0; i < 8; i++) {
		tmp1 = s->rep[typ][i];
		s->rep[typ][i] = tmp0;
		if (tmp1 == r) break;
		tmp0 = tmp1;
	}
	return;
}

int fcode_getSigned(DecodeFcodeStr *s)
{
	int i = hh4ReaderGetSigned(&s->hh4r);
	if (s->hh4r.length == 3) {	// 0,1,2,3,4,5,-1.
		i &= 7;
		if (i == 6)
			i = -1;
	}
	return i;
}

int fcode_getReg(DecodeFcodeStr *s, int typ)
// 4ビット形式の場合(0～6): R00～R04, rep0～rep1.
// そのほかの場合(0～0x47): R00～R17, rep0～rep7, R20～R3F, R18～R1F
{
	int i = hh4ReaderGetUnsigned(&s->hh4r);
	if (s->hh4r.length == 3) {
		if (i >= 5)
			i += 0x18 - 5;
	}
	if (0x18 <= i && i <= 0x1f)
		i = s->rep[typ][i & 7];
	else if (0x40 <= i && i <= 0x47)
		i += 0x18 - 0x40;
	return i;
}

int fcode_getInteger(DecodeFcodeStr *s, const int *len3table)
{
	int i = fcode_getSigned(s), typ = 0;
	s->getIntOrg = i;
	if (s->hh4r.length == 3)
		i = len3table[i + 1];
	if (-0x08 <= i && i <= -0x05)
		i = 0x20 << (i & 3); // 0x20, 0x40, 0x80, 0x100.
	else if (-0x10 <= i && i <= -0x09) {
		i = s->rep[0][i & 7]; // rep0-7.
		typ = 1;
	} else if (i == -0x11) {
		i = 0x3f; // R3F.
		typ = 1;
	} else if (-0x20 <= i && i <= -0x12) {
		i &= 0x0f; // R00-R0E.
		typ = 1;
	} else if (i == -0x21) {
		i = 0x0f; // R0F.
		typ = 1;
	} else if (-0x50 <= i && i <= -0x22) {
		i += 0x60; // R10-R3E.
		typ = 1;
	} else if (-0x68 <= i && i <= -0x51)
		i = 0x200 << (i + 0x68); // 0x200-4G.
	else if (-0x6c <= i && i <= -0x69) {
		s->getIntBit = 4 << (i + 0x6c); // 4, 8, 16, 32.
		i = fcode_getInteger(s, len3table);
		typ = s->getIntTyp;
	} else if (i == -0x6d) {
		i = hh4ReaderGetUnsigned(&s->hh4r);
		if (i <= 16)
			s->getIntBit = 1 << i;
		else
			s->getIntBit = i - 17;
		i = fcode_getInteger(s, len3table);
		typ = s->getIntTyp;
	} else if (i <= -0x71)
		i += 0x71 - 5;
	s->getIntTyp = typ;
	return i;
}

void fcode_putInt32(DecodeFcodeStr *s, int i)
{
	if (s->hh4r.p.p < s->q + 16)
		s->err = 1;
	if (s->err == 0) {
		s->q[ 0] = 0xf7;
		s->q[ 1] = 0x88;
		s->q[ 2] = (i >> 24) & 0xff;
		s->q[ 3] = (i >> 16) & 0xff;
		s->q[ 4] = (i >>  8) & 0xff;
		s->q[ 5] =  i        & 0xff;
		s->q += 6;
	}
	return;
}

void fcode_putR(DecodeFcodeStr *s, int i)
{
	if (s->hh4r.p.p < s->q + 16)
		s->err = 1;
	if (s->err == 0)
		*s->q++ = i + 0x80;
	return;
}

void fcode_putP(DecodeFcodeStr *s, int i)
{
	if (s->hh4r.p.p < s->q + 16)
		s->err = 1;
	if (s->err == 0)
		*s->q++ = i + 0x80;
	return;
}

void fcode_putBit(DecodeFcodeStr *s, int i)
{
	if (s->hh4r.p.p < s->q + 16)
		s->err = 1;
	if (s->err == 0) {
		s->q[ 0] = 0xf7;
		s->q[ 1] = 0x88;
		s->q[ 2] = (i >> 24) & 0xff;
		s->q[ 3] = (i >> 16) & 0xff;
		s->q[ 4] = (i >>  8) & 0xff;
		s->q[ 5] =  i        & 0xff;
		s->q += 6;
	}
	return;
}

void fcode_putOpecode1(DecodeFcodeStr *s, int i)
{
	if (s->hh4r.p.p < s->q + 16)
		s->err = 1;
	if (s->err == 0)
		*s->q++ = i;
	return;
}

void fcode_putLb(DecodeFcodeStr *s, int opt, int i)
{
	fcode_putOpecode1(s, 0xf1);
	fcode_putInt32(s, i);
	fcode_putInt32(s, opt);
	return;
}

void fcode_putLimm(DecodeFcodeStr *s, int bit, int r, int i)
{
	fcode_putOpecode1(s, 0xf2);
	fcode_putInt32(s, i);
	fcode_putR(s, r);
	fcode_putBit(s, bit);
	if (r != 0x3f)
		s->bitR[r] = bit;
	return;
}

void fcode_putPlimm(DecodeFcodeStr *s, int p, int i)
{
	fcode_putOpecode1(s, 0xf3);
	fcode_putInt32(s, i);
	fcode_putP(s, p);
	return;
}

void fcode_putCnd(DecodeFcodeStr *s, int r)
{
	fcode_putOpecode1(s, 0xf4);
	fcode_putR(s, r);
	return;
}

void fcode_putAlu(DecodeFcodeStr *s, int opecode, int bit, int r0, int r1, int r2)
{
	fcode_putOpecode1(s, opecode);
	fcode_putR(s, r1);
	fcode_putR(s, r2);
	fcode_putR(s, r0);
	fcode_putBit(s, bit);
	if (r0 != 0x3f)
		s->bitR[r0] = bit;
	return;
}

void fcode_putCp(DecodeFcodeStr *s, int bit, int r0, int r1)
{
	fcode_putAlu(s, 0x90, bit, r0, r1, r1);
	return;
}

void fcode_putPcp(DecodeFcodeStr *s, int p0, int p1)
{
	fcode_putOpecode1(s, 0x9e);
	fcode_putP(s, p1);
	fcode_putP(s, p0);
	return;
}

void fcode_putCmp(DecodeFcodeStr *s, int opecode, int bit0, int bit1, int r0, int r1, int r2)
{
	fcode_putOpecode1(s, opecode);
	fcode_putR(s, r1);
	fcode_putR(s, r2);
	fcode_putBit(s, bit1);
	fcode_putR(s, r0);
	fcode_putBit(s, bit0);
	if (r0 != 0x3f)
		s->bitR[r0] = bit0;
	return;
}

void fcode_putRemark0(DecodeFcodeStr *s)
{
	if (s->hh4r.p.p < s->q + 16)
		s->err = 1;
	if (s->err == 0) {
		s->q[ 0] = 0xfc;
		s->q[ 1] = 0xfe;
		s->q[ 2] = 0x00;
		s->q += 3;
	}
	return;
}

void fcode_putRemark1(DecodeFcodeStr *s)
{
	if (s->hh4r.p.p < s->q + 16)
		s->err = 1;
	if (s->err == 0) {
		s->q[ 0] = 0xfc;
		s->q[ 1] = 0xfe;
		s->q[ 2] = 0x10;
		s->q += 3;
	}
	return;
}

void fcode_putRemark2(DecodeFcodeStr *s, int i)
{
	if (s->hh4r.p.p < s->q + 16)
		s->err = 1;
	if (s->err == 0) {
		s->q[ 0] = 0xfc;
		s->q[ 1] = 0xfe;
		s->q[ 2] = 0x21;
		s->q += 3;
		fcode_putInt32(s, i);
	}
	return;
}

void fcode_putRemark3(DecodeFcodeStr *s)
{
	if (s->hh4r.p.p < s->q + 16)
		s->err = 1;
	if (s->err == 0) {
		s->q[ 0] = 0xfc;
		s->q[ 1] = 0xfe;
		s->q[ 2] = 0x30;
		s->q += 3;
	}
	return;
}


void fcode_putjmp(DecodeFcodeStr *s, int i)
{
	fcode_putPlimm(s, 0x3f, i);
	return;
}

void fcode_putPjmp(DecodeFcodeStr *s, int p)
{
	fcode_putPcp(s, 0x3f, p);
	return;
}

int fcode_putLimmOrCp(DecodeFcodeStr *s, int bit, int r)
{
	int i;
	s->getIntBit = BIT_UNKNOWN;
	i = fcode_getInteger(s, len3table0);
	if (s->getIntTyp == 0) {
		if (s->getIntBit == BIT_UNKNOWN)
			s->getIntBit = bit;
		fcode_putLimm(s, s->getIntBit, r, i);
	} else {
		if (s->getIntBit == BIT_UNKNOWN)
			s->getIntBit = s->bitR[i];
		if (s->getIntBit == BIT_UNKNOWN)
			s->getIntBit = bit;
		fcode_putCp(s, s->getIntBit, r, i);
	}
	return i;
}

void fcode_putPcallP28(DecodeFcodeStr *s)
{
	s->lastLabel++;
	fcode_putPlimm(s, 0x30, s->lastLabel);
	fcode_putPjmp(s, 0x28);
	fcode_putLb(s, 1, s->lastLabel);
	fcode_putRemark0(s);
	return;
}

void fcode_ope06(DecodeFcodeStr *s)
// プリフィクス4: カウンタ初期値は0以外.
// プリフィクスD: stepの変更（未実装）.
// bitはカウンタ比較値のほうで指定できる.
{
	int v0v = 0, v0t = 0, v0b = BIT_UNKNOWN;
	DecodeForLoop *dfl = &s->floop[s->floopDepth];
	if (s->flagD != 0) { s->err = 1; return; } // 未実装.
	dfl->r = fcode_getReg(s, 0); // reg.
	if (s->flag4 != 0) {
		s->getIntBit = BIT_UNKNOWN;
		v0v = fcode_getInteger(s, len3table0); // v0.
		v0t = s->getIntTyp;
		v0b = s->getIntBit;
	}
	s->lastLabel++;
	dfl->label = s->lastLabel;
	dfl->step = 1;
	s->getIntBit = BIT_UNKNOWN;
	dfl->v1v = fcode_getInteger(s, len3table0);
	dfl->v1t = s->getIntTyp;
	if (v0t == 0 && dfl->v1t == 0 && v0v > dfl->v1v)
		dfl->step = -1;
	fcode_putRemark2(s, s->getIntOrg);
	dfl->bit = MIN(s->getIntBit, v0b);
	if (dfl->bit == BIT_UNKNOWN) {
		if (v0t != 0)
			dfl->bit = s->bitR[v0v];
		if (dfl->v1t != 0)
			dfl->bit = MIN(dfl->bit, s->bitR[dfl->v1v]);
		if (dfl->bit == BIT_UNKNOWN)
			dfl->bit = 32;
	}
	if (v0t == 0)
		fcode_putLimm(s, dfl->bit, dfl->r, v0v);
	else
		fcode_putCp(s, dfl->bit, dfl->r, v0v);
	fcode_putLb(s, 0, s->lastLabel);
	s->floopDepth++;
	fcode_updateRep(s, 0, dfl->r);
	s->flag4 = 0;
	return;
}

void fcode_ope07(DecodeFcodeStr *s)
{
	int i;
	DecodeForLoop *dfl;
	fcode_putRemark3(s);
	s->floopDepth--;
	dfl = &s->floop[s->floopDepth];
	fcode_putLimm(s, dfl->bit, 0x3f, dfl->step);
	fcode_putAlu(s, 0x94, dfl->bit, dfl->r, dfl->r, 0x3f);
	i = dfl->v1v;
	if (dfl->v1t == 0) {
		fcode_putLimm(s, dfl->bit, 0x3f, dfl->v1v);
		i = 0x3f;
	}
	fcode_putCmp(s, 0xa1, 1, dfl->bit, 0x3f, dfl->r, i);
	fcode_putCnd(s, 0x3f);
	fcode_putPlimm(s, 0x3f, dfl->label);
	return;
}

#define R0	-0x10+0 /* rep0 */
#define R1	-0x10+1 /* rep1 */

void fcode_opeAlu(DecodeFcodeStr *s, int opecode)
// プリフィクス4: r0はr1とは異なる（三項演算モード）.
// プリフィクスD: 演算方向の変更.
{
	static int len3table[12][7] = {
	//	{ -1,  0,  1,  2,  3,  4, R0 }, // general
		{ R1, 16,  1,  2,  8,  4, R0 }, // OR
		{ -1, R1,  1,  2,  3,  4, R0 }, // XOR
		{ R1, 15,  1,  7,  3,  5, R0 }, // AND
		{ 64, 16,  1,  2,  8,  4, 32 }, // SBX
		{ R1, R0,  1,  2,  3,  4,  5 }, // ADD
		{ R1, R0,  1,  2,  3,  4,  5 }, // SUB
		{ -1, R0, R1,  7,  3,  6,  5 }, // MUL
		{  0,  0,  0,  0,  0,  0,  0 },
		{ R1, R0,  1,  2,  3,  4,  5 }, // SHL
		{ R1, R0,  1,  2,  3,  4,  5 }, // SAR
		{  9, R0, R1,  7,  3,  6,  5 }, // DIV
		{  9, R0, R1,  7,  3,  6,  5 }, // MOD
	};
	int i, r0, r1, bit, tmp;
	r1 = fcode_getReg(s, 0);
	s->getIntBit = BIT_UNKNOWN;
	i = fcode_getInteger(s, len3table[opecode & 0xf]);
	bit = s->getIntBit;
	if (bit == BIT_UNKNOWN) {
		bit = s->bitR[r1];
		if (s->getIntTyp != 0)
			bit = MIN(bit, s->bitR[i]);
		if (bit == BIT_UNKNOWN)
			bit = 32;
	}
	r0 = r1;
	if (s->flag4 != 0)
		r0 = fcode_getReg(s, 0);
	if (s->getIntTyp == 0) {
		fcode_putLimm(s, bit, 0x3f, i);
		i = 0x3f;
	}
	if (s->flagD != 0) {
		tmp = i;
		i = r1;
		r1 = tmp;
	}
	fcode_putAlu(s, opecode | 0x80, bit, r0, r1, i);
	if (r1 != 0x3f) fcode_updateRep(s, 0, r1);
	if (i  != 0x3f) fcode_updateRep(s, 0, i);
	fcode_updateRep(s, 0, r0);
	s->flag4 = s->flagD = 0;
	return;
}

#undef R0
#undef R1

void fcode_api0002(DecodeFcodeStr *s)
// api_drawPoint(mod, c, x, y).
{
	fcode_putRemark1(s);
	fcode_putLimm(s, 16, 0x30, 0x0002);
	fcode_putLimmOrCp(s, 16, 0x31); // mod.
	if (s->flag4 == 0) {
		fcode_putLimmOrCp(s, 32, 0x32); // c.
		fcode_putLimmOrCp(s, 16, 0x33); // x.
		fcode_putLimmOrCp(s, 16, 0x34); // y.
	} else {
		fcode_putCp(s, 32, 0x32, 0x00); // c.
		fcode_putCp(s, 16, 0x33, 0x01); // x.
		fcode_putCp(s, 16, 0x34, 0x02); // y.
		s->flag4 = 0;
	}
	fcode_putPcallP28(s);
	return;
}

void fcode_api0003(DecodeFcodeStr *s)
// api_drawLine(mod, c, x0, y0, x1, y1).
{
	fcode_putRemark1(s);
	fcode_putLimm(s, 16, 0x30, 0x0003);
	fcode_putLimmOrCp(s, 16, 0x31); // mod.
	if (s->flag4 == 0) {
		fcode_putLimmOrCp(s, 32, 0x32); // c.
		fcode_putLimmOrCp(s, 16, 0x33); // x0.
		fcode_putLimmOrCp(s, 16, 0x34); // y0.
		fcode_putLimmOrCp(s, 16, 0x35); // x1.
		fcode_putLimmOrCp(s, 16, 0x36); // y1.
	} else {
		fcode_putCp(s, 32, 0x32, 0x00); // c.
		fcode_putCp(s, 16, 0x33, 0x01); // x0.
		fcode_putCp(s, 16, 0x34, 0x02); // y0.
		fcode_putCp(s, 16, 0x35, 0x03); // x0.
		fcode_putCp(s, 16, 0x36, 0x04); // y0.
		s->flag4 = 0;
	}
	fcode_putPcallP28(s);
	return;
}

void fcode_api0004(DecodeFcodeStr *s)
// api_fillRect(mod, c, xsiz, ysiz, x0, y0).
// api_drawRect(mod, c, xsiz, ysiz, x0, y0).
{
	fcode_putRemark1(s);
	fcode_putLimm(s, 16, 0x30, 0x0004);
	fcode_putLimmOrCp(s, 16, 0x31); // mod.
	if (s->flag4 == 0) {
		fcode_putLimmOrCp(s, 32, 0x32); // c.
		fcode_putLimmOrCp(s, 16, 0x33); // xsiz.
		fcode_putLimmOrCp(s, 16, 0x34); // ysiz.
		fcode_putLimmOrCp(s, 16, 0x35); // x1.
		fcode_putLimmOrCp(s, 16, 0x36); // y1.
	} else {
		fcode_putCp(s, 32, 0x32, 0x00); // c.
		fcode_putCp(s, 16, 0x33, 0x01); // xsiz.
		fcode_putCp(s, 16, 0x34, 0x02); // ysiz.
		fcode_putCp(s, 16, 0x35, 0x03); // x0.
		fcode_putCp(s, 16, 0x36, 0x04); // y0.
		s->flag4 = 0;
	}
	fcode_putPcallP28(s);
	return;
}

void fcode_api0005(DecodeFcodeStr *s)
// api_fillOval(mod, c, xsiz, ysiz, x0, y0).
// api_drawOval(mod, c, xsiz, ysiz, x0, y0).
{
	fcode_putRemark1(s);
	fcode_putLimm(s, 16, 0x30, 0x0005);
	fcode_putLimmOrCp(s, 16, 0x31); // mod.
	if (s->flag4 == 0) {
		fcode_putLimmOrCp(s, 32, 0x32); // c.
		fcode_putLimmOrCp(s, 16, 0x33); // xsiz.
		fcode_putLimmOrCp(s, 16, 0x34); // ysiz.
		fcode_putLimmOrCp(s, 16, 0x35); // x0.
		fcode_putLimmOrCp(s, 16, 0x36); // y0.
	} else {
		fcode_putCp(s, 32, 0x32, 0x00); // c.
		fcode_putCp(s, 16, 0x33, 0x01); // xsiz.
		fcode_putCp(s, 16, 0x34, 0x02); // ysiz.
		fcode_putCp(s, 16, 0x35, 0x03); // x0.
		fcode_putCp(s, 16, 0x36, 0x04); // y0.
		s->flag4 = 0;
	}
	fcode_putPcallP28(s);
	return;
}

void fcode_api0010(DecodeFcodeStr *s)
// api_openWin(xsiz, ysiz).
{
	fcode_putRemark1(s);
	fcode_putLimm(s, 16, 0x30, 0x0010);
	fcode_putLimmOrCp(s, 16, 0x31); // xsiz.
	fcode_putLimmOrCp(s, 16, 0x32); // ysiz.
	fcode_putPcallP28(s);
	return;
}


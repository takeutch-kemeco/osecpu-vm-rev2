#include "osecpu-vm.h"

// 将来的には、このコードはtek.cも含めてOSECPU-VMのバイトコードに移植される。
// 現在は試行錯誤したいので、OSECPU-VM化していないだけ。

typedef unsigned char UCHAR;

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
	int i = p1 - p, dis;
	UCHAR *q0 = q;
	UCHAR *pp0 = q1 - 8 - i;
	Hh4Reader hh4r;
	memmove(pp0, p, i);
	hh4ReaderInit(&hh4r, pp0, 0, q1 - 8, 0);
	i = hh4ReaderGetUnsigned(&hh4r);
	s.p = (UCHAR *) hh4r.p.p;
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
	int srcsiz = p1 - p, outsiz, i;
	UCHAR *pp0 = q1 - 8 - srcsiz, *pp1 = q1 - 8;
	Hh4Reader hh4r;
	memmove(pp0, p, srcsiz);
	hh4ReaderInit(&hh4r, pp0, 0, pp1, 0);
	i = hh4ReaderGetUnsigned(&hh4r);
	outsiz = hh4ReaderGetUnsigned(&hh4r);
	pp0 = (UCHAR *) hh4r.p.p;
	if (i > 0) {
		pp0--;
		if (i == 1) *pp0 = 0x15;
		if (i == 2) *pp0 = 0x19;
		if (i == 3) *pp0 = 0x21;
		if (i >= 4) goto err;
	}
	i = tek_lzrestore_tek5(pp1 - pp0, pp0, outsiz, q);
	if (i == 0) return outsiz;
err:
	return -1;
}

// フロントエンドコード関係.

#define MODE_REG_LC3	1	// osecpu120dから試験的に導入.

typedef struct _DecodeForLoop {
	int r, bit, v1t, v1v, step, label;
} DecodeForLoop;

typedef struct _DecodeLabel {
	signed char opt;
	int typ, abs;
} DecodeLabel;

typedef struct _DecodeFcodeStr {
	Hh4Reader hh4r;
	unsigned char *q;
	char flag4, flagD, flag14, wait3d, err;
	int rep[3][0x40], bitR[0x40], pxxTyp[0x40], waitLb0;
	int getIntTyp, getIntBit, getIntOrg, getRegBit, lastLabel;
	DecodeForLoop floop[16];
	int floopDepth, vecPrfx, vecPrfxMode;
	DecodeLabel label[DEFINES_MAXLABELS];
} DecodeFcodeStr;

void decode_fcodeStep(DecodeFcodeStr *s);
void fcode_updateRep(DecodeFcodeStr *s, int typ, int r);
int fcode_getSigned(DecodeFcodeStr *s);
int fcode_getReg(DecodeFcodeStr *s, int typ, char mode);
int fcode_getInteger(DecodeFcodeStr *s, const int *len3table);
void fcode_putInt32(DecodeFcodeStr *s, Int32 i);
void fcode_putR(DecodeFcodeStr *s, int i);
void fcode_putP(DecodeFcodeStr *s, int i);
void fcode_putBit(DecodeFcodeStr *s, int i);
void fcode_putTyp(DecodeFcodeStr *s, int i);
void fcode_putOpecode1(DecodeFcodeStr *s, int i);
void fcode_putLb(DecodeFcodeStr *s, int opt, int i);
void fcode_putLimm(DecodeFcodeStr *s, int bit, int r, int i);
void fcode_putPlimm(DecodeFcodeStr *s, int p, int i);
void fcode_putCnd(DecodeFcodeStr *s, int r);
void fcode_putPadd(DecodeFcodeStr *s, int bit, int p0, int typ, int p1, int r);
void fcode_putAlu(DecodeFcodeStr *s, int opecode, int bit, int r0, int r1, int r2);
void fcode_putCp(DecodeFcodeStr *s, int bit, int r0, int r1);
void fcode_putPcp(DecodeFcodeStr *s, int p0, int p1);
void fcode_putCmp(DecodeFcodeStr *s, int opecode, int bit0, int bit1, int r0, int r1, int r2);
void fcode_putRemark0(DecodeFcodeStr *s);
void fcode_putRemark1(DecodeFcodeStr *s);
void fcode_putRemark34(DecodeFcodeStr *s);
int fcode_putLimmOrCp(DecodeFcodeStr *s, int bit, int r);
void fcode_putPcallP2f(DecodeFcodeStr *s);
void fcode_ope2(DecodeFcodeStr *s);
void fcode_ope3(DecodeFcodeStr *s);
void fcode_ope06(DecodeFcodeStr *s);
void fcode_ope07(DecodeFcodeStr *s);
void fcode_ope08(DecodeFcodeStr *s);
void fcode_ope09(DecodeFcodeStr *s);
void fcode_opeAlu(DecodeFcodeStr *s, int opecode);
void fcode_opeCmp(DecodeFcodeStr *s, int opecode);
void fcode_ope2e(DecodeFcodeStr *s);
void fcode_ope3d(DecodeFcodeStr *s);
void fcode_api0002(DecodeFcodeStr *s);
void fcode_api0003(DecodeFcodeStr *s);
void fcode_api0004(DecodeFcodeStr *s);
void fcode_api0005(DecodeFcodeStr *s);
void fcode_api0009(DecodeFcodeStr *s);
void fcode_api000d(DecodeFcodeStr *s);
void fcode_api0010(DecodeFcodeStr *s);
int fcode_initLabel(DecodeFcodeStr *s, unsigned char *q0, unsigned char *q1);
int fcode_fixLabel(DecodeFcodeStr *s, unsigned char *q0, unsigned char *q1);
int fcode_fixTyp(DecodeFcodeStr *s, unsigned char *q0, unsigned char *q1);
void fcode_unknownBitR(DecodeFcodeStr *s, int r0, int r1);

static int len3table0[7] = { -1, 0, 1, 2, 3, 4, -0x10 /* rep0 */ };
//static int len3table0f[7] = { -0xfff, -4, -3, -2, -1, 0, 1 }; // -0xfffはリザーブ.
static int len3table14[7] = { -1, -0x10+1 /* rep1 */, 1,  2,  3,  4, -0x10+0 /* rep0 */ }; // ADD

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
	s.flag14 = 1;
	s.waitLb0 = 0;
	s.wait3d = -1;
	s.lastLabel = -1;
	s.floopDepth = 0;
	s.vecPrfx = -1;
	s.vecPrfxMode = 0;
	for (i = 0; i < 0x40; i++) {
		s.bitR[i] = BIT_UNKNOWN;
		s.pxxTyp[i] = PTR_TYP_NULL;
	}
	for (i = 1; i <= 4; i++)
		s.pxxTyp[i] = PTR_TYP_INVALID;	// 値はまだ不明だが推定は可能.

	for (j = 0; j < 3; j++) {
		for (i = 0; i < 0x40; i++)
			s.rep[j][i] = i;
	}
	*s.q++ = 0x00; // バックエンドのヘッダ.
	while (s.err == 0) {
		decode_fcodeStep(&s);
		if (hh4ReaderEnd(&s.hh4r) != 0) break;
	}
	if (s.waitLb0 > 0) {
//printf("_waitLb0=%d\n", s.waitLb0);
		if (s.waitLb0 != 1) s.err = 1;
		s.waitLb0--;
		s.lastLabel++;
		fcode_putLb(&s, 0, s.lastLabel);
	}
	while (s.err == 0 && s.floopDepth > 0)
		fcode_ope07(&s);
	if (s.err == 0 && s.wait3d > 0)
		fcode_ope3d(&s);
	if (s.err == 0) {
		i = fcode_initLabel(&s, q0 + 1, s.q);
		if (i == 0)
			i = fcode_fixLabel(&s, q0 + 1, s.q);
		if (i == 0)
			i = fcode_fixTyp(&s, q0 + 1, s.q);
		if (i != 0)
			s.err = 1;
	}
	if (s.err != 0)
		s.q = q0 - 1;
	return s.q - q0;
}

int fcode_swapOpecode(DecodeFcodeStr *s, int i)
// 0を14に代用可能にする.
// osecpu-110より試験的に導入.
// OSWP命令によりキャンセルされる（未実装）.
// もしくは14が一度でも出現するとキャンセルされる.
// コード末尾の0.5バイトを埋めるためには0がそのまま使える.
{
	if (s->flag14 != 0) {
		if (i == 0x14) {
			i = 0x00;
			s->flag14 = 0;
		} else if (i == 0x00)
			i = 0x14;
	}
	return i;
}

void decode_fcodeStep(DecodeFcodeStr *s)
{
	int opecode, i, j, k, l;
	if (s->err == 0) {
		opecode = hh4ReaderGetUnsigned(&s->hh4r);
		if (opecode == 0 && hh4ReaderEnd(&s->hh4r) != 0)
			goto fin; // 末尾の0.5バイトを埋めていた0を発見. これは無視する.
		opecode = fcode_swapOpecode(s, opecode);
		if (opecode == 0x0) {
			if (s->flag4 == 0) {
				fcode_putOpecode1(s, 0xf0);
				goto fin;
			}
		}
		if (opecode == 0x1) {
			i = 0;
			if (s->flag4 != 0)
				i = hh4ReaderGetSigned(&s->hh4r);
			s->lastLabel = s->lastLabel + 1 + (i >> 8);
			i &= 0xff;
			fcode_putLb(s, i, s->lastLabel);
			if (0 <= i && i <= 1 && s->waitLb0 > 0)
				s->waitLb0--;
			s->flag4 = 0;
			goto fin;
		}
		if (opecode == 0x2) {
			fcode_ope2(s);
			goto fin;
		}
		if (opecode == 0x3) {
			fcode_ope3(s);
			goto fin;
		}
		if (opecode == 0x4) {
			if (s->flag4 == 0) {
				s->flag4 = 1;
				goto fin;
			}
			s->flag4 = 0;
			fcode_putCnd(s, fcode_getReg(s, 0, 0));
			goto fin;
		}
		if (opecode == 0x5) {
			i = fcode_getSigned(s);
			if (i == 0x0002) { fcode_api0002(s); goto fin; }
			if (i == 0x0003) { fcode_api0003(s); goto fin; }
			if (i == 0x0004) { fcode_api0004(s); goto fin; }
			if (i == 0x0005) { fcode_api0005(s); goto fin; }
			if (i == 0x0009) { fcode_api0009(s); goto fin; }
			if (i == 0x000d) { fcode_api000d(s); goto fin; }
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
		if (opecode == 0x08) {
			fcode_ope08(s);
			goto fin;
		}
		if (opecode == 0x09) {
			fcode_ope09(s);
			goto fin;
		}
		if (opecode == 0x0d) {
			if (s->flagD != 0) goto err;
			s->flagD = 1;
			goto fin;
		}
		if (0x10 <= opecode && opecode <= 0x1b) {
			fcode_opeAlu(s, opecode);
			goto fin;
		}
		if (0x20 <= opecode && opecode <= 0x27) {
			fcode_opeCmp(s, opecode);
			goto fin;
		}
		if (opecode == 0x2e) {
			fcode_ope2e(s);
			goto fin;
		}
		if (opecode == 0x32) {	// malloc
			// 手抜きでbitを反映させていない.
			i = fcode_putLimmOrCp(s, 32, 0x3b);	// typ
			l = s->getIntTyp; // 0ならconst.
			j = fcode_putLimmOrCp(s, 32, 0x3a);	// len
			k = fcode_getReg(s, 1, MODE_REG_LC3);
			fcode_putOpecode1(s, 0xb2);
			fcode_putR(s, 0x3b);
			fcode_putBit(s, 32);
			fcode_putR(s, 0x3a);
			fcode_putBit(s, 32);
			fcode_putP(s, k);
			if (s->flagD == 0) {
				if (l != 0) goto err;
				fcode_putPcp(s, 0x3b, k);
				if (s->flag4 != 0)
					fcode_putLimmOrCp(s, 32, 0x39);
				else
					fcode_putLimm(s, 32, 0x39, 0);
				s->flag4 = 0;
				fcode_putLimm(s, 32, 0x3b, 0);
				s->lastLabel++;
				fcode_putLb(s, 2, s->lastLabel);
				fcode_putOpecode1(s, 0x89);
				fcode_putR(s, 0x39);
				fcode_putBit(s, 32);
				fcode_putP(s, 0x3b);
				fcode_putTyp(s, i);
				fcode_putInt32(s, 0);
				fcode_putLimm(s, 32, 0x3f, 1);
				fcode_putPadd(s, 32, 0x3b, i, 0x3b, 0x3f);
				fcode_putLimm(s, 32, 0x3f, 1);
				fcode_putAlu(s, 0x94, 32, 0x3b, 0x3b, 0x3f);
				fcode_putCmp(s, 0xa0, 1, 32, 0x3f, 0x3a, 0x3b);
				fcode_putCnd(s, 0x3f);
				fcode_putPlimm(s, 0x3f, s->lastLabel);
				s->pxxTyp[k] = i;
				fcode_updateRep(s, 1, k);
				fcode_unknownBitR(s, 0x39, 0x3b);
				goto fin;
			}
		}
		if (opecode == 0x34) {
			s->vecPrfx = 2;
			s->vecPrfxMode = 0;
			fcode_putRemark34(s);
			goto fin;
		}
		if (opecode == 0x3c) {
			if (s->waitLb0 > 0) {
//printf("3c: waitLb0=%d\n", s->waitLb0);
				if (s->waitLb0 != 1) s->err = 1;
				s->waitLb0--;
				s->lastLabel++;
				fcode_putLb(s, 0, s->lastLabel);
			}
			while (s->err == 0 && s->floopDepth > 0)
				fcode_ope07(s);
			if (s->err == 0 && s->wait3d == -1) {
				fcode_putRemark1(s);
				fcode_putLimm(s, 16, 0x30, 0x0009);
				fcode_putLimm(s, 16, 0x31, 0);
				fcode_putLimm(s, 32, 0x32, -1);
				fcode_putPcallP2f(s);
				fcode_unknownBitR(s, 0x30, 0x32);
			}
			if (s->err == 0 && s->wait3d == 1)
				fcode_ope3d(s);
			s->lastLabel++;
			fcode_putLb(s, 1, s->lastLabel);
#if 0
			// waitLb0が管理しているのは関数内での分岐に限定されるので、これをやる必要がない.
			if (s.waitLb0 > 0)
				s.waitLb0--;
#endif
			fcode_putRemark0(s);
			fcode_putOpecode1(s, 0xbc);
			fcode_putInt32(s, 32);
			fcode_putBit(s, 32);
			fcode_putInt32(s, 16);
			fcode_putBit(s, 16);
			fcode_putInt32(s, 64);
			fcode_putTyp(s, 0);
#if 0
			fcode_updateRep(s, 0, 0x33);
			fcode_updateRep(s, 0, 0x32);
			fcode_updateRep(s, 0, 0x31);
			fcode_updateRep(s, 0, 0x30);
			fcode_updateRep(s, 1, 0x32);
			fcode_updateRep(s, 1, 0x31);
			fcode_updateRep(s, 2, 0x31);
			fcode_updateRep(s, 2, 0x30);
#endif
			s->wait3d = 1;
			goto fin;
		}
		if (opecode == 0x3e) {
			// dか4でabsモード.
			i = fcode_getSigned(s);
			// ここではwaitLb0を更新しない.
			// ここで更新するとおかしくなる.
			s->lastLabel++;
			fcode_putPlimm(s, 0x30, s->lastLabel);
			i |= 0x80000000; // 後で補正するためにマークする.
			i &= 0x80ffffff; // 相対補正.
			fcode_putPlimm(s, 0x3f, i);
			fcode_putLb(s, 3, s->lastLabel);
			fcode_putRemark0(s);
			goto fin;
		}
		if (opecode == 0xfd) {
			i = fcode_getSigned(s);
			fcode_putOpecode1(s, 0xfc);
			fcode_putOpecode1(s, 0xfd);
			fcode_putInt32(s, i);
			i = hh4ReaderGetUnsigned(&s->hh4r);
			fcode_putR(s, i);
			goto fin;
		}
		printf("decode_fcodeStep: unknown opecode: 0x%02X\n", opecode); // for debug.
err:
		s->err = 1;
	}
fin:
	return;
}

void fcode_updateRep(DecodeFcodeStr *s, int typ, int r)
{
	int tmp0 = r, tmp1, i;
	for (i = 0; i < 0x40; i++) {
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

#define APPACK_SUB1R_PRM	6

int fcode_getRep(const int *repRawList, const int *len3table, int mode, int l)
{
	int i, j = 0, k, rep[0x40];
	if (mode == 0) {
		// for fcode_getInteger.
		k = -0x11;
		for (i = 0; i < 7; i++) {
			if (len3table[i] <= -0x10+7) {
				if (k < len3table[i])
					k = len3table[i];
			}
		}
		k += 0x11;
		for (i = 0; i < 0x40; i++) {
			if (i < k) {
				// 4ビットで書けるもの.
				rep[j] = repRawList[i];
				j++;
			} else {
				// 8ビット以上.
				if (0x0f <= repRawList[i] && repRawList[i] <= 0x3e) {
					rep[j] = repRawList[i];
					j++;
				}
			}
		}
	}
	if (mode == 1) {
		// for fcode_getReg.
		for (i = 0; i < 0x40; i++) {
			if (i < APPACK_SUB1R_PRM) {
				// 4ビットで書けるもの.
				if (repRawList[i] >= 7 - APPACK_SUB1R_PRM) {
					rep[j] = repRawList[i];
					j++;
				}
			} else {
				// 8ビット以上.
				if (0x20 <= repRawList[i] && repRawList[i] <= 0x27) {
					rep[j] = repRawList[i];
					j++;
				}
			}
		}
	}
	if (mode >= 2) {
		fprintf(stderr, "fcode_getRep: error: mode=%d\n", mode);
		exit(1);
	}
	if (l >= j) {
		fprintf(stderr, "fcode_getRep: error: mode=%d, l=%d, j=%d\n", mode, l, j);
		exit(1);
	}
	return rep[l];
}

int fcode_getReg(DecodeFcodeStr *s, int typ, char mode)
// 4ビット形式の場合(0～6): R00～R04, rep0～rep1.
// そのほかの場合(0～0x47): R00～R17, rep0～rep7, R20～R3F, R18～R1F, bit...
// bit指定に対応しなければいけない（未実装）.
{
	int i = hh4ReaderGetUnsigned(&s->hh4r);
	if (mode == 0) {
		if (s->hh4r.length == 3) {
			if (i >= 7 - APPACK_SUB1R_PRM)
				i += 0x20 - (7 - APPACK_SUB1R_PRM);
		}
		if (s->hh4r.length <= 6) {
			if (0x20 <= i && i <= 0x27)
				i = fcode_getRep(s->rep[typ], NULL, 1, i & 7);
		}
	}
	return i;
}

#if 0

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

#endif

int fcode_getInteger(DecodeFcodeStr *s, const int *len3table)
{
	int i = fcode_getSigned(s), typ = 0;
	s->getIntOrg = i;
	if (s->hh4r.length == 3) {
		i = len3table[i + 1];
		if (i == -0x10+0) {	// rep0
			i = fcode_getRep(s->rep[0], len3table, 0, 0);
			typ = 1;
		} else if (i == -0x10+1) {	// rep1
			i = fcode_getRep(s->rep[0], len3table, 0, 1);
			typ = 1;
		} else if (i == -0x10+2) {	// rep2
			i = fcode_getRep(s->rep[0], len3table, 0, 2);
			typ = 1;
		} else if (i == -0x10+3) {	// rep3
			i = fcode_getRep(s->rep[0], len3table, 0, 3);
			typ = 1;
		}
	}
	if (s->hh4r.length == 6) {
		if (i < -8) {
			i &= 0xff;
			typ = 1;
			if (0xf0 <= i && i <= 0xf7)		// rep0-7.
				i = fcode_getRep(s->rep[0], len3table, 0, i & 0x7);
			else if (0xe0 <= i && i <= 0xee)		// R00-R0E 
				i &= 0x0f;
			else /* if (i == 0xef) */	// R3F
				i = 0x3f;
		} else if (i > 0x16) {
			static int table[] = {
				0x20, 0x18, 0x40, 0x80, 0x100, 0xff, 0x7f, 0x3f, 0x1f
			};
			i = table[i - 0x17];
		}
	}
	if (s->hh4r.length == 9) {
		if (i < -0x90) {
			i &= 0xff;
			if (0x60 <= i && i <= 0x6f)
				i = 1 << (i - (0x60 - 17));
			else if (0x50 <= i && i <= 0x56) {
				s->getIntBit = 1 << (i & 0x7); // 1, 2, 4, 8, 16, 32, 64.
				i = fcode_getInteger(s, len3table);
				typ = s->getIntTyp;
			} else if (i == 0x57) {
				i = hh4ReaderGetUnsigned(&s->hh4r);
				if (i <= 16)
					s->getIntBit = 1 << i;
				else
					s->getIntBit = i - 17;
				i = fcode_getInteger(s, len3table);
				typ = s->getIntTyp;
			} else if (0x40 <= i && i <= 0x5f) {
				fprintf(stderr, "fcode_getInteger: error: len=9, i=0x%x\n", i);
				exit(1);
			} else /* if (0x00 <= i && i <= 0x3f) */ {
				typ = 1;
			}
		} else if (i > 0xee) {
			static int table[] = {
				0x200, 0xf0, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0x10000,
				0xffff, 0x7fff, 0x3fff, 0x1fff, 0xfff, 0x7ff, 0x3ff, 0x1ff
			};
			i = table[i - 0xef];
		}
	}
	if (s->hh4r.length == 12) {
		if (i < -0x620 || 0x7de < i) {
			fprintf(stderr, "fcode_getInteger: error: len=12, i=0x%x\n", i);
			exit(1);
		}
	}

	s->getIntTyp = typ;
	return i;
}


void fcode_putInt32(DecodeFcodeStr *s, Int32 i)
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

void fcode_putTyp(DecodeFcodeStr *s, int i)
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

void fcode_putPadd(DecodeFcodeStr *s, int bit, int p0, int typ, int p1, int r)
{
	fcode_putOpecode1(s, 0x8e);
	fcode_putP(s, p1);
	fcode_putTyp(s, typ);
	fcode_putR(s, r);
	fcode_putBit(s, bit);
	fcode_putP(s, p0);
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

void fcode_putRemark34(DecodeFcodeStr *s)
{
	if (s->hh4r.p.p < s->q + 16)
		s->err = 1;
	if (s->err == 0) {
		s->q[ 0] = 0xfc;
		s->q[ 1] = 0xfe;
		s->q[ 2] = 0xb4;
		s->q[ 3] = 0xf0;
		s->q += 4;
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

void fcode_putPcallP2f(DecodeFcodeStr *s)
{
	s->lastLabel++;
	fcode_putPlimm(s, 0x30, s->lastLabel);
	fcode_putPjmp(s, 0x2f);
	fcode_putLb(s, 3, s->lastLabel);
	fcode_putRemark0(s);
	return;
}

void fcode_ope2(DecodeFcodeStr *s)
{
	int i, r0, bit;
	s->getIntBit = BIT_UNKNOWN;
	i = fcode_getInteger(s, len3table0);
	bit = s->getIntBit;
	if (bit == BIT_UNKNOWN)
		bit = 32;
	r0 = fcode_getReg(s, 0, MODE_REG_LC3);
	if (s->getIntTyp == 0)
		fcode_putLimm(s, bit, r0, i);
	else {
		fcode_putCp(s, bit, r0, i);
		fcode_updateRep(s, 0, i);
	}
	fcode_updateRep(s, 0, r0);
	return;
}

void fcode_ope3(DecodeFcodeStr *s)
{
	Int32 i = fcode_getSigned(s); // 先へ行く方が優遇される. 戻る系はforで支援しているので.
	int p = 0x3f;
	if (s->flag4 != 0)
		p = fcode_getReg(s, 1, MODE_REG_LC3);
	if (s->flagD != 0) {
		fprintf(stderr, "fcode_ope3: Internal error.\n");
		exit(1);
	}
	if (s->waitLb0 < i)
		s->waitLb0 = i;
//printf("waitLb0=%d\n", s->waitLb0);
	i |= 0x80000000; // 後で補正するためにマークする.
	i &= 0x80ffffff; // 相対補正.
	fcode_putPlimm(s, p, i);
	s->flag4 = s->flagD = 0;
	if (p != 0x3f)
		fcode_updateRep(s, 1, p);
	return;
}

void fcode_ope06(DecodeFcodeStr *s)
// プリフィクス4: カウンタ初期値は0以外.
// プリフィクスD: stepの変更（未実装）.
// bitはカウンタ比較値のほうで指定できる.
{
	int v0v = 0, v0t = 0, v0b = BIT_UNKNOWN, rem2, i;
	DecodeForLoop *dfl = &s->floop[s->floopDepth];
	if (s->flagD != 0) { s->err = 1; return; } // 未実装.
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
	rem2 = s->getIntOrg;
	if (dfl->v1t != 0)
		rem2 = 0x80520000 | dfl->v1v;
	dfl->r = fcode_getReg(s, 0, 1); // reg.
	fcode_putRemark2(s, rem2);
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
	fcode_putLb(s, 2, s->lastLabel);
	s->floopDepth++;
	fcode_updateRep(s, 0, dfl->r);
	s->flag4 = 0;
	if (s->vecPrfx != -1) {
		if (s->vecPrfx == 0) {
			fprintf(stderr, "fcode_ope06: vecPrfx == 0\n");
			exit(1);
		}
		if (s->vecPrfxMode != 0) {
			fprintf(stderr, "fcode_ope06: vecPrfxMode != 0\n");
			exit(1);
		}
		for (i = 1; i < s->vecPrfx; i++) {
			s->lastLabel++;
			dfl[i].label = s->lastLabel;
			dfl[i].v1v   = dfl->v1v;
			dfl[i].v1t   = dfl->v1t;
			dfl[i].step  = dfl->step;
			dfl[i].bit   = dfl->bit;
			dfl[i].r     = dfl->r + i;
			if (v0t == 0)
				fcode_putLimm(s, dfl->bit, dfl->r + i, v0v);
			else
				fcode_putCp(s, dfl->bit, dfl->r + i, v0v);
			fcode_putLb(s, 2, s->lastLabel);
			s->floopDepth++;
			fcode_updateRep(s, 0, dfl->r + i);
		}
		fcode_updateRep(s, 0, dfl->r);
		s->vecPrfx = -1;
	}
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

void fcode_ope08(DecodeFcodeStr *s)
{
	int p, typ, i, r, ofs = 0, ofsTyp = 0;
	p = fcode_getReg(s, 1, 0);
	typ = s->pxxTyp[p];
	if (typ == PTR_TYP_NULL || s->flagD != 0)
		s->pxxTyp[p] = typ = hh4ReaderGetUnsigned(&s->hh4r);
	i = hh4ReaderGetUnsigned(&s->hh4r);
	if (i == 1) {
		ofs = fcode_getInteger(s, len3table14);
		ofsTyp = s->getIntTyp;
	}
	s->getRegBit = 32;
	r = fcode_getReg(s, 0, MODE_REG_LC3);
	if (i == 0) {
		fcode_putOpecode1(s, 0x88);
		fcode_putP(s, p);
		fcode_putTyp(s, typ);
		fcode_putInt32(s, 0);
		fcode_putR(s, r);
 		fcode_putBit(s, s->getRegBit);
		if (s->flag4 == 0) {
			fcode_putLimm(s, 32, 0x3f, 1);
			fcode_putPadd(s, 32, p, typ, p, 0x3f);
		}
		s->bitR[r] = s->getRegBit;
		fcode_updateRep(s, 0, r);
		fcode_updateRep(s, 1, p);
		if (s->vecPrfx != -1) {
			if (s->vecPrfx == 0) {
				fprintf(stderr, "fcode_ope08: vecPrfx == 0\n");
				exit(1);
			}
			if (s->vecPrfxMode != 0) {
				fprintf(stderr, "fcode_ope08: vecPrfxMode != 0\n");
				exit(1);
			}
			for (i = 1; i < s->vecPrfx; i++) {
				fcode_putOpecode1(s, 0x88);
				fcode_putP(s, p);
				fcode_putTyp(s, typ);
				fcode_putInt32(s, 0);
				fcode_putR(s, r + i);
 				fcode_putBit(s, s->getRegBit);
				if (s->flag4 == 0) {
					fcode_putLimm(s, 32, 0x3f, 1);
					fcode_putPadd(s, 32, p, typ, p, 0x3f);
				}
				s->bitR[r + i] = s->getRegBit;
				fcode_updateRep(s, 0, r + i);
			}
			fcode_updateRep(s, 0, r);
			s->vecPrfx = -1;
		}
		s->flagD = s->flag4 = 0;
		goto fin;
	}
	if (i == 1 && s->flag4 == 0) {
		if (ofsTyp == 0) {
			fcode_putLimm(s, 32, 0x3f, ofs);
			ofs = 0x3f;
		}
		fcode_putPadd(s, 32, 0x3f, typ, p, ofs);
		fcode_putOpecode1(s, 0x88);
		fcode_putP(s, 0x3f);
		fcode_putTyp(s, typ);
		fcode_putInt32(s, 0);
		fcode_putR(s, r);
 		fcode_putBit(s, s->getRegBit);
		s->flagD = 0;
		if (ofs != 0x3f)
			fcode_updateRep(s, 0, ofs);
		fcode_updateRep(s, 0, r);
		fcode_updateRep(s, 1, p);
		s->bitR[r] = s->getRegBit;
		goto fin;
	}
	s->err = 1;
fin:
	return;
}

void fcode_ope09(DecodeFcodeStr *s)
{
	int p, typ, i, r, rTyp, ofs = 0, ofsTyp = 0, regBit;
	s->getIntBit = 32;
	r = fcode_getInteger(s, len3table0);
	rTyp = s->getIntTyp;
	regBit = s->getIntBit;
	p = fcode_getReg(s, 1, 0);
	typ = s->pxxTyp[p];
	if (typ == PTR_TYP_NULL || s->flagD != 0)
		s->pxxTyp[p] = typ = hh4ReaderGetUnsigned(&s->hh4r);
	i = hh4ReaderGetUnsigned(&s->hh4r);
	if (i == 1) {
		ofs = fcode_getInteger(s, len3table14);
		ofsTyp = s->getIntTyp;
	}
	if (i == 0) {
		if (rTyp == 0) {
			fcode_putLimm(s, regBit, 0x3f, r);
			r = 0x3f;
		}
		fcode_putOpecode1(s, 0x89);
		fcode_putR(s, r);
 		fcode_putBit(s, regBit);
		fcode_putP(s, p);
		fcode_putTyp(s, typ);
		fcode_putInt32(s, 0);
		if (s->flag4 == 0) {
			fcode_putLimm(s, 32, 0x3f, 1);
			fcode_putPadd(s, 32, p, typ, p, 0x3f);
		}
		s->flagD = s->flag4 = 0;
		if (r != 0x3f)
			fcode_updateRep(s, 0, r);
		fcode_updateRep(s, 1, p);
		s->bitR[r] = regBit;
		goto fin;
	}
	if (i == 1 && s->flag4 == 0) {
		if (ofsTyp == 0) {
			fcode_putLimm(s, 32, 0x3f, ofs);
			ofs = 0x3f;
		}
		fcode_putPadd(s, 32, 0x3f, typ, p, ofs);
		if (rTyp == 0) {
			fcode_putLimm(s, regBit, 0x3f, r);
			r = 0x3f;
		}
		fcode_putOpecode1(s, 0x89);
		fcode_putR(s, r);
 		fcode_putBit(s, regBit);
		fcode_putP(s, 0x3f);
		fcode_putTyp(s, typ);
		fcode_putInt32(s, 0);
		s->flagD = 0;
		if (ofs != 0x3f)
			fcode_updateRep(s, 0, ofs);
		if (r != 0x3f)
			fcode_updateRep(s, 0, r);
		fcode_updateRep(s, 1, p);
		s->bitR[r] = regBit;
		goto fin;
	}
	fprintf(stderr, "fcode_ope09: Internal error.\n");
	s->err = 1;
fin:
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
		{ -1, R1,  1,  2,  3,  4, R0 }, // ADD
		{ R1, R0,  1,  2,  3,  4,  5 }, // SUB
		{ 10, R0, R1,  7,  3,  6,  5 }, // MUL
		{ -1,  0,  1,  2,  3,  4, R0 }, // SBR
		{ R1, R0,  1,  2,  3,  4,  5 }, // SHL
		{ R1, R0,  1,  2,  3,  4,  5 }, // SAR
		{ 10, R0, R1,  7,  3,  6,  5 }, // DIV
		{ 10, R0, R1,  7,  3,  6,  5 }, // MOD
	};
	int i, r0, r1, bit, tmp;
	r1 = fcode_getReg(s, 0, 0);
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
		r0 = fcode_getReg(s, 0, MODE_REG_LC3);
	if (s->getIntTyp == 0) {
		fcode_putLimm(s, bit, 0x3f, i);
		i = 0x3f;
	}
	if ((opecode & 0x0f) == 7) {
		opecode ^= 2;
		s->flagD ^= 1;
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

void fcode_opeCmp(DecodeFcodeStr *s, int opecode)
{
	int r1, i, r0 = 0x3f, bit1, bit0 = 1;
	r1 = fcode_getReg(s, 0, 0);
	s->getIntBit = BIT_UNKNOWN;
	i = fcode_getInteger(s, len3table0);
	bit1 = s->getIntBit;
	if (bit1 == BIT_UNKNOWN) {
		bit1 = s->bitR[r1];
		if (bit1 == BIT_UNKNOWN)
			bit1 = 32;
	}
	if (s->getIntTyp == 0) {
		fcode_putLimm(s, bit1, 0x3f, i);
		i = 0x3f;
	}
	if (s->flagD != 0) {
		s->getRegBit = 32;
		r0 = fcode_getReg(s, 0, MODE_REG_LC3);
		bit0 = s->getRegBit;
	}
	fcode_putCmp(s, opecode | 0x80, bit0, bit1, r0, r1, i);
	fcode_updateRep(s, 0, r1);
	if (i  != 0x3f) fcode_updateRep(s, 0, i);
	if (r0 != 0x3f) fcode_updateRep(s, 0, r0);
	if (s->flagD == 0) {
		i = 1;
		if (s->flag4 != 0)
			i = fcode_getSigned(s);
		if (s->waitLb0 < i)
			s->waitLb0 = i;
		i |= 0x80000000; // 後で補正するためにマークする.
		i &= 0x80ffffff; // 相対補正.
		fcode_putCnd(s, 0x3f);
		fcode_putPlimm(s, 0x3f, i);
	}
	s->flag4 = s->flagD = 0;
	return;
}

void fcode_ope2e(DecodeFcodeStr *s)
{
	int i, typ, len, typSize0, typSize1, typSign;
	if (s->flag4 == 0) {
		s->lastLabel++;
		fcode_putLb(s, 1, s->lastLabel);
	}
	fcode_putOpecode1(s, 0xae);
	typ = hh4ReaderGetUnsigned(&s->hh4r);
	len = hh4ReaderGetUnsigned(&s->hh4r);
	fcode_putInt32(s, typ);
	fcode_putInt32(s, len);
	getTypSize(typ, &typSize0, &typSize1, &typSign);
	len = len * typSize0;
	if (typSize0 < 0 || (len % 8) != 0) {
		s->err = 1;
		goto fin;
	}
	len /= 8;
	for (i = 0; i < len; i++) {
		if (s->hh4r.p.p < s->q + 16) {
			s->err = 1;
			goto fin;
		}
		*s->q++ = hh4ReaderGet4Nbit(&s->hh4r, 2);
	}
	s->flag4 = 0;
fin:
	return;
}

void fcode_ope3d(DecodeFcodeStr *s)
{
	fcode_putOpecode1(s, 0xbd);
	fcode_putInt32(s, 32);
	fcode_putInt32(s, 32);
	fcode_putInt32(s, 16);
	fcode_putInt32(s, 16);
	fcode_putInt32(s, 64);
	fcode_putInt32(s, 0);
	fcode_putPcp(s, 0x3f, 0x30);
	return;
}

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
	fcode_putPcallP2f(s);
	fcode_unknownBitR(s, 0x30, 0x34);
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
	fcode_putPcallP2f(s);
	fcode_unknownBitR(s, 0x30, 0x36);
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
	fcode_putPcallP2f(s);
	fcode_unknownBitR(s, 0x30, 0x36);
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
	fcode_putPcallP2f(s);
	fcode_unknownBitR(s, 0x30, 0x36);
	return;
}

void fcode_api0009(DecodeFcodeStr *s)
// api_sleep(opt, msec).
{
	fcode_putRemark1(s);
	fcode_putLimm(s, 16, 0x30, 0x0009);
	fcode_putLimmOrCp(s, 16, 0x31); // opt.
	fcode_putLimmOrCp(s, 32, 0x32); // msec.
	fcode_putPcallP2f(s);
	fcode_unknownBitR(s, 0x30, 0x32);
	return;
}

void fcode_api000d(DecodeFcodeStr *s)
// api_inkey(opt).
{
	int r;
	fcode_putRemark1(s);
	fcode_putLimm(s, 16, 0x30, 0x000d);
	fcode_putLimmOrCp(s, 16, 0x31); // opt.
	fcode_putPcallP2f(s);
	r = fcode_getReg(s, 0, MODE_REG_LC3);
	fcode_putCp(s, 32, r, 0x30);
	fcode_updateRep(s, 0, 0x32);
	fcode_updateRep(s, 0, 0x31);
	fcode_updateRep(s, 0, r);
	s->bitR[0x30] = 32;
	s->bitR[0x31] = 32;
	s->bitR[0x32] = 32;
	return;
}

void fcode_api0010(DecodeFcodeStr *s)
// api_openWin(xsiz, ysiz).
{
	fcode_putRemark1(s);
	fcode_putLimm(s, 16, 0x30, 0x0010);
	if (s->flag4 != 0) {
		fcode_putLimmOrCp(s, 16, 0x31);
		fcode_putLimmOrCp(s, 32, 0x32);
		s->flag4 = 0;
	} else {
		fcode_putLimm(s, 16, 0x31, 0);
		fcode_putLimm(s, 32, 0x32, 0);
	}
	fcode_putLimmOrCp(s, 16, 0x33); // xsiz.
	fcode_putLimmOrCp(s, 16, 0x34); // ysiz.
	fcode_putPcallP2f(s);
	fcode_unknownBitR(s, 0x30, 0x34);
	return;
}

int fcode_getInt32(const unsigned char *p)
{
	return p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
}

int fcode_getInstrLength(const unsigned char *p)
{
	int len = 0, i, j, typSize0, typSize1, typSign;
	unsigned char ope = *p, ope1;
	if (ope == 0xf0) len = 1;	// nop
	if (ope == 0xf1) len = 1 + 6 + 6;	// putLb
	if (ope == 0xf2) len = 1 + 6 + 1 + 6; // putLimm
	if (ope == 0xf3) len = 1 + 6 + 1;	// putPlimm
	if (ope == 0xf4) len = 1 + 1;	// putCnd
	if (ope == 0x88) len = 1 + 1 + 6 + 6 + 1 + 6;	// ope08
	if (ope == 0x89) len = 1 + 1 + 6 + 1 + 6 + 6;	// ope09
	if (ope == 0x8e) len = 1 + 1 + 6 + 1 + 6 + 1;	// putPadd
	if (0x90 <= ope && ope <= 0x9b && ope != 0x97)
		len = 1 + 3 + 6;	// putAlu
	if (ope == 0x9e) len = 1 + 1 + 1;	// putPcp
	if (0xa0 <= ope && ope <= 0xa7)
		len = 1 + 2 + 6 + 1 + 6;	// putCmp
	if (ope == 0xae) {	// ope2e
		i = fcode_getInt32(&p[1]);
		j = fcode_getInt32(&p[7]);
		getTypSize(i, &typSize0, &typSize1, &typSign);
		len = 1 + 6 + 6 + (typSize0 * j / 8);
	}
	if (ope == 0xb2) len = 16;
	if (0xbc <= ope && ope <= 0xbd) len = 37;
	if (ope == 0xfc && p[1] == 0xfd) len = 9;
	if (ope == 0xfc && p[1] == 0xfe) {
		ope1 = p[2];
		if (ope1 == 0x00 || ope1 == 0x10 || ope1 == 0x30)
			len = 3;	// putRemark0, putRemark1, putRemark3
		if (ope1 == 0x21) len = 3 + 6;	// putRemark2
		if (ope1 == 0xb4 || ope1 == 0xb6)
			len = 4;
	}
	if (len == 0) {
		fprintf(stderr, "fcode_getInstrLength: internal error: ope=0x%02X 0x%02X 0x%02X\n", ope, p[1], p[2]);
		exit(1);
		// これに引っかかるのは、アプリコードのバグではない.
		// なぜならfcodeの結果として生成したコードの問題だから.
	}
	return len;
}

int fcode_initLabel(DecodeFcodeStr *s, unsigned char *q0, unsigned char *q1)
{
	int i, j, k, buf[16], bp = 0, retcode = 0;
	unsigned char *q;

	for (i = 0; i < DEFINES_MAXLABELS; i++) {
		s->label[i].opt = -1;	// undefined
		s->label[i].typ = PTR_TYP_INVALID;
		s->label[i].abs = -1;
	}

	// LB命令を探してopt[]に登録.
	for (q = q0; q < q1; ) {
		if (*q == 0xf1) {
			i = fcode_getInt32(&q[1]);
			j = fcode_getInt32(&q[7]);
			if (s->label[i].opt >= 0) goto err; // 二重定義を検出.
			s->label[i].opt = j;
			s->label[i].typ = PTR_TYP_CODE;
			if (bp >= 16) goto err; // 16以上のLB命令の連続には対応できない.
			buf[bp++] = i; // LB命令が連続したらバッファにためる.
		}
		if (*q == 0xae) {
			j = fcode_getInt32(&q[1]);
			for (i = 0; i < bp; i++)
				s->label[buf[i]].typ = j;
		}
		if (*q != 0xf1)
			bp = 0;
		q += fcode_getInstrLength(q);
	}

	// opt=0,1について、abs[]を設定.
	j = k = 0;
	for (i = 0; i < DEFINES_MAXLABELS; i++) {
		if (s->label[i].opt == 0) s->label[i].abs = j++;
		if (s->label[i].opt == 1) s->label[i].abs = k++;
	}
	goto fin;
err:
	retcode = 1;
fin:
	return retcode;
}

int fcode_fixLabelRelative(DecodeFcodeStr *s, int i, int imm, int min);
int fcode_fixLabelAbsolute(DecodeFcodeStr *s, int i, int imm, int min);

int fcode_fixLabel(DecodeFcodeStr *s, unsigned char *q0, unsigned char *q1)
{
	int i, j, lastLabel = -1, retcode = 0;
	unsigned char *q, uimmHigh;

	for (q = q0; q < q1; ) {
		if (*q == 0xf1)
			lastLabel = fcode_getInt32(&q[1]);
		if (*q == 0xf3) {
			i = fcode_getInt32(&q[1]);
			uimmHigh = (i >> 24) & 0xff;
			if (uimmHigh != 0) {
				j = 0;
				if (q[7] != 0xbf) j = 1;
				if (uimmHigh == 0x80) {
					// 相対指定.
					i = fcode_fixLabelRelative(s, lastLabel, i, j);
				} else {
					// 絶対指定.
					i = fcode_fixLabelAbsolute(s, lastLabel, i, j);
				}
				if (i < 0) goto err;
				q[1 + 2] = (i >> 24) & 0xff;
				q[1 + 3] = (i >> 16) & 0xff;
				q[1 + 4] = (i >>  8) & 0xff;
				q[1 + 5] =  i        & 0xff;
			}
		}
		q += fcode_getInstrLength(q);
	}
	goto fin;
err:
	retcode = 1;
fin:
	return retcode;
}

int fcode_fixLabelRelative(DecodeFcodeStr *s, int i, int imm, int min)
{
	// imm値の補正.
	if ((imm & 0x00800000) != 0)
		imm |= 0xff800000;
	else
		imm &= 0x007fffff;

	// 原点の検索.
	for (;;) {
		if (i < 0) break;
		if (min <= s->label[i].opt && s->label[i].opt <= 1) {
			if (min == 1) break;
			if (min == 0 && s->label[i].typ == PTR_TYP_CODE) break;
		}
		i--;
	}
	if (i < 0) {
		imm--;
		for (i = 0; i < DEFINES_MAXLABELS; i++) {
			if (min <= s->label[i].opt && s->label[i].opt <= 1) {
				if (min == 1) break;
				if (min == 0 && s->label[i].typ == PTR_TYP_CODE) break;
			}
		}
		if (i >= DEFINES_MAXLABELS) goto err;
	}

	if (imm > 0) {
		do {
			for (;;) {
				i++;
				if (i >= DEFINES_MAXLABELS) goto err;
				if (min <= s->label[i].opt && s->label[i].opt <= 1) {
					if (min == 1) break;
					if (min == 0 && s->label[i].typ == PTR_TYP_CODE) break;
				}
			}
			imm--;
		} while (imm > 0);
	}
	if (imm < 0) {
		do {
			for (;;) {
				i--;
				if (i < 0) goto err;
				if (min <= s->label[i].opt && s->label[i].opt <= 1) {
					if (min == 1) break;
					if (min == 0 && s->label[i].typ == PTR_TYP_CODE) break;
				}
			}
			imm++;
		} while (imm < 0);
	}
	goto fin;
err:
	i = -1;
fin:
	return i;
}

int fcode_fixLabelAbsolute(DecodeFcodeStr *s, int lastLabel, int imm, int min)
{
	fprintf(stderr, "fcode_fixLabelAbsolute: Internal error.\n");
	exit(1);
}

int fcode_fixTypSub(int *pTyp, int p, unsigned char *typ);

int fcode_fixTyp(DecodeFcodeStr *s, unsigned char *q0, unsigned char *q1)
{
	int i, j, pTyp[0x40], retcode = 0;
	unsigned char *q;
	for (i = 0; i < 0x40; i++)
		pTyp[i] = PTR_TYP_INVALID;
	j = 1;
	for (i = 0; i < DEFINES_MAXLABELS; i++) {
		if (j > 4) break;
		if (s->label[i].opt != 1) continue;
		if (s->label[i].typ == PTR_TYP_CODE) continue;
		pTyp[j] = s->label[i].typ;
		j++;
	}
	for (q = q0; q < q1; ) {
		if (*q == 0xf3) {	// PLIMM
			i = fcode_getInt32(&q[1]);
			j = q[7] & 0x3f;
			if (j != 0x3f)
				pTyp[j] = s->label[i].typ;
		}
		if (*q == 0x88) {	// LMEM
			i = fcode_fixTypSub(pTyp, q[1], &q[2]);
			if (i == PTR_TYP_INVALID) goto err;
		}
		if (*q == 0x8e) {	// PADD
			i = fcode_fixTypSub(pTyp, q[1], &q[2]);
			if (i == PTR_TYP_INVALID) goto err;
			j = q[15] & 0x3f; // p0
			pTyp[j] = i;
		}
		q += fcode_getInstrLength(q);
	}
	goto fin;
err:
	retcode = 1;
fin:
	return retcode;
}

int fcode_fixTypSub(int *pTyp, int p, unsigned char *typ32)
{
	int typ = fcode_getInt32(typ32);
	p &= 0x3f;
	if (typ != PTR_TYP_INVALID)
		pTyp[p] = typ;
	else {
		typ = pTyp[p];
		if (typ > 0) {
			typ32[2] = (typ >> 24) & 0xff;
			typ32[3] = (typ >> 16) & 0xff;
			typ32[4] = (typ >>  8) & 0xff;
			typ32[5] =  typ        & 0xff;
		} else
			typ = PTR_TYP_INVALID;
	}
	return typ;
}

void fcode_unknownBitR(DecodeFcodeStr *s, int r0, int r1)
{
	int r;
	for (r = r0; r <= r1; r++)
		s->bitR[r] = BIT_UNKNOWN;
	return;
}

#include "osecpu-vm.h"

// initŠÖŒW.

void definesInit(Defines *def)
{
	int i;
	for (i = 0; i < DEFINES_MAXLABELS; i++) {
		def->label[i].typ = PTR_TYP_INVALID; // –¢Žg—p.
		def->label[i].opt = -1; // –¢Žg—p.
	}
	return;
}

// hh4ŠÖŒW.

void hh4ReaderInit(Hh4Reader *hh4r, void *p, int half, void *p1, int half1)
{
	hh4r->p.p = p;
	hh4r->p.half = half;
	hh4r->p1.p = p1;
	hh4r->p1.half = half1;
	hh4r->errorCode = 0;
	return;
}

int hh4ReaderEnd(Hh4Reader *hh4r)
{
	int retCode = 0;
	if (hh4r->p.p > hh4r->p1.p)
		retCode = 1;
	if (hh4r->p.p == hh4r->p1.p && hh4r->p.half >= hh4r->p1.half)
		retCode = 1;
	return retCode;
}

int hh4ReaderGet4bit(Hh4Reader *hh4r)
{
	int value = 0;
	if (hh4ReaderEnd(hh4r) != 0) {
		hh4r->errorCode = 1;
		goto fin;
	}
	value = *hh4r->p.p;
	if (hh4r->p.half == 0) {
		value >>= 4;
		hh4r->p.half = 1;
	} else {
		value &= 0xf;
		hh4r->p.p++;
		hh4r->p.half = 0;
	}
fin:
	return value;
}

Int32 hh4ReaderGetUnsigned(Hh4Reader *hh4r)
{
	int i = hh4ReaderGet4bit(hh4r), len = 3;
	if (i <= 0x6)
		;	// 0xxxŒ^
	else if (i == 0x7) {
		len = hh4ReaderGetUnsigned(hh4r) * 4;
		if (len > 32) {
			hh4r->errorCode = 1;
			len = 0;
		}
		int j;
		i = 0;
		for (j = len; j > 0; j -= 4) {
			i <<= 4;
			i |= hh4ReaderGet4bit(hh4r);
		}
	} else if (i <= 0xb) {	// 10xx_xxxxŒ^
		i = i << 4 | hh4ReaderGet4bit(hh4r);
		len = 6;
		i &= 0x3f;
	} else if (i <= 0xd) {	// 110x_xxxx_xxxxŒ^
		i = i << 8 | hh4ReaderGet4bit(hh4r) << 4;
		i |= hh4ReaderGet4bit(hh4r);
		len = 9;
		i &= 0x1ff;
	} else if (i == 0xe) {	// 1110_xxxx_xxxx_xxxxŒ^
		i = hh4ReaderGet4bit(hh4r) << 8;
		i |= hh4ReaderGet4bit(hh4r) << 4;
		i |= hh4ReaderGet4bit(hh4r);
		len = 12;
	} else { // 0x0f‚Í“Ç‚Ý”ò‚Î‚·.
		i = hh4ReaderGetUnsigned(hh4r);
		len = hh4r->length;
	}
	hh4r->length = len;
	return i;
}

Int32 hh4ReaderGetSigned(Hh4Reader *hh4r)
{
	Int32 i = hh4ReaderGetUnsigned(hh4r);
	int len = hh4r->length;
	if (0 < len && len <= 31 && i >= (1 << (len - 1)))
		i -= 1 << len; // MSB‚ª1‚È‚çˆø‚«ŽZ‚µ‚Ä•‰”‚É‚·‚é.
	return i;
}

Int32 hh4ReaderGet4Nbit(Hh4Reader *hh4r, int n)
{
	int i;
	Int32 value = 0;
	for (i = 0; i < n; i++)
		value = value << 4 | hh4ReaderGet4bit(hh4r);
	return value;
}

// bitReaderŠÖŒW.

void bitReaderInit(BitReader *br, Hh4Reader *hh4r)
{
	br->hh4r = hh4r;
	br->bitBuf = 0;
	br->bufLen = 0;
	return;
}

int bitReaderGet(BitReader *br)
{
	int b;
	if (br->bufLen <= 0) {
		br->bitBuf = hh4ReaderGet4bit(br->hh4r);
		br->bufLen = 4;
	}
	br->bufLen--;
	b = (br->bitBuf >> br->bufLen) & 1;
	return b;
}

int bitReaderGetNbitUnsigned(BitReader *br, int n)
{
	int i, value = 0;
	for (i = 0; i < n; i++)
		value = value << 1 | bitReaderGet(br);
	return value;
}

int bitReaderGetNbitSigned(BitReader *br, int n)
{
	int value = bitReaderGetNbitUnsigned(br, n);
	if (n <= 31 && ((value >> (n - 1)) & 1) != 0)
		value |= -1 << n;
	return value;
}

// jitcŠÖŒW.

void jitcInitDstLogSetPhase(OsecpuJitc *jitc, int phase)
{
	int i;
	for (i = 0; i < JITC_DSTLOG_SIZE; i++)
		jitc->dstLog[i] = NULL;
	jitc->dstLogIndex = 0;
	for (i = 0; i < PREFIX2F_SIZE; i++)
		jitc->prefix2f[i] = 0;
	jitc->phase = phase;
	jitcInitInteger(jitc);
	jitcInitOther(jitc);
	jitcInitPointer(jitc);
	jitcInitFloat(jitc);
	jitcInitExtend(jitc);
	return;
}

void jitcSetRetCode(int *pRC, int value)
{
	if (*pRC <= 0)
		*pRC = value; // ‚à‚µ0ˆÈ‰º‚È‚çã‘‚«‚·‚é, ³‚È‚ç‘‚«Š·‚¦‚È‚¢.
	return;
}

void jitcSetHh4BufferSimple(OsecpuJitc *jitc, int length)
{
	int i;
	for (i = 1; i < length; i++)
		jitc->hh4Buffer[i] = hh4ReaderGetUnsigned(&jitc->hh4r);
	jitc->instrLength = length;
	return;
}

int jitcStep(OsecpuJitc *jitc)
{
	int retcode = -1, *pRC = &retcode, i;
	jitc->hh4Buffer[0] = hh4ReaderGetUnsigned(&jitc->hh4r);
	retcode = jitcStepInteger(jitc);	if (retcode >= 0) goto fin;
	retcode = jitcStepOther(jitc);		if (retcode >= 0) goto fin;
	retcode = jitcStepPointer(jitc);	if (retcode >= 0) goto fin;
	retcode = jitcStepFloat(jitc);		if (retcode >= 0) goto fin;
	retcode = jitcStepExtend(jitc);		if (retcode >= 0) goto fin;
	retcode = JITC_BAD_OPECODE;
fin:
	if (jitc->hh4r.errorCode != 0)
		jitcSetRetCode(pRC, JITC_SRC_OVERRUN);
	if (jitc->dst + jitc->instrLength > jitc->dst1)
		jitcSetRetCode(pRC, JITC_DST_OVERRUN);
	if (retcode != 0)
		goto fin1;
	for (i = 0; i < jitc->instrLength; i++)
		jitc->dst[i] = jitc->hh4Buffer[i];
	retcode = jitcAfterStepInteger(jitc);	if (retcode > 0) goto fin1;
	retcode = jitcAfterStepOther(jitc);		if (retcode > 0) goto fin1;
	retcode = jitcAfterStepPointer(jitc);	if (retcode > 0) goto fin1;
	retcode = jitcAfterStepFloat(jitc);		if (retcode > 0) goto fin1;
	retcode = jitcAfterStepExtend(jitc);	if (retcode > 0) goto fin1;
	if (jitc->instrLength > 0) {
		i = jitc->dstLogIndex;
		jitc->dstLog[i] = jitc->dst; // ƒGƒ‰[‚Ì‚È‚©‚Á‚½–½—ß‚Í‹L˜^‚·‚é.
		jitc->dstLogIndex = (i + 1) % JITC_DSTLOG_SIZE;
		jitc->dst += jitc->instrLength;
	}
fin1:
	jitc->errorCode = retcode;
	return retcode;
}

int jitcAll(OsecpuJitc *jitc)
{
	Hh4ReaderPointer src0 = jitc->hh4r.p;
	Int32 *dst0 = jitc->dst;
	definesInit(jitc->defines);
	jitcInitDstLogSetPhase(jitc, 0);
	for (;;) {
		// —‘z‚Í“K“–‚ÈãŒÀ‚ðŒˆ‚ß‚ÄA‹x‚Ý‹x‚Ý‚Å‚â‚é‚×‚«‚©‚à‚µ‚ê‚È‚¢.
		if (hh4ReaderEnd(&jitc->hh4r) != 0) break;
		jitcStep(jitc);
		if (jitc->errorCode != 0) break;
	}
	if (jitc->errorCode != 0) goto fin;
	jitc->hh4r.p = src0;
	jitc->dst = dst0;
	jitcInitDstLogSetPhase(jitc, 1);
	for (;;) {
		// —‘z‚Í“K“–‚ÈãŒÀ‚ðŒˆ‚ß‚ÄA‹x‚Ý‹x‚Ý‚Å‚â‚é‚×‚«‚©‚à‚µ‚ê‚È‚¢.
		jitcStep(jitc);
		if (jitc->errorCode != 0) break;
		if (hh4ReaderEnd(&jitc->hh4r) != 0) break;
	}
fin:
	return jitc->errorCode;
}

// execŠÖŒW.

int execStep(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	vm->errorCode = 0;
	if (ip >= vm->ip1) {
 		vm->errorCode = EXEC_SRC_OVERRUN;
		goto fin;
	}
	if (*ip != 0xfd && *ip != 0x01ff) {
		vm->execSteps0++;
		if (vm->execSteps0 >= 0x7fffffff || vm->execSteps0 < 0)
			vm->execSteps0 = 0x7fffffff;
		if (vm->execSteps0 >= 1 << 30) {
			vm->execSteps1 += vm->execSteps0 >> 30;
			vm->execSteps0 &= (1 << 30) - 1;
			if (vm->execSteps1 >= 0x7fffffff || vm->execSteps1 < 0)
				vm->execSteps1 = 0x7fffffff;
		}
		if (vm->execSteps0Limit >= 0 && vm->execSteps0 > vm->execSteps0Limit) {
 			vm->errorCode = EXEC_EXECSTEP_OVER;
			goto fin;
		}
		if (vm->execSteps1Limit >= 0 && vm->execSteps1 > vm->execSteps1Limit) {
 			vm->errorCode = EXEC_EXECSTEP_OVER;
			goto fin;
		}
	}
	if (vm->disableDebug == 0)
		execStepDebug(vm);
	execStepInteger(vm);	if (ip != vm->ip || vm->errorCode != 0) goto fin;
	execStepOther(vm);		if (ip != vm->ip || vm->errorCode != 0) goto fin;
	execStepPointer(vm);	if (ip != vm->ip || vm->errorCode != 0) goto fin;
	execStepFloat(vm);		if (ip != vm->ip || vm->errorCode != 0) goto fin;
	execStepExtend(vm);		if (ip != vm->ip || vm->errorCode != 0) goto fin;

	fprintf(stderr, "Error: execStep: opecode=0x%02X\n", *ip); // “à•”ƒGƒ‰[.
	exit(1);
fin:
	return vm->errorCode;
}

int execAll(OsecpuVm *vm)
{
	int i;
	for (i = 0; i < PREFIX2F_SIZE; i++)
		vm->prefix2f[i] = 0;
	for (;;) {
		// —‘z‚Í“K“–‚ÈãŒÀ‚ðŒˆ‚ß‚ÄA‹x‚Ý‹x‚Ý‚Å‚â‚é‚×‚«‚©‚à‚µ‚ê‚È‚¢.
		execStep(vm);
		if (vm->errorCode != 0) {
			if (vm->disableDebug == 0 && vm->errorCode != EXEC_SRC_OVERRUN && vm->errorCode != EXEC_EXIT) {
				vm->toDebugMonitor = 1;
				execStepDebug(vm);
			}
			break;
		}
	}
	return vm->errorCode;
}


// ŠÖ˜Aƒc[ƒ‹ŠÖ”.

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


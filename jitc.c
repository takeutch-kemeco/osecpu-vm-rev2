#include "osecpu-vm.h"

int instrLengthSimple(Int32 opecode)
{
	int retcode = 0;
	if (0x00 <= opecode && opecode <= 0x2f) {
		static int table[] = {
			1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			5, 5, 5, 5, 5, 5, 5, 0, 5, 5, 5, 5, 0, 0, 0, 0,
			6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0
		};
		retcode = table[opecode];
	}
	return retcode;
}

int instrLength(const Int32 *src, const Int32 *src1)
{
	Int32 opecode = src[0];
	int retcode = instrLengthSimple(opecode);
	if (retcode > 0) goto fin;
	retcode = ext_instrLength(src, src1);
	if (retcode > 0) goto fin;
	retcode = - JITC_BAD_OPECODE;
fin:
	if (src + retcode > src1)
		retcode = - JITC_SRC_OVERRUN;
	return retcode;
}

void jitcSetRetCode(int *pRC, int value)
{
	if (*pRC == 0)
		*pRC = value; // もし0なら上書きする, 0以外なら書き換えない.
	return;
}

void jitcStep_checkBits32(int *pRC, int bits)
{
	if (!(0 <= bits && bits <= 32))
		jitcSetRetCode(pRC, JITC_BAD_BITS);
	return;
}

void jitcStep_checkRxx(int *pRC, int rxx)
{
	if (!(0x00 <= rxx && rxx <= 0x3f))
		jitcSetRetCode(pRC, JITC_BAD_RXX);
	return;
}

void jitcStep_checkRxxNotR3F(int *pRC, int rxx)
{
	if (!(0x00 <= rxx && rxx <= 0x3e))
		jitcSetRetCode(pRC, JITC_BAD_RXX);
	return;
}

void jitcStep_checkPxx(int *pRC, int pxx)
{
	if (!(0x00 <= pxx && pxx <= 0x3f))
		jitcSetRetCode(pRC, JITC_BAD_PXX);
	return;
}

int jitcStep(Jitc *jitc)
{
	const Int32 *ip = jitc->src;
	Int32 opecode = ip[0], imm;
	int bit, bit0, bit1, r, r0, r1, r2;
	int retcode = 0, *pRC = &retcode, length = instrLength(ip, jitc->src1), i;
	if (length < 0) {
		jitcSetRetCode(pRC, - length);
		goto fin;
	}
	if (opecode == 0x00) { /* NOP */
		goto fin;
	}
	if (opecode == 0x02) { /* LIMM(imm, Rxx, bits); */
		imm = ip[1]; r = ip[2]; bit = ip[3]; 
		jitcStep_checkBits32(pRC, bit);
		jitcStep_checkRxx(pRC, r);
		goto fin;
	}
	if (opecode == 0x03) { /* PLIMM(imm, Pxx); */
		i = ip[1];
		if (!(0 <= i && i <= jitc->defines->maxLabels))
			jitcSetRetCode(pRC, JITC_BAD_LABEL);
		else {
			if (jitc->defines->label[i].typ == 1)
				; /* OK */
			else if (jitc->defines->label[i].typ == 0 && ip[2] == 0x3f)
				; /* OK */
			else
				jitcSetRetCode(pRC, JITC_BAD_LABEL);
		}
		jitcStep_checkPxx(pRC, ip[2]);
		goto fin;
	}
	if (opecode == 0x04) { /* CND(Rxx); */
		r = ip[1];
		jitcStep_checkRxxNotR3F(pRC, r);
		i = ip[2]; // 次のopecode.
		if (i == 0x00 || i == 0x01 || i == 0x04)
			jitcSetRetCode(pRC, JITC_BAD_CND);
		goto fin;
	}
	if (0x10 <= opecode && opecode <= 0x1b && opecode != 0x13 && opecode != 0x17) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		jitcStep_checkRxx(pRC, r1);
		jitcStep_checkRxx(pRC, r2);
		jitcStep_checkRxxNotR3F(pRC, r0);
		jitcStep_checkBits32(pRC, bit);
		goto fin;
	}
	if (opecode == 0x13) {	// SBX.
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		jitcStep_checkRxxNotR3F(pRC, r1);
		jitcStep_checkRxxNotR3F(pRC, r0);
		if (r2 != 0x3f)
			jitcSetRetCode(pRC, JITC_BAD_RXX);
		jitcStep_checkBits32(pRC, bit);
		goto fin;
	}
	if (0x20 <= opecode && opecode <= 0x27) {
		r1 = ip[1]; r2 = ip[2]; bit1 = ip[3]; r0 = ip[4]; bit0 = ip[5];
		jitcStep_checkRxx(pRC, r1);
		jitcStep_checkRxx(pRC, r2);
		jitcStep_checkRxxNotR3F(pRC, r0);
		jitcStep_checkBits32(pRC, bit0);
		jitcStep_checkBits32(pRC, bit1);
		goto fin;
	}
	retcode = ext_jitcStep(jitc);
	goto fin1;

fin:
	if (retcode == 0) {
		if (jitc->dst + length > jitc->dst1)
			retcode = JITC_DST_OVERRUN;
		for (i = 0; i < length; i++) {
			jitc->dst[i] = ip[i];
		}
		jitc->src += length;
		jitc->dst += length;
	}
fin1:
	jitc->errorCode = retcode;
	return retcode;
}

int jitcAll(Jitc *jitc)
{
	for (;;) {
		// 理想は適当な上限を決めて、休み休みでやるべきかもしれない.
		jitcStep(jitc);
		if (jitc->errorCode != 0) break;
	}
	return jitc->errorCode;
}


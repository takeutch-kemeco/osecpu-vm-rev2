#include "osecpu-vm.h"

// init関係.

void definesInit(Defines *def)
{
	int i;
	for (i = 0; i < DEFINES_MAXLABELS; i++) {
		def->label[i].typ = PTR_TYP_INVALID; // 未使用.
		def->label[i].opt = -1; // 未使用.
	}
	return;
}

// jitc関係.

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
		*pRC = value; // もし0以下なら上書きする, 正なら書き換えない.
	return;
}

void jitcSetSrcBufferSimple(OsecpuJitc *jitc, int length)
{
	int i;
	for (i = 1; i < length; i++)
		jitc->srcBuffer[i] = (*(jitc->src)++) & 0x00ffffff; // 76を消している.
	jitc->instrLength = length;
	return;
}

int jitcStep(OsecpuJitc *jitc)
{
	int retcode = -1, *pRC = &retcode, i;
	jitc->srcBuffer[0] = (*(jitc->src)++) & 0x00ffffff;
	retcode = jitcStepInteger(jitc);	if (retcode >= 0) goto fin;
	retcode = jitcStepOther(jitc);		if (retcode >= 0) goto fin;
	retcode = jitcStepPointer(jitc);	if (retcode >= 0) goto fin;
	retcode = jitcStepFloat(jitc);		if (retcode >= 0) goto fin;
	retcode = jitcStepExtend(jitc);		if (retcode >= 0) goto fin;
	retcode = JITC_BAD_OPECODE;
fin:
	if (jitc->src > jitc->src1)
		jitcSetRetCode(pRC, JITC_SRC_OVERRUN);
	if (jitc->dst + jitc->instrLength > jitc->dst1)
		jitcSetRetCode(pRC, JITC_DST_OVERRUN);
	if (retcode != 0)
		goto fin1;
	for (i = 0; i < jitc->instrLength; i++)
		jitc->dst[i] = jitc->srcBuffer[i];
	retcode = jitcAfterStepInteger(jitc);	if (retcode > 0) goto fin1;
	retcode = jitcAfterStepOther(jitc);		if (retcode > 0) goto fin1;
	retcode = jitcAfterStepPointer(jitc);	if (retcode > 0) goto fin1;
	retcode = jitcAfterStepFloat(jitc);		if (retcode > 0) goto fin1;
	retcode = jitcAfterStepExtend(jitc);	if (retcode > 0) goto fin1;
	if (jitc->instrLength > 0) {
		i = jitc->dstLogIndex;
		jitc->dstLog[i] = jitc->dst; // エラーのなかった命令は記録する.
		jitc->dstLogIndex = (i + 1) % JITC_DSTLOG_SIZE;
		jitc->dst += jitc->instrLength;
	}
fin1:
	jitc->errorCode = retcode;
	return retcode;
}

int jitcAll(OsecpuJitc *jitc)
{
	const Int32 *src0 = jitc->src;
	Int32 *dst0 = jitc->dst;
	definesInit(jitc->defines);
	jitcInitDstLogSetPhase(jitc, 0);
	for (;;) {
		// 理想は適当な上限を決めて、休み休みでやるべきかもしれない.
		if (jitc->src >= jitc->src1) break;
		jitcStep(jitc);
		if (jitc->errorCode != 0) break;
	}
	if (jitc->errorCode != 0) goto fin;
	jitc->src = src0;
	jitc->dst = dst0;
	jitcInitDstLogSetPhase(jitc, 1);
	for (;;) {
		// 理想は適当な上限を決めて、休み休みでやるべきかもしれない.
		jitcStep(jitc);
		if (jitc->errorCode != 0) break;
		if (jitc->src >= jitc->src1) break;
	}
fin:
	return jitc->errorCode;
}

// exec関係.

int execStep(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	vm->errorCode = 0;
	if (ip >= vm->ip1) {
 		vm->errorCode = EXEC_SRC_OVERRUN;
		goto fin;
	}
	execStepInteger(vm);	if (ip != vm->ip || vm->errorCode != 0) goto fin;
	execStepOther(vm);		if (ip != vm->ip || vm->errorCode != 0) goto fin;
	execStepPointer(vm);	if (ip != vm->ip || vm->errorCode != 0) goto fin;
	execStepFloat(vm);		if (ip != vm->ip || vm->errorCode != 0) goto fin;
	execStepExtend(vm);		if (ip != vm->ip || vm->errorCode != 0) goto fin;

	fprintf(stderr, "Error: execStep: opecode=0x%02X\n", *ip); // 内部エラー.
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
		// 理想は適当な上限を決めて、休み休みでやるべきかもしれない.
		execStep(vm);
		if (vm->errorCode != 0)
			break;
	}
	return vm->errorCode;
}


// 関連ツール関数.

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


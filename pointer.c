#include "osecpu-vm.h"

// ポインタ関連命令: 01

void osecpuInitPointer()
{
	static int table[] = {
		0, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 // 0x
	};
	instrLengthSimpleInitTool(table, 0x00, 0x0f);
	return;
}

int instrLengthPointer(const Int32 *src, const Int32 *src1)
// instrLengthSimpleInitTool()で登録していないものだけに反応すればよい.
{
	Int32 opecode = src[0];
	int retcode = 0;
	return retcode;
}

Int32 *hh4DecodePointer(OsecpuJitc *jitc, Int32 opecode)
// instrLengthSimpleInitTool()で登録していないものだけに反応すればよい.
{
	HH4Reader *hh4r = jitc->hh4r;
	Int32 *dst = jitc->hh4dst, *dst1 = jitc->hh4dst1;
fin:
	jitc->dst = dst;
	return dst;
err:
	jitc->errorCode = JITC_HH4_DST_OVERRUN;
	return dst;
}

void jitcStep_checkPxx(int *pRC, int pxx)
{
	if (!(0x00 <= pxx && pxx <= 0x3f))
		jitcSetRetCode(pRC, JITC_BAD_PXX);
	return;
}

int jitcStepPointer(OsecpuJitc *jitc)
{
	const Int32 *ip = jitc->src;
	Int32 opecode = ip[0], imm;
	int retcode = -1, *pRC = &retcode;
	int i, opt, p;
	if (opecode == 0x01) { /* LB(opt, uimm); */
		i = ip[1]; opt = ip[2];
		if (jitc->phase > 0) goto fin;
		if (!(0 <= i && i < DEFINES_MAXLABELS)) {
			jitcSetRetCode(pRC, JITC_BAD_LABEL);
			goto fin;
		}
		if (jitc->defines->label[i].typ != 0) {
			jitcSetRetCode(pRC, JITC_LABEL_REDEFINED);
			goto fin;
		}
		if (!(0 <= opt && opt <= 1)) {
			jitcSetRetCode(pRC, JITC_BAD_LABEL_TYPE);
			goto fin;
		}
		jitc->defines->label[i].typ = -1; // とりあえずコードラベルにする.
		jitc->defines->label[i].opt = opt;
		jitc->defines->label[i].dst = jitc->dst;
		goto fin;
	}
	if (opecode == 0x03) { /* PLIMM(imm, Pxx); */
		i = ip[1]; p = ip[2];
		if (!(0 <= i && i < DEFINES_MAXLABELS)) {
			jitcSetRetCode(pRC, JITC_BAD_LABEL);
			goto fin;
		}
		jitcStep_checkPxx(pRC, p);
 		if (jitc->phase == 0) goto fin;
		if (jitc->defines->label[i].typ == 0)
			jitcSetRetCode(pRC, JITC_LABEL_UNDEFINED);
		if (p != 0x3f && jitc->defines->label[i].opt == 0)
			jitcSetRetCode(pRC, JITC_BAD_LABEL_TYPE);
		if (p == 0x3f && jitc->defines->label[i].typ != -1)
			jitcSetRetCode(pRC, JITC_BAD_LABEL_TYPE); // P3Fにデータラベルを代入できない.
		goto fin;
	}
	goto fin1;
fin:
	if (retcode == -1)
		retcode = 0;
fin1:
	return retcode;
}

void execStepPointer(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0];
	int i, p;
	if (opecode == 0x01) { /* LB(opt, uimm); */
		ip += 3;
		goto fin;
	}
	if (opecode == 0x03) { /* PLIMM(imm, Pxx); */
		i = ip[1]; p = ip[2];
		if (p == 0x3f)
			ip = (const Int32 *) vm->defines->label[i].dst;
		else
			ip += 3;
		goto fin;
	}
fin:
	vm->ip = ip;
	return;
}


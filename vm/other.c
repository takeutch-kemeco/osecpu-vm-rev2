#include "osecpu-vm.h"

// その他の命令: 00, 2F, 3C, 3D, FD, FE

void jitcInitOther(OsecpuJitc *jitc)
{
	jitc->ope04 = NULL;
	return;
}

int jitcStepOther(OsecpuJitc *jitc)
{
	Int32 *ip = jitc->hh4Buffer;
	Int32 opecode = ip[0], imm;
	int bit, bit0, bit1, r, r0, r1, r2, p, typ, f;
	int retcode = -1, *pRC = &retcode;
	int i, j;
	if (opecode == 0x00) { /* NOP */
		jitcSetHh4BufferSimple(jitc, 1);
		goto fin;
	}
	if (opecode == 0x2f) {
		jitcSetHh4BufferSimple(jitc, 2);
		i = ip[1];
		if (i < PREFIX2F_SIZE && jitc->prefix2f[i] == 0)
			jitc->prefix2f[i] = 1;
		else
			jitcSetRetCode(pRC, JITC_BAD_PREFIX);
		goto fin;
	}
	if (opecode == 0x3c) {
		jitcSetHh4BufferSimple(jitc, 7);
		r = ip[1]; p = ip[2]; f = ip[3]; bit0 = ip[4]; bit1 = ip[5]; typ = ip[6];
		if (typ != PTR_TYP_NULL)
			jitcSetRetCode(pRC, JITC_UNSUPPORTED);
		goto fin;
	}
	if (opecode == 0x3d) {
		jitcSetHh4BufferSimple(jitc, 7);
		r = ip[1]; p = ip[2]; f = ip[3]; bit0 = ip[4]; bit1 = ip[5]; typ = ip[6];
		if (typ != PTR_TYP_NULL)
			jitcSetRetCode(pRC, JITC_UNSUPPORTED);
		goto fin;
	}
	if (opecode == 0xfd) {
		jitcSetHh4BufferSimple(jitc, 3);
		imm = ip[1]; r = ip[2];
 		if (0 <= r && r <= 3)
			jitc->dr[r] = imm;
		goto fin;
	}
	if (opecode == 0xfe) {	// remark
		jitcSetHh4BufferSimple(jitc, 3);
		imm = ip[1]; i = ip[2];
		jitc->instrLength = 0; // 自前で処理するので、この値は0にする.
		for (j = 0; j < i; j++)
			hh4ReaderGetUnsigned(&jitc->hh4r); // 読み捨てる.
		goto fin;
	}
	goto fin1;
fin:
	if (retcode == -1)
		retcode = 0;
fin1:
	return retcode;
}

int jitcAfterStepOther(OsecpuJitc *jitc)
{
	int i, retcode = 0;
	if (jitc->hh4Buffer[0] != 0x2f) {
		// 未解釈の2Fプリフィクスが残っていないか調査.
		// 解釈したら0クリアするのが作法.
		for (i = 0; i < PREFIX2F_SIZE; i++) {
			if (jitc->prefix2f[i] != 0)
				retcode = JITC_BAD_PREFIX;
		}
	}
	return 0;
}

void execStepOther(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0], imm;
	int bit, bit0, bit1, r, r0, r1, r2, p, typ, typSign, typSize0, typSize1;
	int i, mbit, tbit;
	if (opecode == 0x00) { // NOP();
		ip++;
		goto fin;
	}
	if (opecode == 0x2f) {
		i = ip[1];
		vm->prefix2f[i] = 1;
		ip += 2;
		goto fin;
	}
	if (opecode == 0xfd) {
		imm = ip[1]; r = ip[2];
 		if (0 <= r && r <= 3)
			vm->dr[r] = imm;
		ip += 3;
		goto fin;
	}
fin:
	if (vm->errorCode <= 0)
		vm->ip = ip;
	return;
}


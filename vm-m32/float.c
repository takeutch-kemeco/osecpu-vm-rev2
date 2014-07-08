#include "osecpu-vm.h"

// 浮動小数点命令: 40-43, 48-4D, 50-53

void jitcInitFloat(OsecpuJitc *jitc)
{
	return;
}

void jitcStep_checkBitsF(int *pRC, int bit)
// vm-miniではdoubleは簡略化のためにサポートしない.
{
	if (bit != 0 && bit != 32)
		jitcSetRetCode(pRC, JITC_BAD_BITS);
	return;
}

int jitcStepFloat(OsecpuJitc *jitc)
{
	Int32 *ip = jitc->srcBuffer;
	Int32 opecode = ip[0];
	int retcode = -1, *pRC = &retcode;
	if (opecode == 0x40) {	// FLIMM
		ip[1] = jitc->src[0];
		if (ip[1] == 0) {
			ip[2] = jitc->src[2]; // imm
			ip[3] = jitc->src[3] & 0x00ffffff; // f
			ip[4] = jitc->src[4] & 0x00ffffff; // bit
			jitc->src += 5;
			jitc->instrLength = 5;
		//	f = ip[3]; bit = ip[4];
			goto fin;
		}
		if (ip[1] == 1) {
			ip[2] = jitc->src[1]; // imm
			ip[3] = jitc->src[3] & 0x00ffffff; // f
			ip[4] = jitc->src[4] & 0x00ffffff; // bit
			jitc->instrLength = 5;
		//	f = ip[3]; bit = ip[4];
			goto fin;
		}
		jitcSetRetCode(pRC, JITC_BAD_FLIMM_MODE);
		goto fin;
	}
	if (opecode == 0x41) {	// FCP
		jitcSetSrcBufferSimple(jitc, 5);
	//	f1 = ip[1]; bit1 = ip[2]; f0 = ip[3]; bit0 = ip[4];
		goto fin; // bit0とbit1の大小関係に制約はない. bit0>bit1の場合は精度を拡張することになる.
	}
	if (opecode == 0x42) {	// CNVIF
		jitcSetSrcBufferSimple(jitc, 5);
	//	r = ip[1]; bit1 = ip[2]; f = ip[3]; bit0 = ip[4];
		goto fin;
	}
	if (opecode == 0x43) {	// CNVFI
		jitcSetSrcBufferSimple(jitc, 5);
	//	f = ip[1]; bit1 = ip[2]; r = ip[3]; bit0 = ip[4];
		jitc->prefix2f[0] = 0; // 2F-0.
		goto fin;
	}
	if (0x48 <= opecode && opecode <= 0x4d) {
		jitcSetSrcBufferSimple(jitc, 6);
	//	f1 = ip[1]; f2 = ip[2]; bit1 = ip[3]; r = ip[4]; bit0 = ip[5];
		goto fin;
	}
	if (0x50 <= opecode && opecode <= 0x53) {
		jitcSetSrcBufferSimple(jitc, 5);
	//	f1 = ip[1]; f2 = ip[2]; f0 = ip[3]; bit = ip[4];
		goto fin;
	}

	goto fin1;
fin:
	if (retcode == -1)
		retcode = 0;
fin1:
	return retcode;
}

int jitcAfterStepFloat(OsecpuJitc *jitc)
{
	return 0;
}

void execStepFloat(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0];
	int bit, bit0, bit1, r, f, f0, f1, f2, i;
	if (opecode == 0x40) {
		if (ip[1] == 0) {
			f = ip[3]; bit = ip[4];
			vm->f[f] = (double) ip[2];
			ip += 5;
			goto fin;
		}
		if (ip[1] == 1) {
			f = ip[3]; bit = ip[4];
			union {
				float f32;
				int i32; // ここのintは正確に32bitでなければいけないので注意.
			} u32;
			u32.i32 = ip[2];
			vm->f[f] = (double) u32.f32;
			ip += 5;
			goto fin;
		}
	}
	if (opecode == 0x41) {
		f1 = ip[1]; bit1 = ip[2]; f0 = ip[3]; bit0 = ip[4];
		vm->f[f0] = vm->f[f1];
		ip += 5;
		goto fin;
	}
	if (opecode == 0x42) {
		r = ip[1]; bit1 = ip[2]; f = ip[3]; bit0 = ip[4];
		vm->f[f] = (double) vm->r[r];
		ip += 5;
		goto fin;
	}
	if (opecode == 0x43) {
		f = ip[1]; bit1 = ip[2]; r = ip[3]; bit0 = ip[4];
		vm->r[r] = (Int32) vm->f[f];
		ip += 5;
		goto fin;
	}
	if (0x48 <= opecode && opecode <= 0x4d) {
		f1 = ip[1]; f2 = ip[2]; bit1 = ip[3]; r = ip[4]; bit0 = ip[5];
		i = 0;	// gccの警告を黙らせるため.
		if (opecode == 0x48) i = vm->f[f1] == vm->f[f2];
		if (opecode == 0x49) i = vm->f[f1] != vm->f[f2];
		if (opecode == 0x4a) i = vm->f[f1] <  vm->f[f2];
		if (opecode == 0x4b) i = vm->f[f1] >= vm->f[f2];
		if (opecode == 0x4c) i = vm->f[f1] <= vm->f[f2];
		if (opecode == 0x4d) i = vm->f[f1] >  vm->f[f2];
		if (i != 0)
			i = -1;
		vm->r[r] = i;
		ip += 6;
		goto fin;
	}
	if (0x50 <= opecode && opecode <= 0x53) {
		f1 = ip[1]; f2 = ip[2]; f0 = ip[3]; bit = ip[4];
		if (opecode == 0x50) vm->f[f0] = vm->f[f1] + vm->f[f2];
		if (opecode == 0x51) vm->f[f0] = vm->f[f1] - vm->f[f2];
		if (opecode == 0x52) vm->f[f0] = vm->f[f1] * vm->f[f2];
		if (opecode == 0x53) vm->f[f0] = vm->f[f1] / vm->f[f2];
		ip += 5;
		goto fin;
	}

fin:
	vm->ip = ip;
	return;
}


#include "osecpu-vm.h"

// 浮動小数点命令: 40-43, 48-4D, 50-53

void jitcInitFloat(OsecpuJitc *jitc)
{
	return;
}

void jitcStep_checkFxx(int *pRC, int fxx)
{
	if (!(0x00 <= fxx && fxx <= 0x3f))
		jitcSetRetCode(pRC, JITC_BAD_FXX);
	return;
}

void jitcStep_checkFxxNotF3F(int *pRC, int fxx)
{
	if (!(0x00 <= fxx && fxx <= 0x3e))
		jitcSetRetCode(pRC, JITC_BAD_FXX);
	return;
}

void jitcStep_checkBitsF(int *pRC, int bit)
{
	if (bit != 0 && bit != 32 && bit != 64)
		jitcSetRetCode(pRC, JITC_BAD_BITS);
	return;
}

int jitcStepFloat(OsecpuJitc *jitc)
{
	Int32 *ip = jitc->hh4Buffer;
	Int32 opecode = ip[0];
	int bit, bit0, bit1, r, f, f0, f1, f2;
	int retcode = -1, *pRC = &retcode;
	if (opecode == 0x40) {	// FLIMM
		ip[1] = hh4ReaderGetUnsigned(&jitc->hh4r);
		if (ip[1] == 0) {
			ip[2] = hh4ReaderGetSigned(&jitc->hh4r); // imm
			ip[3] = hh4ReaderGetUnsigned(&jitc->hh4r); // f
			ip[4] = hh4ReaderGetUnsigned(&jitc->hh4r); // bit
			jitc->instrLength = 5;
			f = ip[3]; bit = ip[4];
			jitcStep_checkBitsF(pRC, bit);
			jitcStep_checkFxx(pRC, f);
			goto fin;
		}
		if (ip[1] == 1) {
			ip[2] = hh4ReaderGet4Nbit(&jitc->hh4r, 32 / 4); // imm
			ip[3] = hh4ReaderGetUnsigned(&jitc->hh4r); // f
			ip[4] = hh4ReaderGetUnsigned(&jitc->hh4r); // bit
			jitc->instrLength = 5;
			f = ip[3]; bit = ip[4];
			jitcStep_checkBitsF(pRC, bit);
			jitcStep_checkFxx(pRC, f);
			goto fin;
		}
		if (ip[1] == 2) {
			ip[2] = hh4ReaderGet4Nbit(&jitc->hh4r, 32 / 4); // imm-high
			ip[3] = hh4ReaderGet4Nbit(&jitc->hh4r, 32 / 4); // imm-low
			ip[4] = hh4ReaderGetUnsigned(&jitc->hh4r); // f
			ip[5] = hh4ReaderGetUnsigned(&jitc->hh4r); // bit
			jitc->instrLength = 6;
			f = ip[4]; bit = ip[5];
			jitcStep_checkBitsF(pRC, bit);
			jitcStep_checkFxx(pRC, f);
			goto fin;
		}
		jitcSetRetCode(pRC, JITC_BAD_FLIMM_MODE);
		goto fin;
	}
	if (opecode == 0x41) {	// FCP
		jitcSetHh4BufferSimple(jitc, 5);
		f1 = ip[1]; bit1 = ip[2]; f0 = ip[3]; bit0 = ip[4];
		jitcStep_checkBitsF(pRC, bit1);
		jitcStep_checkFxx(pRC, f1);
		jitcStep_checkBitsF(pRC, bit0);
		jitcStep_checkFxx(pRC, f0);
		goto fin; // bit0とbit1の大小関係に制約はない. bit0>bit1の場合は精度を拡張することになる.
	}
	if (opecode == 0x42) {	// CNVIF
		jitcSetHh4BufferSimple(jitc, 5);
		r = ip[1]; bit1 = ip[2]; f = ip[3]; bit0 = ip[4];
		jitcStep_checkBits32(pRC, bit1);
		jitcStep_checkRxx(pRC, r);
		jitcStep_checkBitsF(pRC, bit0);
		jitcStep_checkFxxNotF3F(pRC, f);
		goto fin;
	}
	if (opecode == 0x43) {	// CNVFI
		jitcSetHh4BufferSimple(jitc, 5);
		f = ip[1]; bit1 = ip[2]; r = ip[3]; bit0 = ip[4];
		jitcStep_checkBitsF(pRC, bit1);
		jitcStep_checkFxx(pRC, f);
		jitcStep_checkBits32(pRC, bit0);
		jitcStep_checkRxxNotR3F(pRC, r);
		goto fin;
	}
	if (0x48 <= opecode && opecode <= 0x4d) {
		jitcSetHh4BufferSimple(jitc, 6);
		f1 = ip[1]; f2 = ip[2]; bit1 = ip[3]; r = ip[4]; bit0 = ip[5];
		jitcStep_checkFxx(pRC, f1);
		jitcStep_checkFxx(pRC, f2);
		jitcStep_checkBitsF(pRC, bit1);
		jitcStep_checkRxx(pRC, r);
		jitcStep_checkBits32(pRC, bit0);
		goto fin;
	}
	if (0x50 <= opecode && opecode <= 0x53) {
		jitcSetHh4BufferSimple(jitc, 5);
		f1 = ip[1]; f2 = ip[2]; f0 = ip[3]; bit = ip[4];
		jitcStep_checkFxx(pRC, f1);
		jitcStep_checkFxx(pRC, f2);
		jitcStep_checkFxxNotF3F(pRC, f0);
		jitcStep_checkBitsF(pRC, bit);
		goto fin;
	}

	goto fin1;
fin:
	if (retcode == -1)
		retcode = 0;
fin1:
	return retcode;
}

void jitcAfterStepFloat(OsecpuJitc *jitc)
{
	return;
}

void execStepFloat(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0];
	int bit, bit0, bit1, r, f, f0, f1, f2;
	int i;
	if (opecode == 0x40) {
		if (ip[1] == 0) {
			f = ip[3]; bit = ip[4];
			vm->f[f] = (double) ip[2];
			vm->bitF[f] = bit;
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
			vm->bitF[f] = bit;
			ip += 5;
			goto fin;
		}
		if (ip[1] == 2) {
			f = ip[4]; bit = ip[5];
			union {
				double f64;
				int i32[2]; // ここのintは正確に32bitでなければいけないので注意.
			} u64;
			u64.i32[1] = ip[2]; // リトルエンディアンを想定.
			u64.i32[0] = ip[3];
			vm->f[f] = u64.f64;
			vm->bitF[f] = bit;
			ip += 6;
			goto fin;
		}
	}
	if (opecode == 0x41) {
		f1 = ip[1]; bit1 = ip[2]; f0 = ip[3]; bit0 = ip[4];
		if (bit1 != vm->bitF[f1]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		vm->f[f0] = vm->f[f1];
		vm->bitF[f0] = bit0;
		ip += 5;
		goto fin;
	}
	if (opecode == 0x42) {
		r = ip[1]; bit1 = ip[2]; f = ip[3]; bit0 = ip[4];
		if (bit1 > vm->bit[r]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		vm->f[f] = (double) vm->r[r];
		vm->bitF[f] = bit0;
		ip += 5;
		goto fin;
	}
	if (opecode == 0x43) {
		f = ip[1]; bit1 = ip[2]; r = ip[3]; bit0 = ip[4];
		if (bit1 != vm->bitF[f]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		vm->r[r] = (Int32) vm->f[f];
		vm->bit[r] = bit0;
		execStep_checkBitsRange(vm->r[r], bit0, vm);
		ip += 5;
		goto fin;
	}
	if (0x48 <= opecode && opecode <= 0x4d) {
		f1 = ip[1]; f2 = ip[2]; bit1 = ip[3]; r = ip[4]; bit0 = ip[5];
		if (bit1 != vm->bitF[f1] || bit1 != vm->bitF[f2]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (opecode == 0x48) i = vm->f[f1] == vm->f[f2];
		if (opecode == 0x49) i = vm->f[f1] != vm->f[f2];
		if (opecode == 0x4a) i = vm->f[f1] <  vm->f[f2];
		if (opecode == 0x4b) i = vm->f[f1] >= vm->f[f2];
		if (opecode == 0x4c) i = vm->f[f1] <= vm->f[f2];
		if (opecode == 0x4d) i = vm->f[f1] >  vm->f[f2];
		if (i != 0)
			i = -1;
		vm->r[r] = i;
		vm->bit[r] = bit0;
		ip += 6;
		goto fin;
	}
	if (0x50 <= opecode && opecode <= 0x53) {
		f1 = ip[1]; f2 = ip[2]; f0 = ip[3]; bit = ip[4];
		if (bit != vm->bitF[f1] || bit != vm->bitF[f2]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (opecode == 0x50) vm->f[f0] = vm->f[f1] + vm->f[f2];
		if (opecode == 0x51) vm->f[f0] = vm->f[f1] - vm->f[f2];
		if (opecode == 0x52) vm->f[f0] = vm->f[f1] * vm->f[f2];
		if (opecode == 0x53) vm->f[f0] = vm->f[f1] / vm->f[f2];
		vm->bitF[f0] = bit;
		ip += 5;
		goto fin;
	}

fin:
	vm->ip = ip;
	return;
}


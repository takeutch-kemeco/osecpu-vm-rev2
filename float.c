#include "osecpu-vm.h"

// ïÇìÆè¨êîì_ñΩóﬂ: 40-43, 48-4D, 50-53

void OsecpuInitFloat()
{
	static int table[] = {
		0, 5, 5, 5, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 0, 0, // 4x
		5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // 5x
	};
	instrLengthSimpleInitTool(table, 0x40, 0x5f);
	return;
}

int instrLengthFloat(const Int32 *src, const Int32 *src1)
// instrLengthSimpleInitTool()Ç≈ìoò^ÇµÇƒÇ¢Ç»Ç¢Ç‡ÇÃÇæÇØÇ…îΩâûÇ∑ÇÍÇŒÇÊÇ¢.
{
	Int32 opecode = src[0];
	int retcode = 0;
	if (opecode == 0x40) {
		if (src[1] == 0) retcode = 5; // 40, imm-typ(0), imm, f, bit.
		if (src[1] == 1) retcode = 5; // 40, imm-typ(1), imm-f32, f, bit.
		if (src[1] == 2) retcode = 6; // 40, imm-typ(2), imm-f64, f, bit.
	}
	return retcode;
}

Int32 *hh4DecodeFloat(OsecpuJitc *jitc, Int32 opecode)
// instrLengthSimpleInitTool()Ç≈ìoò^ÇµÇƒÇ¢Ç»Ç¢Ç‡ÇÃÇæÇØÇ…îΩâûÇ∑ÇÍÇŒÇÊÇ¢.
{
	HH4Reader *hh4r = jitc->hh4r;
	Int32 *dst = jitc->hh4dst, *dst1 = jitc->hh4dst1;
	int i;
	if (opecode == 0x40) {
		i = hh4GetUnsigned(hh4r);
		if (i == 0) {
 			if (dst + 5 > dst1)
				goto err;
			*dst = opecode;
			dst[1] = i; // imm-typ(0)
			dst[2] = hh4GetSigned(hh4r); // imm
			dst[3] = hh4GetUnsigned(hh4r); // f
			dst[4] = hh4GetUnsigned(hh4r); // bit
			dst += 5;
			goto fin;
		}
		if (i == 1) {
			if (dst + 5 > dst1)
				goto err;
			*dst = opecode;
			dst[1] = i; // imm-typ(1)
			dst[2] = hh4Get4Nbit(hh4r, 32 / 4); // imm
			dst[3] = hh4GetUnsigned(hh4r); // f
			dst[4] = hh4GetUnsigned(hh4r); // bit
			dst += 5;
			goto fin;
		}
		if (i == 2) {
			if (dst + 6 > dst1)
				goto err;
			*dst = opecode;
			dst[1] = i; // imm-typ(2)
			dst[2] = hh4Get4Nbit(hh4r, 32 / 4); // imm-high
			dst[3] = hh4Get4Nbit(hh4r, 32 / 4); // imm-low
			dst[4] = hh4GetUnsigned(hh4r); // f
			dst[5] = hh4GetUnsigned(hh4r); // bit
			dst += 6;
			goto fin;
		}
	}
fin:
	jitc->dst = dst;
	return dst;
err:
	jitc->errorCode = JITC_HH4_DST_OVERRUN;
	return dst;
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
	if (bit != 32 && bit != 64)
		jitcSetRetCode(pRC, JITC_BAD_BITS);
	return;
}

int jitcStepFloat(OsecpuJitc *jitc)
{
	const Int32 *ip = jitc->src;
	Int32 opecode = ip[0];
	int bit, bit0, bit1, r, f, f0, f1, f2;
	int retcode = -1, *pRC = &retcode;
	if (opecode == 0x40) {	// FLIMM
		if (ip[1] == 0 || ip[1] == 1) {
			f = ip[3]; bit = ip[4];
			jitcStep_checkBitsF(pRC, bit);
			jitcStep_checkFxx(pRC, f);
		} else if (ip[1] == 2) {
			f = ip[4]; bit = ip[5];
			jitcStep_checkBitsF(pRC, bit);
			jitcStep_checkFxx(pRC, f);
		} else
			jitcSetRetCode(pRC, JITC_BAD_FLIMM_MODE);
		goto fin;
	}
	if (opecode == 0x41) {	// FCP
		f1 = ip[1]; bit1 = ip[2]; f0 = ip[3]; bit0 = ip[4];
		jitcStep_checkBitsF(pRC, bit1);
		jitcStep_checkFxx(pRC, f1);
		jitcStep_checkBitsF(pRC, bit0);
		jitcStep_checkFxx(pRC, f0);
		goto fin; // bit0Ç∆bit1ÇÃëÂè¨ä÷åWÇ…êßñÒÇÕÇ»Ç¢. bit0>bit1ÇÃèÍçáÇÕê∏ìxÇägí£Ç∑ÇÈÇ±Ç∆Ç…Ç»ÇÈ.
	}
	if (opecode == 0x42) {	// CNVIF
		r = ip[1]; bit1 = ip[2]; f = ip[3]; bit0 = ip[4];
		jitcStep_checkBits32(pRC, bit1);
		jitcStep_checkRxx(pRC, r);
		jitcStep_checkBitsF(pRC, bit0);
		jitcStep_checkFxxNotF3F(pRC, f);
		goto fin;
	}
	if (opecode == 0x43) {	// CNVFI
		f = ip[1]; bit1 = ip[2]; r = ip[3]; bit0 = ip[4];
		jitcStep_checkBitsF(pRC, bit1);
		jitcStep_checkFxx(pRC, f);
		jitcStep_checkBits32(pRC, bit0);
		jitcStep_checkRxxNotR3F(pRC, r);
		goto fin;
	}
	if (0x48 <= opecode && opecode <= 0x4d) {
		f1 = ip[1]; f2 = ip[2]; bit1 = ip[3]; r = ip[4]; bit0 = ip[5];
		jitcStep_checkFxx(pRC, f1);
		jitcStep_checkFxx(pRC, f2);
		jitcStep_checkBitsF(pRC, bit1);
		jitcStep_checkRxx(pRC, r);
		jitcStep_checkBits32(pRC, bit0);
		goto fin;
	}
	if (0x50 <= opecode && opecode <= 0x53) {
		f1 = ip[1]; f2 = ip[2]; f0 = ip[3]; bit = ip[4];
		jitcStep_checkFxx(pRC, f1);
		jitcStep_checkFxx(pRC, f2);
		jitcStep_checkFxxNotF3F(pRC, f0);
		jitcStep_checkBitsF(pRC, bit);
		goto fin;
	}

	goto fin1;
fin:
	retcode = 0;
fin1:
	return retcode;
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
				int i32; // Ç±Ç±ÇÃintÇÕê≥ämÇ…32bitÇ≈Ç»ÇØÇÍÇŒÇ¢ÇØÇ»Ç¢ÇÃÇ≈íçà”.
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
				int i32[2]; // Ç±Ç±ÇÃintÇÕê≥ämÇ…32bitÇ≈Ç»ÇØÇÍÇŒÇ¢ÇØÇ»Ç¢ÇÃÇ≈íçà”.
			} u64;
			u64.i32[1] = ip[2]; // ÉäÉgÉãÉGÉìÉfÉBÉAÉìÇëzíË.
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
		if (opecode == 0x48)
			i = vm->f[f1] == vm->f[f2];
		if (opecode == 0x49)
			i = vm->f[f1] != vm->f[f2];
		if (opecode == 0x4a)
			i = vm->f[f1] <  vm->f[f2];
		if (opecode == 0x4b)
			i = vm->f[f1] >= vm->f[f2];
		if (opecode == 0x4c)
			i = vm->f[f1] <= vm->f[f2];
		if (opecode == 0x4d)
			i = vm->f[f1] >  vm->f[f2];
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
		if (opecode == 0x10)
			vm->f[f0] = vm->f[f1] + vm->f[f2];
		if (opecode == 0x11)
			vm->f[f0] = vm->f[f1] - vm->f[f2];
		if (opecode == 0x12)
			vm->f[f0] = vm->f[f1] * vm->f[f2];
		if (opecode == 0x13)
			vm->f[f0] = vm->f[f1] / vm->f[f2];
		vm->bitF[f0] = bit;
		ip += 5;
		goto fin;
	}

fin:
	vm->ip = ip;
	return;
}


#include "osecpu-vm.h"

// 整数命令: 02, 08-09, 10-16, 18-1B, 20-27, 2F, FD

int getTypBitInteger(int typ)
{
	int retcode = 0;
	if (2 <= typ && typ <= 21) {
		static unsigned char table[10] = { 8, 16, 32, 4, 2, 1, 12, 20, 24, 28 };
		retcode = table[(typ - 2) / 2];
	}
	return retcode;
}

void getTypInfoInteger(int typ, int *typSize0, int *typSize1, int *typSign)
// typSize0: 公式サイズ(ビット単位).
// typSize1: 内部サイズ(バイト単位).
// typSign:  符号の有無(0は符号なし).
{
	*typSize0 = *typSize1 = -1;
	if (2 <= typ && typ <= 21) {
		if ((typ & 1) == 0)
			*typSign = -1; // typが偶数なら符号あり.
		else
			*typSign = 0; // typが奇数なら符号なし.
		*typSize0 = getTypBitInteger(typ);
		*typSize1 = sizeof (Int32);
	}
	return;
}


void jitcInitInteger(OsecpuJitc *jitc)
{
	jitc->ope04 = NULL;
	return;
}

int jitcStepInteger(OsecpuJitc *jitc)
{
	Int32 *ip = jitc->hh4Buffer;
	Int32 opecode = ip[0], imm;
	int bit, bit0, bit1, r, r0, r1, r2, p, typ;
	int retcode = -1, *pRC = &retcode;
	int i;
	if (opecode == 0x02) { /* LIMM(imm, Rxx, bits); */
		ip[1] = hh4ReaderGetSigned(&jitc->hh4r);
		ip[2] = hh4ReaderGetUnsigned(&jitc->hh4r);
		ip[3] = hh4ReaderGetUnsigned(&jitc->hh4r);
		jitc->instrLength = 4;
		imm = ip[1]; r = ip[2]; bit = ip[3];
		jitcStep_checkBits32(pRC, bit);
		goto fin;
	}
	if (opecode == 0x04) { /* CND(Rxx); */
		jitcSetHh4BufferSimple(jitc, 2);
		ip[2] = 0; // skipする命令長.
		jitc->instrLength = 3;
		r = ip[1];
		jitc->ope04 = jitc->dst;
		goto fin;
	}
	if (opecode == 0x08) {	/* LMEM(p, typ, 0, r, bit); */
		jitcSetHh4BufferSimple(jitc, 6);
		p = ip[1]; typ = ip[2]; i = ip[3]; r = ip[4]; bit = ip[5];
		if (i != 0) {
			jitcSetRetCode(pRC, JITC_UNSUPPORTED);
			goto fin;
		}
		jitcStep_checkBits32(pRC, bit);
		jitc->prefix2f[0] = 0; // 2F-0.
		jitc->prefix2f[1] = 0; // 2F-1.
		goto fin;
	}
	if (opecode == 0x09) {	/* SMEM(r, bit, p, typ, 0); */
		jitcSetHh4BufferSimple(jitc, 6);
		r = ip[1]; bit = ip[2]; p = ip[3]; typ = ip[4]; i = ip[5];
		if (i != 0) {
			jitcSetRetCode(pRC, JITC_UNSUPPORTED);
			goto fin;
		}
		jitc->prefix2f[0] = 0; // 2F-0.
		jitc->prefix2f[1] = 0; // 2F-1.
		jitc->prefix2f[2] = 0; // 2F-2.
		goto fin;
	}
	if (0x10 <= opecode && opecode <= 0x1b && opecode != 0x13 && opecode != 0x17) {
		jitcSetHh4BufferSimple(jitc, 5);
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		jitcStep_checkBits32(pRC, bit);
		jitc->prefix2f[0] = 0; // 2F-0.
		if (opecode <= 0x19)
			jitc->prefix2f[1] = 0; // 2F-1.
		goto fin;
	}
	if (opecode == 0x13) {	// SBX.
		jitcSetHh4BufferSimple(jitc, 5);
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		jitcStep_checkBits32(pRC, bit);
		jitc->prefix2f[0] = 0; // 2F-0.
		goto fin;
	}
	if (0x20 <= opecode && opecode <= 0x27) {
		jitcSetHh4BufferSimple(jitc, 6);
		r1 = ip[1]; r2 = ip[2]; bit1 = ip[3]; r0 = ip[4]; bit0 = ip[5];
		jitcStep_checkBits32(pRC, bit0);
		jitcStep_checkBits32(pRC, bit1);
		goto fin;
	}
	goto fin1;
fin:
	if (retcode == -1)
		retcode = 0;
fin1:
	return retcode;
}

int jitcAfterStepInteger(OsecpuJitc *jitc)
{
	int opecode = jitc->hh4Buffer[0], retcode = 0;
	Int32 *dst04 = jitc->ope04;
	if (jitc->ope04 != NULL && opecode != 0x04 && opecode != 0x2f) {
		// CND命令の直後の命令を検出.
		dst04[2] = (jitc->dst + jitc->instrLength) - (dst04 + 3); // 直後の命令の命令長.
		jitc->ope04 = NULL;
	}
	return retcode;
}

void execStepInteger(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0], imm;
	int bit, bit0, bit1, r, r0, r1, r2, p, typ, typSign, typSize0, typSize1, i, tmax;
	if (opecode == 0x02) { // LIMM(imm, Rxx, bits);
		imm = ip[1]; r = ip[2]; bit = ip[3]; 
		vm->r[r] = imm;
		ip += 4;
		goto fin;
	}
	if (opecode == 0x04) { /* CND(Rxx); */
		r = ip[1]; i = ip[2];
		ip += 3;
		if ((vm->r[r] & 1) == 0)
			ip += i;
		goto fin;
	}
	if (opecode == 0x08) {	/* LMEM(p, typ, 0, r, bit); */
		Int32 *pi;
		p = ip[1]; typ = ip[2]; r = ip[4]; bit = ip[5];
		pi = (Int32 *) vm->p[p].p;
		vm->r[r] = *pi;
		ip += 6;
		goto fin;
	}
	if (opecode == 0x09) {	/* SMEM(r, bit, p, typ, 0); */
		Int32 *pi;
		r = ip[1]; bit = ip[2]; p = ip[3]; typ = ip[4]; i = ip[5];
		i = vm->r[r];
		if (vm->prefix2f[0] != 0) {
			// 2F-0: マスクライト.
			getTypInfoInteger(typ, &typSize0, &typSize1, &typSign);
			if (typSize0 < 32) {
				if (typSign == 0) {
					tmax = (1 << typSize0) - 1;
					i &= tmax;
				} else
					i = execStep_signBitExtend(i, typSize0 - 1);
			}
		}
		pi = (Int32 *) vm->p[p].p;
		*pi = i;
		vm->prefix2f[0] = 0;
		vm->prefix2f[1] = 0;
		ip += 6;
		goto fin;
	}
	if (0x10 <= opecode && opecode <= 0x16 && opecode != 0x13) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		if (opecode == 0x10) vm->r[r0] = vm->r[r1] | vm->r[r2]; // OR(r0, r1, r2, bits);
		if (opecode == 0x11) vm->r[r0] = vm->r[r1] ^ vm->r[r2]; // XOR(r0, r1, r2, bits);
		if (opecode == 0x12) vm->r[r0] = vm->r[r1] & vm->r[r2]; // AND(r0, r1, r2, bits);
		if (opecode == 0x14) vm->r[r0] = vm->r[r1] + vm->r[r2]; // ADD(r0, r1, r2, bits);
		if (opecode == 0x15) vm->r[r0] = vm->r[r1] - vm->r[r2]; // SUB(r0, r1, r2, bits);
		if (opecode == 0x16) vm->r[r0] = vm->r[r1] * vm->r[r2]; // MUL(r0, r1, r2, bits);
		vm->prefix2f[1] = 0;
		ip += 5;
		goto fin;
	}
	if (opecode == 0x13) {	// SBX.
		r1 = ip[1]; r0 = ip[3]; bit = ip[4];
		vm->r[r0] = execStep_signBitExtend(vm->r[r1], vm->r[0x3f] - 1);
		ip += 5;
		goto fin;
	}
	if (0x18 <= opecode && opecode <= 0x19) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		if (opecode == 0x18)
			vm->r[r0] = vm->r[r1] << vm->r[r2];	// SHL
		else
			vm->r[r0] = vm->r[r1] >> vm->r[r2];	// SAR
		vm->prefix2f[1] = 0;
		ip += 5;
		goto fin;
	}
	if (0x1a <= opecode && opecode <= 0x1b) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		if (vm->r[r2] == 0) {
			jitcSetRetCode(&vm->errorCode, EXEC_DIVISION_BY_ZERO);
			goto fin;
		}
		if (opecode == 0x1a)
			vm->r[r0] = vm->r[r1] / vm->r[r2];
		else
			vm->r[r0] = vm->r[r1] % vm->r[r2];
		ip += 5;
		goto fin;
	}
	if (0x20 <= opecode && opecode <= 0x27) {
		r1 = ip[1]; r2 = ip[2]; bit1 = ip[3]; r0 = ip[4]; bit0 = ip[5];
		i = 0; // gccの警告を黙らせるため.
		if (opecode == 0x20)
			i = vm->r[r1] == vm->r[r2];
		if (opecode == 0x21)
			i = vm->r[r1] != vm->r[r2];
		if (opecode == 0x22)
			i = vm->r[r1] <  vm->r[r2];
		if (opecode == 0x23)
			i = vm->r[r1] >= vm->r[r2];
		if (opecode == 0x24)
			i = vm->r[r1] <= vm->r[r2];
		if (opecode == 0x25)
			i = vm->r[r1] >  vm->r[r2];
		if (opecode == 0x26)
			i = (vm->r[r1] & vm->r[r2]) == 0;
		if (opecode == 0x27)
			i = (vm->r[r1] & vm->r[r2]) != 0;
		if (i != 0)
			i = -1;
		vm->r[r0] = i;
		ip += 6;
		goto fin;
	}

fin:
	if (vm->errorCode <= 0)
		vm->ip = ip;
	return;
}


void jitcStep_checkBits32(int *pRC, int bits)
{
	if (!(0 <= bits && bits <= 32))
		jitcSetRetCode(pRC, JITC_BAD_BITS);
	return;
}

Int32 execStep_signBitExtend(Int32 value, int bit)
// bitは符号ビットのある位置で、0～31を想定.
{
	if (bit >= 0) {
		Int32 mask = (-1) << bit;
		if ((value & (1 << bit)) == 0) {
			// 符号ビットが0だった.
			value &= ~mask;
		} else {
			// 符号ビットが1だった.
			value |=  mask;
		}
	} else
		value = 0; // bitが負の場合.
	return value;
}

Int32 execStep_getRxx(OsecpuVm *vm, int r, int bit)
{
	return execStep_signBitExtend(vm->r[r], bit - 1);
}


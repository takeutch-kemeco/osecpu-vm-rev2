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
		int bytes;
		if ((typ & 1) == 0)
			*typSign = -1; // typが偶数なら符号あり.
		else
			*typSign = 0; // typが奇数なら符号なし.
		*typSize0 = getTypBitInteger(typ);
		bytes = (*typSize0 + 7) / 8;
		if (bytes == 3) bytes = 4;
		*typSize1 = bytes;
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
	int i, j;
	if (opecode == 0x02) { /* LIMM(imm, Rxx, bits); */
		ip[1] = hh4ReaderGetSigned(&jitc->hh4r);
		ip[2] = hh4ReaderGetUnsigned(&jitc->hh4r);
		ip[3] = hh4ReaderGetUnsigned(&jitc->hh4r);
		jitc->instrLength = 4;
		imm = ip[1]; r = ip[2]; bit = ip[3];
		jitcStep_checkBits32(pRC, bit);
		jitcStep_checkRxx(pRC, r);
		goto fin;
	}
	if (opecode == 0x04) { /* CND(Rxx); */
		jitcSetHh4BufferSimple(jitc, 2);
		ip[2] = 0; // skipする命令長.
		jitc->instrLength = 3;
		r = ip[1];
		jitcStep_checkRxx(pRC, r);
		Hh4Reader hh4r = jitc->hh4r; // これを使ってもjitcのhh4rは進まない.
		i = hh4ReaderGetUnsigned(&hh4r); // 次のopecode.
		if (i == 0x00 || i == 0x01 || i == 0x04 || i == 0x2e)
			jitcSetRetCode(pRC, JITC_BAD_CND);
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
		jitcStep_checkPxx(pRC, p);
		jitcStep_checkRxxNotR3F(pRC, r);
		jitcStep_checkBits32(pRC, bit);
		if (jitc->prefix2f[0] != 0 && jitc->prefix2f[1] != 0)
			jitcSetRetCode(pRC, JITC_BAD_PREFIX);	// 2F-n系プリフィクスは1個まで.
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
		jitcStep_checkPxx(pRC, p);
		jitcStep_checkRxxNotR3F(pRC, r);
		j = 0;
		for (i = 0; i < 3; i++) {
			if (jitc->prefix2f[i] != 0)
				j++;
		}
		if (j > 1)
			jitcSetRetCode(pRC, JITC_BAD_PREFIX);	// 2F-n系プリフィクスは1個まで.
		jitc->prefix2f[0] = 0; // 2F-0.
		jitc->prefix2f[1] = 0; // 2F-1.
		jitc->prefix2f[2] = 0; // 2F-2.
		goto fin;
	}
	if (0x10 <= opecode && opecode <= 0x1b && opecode != 0x13 && opecode != 0x17) {
		jitcSetHh4BufferSimple(jitc, 5);
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		jitcStep_checkRxx(pRC, r1);
		jitcStep_checkRxx(pRC, r2);
		jitcStep_checkRxxNotR3F(pRC, r0);
		jitcStep_checkBits32(pRC, bit);
		jitc->prefix2f[0] = 0; // 2F-0.
		goto fin;
	}
	if (opecode == 0x13) {	// SBX.
		jitcSetHh4BufferSimple(jitc, 5);
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		jitcStep_checkRxxNotR3F(pRC, r1);
		jitcStep_checkRxxNotR3F(pRC, r0);
		if (r2 != 0x3f)
			jitcSetRetCode(pRC, JITC_BAD_RXX);
		jitcStep_checkBits32(pRC, bit);
		jitc->prefix2f[0] = 0; // 2F-0.
		goto fin;
	}
	if (0x20 <= opecode && opecode <= 0x27) {
		jitcSetHh4BufferSimple(jitc, 6);
		r1 = ip[1]; r2 = ip[2]; bit1 = ip[3]; r0 = ip[4]; bit0 = ip[5];
		jitcStep_checkRxx(pRC, r1);
		jitcStep_checkRxx(pRC, r2);
		jitcStep_checkRxx(pRC, r0);
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
	int i, retcode = 0;
	if (jitc->ope04 != NULL && jitc->dst != jitc->ope04) {
		// CND命令の直後の命令を検出.
		Int32 *dst04 = jitc->ope04;
		dst04[2] = jitc->instrLength; // 直後の命令の命令長.
		jitc->ope04 = NULL;
	}
	return 0;
}

Int32 execStep_checkBitsRange(Int32 value, int bit, OsecpuVm *vm, int bit1, int bit2)
{
	int max, min, i;
	if (bit1 != BIT_DISABLE_REG && bit2 != BIT_DISABLE_REG && vm->prefix2f[0] == 0) {
		max = 1 << (bit - 1); // 例: bits=8だとmax=128になる.
		max--; // 例: bits=8だとmax=127になる.
		min = - max - 1; // 例: bits=8だとmin=-128になる。
		if (!(min <= value && value <= max))
			jitcSetRetCode(&vm->errorCode, EXEC_BITS_RANGE_OVER);
	} else {
		vm->prefix2f[0] = 0;
		value = execStep_SignBitExtend(value, bit - 1);
	}
	return value;
}

void execStepInteger(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0], imm;
	int bit, bit0, bit1, r, r0, r1, r2, p, typ, typSign, typSize0, typSize1;
	int i, mbit, tbit;
	if (opecode == 0x02) { // LIMM(imm, Rxx, bits);
		imm = ip[1]; r = ip[2]; bit = ip[3]; 
		vm->r[r] = imm;
		vm->bit[r] = bit;
		execStep_checkBitsRange(vm->r[r], bit, vm, 0, 0);
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
		p = ip[1]; typ = ip[2]; r = ip[4]; bit = ip[5];
		execStep_checkMemAccess(vm, p, typ, EXEC_CMA_FLAG_READ);
		if (vm->errorCode != 0) goto fin;
		mbit = *(vm->p[p].bit);
		tbit = getTypBitInteger(typ);
		if (mbit == BIT_DISABLE_MEM)
			tbit = mbit;
		getTypInfoInteger(typ, &typSize0, &typSize1, &typSign);
		if (typSize1 == 1 && typSign == 0) {
			unsigned char *puc = (unsigned char *) vm->p[p].p;
			vm->r[r] = *puc;
		}
		if (typSize1 == 1 && typSign != 0) {
			signed char *psc = (signed char *) vm->p[p].p;
			vm->r[r] = *psc;
		}
		if (typSize1 == 2 && typSign == 0) {
			unsigned short *pus = (unsigned short *) vm->p[p].p;
			vm->r[r] = *pus;
		}
		if (typSize1 == 2 && typSign != 0) {
			signed short *pss = (signed short *) vm->p[p].p;
			vm->r[r] = *pss;
		}
		if (typSize1 == 4 && typSign == 0) {
			unsigned int *pui = (unsigned int *) vm->p[p].p;
			vm->r[r] = *pui;
		}
		if (typSize1 == 4 && typSign != 0) {
			signed int *psi = (signed int *) vm->p[p].p;
			vm->r[r] = *psi;
		}
		if (vm->prefix2f[1] == 0) {
			if (mbit < tbit && mbit < bit)
				jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);	// 不確定ビットの参照がある場合.
		} else {
			if (bit > mbit)
				bit = mbit;
			vm->prefix2f[1] = 0;
		}
		vm->bit[r] = bit;
		if (tbit > bit)
			vm->r[r] = execStep_checkBitsRange(vm->r[r], bit, vm, 0, 0); // 部分リードの場合は、ゴミビットを消す.
		ip += 6;
		goto fin;
	}

	if (0x10 <= opecode && opecode <= 0x16 && opecode != 0x13) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		if (vm->bit[r1] != BIT_DISABLE_REG && bit > vm->bit[r1]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (vm->bit[r2] != BIT_DISABLE_REG && bit > vm->bit[r2]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (opecode == 0x10) vm->r[r0] = vm->r[r1] | vm->r[r2]; // OR(r0, r1, r2, bits);
		if (opecode == 0x11) vm->r[r0] = vm->r[r1] ^ vm->r[r2]; // XOR(r0, r1, r2, bits);
		if (opecode == 0x12) vm->r[r0] = vm->r[r1] & vm->r[r2]; // AND(r0, r1, r2, bits);
		if (opecode == 0x14) vm->r[r0] = vm->r[r1] + vm->r[r2]; // ADD(r0, r1, r2, bits);
		if (opecode == 0x15) vm->r[r0] = vm->r[r1] - vm->r[r2]; // SUB(r0, r1, r2, bits);
		if (opecode == 0x16) vm->r[r0] = vm->r[r1] * vm->r[r2]; // MUL(r0, r1, r2, bits);
		vm->bit[r0] = bit;
		vm->r[r0] = execStep_checkBitsRange(vm->r[r0], bit, vm, vm->bit[r1], vm->bit[r2]);
		ip += 5;
		goto fin;
	}
	if (opecode == 0x13) {	// SBX.
		r1 = ip[1]; r0 = ip[3]; bit = ip[4];
		if (vm->bit[r1] != BIT_DISABLE_REG && bit > vm->bit[r1]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (bit <= vm->r[0x3f] || vm->r[0x3f] < 0) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_R2);
			goto fin;
		}
		vm->r[r0] = execStep_SignBitExtend(vm->r[r1], vm->r[0x3f]);
		vm->bit[r0] = bit;
		vm->r[r0] = execStep_checkBitsRange(vm->r[r0], bit, vm, vm->bit[r1], 0);
		ip += 5;
		goto fin;
	}
	if (0x18 <= opecode && opecode <= 0x19) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		if (vm->bit[r1] != BIT_DISABLE_REG && bit > vm->bit[r1]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (vm->r[r2] < 0) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_R2);
			goto fin;
		}
		if (opecode == 0x18)
			vm->r[r0] = vm->r[r1] << vm->r[r2];	// SHL
		else
			vm->r[r0] = vm->r[r1] >> vm->r[r2];	// SAR
		vm->bit[r0] = bit;
		vm->r[r0] = execStep_checkBitsRange(vm->r[r0], bit, vm, vm->bit[r1], 0);
		ip += 5;
		goto fin;
	}
	if (0x1a <= opecode && opecode <= 0x1b) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		if (vm->bit[r1] != BIT_DISABLE_REG && bit > vm->bit[r1]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (vm->bit[r2] != BIT_DISABLE_REG && bit > vm->bit[r2]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (vm->r[r2] == 0) {
			jitcSetRetCode(&vm->errorCode, EXEC_DIVISION_BY_ZERO);
			goto fin;
		}
		if (opecode == 0x1a)
			vm->r[r0] = vm->r[r1] / vm->r[r2];
		else
			vm->r[r0] = vm->r[r1] % vm->r[r2];
		vm->bit[r0] = bit;
		vm->r[r0] = execStep_checkBitsRange(vm->r[r0], bit, vm, vm->bit[r1], vm->bit[r2]);
		ip += 5;
		goto fin;
	}
	if (0x20 <= opecode && opecode <= 0x27) {
		r1 = ip[1]; r2 = ip[2]; bit1 = ip[3]; r0 = ip[4]; bit0 = ip[5];
		if (vm->bit[r1] != BIT_DISABLE_REG && bit > vm->bit[r1]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (vm->bit[r2] != BIT_DISABLE_REG && bit > vm->bit[r2]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
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
		vm->bit[r0] = bit0;
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

Int32 execStep_SignBitExtend(Int32 value, int bit)
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


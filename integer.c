#include "osecpu-vm.h"

// 整数命令: 02, 10-16, 18-1B, 20-27, FD

void OsecpuInitInteger()
{
	static int table[] = {
		1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x
		5, 5, 5, 5, 5, 5, 5, 0, 5, 5, 5, 5, 0, 0, 0, 0, // 1x
		6, 6, 6, 6, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0  // 2x
	};
	instrLengthSimpleInitTool(table, 0x00, 0x2f);
	return;
}

int instrLengthInteger(const Int32 *src, const Int32 *src1)
// instrLengthSimpleInitTool()で登録していないものだけに反応すればよい.
{
	Int32 opecode = src[0];
	int retcode = 0;
	if (opecode == 0x02)
		retcode = 4;
	if (opecode == 0xfd)
		retcode = 3;
	return retcode;
}

Int32 *hh4DecodeInteger(OsecpuJitc *jitc, Int32 opecode)
// instrLengthSimpleInitTool()で登録していないものだけに反応すればよい.
{
	HH4Reader *hh4r = jitc->hh4r;
	Int32 *dst = jitc->hh4dst, *dst1 = jitc->hh4dst1;
	int i, v;
	if (opecode == 0x02) {
		if (dst + 4 > dst1)
			goto err;
		*dst = opecode;
		dst[1] = hh4GetSigned(hh4r);
		dst[2] = hh4GetUnsigned(hh4r);
		dst[3] = hh4GetUnsigned(hh4r);
		dst += 4;
		goto fin;
	}
	if (opecode == 0xfd) {
		if (dst + 3 > dst1)
			goto err;
		*dst = opecode;
		dst[1] = v = hh4GetUnsigned(hh4r);
		dst[2] = i = hh4GetUnsigned(hh4r);
		if (0 <= i && i <= 3)
			jitc->dr[i] = v;
		dst += 3;
		goto fin;
	}
fin:
	jitc->dst = dst;
	return dst;
err:
	jitc->errorCode = JITC_HH4_DST_OVERRUN;
	return dst;
}

int jitcStepInteger(OsecpuJitc *jitc)
{
	const Int32 *ip = jitc->src;
	Int32 opecode = ip[0], imm;
	int bit, bit0, bit1, r, r0, r1, r2;
	int retcode = -1, *pRC = &retcode;
	int i;
	if (opecode == 0x00) { /* NOP */
		goto fin;
	}
	if (opecode == 0x02) { /* LIMM(imm, Rxx, bits); */
		imm = ip[1]; r = ip[2]; bit = ip[3]; 
		jitcStep_checkBits32(pRC, bit);
		jitcStep_checkRxx(pRC, r);
		goto fin;
	}
	if (opecode == 0x04) { /* CND(Rxx); */
		r = ip[1];
		jitcStep_checkRxx(pRC, r);
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
		jitcStep_checkRxx(pRC, r0);
		jitcStep_checkBits32(pRC, bit0);
		jitcStep_checkBits32(pRC, bit1);
		goto fin;
	}
	if (opecode == 0xfd) {
		imm = ip[1]; r = ip[2];
 		if (0 <= r && r <= 3)
			jitc->dr[r] = imm;
		goto fin;
	}
	goto fin1;
fin:
	retcode = 0;
fin1:
	return retcode;
}

void execStepInteger(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0], imm;
	int bit, bit0, bit1, r, r0, r1, r2;
	int i;
	if (opecode == 0x00) { // NOP();
		ip++;
		goto fin;
	}
	if (opecode == 0x02) { // LIMM(imm, Rxx, bits);
		imm = ip[1]; r = ip[2]; bit = ip[3]; 
		vm->r[r] = imm;
		vm->bit[r] = bit;
		execStep_checkBitsRange(vm->r[r], bit, vm);
		ip += 4;
		goto fin;
	}
	if (opecode == 0x04) { /* CND(Rxx); */
		r = ip[1];
		ip += 2;
		if ((vm->r[r] & 1) == 0) {
			i = instrLength(ip, vm->ip1);
			if (i < 0) {	// これはオーバーランしかありえない.
				jitcSetRetCode(&vm->errorCode, EXEC_SRC_OVERRUN);
				goto fin;
			}
			ip += i;
		}
		goto fin;
	}
	if (0x10 <= opecode && opecode <= 0x16 && opecode != 0x13) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		if (bit > vm->bit[r1] || bit > vm->bit[r2]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (opecode == 0x10) // OR(r0, r1, r2, bits);
			vm->r[r0] = vm->r[r1] | vm->r[r2];
		if (opecode == 0x11) // XOR(r0, r1, r2, bits);
			vm->r[r0] = vm->r[r1] ^ vm->r[r2];
		if (opecode == 0x12) // AND(r0, r1, r2, bits);
			vm->r[r0] = vm->r[r1] & vm->r[r2];
		if (opecode == 0x14) // ADD(r0, r1, r2, bits);
			vm->r[r0] = vm->r[r1] + vm->r[r2];
		if (opecode == 0x15) // SUB(r0, r1, r2, bits);
			vm->r[r0] = vm->r[r1] - vm->r[r2];
		if (opecode == 0x16) // MUL(r0, r1, r2, bits);
			vm->r[r0] = vm->r[r1] * vm->r[r2];
		vm->bit[r0] = bit;
		execStep_checkBitsRange(vm->r[r0], bit, vm);
		ip += 5;
		goto fin;
	}
	if (opecode == 0x13) {	// SBX.
		r1 = ip[1]; r0 = ip[3]; bit = ip[4];
		if (bit > vm->bit[r1]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (bit <= vm->r[0x3f] || vm->r[0x3f] < 0) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_R2);
			goto fin;
		}
		i = (-1) << vm->r[0x3f];
		if ((vm->r[r1] & (1 << vm->r[0x3f])) == 0) {
			// 符号ビットが0だった.
			vm->r[r0] = vm->r[r1] & ~i;
		} else {
			// 符号ビットが1だった.
			vm->r[r0] = vm->r[r1] |  i;
		}
		vm->bit[r0] = bit;
		execStep_checkBitsRange(vm->r[r0], bit, vm);
		ip += 5;
		goto fin;
	}
	if (0x18 <= opecode && opecode <= 0x19) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		if (bit > vm->bit[r1]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (bit <= vm->r[r2] || vm->r[r2] < 0) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_R2);
			goto fin;
		}
		if (opecode == 0x18)
			vm->r[r0] = vm->r[r1] << vm->r[r2];	// SHL
		else
			vm->r[r0] = vm->r[r1] >> vm->r[r2];	// SAR
		vm->bit[r0] = bit;
		execStep_checkBitsRange(vm->r[r0], bit, vm);
		ip += 5;
		goto fin;
	}
	if (0x1a <= opecode && opecode <= 0x1b) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		if (bit > vm->bit[r1] || bit > vm->bit[r2]) {
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
		execStep_checkBitsRange(vm->r[r0], bit, vm);
		ip += 5;
		goto fin;
	}
	if (0x20 <= opecode && opecode <= 0x27) {
		r1 = ip[1]; r2 = ip[2]; bit1 = ip[3]; r0 = ip[4]; bit0 = ip[5];
		if (bit1 > vm->bit[r1] || bit1 > vm->bit[r2]) {
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
	vm->ip = ip;
	return;
}


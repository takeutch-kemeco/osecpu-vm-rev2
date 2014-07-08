#include "osecpu-vm.h"

void execStep_checkBitsRange(Int32 value, int bits, VM *vm)
{
	int max, min;
	max = 1 << (bits - 1); // 例: bits=8だとmax=128になる.
	max--; // 例: bits=8だとmax=127になる.
	min = - max - 1; // 例: bits=8だとmin=-128になる。
	if (!(min <= value && value <= max))
		jitcSetRetCode(&vm->errorCode, EXEC_BITS_RANGE_OVER);
	return;
}

int execStep(VM *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0], imm, i;
	int bit, bit0, bit1, r, r0, r1, r2;
	vm->errorCode = 0;
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
			vm->r[r0] = vm->bit[r1] | vm->bit[r2];
		if (opecode == 0x11) // XOR(r0, r1, r2, bits);
			vm->r[r0] = vm->bit[r1] ^ vm->bit[r2];
		if (opecode == 0x12) // AND(r0, r1, r2, bits);
			vm->r[r0] = vm->bit[r1] & vm->bit[r2];
		if (opecode == 0x14) // ADD(r0, r1, r2, bits);
			vm->r[r0] = vm->bit[r1] + vm->bit[r2];
		if (opecode == 0x15) // SUB(r0, r1, r2, bits);
			vm->r[r0] = vm->bit[r1] - vm->bit[r2];
		if (opecode == 0x16) // MUL(r0, r1, r2, bits);
			vm->r[r0] = vm->bit[r1] * vm->bit[r2];
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
			vm->r[r0] = vm->bit[r1] & ~i;
		} else {
			// 符号ビットが1だった.
			vm->r[r0] = vm->bit[r1] |  i;
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
			vm->r[r0] = vm->bit[r1] << vm->bit[r2];	// SHL
		else
			vm->r[r0] = vm->bit[r1] >> vm->bit[r2];	// SAR
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
			vm->r[r0] = vm->bit[r1] / vm->bit[r2];
		else
			vm->r[r0] = vm->bit[r1] % vm->bit[r2];
		vm->bit[r0] = bit;
		execStep_checkBitsRange(vm->r[r0], bit, vm);
		ip += 5;
		goto fin;
	}
	if (0x20 <= opecode && opecode <= 0x27) {
		r1 = ip[1]; r2 = ip[2]; bit1 = ip[3]; r0 = ip[4]; bit0 = ip[5];
		if (bit > vm->bit[r1] || bit > vm->bit[r2]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		if (opecode == 0x20)
			i = vm->bit[r1] == vm->bit[r2];
		if (opecode == 0x21)
			i = vm->bit[r1] != vm->bit[r2];
		if (opecode == 0x22)
			i = vm->bit[r1] <  vm->bit[r2];
		if (opecode == 0x23)
			i = vm->bit[r1] >= vm->bit[r2];
		if (opecode == 0x24)
			i = vm->bit[r1] <= vm->bit[r2];
		if (opecode == 0x25)
			i = vm->bit[r1] >  vm->bit[r2];
		if (opecode == 0x26)
			i = (vm->bit[r1] & vm->bit[r2]) == 0;
		if (opecode == 0x27)
			i = (vm->bit[r1] & vm->bit[r2]) != 0;
		if (i != 0)
			i = -1;
		vm->r[r0] = i;
		vm->bit[r0] = bit;
		ip += 6;
		goto fin;
	}
	if (opecode == -1) {
		vm->errorCode = EXEC_ABORT_OPECODE_M1; // デバッグ用.
		goto fin;
	}
	ext_execStep(vm);
	if (ip != vm->ip)
		goto fin1;

	fprintf(stderr, "Error: execStep: opecode=0x%02X\n", opecode); // 内部エラー.
	exit(1);
fin:
	vm->ip = ip;
fin1:
	return vm->errorCode;
}

int execAll(VM *vm)
{
	for (;;) {
		// 理想は適当な上限を決めて、休み休みでやるべきかもしれない.
		execStep(vm);
		if (vm->errorCode != 0) break;
	}
	return vm->errorCode;
}


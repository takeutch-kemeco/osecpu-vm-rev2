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
	Int32 opecode = ip[0];
	int bit, r, r0, r1, r2;
	vm->errorCode = 0;
	if (opecode == 0x00) { // NOP();
		ip++;
		goto fin;
	}
	if (opecode == 0x02) { // LIMM(imm, Rxx, bits);
		bit = ip[3]; r = ip[2];
		vm->r[r] = ip[1];
		vm->bit[r] = bit;
		execStep_checkBitsRange(vm->r[r], bit, vm);
		ip += 4;
		goto fin;
	}
	if (0x10 <= opecode && opecode <= 0x16 && opecode != 0x13) {
		r1 = ip[1]; r2 = ip[2]; r0 = ip[3]; bit = ip[4];
		if (bit > vm->bit[r1] || bit > vm->bit[r2]) {
			jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
			goto fin;
		}
		vm->bit[r0] = bit;
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
		ip += 5;
		goto fin;
	}
	if (opecode == -1) {
		vm->errorCode = EXEC_ABORT_OPECODE_M1; // デバッグ用.
		goto fin;
	}

	fprintf(stderr, "Error: execStep: opecode=0x%02X\n", opecode); // 内部エラー.
	exit(1);
fin:
	vm->ip = ip;
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


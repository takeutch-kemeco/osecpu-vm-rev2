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
	if (opecode == 0x30) {
		jitcSetHh4BufferSimple(jitc, 6);
		typ = ip[1]; bit0 = ip[2]; r = ip[3]; bit1 = ip[4]; p = ip[5];
		jitcStep_checkRxx(pRC, typ);
		jitcStep_checkRxx(pRC, r);
		jitcStep_checkBits32(pRC, bit0);
		jitcStep_checkBits32(pRC, bit1);
		jitcStep_checkPxx(pRC, p);
		goto fin;
	}
	if (opecode == 0x31) {
		jitcSetHh4BufferSimple(jitc, 6);
		p = ip[1]; typ = ip[2]; bit0 = ip[3]; r = ip[4]; bit1 = ip[5]; 
		jitcStep_checkRxx(pRC, typ);
		jitcStep_checkRxx(pRC, r);
		jitcStep_checkBits32(pRC, bit0);
		jitcStep_checkBits32(pRC, bit1);
		jitcStep_checkPxx(pRC, p);
		goto fin;
	}
	if (opecode == 0x32) {
		jitcSetHh4BufferSimple(jitc, 6);
		typ = ip[1]; bit0 = ip[2]; r = ip[3]; bit1 = ip[4]; p = ip[5];
		jitcStep_checkRxx(pRC, typ);
		jitcStep_checkRxx(pRC, r);
		jitcStep_checkBits32(pRC, bit0);
		jitcStep_checkBits32(pRC, bit1);
		jitcStep_checkPxx(pRC, p);
		goto fin;
	}
	if (opecode == 0x33) {
		jitcSetHh4BufferSimple(jitc, 6);
		p = ip[1]; typ = ip[2]; bit0 = ip[3]; r = ip[4]; bit1 = ip[5]; 
		jitcStep_checkRxx(pRC, typ);
		jitcStep_checkRxx(pRC, r);
		jitcStep_checkBits32(pRC, bit0);
		jitcStep_checkBits32(pRC, bit1);
		jitcStep_checkPxx(pRC, p);
		goto fin;
	}
	if (opecode == 0x3c) {
		jitcSetHh4BufferSimple(jitc, 7);
		r = ip[1]; bit0 = ip[2]; p = ip[3]; f = ip[4]; bit1 = ip[5]; typ = ip[6];
		if (typ != PTR_TYP_NULL)
			jitcSetRetCode(pRC, JITC_UNSUPPORTED);
		if (r < 0 || r > 0x30 || p < 0 || p > 0x30 || f < 0 || f > 0x30)
			jitcSetRetCode(pRC, JITC_BAD_ENTER);
		if (bit0 < 0 || bit0 > 32)
			jitcSetRetCode(pRC, JITC_BAD_BITS);
		if (bit1 != 0 && bit1 != 32 && bit1 != 64)
			jitcSetRetCode(pRC, JITC_BAD_BITS);
		goto fin;
	}
	if (opecode == 0x3d) {
		jitcSetHh4BufferSimple(jitc, 7);
		r = ip[1]; bit0 = ip[2]; p = ip[3]; f = ip[4]; bit1 = ip[5]; typ = ip[6];
		if (typ != PTR_TYP_NULL)
			jitcSetRetCode(pRC, JITC_UNSUPPORTED);
		if (r < 0 || r > 0x30 || p < 0 || p > 0x30 || f < 0 || f > 0x30)
			jitcSetRetCode(pRC, JITC_BAD_ENTER);
		if (bit0 < 0 || bit0 > 32)
			jitcSetRetCode(pRC, JITC_BAD_BITS);
		if (bit1 != 0 && bit1 != 32 && bit1 != 64)
			jitcSetRetCode(pRC, JITC_BAD_BITS);
		goto fin;
	}
	if (opecode == 0xfd) {
		ip[1] = hh4ReaderGetSigned(&jitc->hh4r);
		ip[2] = hh4ReaderGetUnsigned(&jitc->hh4r);
		jitc->instrLength = 3;
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

int osecpuVmStackPush(OsecpuVm *vm, int r1, int bitR, int p1, int f1, int bitF);
int osecpuVmStackPull(OsecpuVm *vm, int r1, int bitR, int p1, int f1, int bitF);
char *osecpuVmStackAlloc(OsecpuVm *vm, int totalSize, int typ, int len);
int osecpuVmStackFree(OsecpuVm *vm, int totalSize, char *p, int typ, int len);

void execStepOther(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0], imm;
	int bit, bit0, bit1, r, r0, r1, r2, p, f, typ, typSign, typSize0, typSize1;
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
	if (opecode == 0x30) {
		typ = ip[1]; bit0 = ip[2]; r = ip[3]; bit1 = ip[4]; p = ip[5];
		typ = apiGetRxx(vm, typ, bit0);
		i = apiGetRxx(vm, r, bit1);
		getTypSize(typ, &typSize0, &typSize1, &typSign);
		if (i < 0 || typSize0 < 0 || i > 256 * 1024 * 1024)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_ALLOC_ERROR);
		vm->p[p].p = (unsigned char *) osecpuVmStackAlloc(vm, typSize1 * i, typ, i);
		if (vm->p[p].p == NULL)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_ALLOC_ERROR);
		vm->p[p].p0 = vm->p[p].p;
		vm->p[p].p1 = vm->p[p].p + typSize1 * i;
		vm->p[p].typ = typ;
		vm->p[p].flags = 6; // over-seek:ok, read:ok, write:ok
		vm->p[p].bit = (unsigned char *) osecpuVmStackAlloc(vm, i, 0, i);
		if (vm->p[p].bit == NULL)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_ALLOC_ERROR);
		for (r = 0; r < i; i++)
			vm->p[p].bit[r] = 0;
		ip += 6;
		goto fin;
	}
	if (opecode == 0x31) {
		p = ip[1]; typ = ip[2]; bit0 = ip[3]; r = ip[4]; bit1 = ip[5]; 
		typ = apiGetRxx(vm, typ, bit0);
		i = apiGetRxx(vm, r, bit1);
		getTypSize(typ, &typSize0, &typSize1, &typSign);
		if (i < 0 || typSize0 < 0 || i > 256 * 1024 * 1024)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_FREE_ERROR);
		r = (vm->p[p].p - vm->p[p].p0) / typSize1;
		r = osecpuVmStackFree(vm, i, vm->p[p].bit - r, 0, i);
		if (r != 0)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_FREE_ERROR);
		r = osecpuVmStackFree(vm, typSize1 * i, vm->p[p].p0, typ, i);
		if (r != 0)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_FREE_ERROR);
		ip += 6;
		goto fin;
	}
	if (opecode == 0x32) {
		typ = ip[1]; bit0 = ip[2]; r = ip[3]; bit1 = ip[4]; p = ip[5];
		typ = apiGetRxx(vm, typ, bit0);
		i = apiGetRxx(vm, r, bit1);
		getTypSize(typ, &typSize0, &typSize1, &typSign);
		if (i < 0 || typSize0 < 0 || i > 256 * 1024 * 1024)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_ALLOC_ERROR);
		vm->p[p].p = (unsigned char *) malloc(typSize1 * i);
		if (vm->p[p].p == NULL)
			jitcSetRetCode(&vm->errorCode, EXEC_MALLOC_ERROR);
		vm->p[p].p0 = vm->p[p].p;
		vm->p[p].p1 = vm->p[p].p + typSize1 * i;
		vm->p[p].typ = typ;
		vm->p[p].flags = 6; // over-seek:ok, read:ok, write:ok
		vm->p[p].bit = (unsigned char *) malloc(i);
		if (vm->p[p].bit == NULL)
			jitcSetRetCode(&vm->errorCode, EXEC_MALLOC_ERROR);
		for (r = 0; r < i; i++)
			vm->p[p].bit[r] = 0;
		ip += 6;
		goto fin;
	}
	if (opecode == 0x33) {
		p = ip[1]; typ = ip[2]; bit0 = ip[3]; r = ip[4]; bit1 = ip[5];
		// 手抜きにより、なんと何もしない！.
		// 後日、ちゃんとfreeさせます.
		goto fin;
	}
	if (opecode == 0x3c) {
		r = ip[1]; bit0 = ip[2]; p = ip[3]; f = ip[4]; bit1 = ip[5]; typ = ip[6];
		i = osecpuVmStackPush(vm, r, bit0, p, f, bit1);
		if (i != 0)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_ALLOC_ERROR);
		ip += 7;
		goto fin;
	}
	if (opecode == 0x3d) {
		r = ip[1]; bit0 = ip[2]; p = ip[3]; f = ip[4]; bit1 = ip[5]; typ = ip[6];
		i = osecpuVmStackPull(vm, r, bit0, p, f, bit1);
		if (i != 0)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_FREE_ERROR);
		ip += 7;
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

typedef struct _StackHeader {
	int totalSize;
	char *prevStackTop;
	int r1, bitR, p1, f1, bitF;
	PReg p30;
	Int32 *r0p;
	unsigned char *bitR0p;
	PReg *p0p;
	double *f0p;
	unsigned char *bitF0p;
} StackHeader;

#define STACKHEADERSIZE16	((sizeof (StackHeader) + 15) & -16)	// サイズを16バイト単位に切り上げたもの.

int osecpuVmStackInit(OsecpuVm *vm, int stackSize)
{
	int retcode = 1;
	StackHeader *psh;
	if (stackSize < STACKHEADERSIZE16) goto fin;
	vm->stack0 = malloc(stackSize);
	if (vm->stack0 == NULL) goto fin;
	vm->stack1 = vm->stack0 + stackSize;
	psh = (StackHeader *) vm->stack0;
	vm->stack0 += STACKHEADERSIZE16;
	vm->stackTop = (char *) psh;
	psh->totalSize = 0;
	retcode = 0;
fin:
	return retcode;
}

int osecpuVmStackPush(OsecpuVm *vm, int r1, int bitR, int p1, int f1, int bitF)
// r1, p1, f1 が不正な値ではないことは、呼び出し元が保証する.
// bitRが32以下であることとbitFが64以下であることも呼び出し元が保証する.
{
	int retcode = 1;
	int r1Size = r1 * sizeof (Int32), p1Size = p1 * sizeof (PReg);
	int f1Size = f1 * sizeof (double), bitSize = (r1 + f1) * sizeof (unsigned char);
	int totalSize, i, bit;
	StackHeader *psh;
	char *p, *prevStackTop;
	r1Size  = (r1Size  + 15) & -16;
	p1Size  = (p1Size  + 15) & -16;
	f1Size  = (f1Size  + 15) & -16;
	bitSize = (bitSize + 15) & -16;
	totalSize = STACKHEADERSIZE16 + r1Size + p1Size + f1Size + bitSize;
	if (vm->stack0 + totalSize > vm->stack1) goto fin;
	p = vm->stack0;
	prevStackTop = vm->stackTop;
	vm->stack0 += totalSize;
	vm->stackTop = p;
	psh = (StackHeader *) p;
	p += STACKHEADERSIZE16;
	psh->totalSize = totalSize;
	psh->prevStackTop = prevStackTop;
	psh->r1 = r1;
	psh->bitR = bitR;
	psh->p1 = p1;
	psh->f1 = f1;
	psh->bitF = bitF;
	psh->p30 = vm->p[0x30];
	psh->r0p = (Int32 *) p;
	p += r1Size;
	psh->p0p = (PReg *) p;
	p += p1Size;
	psh->f0p = (double *) p;
	p += f1Size;
	psh->bitR0p = (unsigned char *) p;
	p += r1 * sizeof (unsigned char);
	psh->bitF0p = (unsigned char *) p;
	for (i = 0; i < r1; i++) {
		psh->r0p[i] = vm->r[i];
		bit = vm->bit[i];
		if (bit == BIT_DISABLE_REG)
			bit = BIT_DISABLE_MEM;
		else {
			if (bit > bitR) {
				bit = 0;	// 値が壊れた.
				psh->r0p[i] = 0;
			}
		}
		psh->bitR0p[i] = bit;
	}
	for (i = 0; i < p1; i++)
		psh->p0p[i] = vm->p[i];
	for (i = 0; i < f1; i++) {
		psh->f0p[i] = vm->f[i];
		bit = vm->bitF[i];
		if (bit == BIT_DISABLE_REG)
			bit = BIT_DISABLE_MEM;
		else {
			if (bit > bitF)
				bit = bitF;	// Fxxではbitを小さいほうに合わせるだけでよい...いいのだろうか...
		}
		psh->bitF0p[i] = bit;
	}
	retcode = 0;
fin:
	return retcode;
}

int osecpuVmStackPull(OsecpuVm *vm, int r1, int bitR, int p1, int f1, int bitF)
{
	int retcode = 1, i, bit;
	StackHeader *psh = (StackHeader *) vm->stackTop;
	if (psh->totalSize <= 0 || psh->r1 != r1 || psh->bitR != bitR) goto fin;
	if (psh->p1 != p1 || psh->f1 != f1 || psh->bitF != bitF) goto fin;
	for (i = 0; i < r1; i++) {
		vm->r[i] = psh->r0p[i];
		bit = psh->bitR0p[i];
		if (bit == BIT_DISABLE_MEM)
			bit = BIT_DISABLE_REG;
		vm->bit[i] = bit;
	}
	for (i = 0; i < p1; i++)
		vm->p[i] = psh->p0p[i];
	for (i = 0; i < f1; i++) {
		vm->f[i] = psh->f0p[i];
		bit = psh->bitF0p[i];
		if (bit == BIT_DISABLE_MEM)
			bit = BIT_DISABLE_REG;
		vm->bitF[i] = bit;
	}
	vm->p[0x30] = psh->p30;
	vm->stack0 -= psh->totalSize;
	vm->stackTop = psh->prevStackTop;
	retcode = 0;
fin:
	return retcode;
}

typedef struct _TallocHeader {
	int totalSize;
	char *prevStackTop;
	int r1, typ, len; // r1は-1にする.
} TallocHeader;

#define TALLOCHEADERSIZE16	((sizeof (TallocHeader) + 15) & -16)	// サイズを16バイト単位に切り上げたもの.

char *osecpuVmStackAlloc(OsecpuVm *vm, int totalSize, int typ, int len)
{
	char *p = NULL, *prevStackTop;
	TallocHeader *pth;
	totalSize = ((totalSize + 15) & -16) + TALLOCHEADERSIZE16;
	if (vm->stack0 + totalSize > vm->stack1) goto fin;
	p = vm->stack0;
	prevStackTop = vm->stackTop;
	vm->stack0 += totalSize;
	vm->stackTop = p;
	pth = (TallocHeader *) p;
	p += TALLOCHEADERSIZE16;
	pth->totalSize = totalSize;
	pth->prevStackTop = prevStackTop;
	pth->r1 = -1;	// talloc mark.
	pth->typ = typ;
	pth->len = len;
fin:
	return p;
}

int osecpuVmStackFree(OsecpuVm *vm, int totalSize, char *p, int typ, int len)
{
	int retcode = 1;
	TallocHeader *pth = (TallocHeader *) vm->stackTop;
	totalSize = ((totalSize + 15) & -16) + TALLOCHEADERSIZE16;
	if (pth->totalSize != totalSize || pth->r1 != -1 || pth->typ != typ || pth->len != len || (p - TALLOCHEADERSIZE16) != vm->stackTop) goto fin;
	vm->stack0 -= totalSize;
	vm->stackTop = pth->prevStackTop;
	retcode = 0;
fin:
	return retcode;
}

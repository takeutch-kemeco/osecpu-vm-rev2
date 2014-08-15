#include "osecpu-vm.h"

// その他の命令: 00, 2F, 3C, 3D, FD, FE

void jitcInitOther(OsecpuJitc *jitc)
{
	jitc->ope04 = NULL;
	return;
}

int jitcStepOther(OsecpuJitc *jitc)
{
	Int32 *ip = jitc->srcBuffer;
	Int32 opecode = ip[0], imm;
	int bit0, bit1, r, p, typ, f, i;
	int retcode = -1, *pRC = &retcode;
	if (opecode == 0x00) { /* NOP */
		jitcSetSrcBufferSimple(jitc, 1);
		goto fin;
	}
	if (opecode == 0x2f) {
		jitcSetSrcBufferSimple(jitc, 2);
		i = ip[1];
		jitc->prefix2f[i] = 1;
		goto fin;
	}
	if (opecode == 0x30) {
		jitcSetSrcBufferSimple(jitc, 6);
	//	typ = ip[1]; bit0 = ip[2]; r = ip[3]; bit1 = ip[4]; p = ip[5];
		goto fin;
	}
	if (opecode == 0x31) {
		jitcSetSrcBufferSimple(jitc, 6);
	//	p = ip[1]; typ = ip[2]; bit0 = ip[3]; r = ip[4]; bit1 = ip[5]; 
		goto fin;
	}
	if (opecode == 0x32) {
		jitcSetSrcBufferSimple(jitc, 6);
	//	typ = ip[1]; bit0 = ip[2]; r = ip[3]; bit1 = ip[4]; p = ip[5];
		goto fin;
	}
	if (opecode == 0x33) {
		jitcSetSrcBufferSimple(jitc, 6);
	//	p = ip[1]; typ = ip[2]; bit0 = ip[3]; r = ip[4]; bit1 = ip[5]; 
		goto fin;
	}
	if (opecode == 0x3c) {
		jitcSetSrcBufferSimple(jitc, 7);
		r = ip[1]; bit0 = ip[2]; p = ip[3]; f = ip[4]; bit1 = ip[5]; typ = ip[6];
		if (typ != PTR_TYP_NULL)
			jitcSetRetCode(pRC, JITC_UNSUPPORTED);
		goto fin;
	}
	if (opecode == 0x3d) {
		jitcSetSrcBufferSimple(jitc, 7);
		r = ip[1]; bit0 = ip[2]; p = ip[3]; f = ip[4]; bit1 = ip[5]; typ = ip[6];
		if (typ != PTR_TYP_NULL)
			jitcSetRetCode(pRC, JITC_UNSUPPORTED);
		goto fin;
	}
	if (opecode == 0xfd) {
		ip[1] = jitc->src[1];
		ip[2] = jitc->src[2] & 0x00ffffff; 
		jitc->src += 3;
		jitc->instrLength = 3;
		imm = ip[1]; r = ip[2];
 		if (0 <= r && r <= 3)
			jitc->dr[r] = imm;
		goto fin;
	}
	if (opecode == 0xfe) {	// remark
		jitcSetSrcBufferSimple(jitc, 3);
		imm = ip[1]; i = ip[2];
		jitc->instrLength = 0; // 自前で処理するので、この値は0にする.
		jitc->src += i * 2; // 読み飛ばす.
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
	if (jitc->srcBuffer[0] != 0x2f) {
		// 2Fプリフィクスフラグをクリアする.
		for (i = 0; i < PREFIX2F_SIZE; i++)
			jitc->prefix2f[i] = 0;
	}
	return retcode;
}

int osecpuVmStackPush(OsecpuVm *vm, int r1, int bitR, int p1, int f1, int bitF);
int osecpuVmStackPull(OsecpuVm *vm, int r1, int bitR, int p1, int f1, int bitF);
char *osecpuVmStackAlloc(OsecpuVm *vm, int totalSize, int typ, int len);
int osecpuVmStackFree(OsecpuVm *vm, int totalSize, char *p, int typ, int len);

void execStepOther(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0], imm;
	int bit0, bit1, r, p, f, typ, typSign, typSize0, typSize1, i;
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
		typ = execStep_getRxx(vm, typ, bit0);
		i = execStep_getRxx(vm, r, bit1);
		getTypSize(typ, &typSize0, &typSize1, &typSign);
		if (i < 0 || typSize0 < 0 || i > 256 * 1024 * 1024)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_ALLOC_ERROR);
		vm->p[p].p = (unsigned char *) osecpuVmStackAlloc(vm, typSize1 * i, typ, i);
		if (vm->p[p].p == NULL)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_ALLOC_ERROR);
		vm->p[p].typ = typ;
		ip += 6;
		goto fin;
	}
	if (opecode == 0x31) {
		p = ip[1]; typ = ip[2]; bit0 = ip[3]; r = ip[4]; bit1 = ip[5]; 
		typ = execStep_getRxx(vm, typ, bit0);
		i = execStep_getRxx(vm, r, bit1);
		getTypSize(typ, &typSize0, &typSize1, &typSign);
		if (i < 0 || typSize0 < 0 || i > 256 * 1024 * 1024)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_FREE_ERROR);
		r = osecpuVmStackFree(vm, typSize1 * i, NULL, typ, i);
		if (r != 0)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_FREE_ERROR);
		ip += 6;
		goto fin;
	}
	if (opecode == 0x32) {
		typ = ip[1]; bit0 = ip[2]; r = ip[3]; bit1 = ip[4]; p = ip[5];
		typ = execStep_getRxx(vm, typ, bit0);
		i = execStep_getRxx(vm, r, bit1);
		getTypSize(typ, &typSize0, &typSize1, &typSign);
		if (i < 0 || typSize0 < 0 || i > 256 * 1024 * 1024)
			jitcSetRetCode(&vm->errorCode, EXEC_STACK_ALLOC_ERROR);
		vm->p[p].p = (unsigned char *) malloc(typSize1 * i);
		if (vm->p[p].p == NULL)
			jitcSetRetCode(&vm->errorCode, EXEC_MALLOC_ERROR);
		vm->p[p].typ = typ;
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
	PReg *p0p;
	double *f0p;
} StackHeader;

#define STACKHEADERSIZE16	((sizeof (StackHeader) + 15) & -16)	// サイズを16バイト単位に切り上げたもの.

int osecpuVmStackInit(OsecpuVm *vm, int stackSize)
{
	int retcode = 1;
	StackHeader *psh;
	if (stackSize < STACKHEADERSIZE16) goto fin;
	vm->stack0 = malloc(stackSize);
	vm->stack00 = vm->stack0;
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
	int f1Size = f1 * sizeof (double);
	int totalSize, i;
	StackHeader *psh;
	char *p, *prevStackTop;
	r1Size  = (r1Size  + 15) & -16;
	p1Size  = (p1Size  + 15) & -16;
	f1Size  = (f1Size  + 15) & -16;
	totalSize = STACKHEADERSIZE16 + r1Size + p1Size + f1Size;
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
	for (i = 0; i < r1; i++)
		psh->r0p[i] = vm->r[i];
	for (i = 0; i < p1; i++)
		psh->p0p[i] = vm->p[i];
	for (i = 0; i < f1; i++)
		psh->f0p[i] = vm->f[i];
	retcode = 0;
fin:
	return retcode;
}

int osecpuVmStackPull(OsecpuVm *vm, int r1, int bitR, int p1, int f1, int bitF)
{
	int retcode = 1, i;
	StackHeader *psh = (StackHeader *) vm->stackTop;
	if (psh->totalSize <= 0 || psh->r1 != r1 || psh->bitR != bitR) goto fin;
	if (psh->p1 != p1 || psh->f1 != f1 || psh->bitF != bitF) goto fin;
	for (i = 0; i < r1; i++)
		vm->r[i] = psh->r0p[i];
	for (i = 0; i < p1; i++)
		vm->p[i] = psh->p0p[i];
	for (i = 0; i < f1; i++)
		vm->f[i] = psh->f0p[i];
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
	if (pth->totalSize != totalSize || pth->r1 != -1 || pth->typ != typ || pth->len != len) goto fin;
	if (p != NULL && (p - TALLOCHEADERSIZE16) != vm->stackTop) goto fin;
	vm->stack0 -= totalSize;
	vm->stackTop = pth->prevStackTop;
	retcode = 0;
fin:
	return retcode;
}

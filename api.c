#include "osecpu-vm.h"
#include <setjmp.h>

const Int32 *apiEntry(OsecpuVm *vm);

/* driver.c */
void *mallocRWE(int bytes); // 実行権付きメモリのmalloc.
void drv_openWin(int x, int y, unsigned char *buf, char *winClosed);
void drv_flshWin(int sx, int sy, int x0, int y0);
void drv_sleep(int msec);

void apiInit(OsecpuVm *vm)
{
	vm->p[0x28].typ = PTR_TYP_NATIVECODE;
	vm->p[0x28].p = (void *) &apiEntry;
	return;
}

Int32 apiGetRxx(OsecpuVm *vm, int r, int bit);

void api0001_putString(OsecpuVm *vm);
void api0002_drawPoint(OsecpuVm *vm);
void api0003_drawLine(OsecpuVm *vm);
void api0004_rect(OsecpuVm *vm);
void api0005_oval(OsecpuVm *vm);
void api0006_drawString(OsecpuVm *vm);
void api0008_exit(OsecpuVm *vm);
void api0009_sleep(OsecpuVm *vm);

const Int32 *apiEntry(OsecpuVm *vm)
// VMの再開地点を返す.
{
	int func = apiGetRxx(vm, 0x30, 16);
	if (vm->errorCode != 0) goto fin;
	if (func == 0x0001) { api0001_putString(vm);	goto fin; }
	if (func == 0x0002) { api0002_drawPoint(vm);	goto fin; }
	if (func == 0x0003) { api0003_drawLine(vm);		goto fin; }
	if (func == 0x0004) { api0004_rect(vm);			goto fin; }
	if (func == 0x0005) { api0005_oval(vm);			goto fin; }
	if (func == 0x0006) { api0006_drawString(vm);	goto fin; }
	if (func == 0x0008) { api0008_exit(vm);			goto fin; }
	if (func == 0x0009) { api0009_sleep(vm);		goto fin; }
	jitcSetRetCode(&vm->errorCode, EXEC_API_ERROR);
fin: ;
	const Int32 *retcode = NULL;
	execStep_checkMemAccess(vm, 0x30, PTR_TYP_CODE, EXEC_CMA_FLAG_EXEC); // 主にliveSignのチェック.
	if (vm->errorCode == 0)
		retcode = (const Int32 *) vm->p[0x30].p;
	return retcode;
}

Int32 apiGetRxx(OsecpuVm *vm, int r, int bit)
{
	Int32 i, value = vm->r[r];
	if (vm->bit[r] != BIT_DISABLE_REG && vm->bit[r] < bit)
		jitcSetRetCode(&vm->errorCode, EXEC_BAD_BITS);
	i = (-1) << bit;
	if ((value & (1 << (bit - 1))) == 0) {
		// 符号ビットが0だった.
		value &= ~i;
	} else {
		// 符号ビットが1だった.
		value |=  i;
	}
	return value;
}

void api0001_putString(OsecpuVm *vm) { }
void api0002_drawPoint(OsecpuVm *vm) { }
void api0003_drawLine(OsecpuVm *vm) { }
void api0004_rect(OsecpuVm *vm) { puts("api0004_rect");  }
void api0005_oval(OsecpuVm *vm) { }
void api0006_drawString(OsecpuVm *vm) { }
void api0008_exit(OsecpuVm *vm) { }
void api0009_sleep(OsecpuVm *vm) { }


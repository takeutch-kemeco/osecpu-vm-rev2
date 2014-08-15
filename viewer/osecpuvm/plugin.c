#include "osecpu-vm.h"

void apiInit(OsecpuVm *vm, void *apiFunc);

int execPlugIn(const unsigned char *path, void *apiFunc, void *env, int bsiz)
// path: プラグインのパス.
// apiFunc: P2Fの処理関数.
// env: 追加環境情報.
{
	Defines defs;
	OsecpuJitc jitc;
	OsecpuVm vm;
	unsigned char *byteBuf0 = malloc(bsiz);
	Int32 *j32buf = malloc(bsiz * sizeof (Int32));
	int fileSize, rc = 0;
	int stackSize = 1; /* メガバイト単位 */
	FILE *fp;
	jitc.defines = &defs;
	vm.defines = &defs;
	vm.toDebugMonitor = 0;
	vm.exitToDebug = 0;
	vm.debugAutoFlsh = 1;
	vm.debugBreakPointIndex = -1;
	vm.debugBreakPointValue = 0;
	vm.extEnv = env;
	osecpuVmStackInit(&vm, stackSize * (1024 * 1024));
	osecpuVmPtrCtrlInit(&vm, 4096); 
	fp = fopen(path, "rb");
	if (fp == NULL) {
		rc = OSECPUVM_FILE_NOT_FOUND;
		goto fin;
	}
	fileSize = fread(byteBuf0, 1, bsiz, fp);
	fclose(fp);
	if (fileSize >= bsiz) {
		rc = OSECPUVM_DOWN;
		goto fin;
	}
	if (byteBuf0[0] != 0x05 || byteBuf0[1] != 0xe2 || fileSize < 3) {
		rc = OSECPUVM_DOWN;
		goto fin;
	}

	// フロントエンドコードからバックエンドコードを得るためのループ.
	for (;;) {
		if (fileSize < 0) break;
		if (byteBuf0[2] == 0x02) {
			fileSize = decode_tek5 (byteBuf0 + 3, byteBuf0 + fileSize, byteBuf0 + 2, byteBuf0 + bsiz);
			if (fileSize > 0) fileSize += 2;
			continue;
		}
		if (byteBuf0[2] == 0x01) {
			fileSize = decode_upx  (byteBuf0 + 3, byteBuf0 + fileSize, byteBuf0 + 2, byteBuf0 + bsiz);
			if (fileSize > 0) fileSize += 2;
			continue;
		}
		if (byteBuf0[2] >= 0x10) {
			fileSize = decode_fcode(byteBuf0 + 2, byteBuf0 + fileSize, byteBuf0 + 2, byteBuf0 + bsiz);
			if (fileSize > 0) fileSize += 2;
			continue;
		}
		break;
	}
	if (fileSize <= 0 || byteBuf0[2] != 0x00) {
		rc = OSECPUVM_DOWN;
		goto fin;
	}
	hh4ReaderInit(&jitc.hh4r, byteBuf0 + 3, 0, byteBuf0 + fileSize, 0);
	jitc.dst  = j32buf;
	jitc.dst1 = j32buf + bsiz;
	rc = jitcAll(&jitc);
	if (rc != 0) {
		rc = OSECPUVM_DOWN;
		goto fin;
	}
	vm.ip  = j32buf;
	vm.ip1 = jitc.dst;

	apiInit(&vm, apiFunc);
	rc = execAll(&vm);
	if (rc != EXEC_SRC_OVERRUN && rc != EXEC_API_END)
		rc = OSECPUVM_DOWN;
	else
		rc = OSECPUVM_END;
fin:
	free(byteBuf0);
	free(j32buf);
	free(vm.stack00);
	free(vm.ptrCtrl);
	return rc;
}

void apiInit(OsecpuVm *vm, void *apiFunc)
{
	int i, j;
	for (i = 0; i <= 0x3f; i++) {
		vm->r[i] = 0; vm->bit[i] = 32; // Rxx: すべて32ビットの0.
		vm->p[i].typ = PTR_TYP_INVALID; // Pxx: すべて不正.
	}
	j = 1;
	for (i = 0; i < DEFINES_MAXLABELS; i++) {
		if (j > 4) break;
		if (vm->defines->label[i].opt != 1) continue;
		if (vm->defines->label[i].typ == PTR_TYP_CODE) continue;
		execStep_plimm(vm, j, i);
		j++;
	}
	vm->p[0x2f].typ = PTR_TYP_NATIVECODE;
	vm->p[0x2f].p = apiFunc;
	vm->debugWatchIndex[0] = 0;
	vm->debugWatchIndex[1] = 1;
	vm->debugWatchs = 0;
	return;
}


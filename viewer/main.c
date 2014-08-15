#include "osecpuvm/osecpu-vm.h"

#define BUFFER_SIZE		1024 * 1024	// 1M

const Int32 *apiEntry(OsecpuVm *vm);

typedef struct _ViewerEnv {
	unsigned char *pic, *bit;
	int fsiz;
	int retcode;
	unsigned char pathPlugin[32];
	char winClosed;
} ViewerEnv;

void drv_openWin(int x, int y, unsigned char *buf, char *winClosed);
void drv_flshWin(int sx, int sy, int x0, int y0);
void drv_sleep(int msec);

#define KEYBUFSIZ		4096	// 今回キー入力は必要ないのだが、OSECPU-VMのドライバを使って安易にviewerをマルチOS対応にしたいので、この定義が残っている.
extern int *keybuf, keybuf_r, keybuf_w, keybuf_c;
extern int *vram, v_xsiz, v_ysiz;

int OsecpuMain(int argc, const unsigned char **argv)
// ヘンテコな名前のmain関数だけど、描画にOSECPU-VMのドライバを使った副作用なので許してください.
{
	FILE *fp;
	ViewerEnv env;
	int retcode, i;
	if (argc < 2) {
err:
		fputs("usage>picview pic-file\n", stderr);
		exit(1);
	}
	env.pic = malloc(8 * 1024 * 1024 + 1);
	fp = fopen(argv[1], "rb");
	if (fp == NULL) goto err;
	env.fsiz = fread(env.pic, 1, 8 * 1024 * 1024, fp);
	if (env.fsiz >= 8 * 1024 * 1024) {
		fputs("too large file\n", stderr);
		exit(1);
	}
	fclose(fp);
	env.bit = malloc(env.fsiz);
	for (i = 0; i < env.fsiz; i++)
		env.bit[i] = 8; // 全てのデータが8ビット有効.

	keybuf_r = keybuf_w = keybuf_c = 0;
	keybuf = malloc(KEYBUFSIZ * sizeof (int));

	for (i = 0; i < 10; i++) {
		sprintf(env.pathPlugin, "plugin%02d.ose", i);
		env.retcode = 0;
		env.winClosed = 0;
		retcode = execPlugIn(env.pathPlugin, &apiEntry, &env, BUFFER_SIZE);
		if (env.retcode == 1) continue; // format errorなら次のプラグインへ.
		if (retcode == OSECPUVM_END) break; // ダウンせずに最後まで実行したのならbreak.
		if (retcode == OSECPUVM_DOWN)
			fprintf(stderr, "down: %s\n", env.pathPlugin);
	}
	if (i >= 10)
		puts("unknown format");
	else {
		drv_flshWin(v_xsiz, v_ysiz, 0, 0);
		while (env.winClosed == 0)
			drv_sleep(100);
	}
	return 0;
}

void api0010_openWin(OsecpuVm *vm);
void api0002_drawPoint(OsecpuVm *vm);

const Int32 *apiEntry(OsecpuVm *vm)
{
	ViewerEnv *env = vm->extEnv;
	int func = execStep_getRxx(vm, 0x30, 16); // 下位16bitしかみない.
	const Int32 *retcode = NULL;
	if (func == 0x07c0) {	// api07c0_fileRead
		vm->bit[0x30] = 32;
		vm->r[0x30] = env->fsiz;
		vm->p[0x31].p = env->pic;
		vm->p[0x31].p0 = env->pic;
		vm->p[0x31].p1 = env->pic + env->fsiz;
		vm->p[0x31].typ = 3; // T_UINT8
		vm->p[0x31].flags = EXEC_CMA_FLAG_READ; // over-seek:ok, read:ok, write:err
		vm->p[0x31].bit = env->bit;
		goto fin;
	}
	if (func == 0x0001) {	// api0001_putString
		env->retcode = 1;
		jitcSetRetCode(&vm->errorCode, EXEC_API_END); // これで強制終了できる.
		goto fin;
 	}
	if (func == 0x0009) {	// api0009_sleep
		if (vram != 0) {
			jitcSetRetCode(&vm->errorCode, EXEC_API_END); // これで強制終了できる.
			goto fin;
		}
	}
	if (func == 0x0010) { api0010_openWin(vm);		goto fin; }
	if (func == 0x0002) { api0002_drawPoint(vm);	goto fin; }
	jitcSetRetCode(&vm->errorCode, EXEC_API_ERROR);
fin:
	execStep_checkMemAccess(vm, 0x30, PTR_TYP_CODE, EXEC_CMA_FLAG_EXEC); // 主にliveSignのチェック.
	if (vm->errorCode == 0)
		retcode = (const Int32 *) vm->p[0x30].p;
	return retcode;
}

static int iColor1[] = {
	0x000000, 0xff0000, 0x00ff00, 0xffff00,
	0x0000ff, 0xff00ff, 0x00ffff, 0xffffff
};

int apiLoadColor(OsecpuVm *vm, int rxx)
{
	int c = execStep_getRxx(vm, rxx, 16), m, mm, rr, gg, bb;
	mm = execStep_getRxx(vm, 0x31, 16);
    if ((mm & 0x103) == 0x100) mm |= 0x03;
	if ((mm & 0xf00) == 0x100) mm |= 0xe00;
	m = mm & 3;
	if (m == 0x00) {
	//	static col3_bgr_table[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
		if (c < -1 || c > 7)
			jitcSetRetCode(&vm->errorCode, EXEC_API_ERROR);
	//	if (apiWork.col3bgr != 0)
	//		c = col3_bgr_table[c & 7];
		c = iColor1[c & 0x07];
	}
	if (m == 0x01) {
		// 00, 24, 48, 6d, 91, b6, da, ff
		if ((mm & 0x100) != 0)
			c *= 1 << 6 | 1 << 3 | 1;
		if (c < 0 || c >= (1 << 9))
			jitcSetRetCode(&vm->errorCode, EXEC_API_ERROR);
		rr = (c >>  6) & 0x07;
		gg = (c >>  3) & 0x07;
		bb =  c        & 0x07;
		rr = (rr * 255) / 7;
		gg = (gg * 255) / 7;
		bb = (bb * 255) / 7;
		c = rr << 16 | gg << 8 | bb;
	}
	if (m == 0x02) {
		// 00, 08, 10, 18, 20, 29, 31, 39,
		// 41, 4a, 52, 5a, 62, 6a, 73, 7b,
		// 83, 8b, 94, 9c, a4, ac, b4, bd,
		// c5, cd, d5, de, e6, ee, f6, ff
		if (c < 0 || c >= (1 << 15))
			jitcSetRetCode(&vm->errorCode, EXEC_API_ERROR);
		if ((mm & 0x100) != 0)
			c *= 1 << 10 | 1 << 5 | 1;
		rr = (c >> 10) & 0x1f;
		gg = (c >>  5) & 0x1f;
		bb =  c        & 0x1f;
		rr = (rr * 255) / 31;
		gg = (gg * 255) / 31;
		bb = (bb * 255) / 31;
		c = rr << 16 | gg << 8 | bb;
	}
	if (m == 0x03) {
		c = execStep_getRxx(vm, rxx, 32);
		if ((mm & 0x100) != 0)
			c *= 1 << 16 | 1 << 8 | 1;
	}
	if ((mm & 0x100) != 0)
		c &= iColor1[(mm >> 9) & 7];
//	if (vm->errorCode > 0)
//		longjmp(apiWork.setjmpErr, 1);
	return c;
}

void apiCheckPoint(OsecpuVm *vm, int x, int y)
{
	if (x < 0 || v_xsiz <= x || y < 0 || v_ysiz <= y) {
		jitcSetRetCode(&vm->errorCode, EXEC_API_ERROR);
//		longjmp(apiWork.setjmpErr, 1);
	}
	return;
}

void api0010_openWin(OsecpuVm *vm)
{
	ViewerEnv *env = vm->extEnv;
	int i, c, x, y;
	c = apiLoadColor(vm, 0x32);
	x = execStep_getRxx(vm, 0x33, 16);
	y = execStep_getRxx(vm, 0x34, 16);
	if (y == 0 && x > 0) y = x;
	if (vram == 0) {
		if (x <= 0 || 4096 < x || y < 0 || 4096 < y)
			jitcSetRetCode(&vm->errorCode, EXEC_API_ERROR);
		if (vm->errorCode > 0) goto fin;
		v_xsiz = x;
		v_ysiz = y;
		vram = malloc(x * y * sizeof (int));
		drv_openWin(x, y, (void *) vram, &env->winClosed);
//		apiWork.autoSleep = 1;
		printf("%s : %dx%d\n", env->pathPlugin, x, y);
	} else {
		if (x != v_xsiz || y != v_ysiz)
			jitcSetRetCode(&vm->errorCode, EXEC_API_ERROR);
	}
	for (i = 0; i < x * y; i++)
		vram[i] = c;
fin:
	return;
}

void api0002_drawPoint(OsecpuVm *vm)
// Point(mode:R31, c:R32, x:R33, y:R34)
{
	int c = apiLoadColor(vm, 0x32), modeC = execStep_getRxx(vm, 0x31, 4) & 0x0c;
	int x = execStep_getRxx(vm, 0x33, 16), y = execStep_getRxx(vm, 0x34, 16);
	apiCheckPoint(vm, x, y);
	if (vm->errorCode == 0) {
		if (modeC == 0x00) vram[x + y * v_xsiz]  = c;
		if (modeC == 0x04) vram[x + y * v_xsiz] |= c;
		if (modeC == 0x08) vram[x + y * v_xsiz] ^= c;
		if (modeC == 0x0c) vram[x + y * v_xsiz] &= c;
	}
	return;
}


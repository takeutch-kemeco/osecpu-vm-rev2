#include "osecpu-vm.h"

// ポインタ関連命令: 01, 03, 0E, 1E, 2E

void jitcInitPointer(OsecpuJitc *jitc)
{
	return;
}

void getTypSize(int typ, int *typSize0, int *typSize1, int *typSign)
// typSize0: 入力バイナリ内でのビット数.
// typSize1: 出力バイナリ内でのバイト数.
{
	*typSize0 = *typSize1 = -1;
	if (2 <= typ && typ <= 21) {
		static unsigned char table[10] = { 8, 16, 32, 4, 2, 1, 12, 20, 24, 28 };
		if ((typ & 1) == 0)
			*typSign = -1; // typが偶数なら符号あり.
		else
			*typSign = 0; // typが奇数なら符号なし.
		*typSize0 = table[(typ - 2) / 2];
		*typSize1 = sizeof (Int32);
	}
	return;
}

int jitcStepPointer(OsecpuJitc *jitc)
{
	Int32 *ip = jitc->hh4Buffer;
	Int32 opecode = ip[0];
	int retcode = -1, *pRC = &retcode;
	int i, j, opt, p, p0, p1, r, bit, typ, len, typSign, typSize0, typSize1;
	if (opecode == 0x01) { /* LB(opt, uimm); */
		jitcSetHh4BufferSimple(jitc, 3);
		i = ip[1]; opt = ip[2];
		if (jitc->phase > 0) goto fin;
		if (!(0 <= i && i < DEFINES_MAXLABELS)) {
			jitcSetRetCode(pRC, JITC_BAD_LABEL);
			goto fin;
		}
		if (jitc->defines->label[i].typ != PTR_TYP_INVALID) {
			jitcSetRetCode(pRC, JITC_LABEL_REDEFINED);
			goto fin;
		}
		if (!(0 <= opt && opt <= 3)) {
			jitcSetRetCode(pRC, JITC_BAD_LABEL_TYPE);
			goto fin;
		}
		jitc->defines->label[i].typ = PTR_TYP_CODE; // とりあえずコードラベルにする.
		jitc->defines->label[i].opt = opt;
		jitc->defines->label[i].dst = jitc->dst;
		goto fin;
	}
	if (opecode == 0x03) { /* PLIMM(imm, Pxx); */
		jitcSetHh4BufferSimple(jitc, 3);
		i = ip[1]; p = ip[2];
		goto fin;
	}
	if (opecode == 0x0e) {	// PADD(p1, typ, r, bit, p0);
		jitcSetHh4BufferSimple(jitc, 6);
		p1 = ip[1]; typ = ip[2]; r = ip[3]; bit = ip[4]; p0 = ip[5];
		jitcStep_checkBits32(pRC, bit);
		goto fin;
	}
	if (opecode == 0x1e) {	// PCP(p1, p0);
		jitcSetHh4BufferSimple(jitc, 3);
		p1 = ip[1]; p0 = ip[2];
		goto fin;
	}
	if (opecode == 0x2e) { // data...これはother.cへ移動させるべき
		BitReader br;
		typ = hh4ReaderGetUnsigned(&jitc->hh4r);
		len = hh4ReaderGetUnsigned(&jitc->hh4r);
		getTypSize(typ, &typSize0, &typSize1, &typSign);
		if (typSize0 < 0) {
			jitcSetRetCode(pRC, JITC_BAD_TYPE);
			goto fin;
		}
		jitc->instrLength = 0; // 自前で処理するので、この値は0にする.
		if (jitc->dst + 3 + len > jitc->dst1) {
			jitcSetRetCode(pRC, JITC_DST_OVERRUN);
			goto fin;
		}
		ip = jitc->dst;
		ip[0] = 0x2e;
		ip[1] = typ;
		ip[2] = len;
		jitc->dst += 3;
		if (typ != 1) {
			bitReaderInit(&br, &jitc->hh4r);
			if (typSign == 0) {
				Int32 *pi = (Int32 *) jitc->dst;
				for (i = 0; i < len; i++)
					pi[i] = bitReaderGetNbitUnsigned(&br, typSize0);
			}
			if (typSign != 0) {
				Int32 *pi = (Int32 *) jitc->dst;
				for (i = 0; i < len; i++)
					pi[i] = bitReaderGetNbitSigned(&br, typSize0);
			}
			jitc->dst += len;
		}
		for (j = 0; j < JITC_DSTLOG_SIZE; j++) {
			Int32 *dstLog = jitc->dstLog[(jitc->dstLogIndex + JITC_DSTLOG_SIZE - 1 - j) % JITC_DSTLOG_SIZE];
				// 1つ前、2つ前、3つ前...の命令をチェックしている.
			if (dstLog == NULL) break;
			if (dstLog[0] != 0x01) break;
			if (dstLog[2] != 1) break; // opt=1以外はデータラベルにはできない.
			i = dstLog[1];
			jitc->defines->label[i].typ = typ;
			jitc->defines->label[i].dst = ip;
		}
		i = jitc->dstLogIndex;
		jitc->dstLog[i] = ip; // エラーのなかった命令は記録する.
		jitc->dstLogIndex = (i + 1) % JITC_DSTLOG_SIZE;
		goto fin;

	}
	goto fin1;
fin:
	if (retcode == -1)
		retcode = 0;
fin1:
	return retcode;
}

int jitcAfterStepPointer(OsecpuJitc *jitc)
{
	return 0;
}

void execStepPointer(OsecpuVm *vm)
{
	const Int32 *ip = vm->ip;
	Int32 opecode = ip[0];
	int i, p, r, p0, p1, bit, typ, len;
	if (opecode == 0x01) { /* LB(opt, uimm); */
		ip += 3;
		goto fin;
	}
	if (opecode == 0x03) { /* PLIMM(imm, Pxx); */
		i = ip[1]; p = ip[2];
		if (p == 0x3f)
			ip = (const Int32 *) vm->defines->label[i].dst;
		else {
			execStep_plimm(vm, p, i);
			ip += 3;
		}
		goto fin;
	}
	if (opecode == 0x0e) {	// PADD(p1, typ, r, bit, p0);
		p1 = ip[1]; typ = ip[2]; r = ip[3]; bit = ip[4]; p0 = ip[5];
		vm->p[p0] = vm->p[p1];
		vm->p[p0].p += vm->r[r] * sizeof (Int32);
		ip += 6;
	}
	if (opecode == 0x1e) {	// PCP(p1, p0);
		p1 = ip[1]; p0 = ip[2];
		if (p0 == 0x3f) {
			if (vm->p[p1].typ == PTR_TYP_CODE) {	// code
				ip = (const Int32 *) vm->p[p1].p;
			}
			if (vm->p[p1].typ == PTR_TYP_NATIVECODE) {	// native-code (API)
				const Int32 *(*func)(OsecpuVm *);
				const Int32 *nextIp;
				func = (void *) vm->p[p1].p;
				nextIp = (*func)(vm);
				if (nextIp != NULL)
					ip = nextIp;
				else
					jitcSetRetCode(&vm->errorCode, EXEC_API_ERROR);
			}
		} else {
			vm->p[p0] = vm->p[p1];
			ip += 3;
		}
		goto fin;
	}
	if (opecode == 0x2e) {	// data
		typ = ip[1]; len = ip[2];
		ip += 3 + len;
		goto fin;
	}
fin:
	vm->ip = ip;
	return;
}

void execStep_plimm(OsecpuVm *vm, int p, int i)
{
	int typ;
	typ = vm->defines->label[i].typ;
	vm->p[p].typ = typ;
	if (typ >= 2) {
		vm->p[p].p = (unsigned char *) (vm->defines->label[i].dst + 3);
	}
	if (typ == PTR_TYP_CODE) {	// コードラベル.
		vm->p[p].p = (unsigned char *) vm->defines->label[i].dst;
	}
	if (typ == 1) {		// VPtr.
	}
	return;
}


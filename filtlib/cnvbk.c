#include "filtlib.h"
#include "osecode.h"
#include <stdio.h>

// Int32->UInt4: bs32形式からbackend形式を生成.

typedef UInt4 OutTyp;
#define OutBufSiz		512

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
} Work;

static OutTyp dummyOutDat = 0;

static void putHh4s(Work *wp, Int32 i, int len);
static void putHh4u(Work *wp, Int32 i, int len);

void *OsaFL_cnvBk(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_cnvBk;
		wp->cw.drvnam = "OsaFL_cnvBk";
		wp->cw.customFlags = 0;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "Int32"; // InTyp
		wp->cw.info1 = "UInt4"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		Int32 i32[64], j;
		int k;
		i = (int) OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 1, i32);
		OsaFL_Common_testEod(i);
		j = i32[0];
		putHh4u(wp, j, 0);
		if (j < OsecpuCode_BackendTableLength) {
			k = osecpuCode_backendTable[j][0];
			if ('0' <= k && k <= '9') {
				k -= '0';
				if (k > 0) {
					OsaFL_drvfnc(wp->cw.source, OsaFL_GET, k, i32);
					for (i = 0; i < k; i++) {
						if (osecpuCode_backendTable[j][i + 1] == 'u')
							putHh4u(wp, i32[i], 0);
						else if (osecpuCode_backendTable[j][i + 1] == 's')
							putHh4s(wp, i32[i], 0);
						else {
							putHh4u(wp, i32[i], 0);
							OsaFL_Common_error(vw, "OsaFL_FLB: backendTable error: ope=0x%x.", j);
						}
					}
				}
				goto fin;
			}
		}
		if (j == 0xfd) {	// LIDR.
			OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 2, i32);
			putHh4s(wp, i32[0], 0);
			putHh4u(wp, i32[1], 0);
			goto fin;
		}
		if (j == 0xfe) {	// REM.
			OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 2, i32);
			putHh4u(wp, i32[0], 0);
			putHh4u(wp, i32[1], 0);
			if (i32[1] > 0) {
				if (i32[1] > 64 - 2) {
					OsaFL_Common_error(vw, "OsaFL_FLB: too many REM parameters: 0x%x, opt-len=%d.", i32[0], i32[1]);
					goto fin;
				}
				OsaFL_drvfnc(wp->cw.source, OsaFL_GET, i32[1], i32 + 2);
				if (i32[0] == 2 && i32[1] == 1) {
					putHh4s(wp, i32[2], 0);
					goto fin;
				}
				OsaFL_Common_error(vw, "OsaFL_FLB: unknown remark code: 0x%x, opt-len=%d.", i32[0], i32[1]);
			}
			goto fin;
		}
		OsaFL_Common_error(vw, "OsaFL_FLB: unknown opecode: %04x.\n", j);
		goto fin;
	}
	OsaFL_Common_unknownFunc(vw);
fin:
	return rvp;
}

static void putHh4s(Work *wp, Int32 i, int len)
{
	int j, k;
	Int32 min, max;
	if (len <= 0 && -4 <= i && i <= 3 && i != -1)
		len = 3;
	if (len <= 0) {
		// 6, 9, 12, 16, 20, 24, 28, 32
		for (j = 4; j <= 28; j += 4) {
			k = j;
			if (j == 4) k = 6;
			if (j == 8) k = 9;
			min = (-1) << (k - 1);
			max = ~min;
			if (min <= i && i <= max) {
				len = k;
				goto skip;
			}
		}
		len = 32;
	}
skip:
	putHh4u(wp, i, len);
	return;
}

static void putHh4u(Work *wp, Int32 i, int len)
{
	int j, k, l = wp->cw.obp1, m;
	if (l > OutBufSiz - 16) {
		OsaFL_Common_error(wp, "putHh4u: buffer full.\n");
		goto fin;
	}
	if (len <= 0 && i < 0)
		len = 32;
	if (len <= 0 && i <= 6)
		len = 3;
	if (len <= 0) {
		// 6, 9, 12, 16, 20, 24, 28, 32
		for (j = 4; j <= 28; j += 4) {
			k = j;
			if (j == 4) k = 6;
			if (j == 8) k = 9;
			if (i < (1 << k)) {
				len = k;
				goto skip;
			}
		}
		len = 32;
	}
skip:
	if (len == 3)
		wp->outBuf[l++] = i & 0x7;
	if (len == 6)
		wp->outBuf[l++] = ((i >> 4) & 0x3) | 0x8;
	if (len == 9)
		wp->outBuf[l++] = ((i >> 8) & 0x1) | 0xc;
	if (len == 12)
		wp->outBuf[l++] = 0xe;
	if (len >= 16) {
		wp->outBuf[l++] = 0x7;
		if (len >= 28)
			wp->outBuf[l++] = 0x8;
		wp->outBuf[l++] = len / 4;
	}
	for (m = (len & (-4)) - 4; m >= 0; m -= 4)	//	x & (-4) は、4で割った余りを切り捨てる. 
		wp->outBuf[l++] = (i >> m) & 0xf;
fin:
	wp->cw.obp1 = l;
	return;
}


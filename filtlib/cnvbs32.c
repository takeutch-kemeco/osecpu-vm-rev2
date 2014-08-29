#include "filtlib.h"
#include "osecode.h"

// DecodedData->bs32にするフィルタ.

typedef Int32 OutTyp;
#define OutBufSiz		64

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
} Work;

static OutTyp dummyOutDat = 0;

void *OsaFL_cnvBs32(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_cnvBs32;
		wp->cw.drvnam = "OsaFL_cnvBs32";
		wp->cw.customFlags = 0;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "OsaFL_decodHh4"; // InTyp
		wp->cw.info1 = "Int32"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		OsaFL_DecodedData d[64];
		Int32 *p = wp->outBuf, j;
		int k;
		i = (int) OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 1, d);
		OsaFL_Common_testEod(i);
		*p = j = d->value[0];
		if (j < OsecpuCode_BackendTableLength) {
			k = osecpuCode_backendTable[j][0];
			if ('0' <= k && k <= '9') {
				k -= '0';
				if (k > 0) {
					OsaFL_drvfnc(wp->cw.source, OsaFL_GET, k, d);
					wp->cw.obp1 = k + 1;
					for (i = 0; i < k; i++) {
						if (osecpuCode_backendTable[j][i + 1] == 'u')
							p[i + 1] = d[i].value[0];
						else if (osecpuCode_backendTable[j][i + 1] == 's')
							p[i + 1] = d[i].value[1];
						else {
							p[i + 1] = 0;
							OsaFL_Common_error(vw, "OsaFL_FLB: backendTable error: ope=0x%x.", j);
						}
					}
				}
				goto fin;
			}
		}
		if (j == 0xfd) {	// LIDR.
			OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 2, d);
			wp->cw.obp1 = 3;
			p[1] = d[0].value[1];
			p[2] = d[1].value[0];
			goto fin;
		}
		if (j == 0xfe) {	// REM.
			OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 2, d);
			wp->cw.obp1 = 3;
			p[1] = d[0].value[0];
			p[2] = d[1].value[0];
			if (p[2] > 0) {
				if (p[2] > OutBufSiz - 4) {
					OsaFL_Common_error(vw, "OsaFL_FLB: too many REM parameters: 0x%x, opt-len=%d.", p[1], p[2]);
					goto fin;
				}
				wp->cw.obp1 = 3 + p[2];
				OsaFL_drvfnc(wp->cw.source, OsaFL_GET, p[2], d);
				if (p[1] == 2 && p[2] == 1) {
					p[3] = d[0].value[1];
					goto fin;
				}
				OsaFL_Common_error(vw, "OsaFL_FLB: unknown remark code: 0x%x, opt-len=%d.", p[1], p[2]);
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


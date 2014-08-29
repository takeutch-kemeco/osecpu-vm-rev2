#include "filtlib.h"
#include <stdio.h>

// UInt8をファイルに出力するだけのフィルタ.

typedef UInt8 OutTyp;	// ダミー.
#define OutBufSiz		256

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
	FILE *fp;
} Work;

static OutTyp dummyOutDat = 0;

void *OsaFL_writFile(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_writFile;
		wp->cw.drvnam = "OsaFL_writFile";
		wp->cw.customFlags = 0x40 | 0x20 | 0x08;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "UInt8"; // InTyp
		wp->cw.info1 = "(null)"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_CON) {
		if (i == 0) {
			OsaFL_drvfnc(wp, OsaFL_CON, -1, vp);
			goto fin;
		}
		if (i == 1) {
			if (wp->cw.errCode == 0) {
				if (wp->fp != NULL)
					OsaFL_Common_error(vw, "OsaFL_CON(1): double connect error.");
				if (wp->cw.errCode == 0) {
					wp->fp = fopen(vp, "wb");
					if (wp->fp == NULL)
						OsaFL_Common_error(vw, "OsaFL_CON(1): fopen error.");
				}
			}
			goto fin;
		}
		OsaFL_Common_error(vw, "OsaFL_CON: unknown request: i=%d.", i);
		goto fin;
	}
	if (fnc == OsaFL_DIS) {
		if (i == 0) {
			OsaFL_drvfnc(wp, OsaFL_CON, -1, vp);
			goto fin;
		}
		if (i == 1) {
			if (wp->fp != NULL) {
				fclose(wp->fp);
				wp->fp = NULL;
			}
			goto fin;
		}
		OsaFL_Common_error(vw, "OsaFL_DIS: unknown request: i=%d.", i);
		goto fin;
	}
	if (fnc == OsaFL_SEK) {
		OsaFL_Common_error(vw, "OsaFL_SEK: error (must not seek).");
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		i = (int) OsaFL_drvfnc(wp->cw.source, OsaFL_GET, OutBufSiz, wp->outBuf);
		OsaFL_Common_testEod(i);
		if (wp->fp != NULL)
			fwrite(wp->outBuf, 1, i, wp->fp);
		wp->cw.obp1 = i;
		goto fin;
	}
	OsaFL_Common_unknownFunc(vw);
fin:
	return rvp;
}

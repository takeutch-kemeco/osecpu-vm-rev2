#include "filtlib.h"
#include <stdio.h>

// ファイルを開いてUInt8として出力するフィルタ.

typedef UInt8 OutTyp;
#define OutBufSiz		256

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
} Work;

static OutTyp dummyOutDat = 0;

void *OsaFL_charFile(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_charFile;
		wp->cw.drvnam = "OsaFL_charFile";
		wp->cw.customFlags = 0x40 | 0x20 | 0x02;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "(file)"; // InTyp
		wp->cw.info1 = "UInt8"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_CON) {
		if (wp->cw.errCode != 0) goto fin;
		if (wp->cw.source != NULL)
			OsaFL_Common_error(vw, "OsaFL_CON: double connect error.");
		wp->cw.source = fopen(vp, "rb");
		if (wp->cw.source == NULL)
			OsaFL_Common_error(vw, "OsaFL_CON: fopen error.");
 		OsaFL_drvfnc(vw, OsaFL_SK0, 0, NULL);
		goto fin;
	}
	if (fnc == OsaFL_DIS) {
		if (wp->cw.source != NULL) {
			fclose(wp->cw.source);
			wp->cw.source = NULL;
		}
		goto fin;
	}
	if (fnc == OsaFL_SK0) {
		if (wp->cw.errCode != 0) goto fin;
		if (wp->cw.source == NULL)
			OsaFL_Common_error(vw, "OsaFL_SK0: no source.");
		wp->cw.pos = 0;
		wp->cw.obp = wp->cw.obp1 = 0;
		fseek(wp->cw.source, 0, SEEK_SET);
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		i = fread(wp->outBuf, 1, OutBufSiz, wp->cw.source);
		OsaFL_Common_testEod(i);
		wp->cw.obp1 = i;
		goto fin;
	}
	OsaFL_Common_unknownFunc(vw);
fin:
	return rvp;
}


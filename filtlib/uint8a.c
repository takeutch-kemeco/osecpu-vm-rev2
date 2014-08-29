#include "filtlib.h"

// UInt4->UInt8: ビッグエンディアンで変換する.

typedef UInt8 OutTyp;
#define OutBufSiz		1

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
} Work;

static OutTyp dummyOutDat = 0;

void *OsaFL_uInt8a(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_uInt8a;
		wp->cw.drvnam = "OsaFL_uInt8a";
		wp->cw.customFlags = 0;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "UInt4"; // InTyp
		wp->cw.info1 = "UInt8"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		UInt4 uc4[2];
		i = (int) OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 2, uc4);
		OsaFL_Common_testEod(i);
		OsaFL_Common_putBuf(wp, uc4[0] << 4 | uc4[1]);
		goto fin;
	}
	OsaFL_Common_unknownFunc(vw);
fin:
	return rvp;
}


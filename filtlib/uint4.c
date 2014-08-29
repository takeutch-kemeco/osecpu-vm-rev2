#include "filtlib.h"

// UInt8を2つに分割してUInt4にするだけのフィルタ.

typedef UInt4 OutTyp;
#define OutBufSiz		2

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
} Work;

static OutTyp dummyOutDat = 0;

void *OsaFL_uInt4(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_uInt4;
		wp->cw.drvnam = "OsaFL_uInt4";
		wp->cw.customFlags = 0;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "UInt8"; // InTyp
		wp->cw.info1 = "UInt4"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		UInt8 uc8;
		i = (int) OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 1, &uc8);
		OsaFL_Common_testEod(i);
		OsaFL_Common_putBuf(wp, (uc8 >> 4) & 0xf);
		OsaFL_Common_putBuf(wp,  uc8       & 0xf);
		goto fin;
	}
	OsaFL_Common_unknownFunc(vw);
fin:
	return rvp;
}


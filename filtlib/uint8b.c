#include "filtlib.h"

// Int32を4つに分割してUInt8にするだけのフィルタ.

typedef UInt8 OutTyp;
#define OutBufSiz		4

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
} Work;

static OutTyp dummyOutDat = 0;

void *OsaFL_uInt8b(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_uInt8b;
		wp->cw.drvnam = "OsaFL_uInt8b";
		wp->cw.customFlags = 0;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "Int32"; // InTyp
		wp->cw.info1 = "UInt8"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		Int32 i32;
		i = (int) OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 1, &i32);
		OsaFL_Common_testEod(i);
		OsaFL_Common_putBuf(wp, (i32 >> 24) & 0xff);
		OsaFL_Common_putBuf(wp, (i32 >> 16) & 0xff);
		OsaFL_Common_putBuf(wp, (i32 >>  8) & 0xff);
		OsaFL_Common_putBuf(wp,  i32        & 0xff);
		goto fin;
	}
	OsaFL_Common_unknownFunc(vw);
fin:
	return rvp;
}

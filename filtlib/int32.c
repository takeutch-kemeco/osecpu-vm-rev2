#include "filtlib.h"

// UInt8を4つまとめてビッグエンディアンでInt32にするだけのフィルタ.

typedef Int32 OutTyp;
#define OutBufSiz		1

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
} Work;

static OutTyp dummyOutDat = 0;

void *OsaFL_int32(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_int32;
		wp->cw.drvnam = "OsaFL_int32";
		wp->cw.customFlags = 0;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "UInt8"; // InTyp
		wp->cw.info1 = "Int32"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		UInt8 uc8[4];
		i = (int) OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 4, uc8);
		OsaFL_Common_testEod(i);
		i = uc8[0] << 24 | uc8[1] << 16 | uc8[2] << 8 | uc8[3];
		if ((uc8[0] & 0x80) != 0) {
			i -= 0x40000000;
			i -= 0x40000000;
			i -= 0x40000000;
			i -= 0x40000000;
		}
		OsaFL_Common_putBuf(wp, i);
		goto fin;
	}
	OsaFL_Common_unknownFunc(vw);
fin:
	return rvp;
}


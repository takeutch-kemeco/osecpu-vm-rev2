#include "filtlib.h"

// UInt4[]に"05e200"のヘッダを付けるフィルタ.

typedef UInt4 OutTyp;
#define OutBufSiz		256

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
} Work;

static OutTyp dummyOutDat = 0;

void *OsaFL_hedSig00(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_hedSig00;
		wp->cw.drvnam = "OsaFL_hedSig00";
		wp->cw.customFlags = 0;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "UInt4"; // InTyp
		wp->cw.info1 = "UInt4"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		if (wp->cw.sk0flg == 0) {
			static OutTyp sig[] = { 0x0, 0x5, 0xe, 0x2, 0x0, 0x0 };
			for (i = 0; i < sizeof sig / sizeof (OutTyp); i++)
				wp->outBuf[i] = sig[i];
			wp->cw.obp1 = sizeof sig / sizeof (OutTyp);
			wp->cw.sk0flg = 1;
			goto fin;
		}
		i = (int) OsaFL_drvfnc(wp->cw.source, OsaFL_GET, OutBufSiz, wp->outBuf);
		OsaFL_Common_testEod(i);
		wp->cw.obp1 = i;
		goto fin;
	}
	OsaFL_Common_unknownFunc(vw);
fin:
	return rvp;
}


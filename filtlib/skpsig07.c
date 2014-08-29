#include "filtlib.h"

// Int32[]から先頭の"05e20f07"を取り除くだけのフィルタ.

typedef Int32 OutTyp;
#define OutBufSiz		256

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
} Work;

static OutTyp dummyOutDat = 0;

void *OsaFL_skpSig07(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_skpSig07;
		wp->cw.drvnam = "OsaFL_skpSig07";
		wp->cw.customFlags = 0;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "Int32"; // InTyp
		wp->cw.info1 = "Int32"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		i = (int) OsaFL_drvfnc(wp->cw.source, OsaFL_GET, OutBufSiz, wp->outBuf);
		OsaFL_Common_testEod(i);
		wp->cw.obp1 = i;
		if (wp->cw.sk0flg == 0) {
			static OutTyp sig[] = { 0x05e20f07 };
			if (i >= sizeof sig / sizeof (OutTyp)) {
				for (i = 0; i < sizeof sig / sizeof (OutTyp); i++)
					if (wp->outBuf[i] != sig[i]) goto skip;
				wp->cw.sk0flg = 1;
			}
skip:
			if (wp->cw.sk0flg == 0)
				OsaFL_Common_error(vw, "OsaFL_FLB: signature error.");
			wp->cw.obp += sizeof sig / sizeof (OutTyp);
		}
		wp->cw.sk0flg = 1;
		goto fin;
	}
	OsaFL_Common_unknownFunc(vw);
fin:
	return rvp;
}


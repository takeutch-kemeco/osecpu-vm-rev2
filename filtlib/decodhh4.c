#include "filtlib.h"

// UInt4->OsaFL_DecodedDataにするフィルタ.

typedef OsaFL_DecodedData OutTyp;
#define OutBufSiz		1

typedef struct _Work {
	// 標準メンバ.
	OsaFL_CommonWork cw;
	OutTyp outBuf[OutBufSiz];

	// 追加メンバ.
} Work;

static OutTyp dummyOutDat;

void *OsaFL_decodHh4(void *vw, int fnc, int i, void *vp)
{
	Work *wp = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		dummyOutDat.value[0] = 0;
		dummyOutDat.value[1] = 0;
		dummyOutDat.sourceLength[0] = 0;
		dummyOutDat.sourceLength[1] = 0;
		wp = malloc(sizeof (Work));
		wp->cw.drvfnc = &OsaFL_decodHh4;
		wp->cw.drvnam = "OsaFL_decodHh4";
		wp->cw.customFlags = 0;
		wp->cw.getUint = sizeof (OutTyp);
		wp->cw.connectable = "UInt4"; // InTyp
		wp->cw.info1 = "OsaFL_decodHh4"; // OutTyp
		wp->cw.obp2 = OutBufSiz;
		rvp = OsaFL_drvfnc(wp, OsaFL_NEW, i, vp);
		goto fin;
	}
	if (fnc == OsaFL_FLB) {	// fill-buffer.
		UInt4 ui4;
		int ph0 = -1, ph1 = 0;
		OutTyp *p = wp->outBuf;
		i = (int) OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 1, &ui4);
		OsaFL_Common_testEod(i);
		p->value[0] = 0;
		p->sourceLength[0] = 0;
		p->sourceLength[1] = 0;
		rvp = (void *) 1;
		goto enterLoop;

		for (;;) {
			OsaFL_drvfnc(wp->cw.source, OsaFL_GET, 1, &ui4);
enterLoop:
			if (p->sourceLength[0] >= 64) {
				OsaFL_Common_error(vw, "OsaFL_FLB: too long hh4.");
				goto skip;
			}
			p->source[p->sourceLength[0]++] = ui4;

			if (ph0 >= 0) {
				p->value[0] |= ui4 << ph0;
				ph0 -= 4;
				if (ph0 >= 0) continue;
test_ph1:
				if (ph1 == 0) break;
				ph1--;
				ph0 = p->value[0] * 4;
				p->value[0] = 0;
				if (ph1 == 0)
					p->sourceLength[1] = ph0;
				ph0 -= 4;
				if (ph0 < 0) break;
				continue;
			}
			if (ui4 == 0xf) {
				if (ph1 == 0) continue;
				OsaFL_Common_error(vw, "OsaFL_FLB: hh4 format error.");
				goto skip;
			}
			if (ui4 <= 0x6) {
				p->sourceLength[1] = 3;
				p->value[0] = ui4;
				goto test_ph1;
			}
			if (ui4 == 0x7) {
				ph1++;
				continue;
			}
			if (ui4 <= 0xb) {
				p->sourceLength[1] = 6;
				p->value[0] = (ui4 << 4) & 0x30;
				ph0 = 0;
				continue;
			} 
			if (ui4 <= 0xd) {
				p->sourceLength[1] = 9;
				p->value[0] = (ui4 << 8) & 0x100;
				ph0 = 4;
				continue;
			}
			p->sourceLength[1] = 12;
			ph0 = 8;
		//	continue;
		}
skip:
		if (wp->cw.errCode != 0) {
			rvp = (void *) 0;
			p->value[0] = 0;
		}
		p->value[1] = p->value[0];
		i = 1 << (p->sourceLength[1] - 1);
		if ((p->value[0] & i) != 0) {
			p->value[1] -= i;
			p->value[1] -= i;
		}
		wp->cw.obp1 = 1;
		goto fin;
	}
	OsaFL_Common_unknownFunc(vw);
fin:
	return rvp;
}

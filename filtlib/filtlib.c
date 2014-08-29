#include "filtlib.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void *OsaFL_drvfnc(void *vw, int fnc, int i, void *vp)
{
	OsaFL_CommonWork *cw = vw;
	void *rvp = (void *) 0;
	if (fnc == OsaFL_NEW) {
		if (cw == NULL) {
			fprintf(stderr, "OsaFL_drvfnc: OsaFL_NEW is not supported.\n");
			exit(1);
		}
		cw->errCode = 0;
		cw->source = NULL;
		OsaFL_drvfnc(cw, OsaFL_MOD, i, NULL);
		if (vp != NULL)
			OsaFL_drvfnc(cw, OsaFL_CON, 0, vp);
		rvp = vw;
		goto fin;
	}
	if (cw == NULL) {
		fprintf(stderr, "OsaFL_drvfnc: vw == NULL.\n");
		exit(1);
	}
	if (fnc == OsaFL_DEL) {
		if ((cw->customFlags & 0x80) != 0) {
			rvp = (*(cw->drvfnc))(vw, fnc, i, vp);
			goto fin;
		}
		if (cw->source != NULL)
			(*(cw->drvfnc))(vw, OsaFL_DIS, 0, NULL);
		free(vw);
		goto fin;
	}
	if (fnc == OsaFL_CON) {
		if (((cw->customFlags & 0x40) != 0 && i != -1) || (i != 0 && i != -1)) {
			rvp = (*(cw->drvfnc))(vw, fnc, i, vp);
			goto fin;
		}
		if (cw->errCode == 0) {
			if (cw->source != NULL)
				OsaFL_Common_error(vw, "OsaFL_CON: double connect error.");
			if (vp == NULL)
				OsaFL_Common_error(vw, "OsaFL_CON: vp == NULL.");
		}
		if (cw->errCode == 0) {
			if (strcmp(cw->connectable, OsaFL_drvfnc(vp, OsaFL_INF, 1, NULL)) != 0) {
				fprintf(stderr, "%s: OsaFL_CON: connectable mismatch.\n", cw->drvnam);
				exit(1);
			}
			cw->source = vp;
			OsaFL_drvfnc(vw, OsaFL_SK0, 0, NULL);
		}
		goto fin;
	}
	if (fnc == OsaFL_DIS) {
		if ((cw->customFlags & 0x20) != 0 && i != -1) {
			rvp = (*(cw->drvfnc))(vw, fnc, i, vp);
			goto fin;
		}
		cw->source = NULL;
		goto fin;
	}
	if (fnc == OsaFL_MOD) {
		if (cw != NULL)
			cw->mode = i;
		goto fin;
	}
	if (fnc == OsaFL_GET) {
		char *cp = vp;
		int c = 0, j;
		if ((cw->customFlags & 0x10) != 0) {
			rvp = (*(cw->drvfnc))(vw, fnc, i, vp);
			cw->pos += (int) rvp;
			goto fin;
		}
		if (cw->source == NULL)
			OsaFL_Common_error(vw, "OsaFL_GET: no source.");
		if (cw->errCode != 0) {
			(*(cw->drvfnc))(vw, 99, 0, vp);	// put(dummy).
			goto fin;
		}
		if (i == -1 && vp == NULL) {
			for (;;) {
				if (cw->obp >= cw->obp1) {
					cw->obp = cw->obp1 = 0;
					(*(cw->drvfnc))(vw, OsaFL_FLB, 0, NULL);
					if (cw->obp1 > cw->obp2) {
						fprintf(stderr, "%s: OsaFL_GET: output buffer overrun.\n", cw->drvnam);
						exit(1);
					} 
				}
				if (cw->obp >= cw->obp1) break;
				cw->obp++;
				c++;
			}
		} else {
			for (j = 0; j < i; j++) {
				if (cw->obp >= cw->obp1) {
					cw->obp = cw->obp1 = 0;
					(*(cw->drvfnc))(vw, OsaFL_FLB, 0, NULL);
					if (cw->obp1 > cw->obp2) {
						fprintf(stderr, "%s: OsaFL_GET: output buffer overrun.\n", cw->drvnam);
						exit(1);
					} 
				}
				if (cw->obp < cw->obp1) {
					(*(cw->drvfnc))(vw, 98, 0, cp);	// put from outBuf.
					c++;
				} else {
					do {
						(*(cw->drvfnc))(vw, 99, 0, cp);	// put(dummy).
						if (cp != NULL)
							cp += cw->getUint;
						j++;
					} while (j < i);
				}
				if (cp != NULL)
					cp += cw->getUint;
			}
		}
		rvp = (void *) c;
		cw->pos += c;
		goto fin;
	}
	if (fnc == OsaFL_SEK) {
		if ((cw->customFlags & 0x08) != 0) {
			rvp = (*(cw->drvfnc))(vw, fnc, i, vp);
			goto fin;
		}
		if (cw->source == NULL)
			OsaFL_Common_error(vw, "OsaFL_SEK: no source.");
		if (cw->errCode != 0) goto fin;
		if (i == -1) {
			rvp = (void *) cw->pos;
			goto fin;
		}
		OsaFL_drvfnc(vw, OsaFL_SK0, 0, NULL);
		if (i > 0)
			rvp = OsaFL_drvfnc(vw, OsaFL_GET, i, NULL);
		cw->pos = (int) rvp;
		goto fin;
	}
	if (fnc == OsaFL_INF) {
		if ((cw->customFlags & 0x04) != 0) {
			rvp = (*(cw->drvfnc))(vw, fnc, i, vp);
			goto fin;
		}
		if (i == 0)
			rvp = (void *) cw->errCode;
		if (i == 1)
			rvp = (void *) cw->info1;
		goto fin;
	}
	if (fnc == OsaFL_SK0) {
		if ((cw->customFlags & 0x02) != 0 && i != -1) {
			rvp = (*(cw->drvfnc))(vw, fnc, i, vp);
			goto fin;
		}
		if (cw->errCode != 0) goto fin;
		if (cw->source == NULL)
			OsaFL_Common_error(vw, "OsaFL_SK0: no source.");
		cw->pos = 0;
		cw->obp = cw->obp1 = 0;
		cw->sk0flg = 0;
		OsaFL_drvfnc(cw->source, OsaFL_SK0, 0, NULL);
		goto fin;
	}
	rvp = (*(cw->drvfnc))(vw, fnc, i, vp);
fin:
	return rvp;
}

void OsaFL_Common_error(void *vw, const char *fmt, ...)
{
	OsaFL_CommonWork *cw = vw;
	va_list ap;

	if ((cw->mode & 1) != 0) {
		va_start(ap, fmt);
		fprintf(stderr, "%s: ", cw->drvnam);
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, "\n");
		va_end(ap);
		exit(1);
	}
	cw->errCode = 1;
	return;
}


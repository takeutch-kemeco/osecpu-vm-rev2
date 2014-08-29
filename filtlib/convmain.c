#include "filtlib.h"
#include <stdio.h>
#include <string.h>

void oseconvBkBs32(const unsigned char *infle, const unsigned char *outfile);
void oseconvBs32Bk(const unsigned char *infle, const unsigned char *outfile);
void oseconvBkBk(const unsigned char *infle, const unsigned char *outfile);

int main(int argc, const char **argv)
{
	int retCode = 0;
	if (argc < 4) goto err;
	if (strcmp(argv[1], "bkbs32") == 0) { oseconvBkBs32(argv[2], argv[3]); goto fin; }
	if (strcmp(argv[1], "bs32bk") == 0) { oseconvBs32Bk(argv[2], argv[3]); goto fin; }
	if (strcmp(argv[1], "bkbk") == 0)   { oseconvBkBk  (argv[2], argv[3]); goto fin; }
err:
	fprintf(stderr, "usage>oseconv tool infile outfile\n"
		"\ttool = { bkbs32, bs32bk, bkbk }\n");
	retCode = 1;
fin:
	return retCode;
}

void oseconvSub(const unsigned char *infile, const unsigned char *outfile, void **filterFuncs)
{
	int i, j, mod = 1, filters;
	void *filterWorks[99];
	void *charFile = OsaFL_charFile(NULL, OsaFL_NEW, mod, (void *) infile);
	filterWorks[0] = charFile;
	for (i = 1; filterFuncs[i] != NULL; i++) {
		void *(*drvfnc)(void *, int, int, void *);
		drvfnc = filterFuncs[i];
		filterWorks[i] = (*drvfnc)(NULL, OsaFL_NEW, mod, filterWorks[i - 1]);
	}
	filters = i;
	void *last = filterWorks[i - 1];
	OsaFL_drvfnc(last, OsaFL_CON, 1, (void *) outfile);
	j = (int) OsaFL_drvfnc(last, OsaFL_GET, -1, NULL);
	i = (int) OsaFL_drvfnc(last, OsaFL_INF, 0, NULL);
	printf("output-size=%d, errCode=%d\n", j, i);
	return;
}

void oseconvBkBs32(const unsigned char *infile, const unsigned char *outfile)
{
	static void *filterFuncs[] = {
		OsaFL_charFile,
		OsaFL_uInt4,
		OsaFL_skpSig00,
		OsaFL_decodHh4,
		OsaFL_cnvBs32,
		OsaFL_hedSig07,
		OsaFL_uInt8b,
		OsaFL_writFile,
		NULL
	};
	oseconvSub(infile, outfile, filterFuncs);
	return;
}

void oseconvBs32Bk(const unsigned char *infile, const unsigned char *outfile)
{
	static void *filterFuncs[] = {
		OsaFL_charFile,
		OsaFL_int32,
		OsaFL_skpSig07,
		OsaFL_cnvBk,
		OsaFL_hedSig00,
		OsaFL_uInt8a,
		OsaFL_writFile,
		NULL
	};
	oseconvSub(infile, outfile, filterFuncs);
	return;
}

void oseconvBkBk(const unsigned char *infile, const unsigned char *outfile)
{
	static void *filterFuncs[] = {
		OsaFL_charFile,
		OsaFL_uInt4,
		OsaFL_skpSig00,
		OsaFL_decodHh4,
		OsaFL_cnvBs32,
		OsaFL_cnvBk,
		OsaFL_hedSig00,
		OsaFL_uInt8a,
		OsaFL_writFile,
		NULL
	};
	oseconvSub(infile, outfile, filterFuncs);
	return;
}



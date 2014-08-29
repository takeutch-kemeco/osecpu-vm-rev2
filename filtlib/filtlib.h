// C++で書いたほうがすっきりかけるのは分かっているんだけど、そうなるとCでは動かなくなってしまうので・・・.

#define OsaFL_NEW	0	// void *drvfnc(NULL, OsaFL_NEW, 0, NULL);
#define OsaFL_DEL	1	// void drvfnc(w, OsaFL_DEL, 0, NULL);
#define OsaFL_CON	2	// void drvfnc(w, OsaFL_CON, 0, p);
#define OsaFL_DIS	3	// void drvfnc(w, OsaFL_DIS, 0, NULL);
#define OsaFL_MOD	4	// void drvfnc(w, OsaFL_MOD, flags, NULL);
#define OsaFL_GET	5	// int drvfnc(w, OsaFL_GET, n, p);
#define OsaFL_SEK	6	// int drvfnc(w, OsaFL_SEK, i, NULL);
#define OsaFL_INF	7	// const char *drvfnc(w, OsaFL_INF, 0/1, NULL);
// NEWでvpをNULL以外にすると、CONも実行される.
// NEWでMODも設定される.
// OsaFL_INF-0: errCode
// OsaFL_INF-1: GET-type
#define OsaFL_FLB	8	// int drvfnc(w, OsaFL_FLB, 0, NULL);
#define OsaFL_SK0	9	// int drvfnc(w, OsaFL_SK0, 0, NULL);

void *OsaFL_drvfnc(void *vw, int fnc, int i, void *vp);

void *OsaFL_charFile(void *vw, int fnc, int i, void *vp);	// file->UInt8: バイナリも読める.
void *OsaFL_csHex32 (void *vw, int fnc, int i, void *vp);	// UInt8->Int32: コンマ区切りの16進数テキストファイルをバイナリにする. コメントもOK.
void *OsaFL_uInt4   (void *vw, int fnc, int i, void *vp);	// UInt8->UInt4: ビッグエンディアンで変換する.
void *OsaFL_skpSig00(void *vw, int fnc, int i, void *vp);	// UInt4->UInt4: "05e200"を読み飛ばしてくれる.
void *OsaFL_decodHh4(void *vw, int fnc, int i, void *vp);	// UInt4->DecodedData: hh4デコード.
void *OsaFL_cnvBs32 (void *vw, int fnc, int i, void *vp);	// DecodedData->Int32: backend形式からbs32形式を生成.
void *OsaFL_hedSig07(void *vw, int fnc, int i, void *vp);	// Int32->Int32: "05e20f07"を付与.
void *OsaFL_uInt8b  (void *vw, int fnc, int i, void *vp);	// Int32->UInt8: ビッグエンディアンで変換する.
void *OsaFL_writFile(void *vw, int fnc, int i, void *vp);	// UInt8->file: ファイルに出力する.
void *OsaFL_int32   (void *vw, int fnc, int i, void *vp);	// UInt8->Int32: ビッグエンディアンで変換する.
void *OsaFL_skpSig07(void *vw, int fnc, int i, void *vp);	// UInt32->UInt32: "05e20f07"を読み飛ばしてくれる.
void *OsaFL_cnvBk   (void *vw, int fnc, int i, void *vp);	// Int32->UInt4: bs32形式からbackend形式を生成.
void *OsaFL_hedSig00(void *vw, int fnc, int i, void *vp);	// UInt4->UInt4: "05e200"を付与.

void *OsaFL_uInt8a  (void *vw, int fnc, int i, void *vp);	// UInt4->UInt8: ビッグエンディアンで変換する.

typedef struct _OsaFL_CommonWork {
	void *(*drvfnc)(void *, int, int, void *);
	int errCode, mode, customFlags, getUint, pos;
	const char *drvnam, *info1, *connectable;
	void *source;
	int obp, obp1, obp2;
	char sk0flg;
} OsaFL_CommonWork;

typedef int Int32;
typedef unsigned char UInt8;
typedef unsigned char UInt4;

typedef struct _OsaFL_DecodedData {
	Int32 value[2];
	char source[64]; // 256bitまで対応可能.
	int sourceLength[2];
} OsaFL_DecodedData;

void OsaFL_Common_unknownFunc(void *vw);
void OsaFL_Common_error(void *vw, const char *fmt, ...);

#define OsaFL_Common_putBuf(wp, dat)	wp->outBuf[wp->cw.obp1++] = dat;
#define OsaFL_Common_testEod(i)			if (i <= 0) goto fin;
#define OsaFL_Common_unknownFunc(vw)	\
	if (fnc == 98) { OutTyp *p = vp; if (p != NULL) { *p = wp->outBuf[wp->cw.obp++]; } goto fin; } \
	if (fnc == 99) { OutTyp *p = vp; if (p != NULL) { *p = dummyOutDat; } goto fin; } \
	OsaFL_Common_error(vw, "unknown function code.");

#include <stdlib.h>


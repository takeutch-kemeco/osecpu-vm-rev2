#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int Int32; // 32bit以上であればよい（64bit以上でもよい）.

typedef struct _PReg {
	void *p;
} PReg;

typedef struct _Label {
	int typ;
} Label;

typedef struct _Defines {
	// ラベルや構造体の定義.
	int maxLabels;
	Label label[4096];
} Defines;

typedef struct _HH4Reader {
	const unsigned char *p, *p1;
	int half, len, errorCode;
} HH4Reader;

typedef struct _Jitc {
	Defines *defines;
	const Int32 *src, *src1;
	Int32 *dst, *dst1;
	Int32 dr[4];
	int errorCode;
	HH4Reader *hh4r;
	const unsigned char *hh4src1;
	Int32 *hh4dst, *hh4dst1;
} Jitc;

typedef struct _VM {
	Int32 r[0x40];
	int bit[0x40];
	PReg p[0x40];
	const Int32 *ip, *ip1; /* instruction-pointer, program-counter */
	Int32 dr[4];
	const Defines *defines;
	int errorCode;
} VM;

// jitc.c : JITコンパイラ関係
int instrLengthSimple(Int32 opecode); // 簡単な命令について命令長を返す.
int instrLength(const Int32 *src, const Int32 *src1); // 命令長を返す.
int jitcStep(Jitc *jitc); // OSECPU命令を一つだけ検証する. エラーがなければ0を返す.
int jitcAll(Jitc *jitc);
void jitcSetRetCode(int *pRC, int value);

#define JITC_BAD_OPECODE		1
#define JITC_BAD_BITS			2
#define JITC_BAD_RXX			3
#define JITC_BAD_PXX			4
#define JITC_BAD_CND			5
#define JITC_BAD_LABEL			6
#define JITC_SRC_OVERRUN		7
#define JITC_DST_OVERRUN		8
#define JITC_HH4_DST_OVERRUN	9
#define JITC_HH4_BITLENGTH_OVER	10

// exec.c : VMインタプリタ関係
int execStep(VM *r); // 検証済みのOSECPU命令を一つだけ実行する.
int execAll(VM *vm);

#define EXEC_BAD_BITS			1
#define EXEC_BITS_RANGE_OVER	2
#define EXEC_ABORT_OPECODE_M1	0xffff

// JITコンパイラとVMインタプリタの双方に必要な関数は、jitc.cのほうにおく.

// hh4.c : hh4関係
void hh4Init(HH4Reader *hh4r, void *p, int half, void *p1);
int hh4Get4bit(HH4Reader *hh4r);
Int32 hh4GetUnsigned(HH4Reader *hh4r);
Int32 hh4GetSigned(HH4Reader *hh4r);
Int32 *hh4Decode(Jitc *jitc);
unsigned char *hh4StrToBin(unsigned char *src, unsigned char *src1, unsigned char *dst, unsigned char *dst1);




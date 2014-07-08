#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int Int32; // 32bit以上であればよい（64bit以上でもよい）.

#define DEFINES_MAXLABELS	4096

typedef struct _PReg {
	void *p;
} PReg;

typedef struct _Label {
	int typ, opt;
	Int32 *dst;
} Label;

typedef struct _Defines {
	// ラベルや構造体の定義.
	Label label[DEFINES_MAXLABELS];
} Defines;

typedef struct _HH4Reader {
	const unsigned char *p, *p1;
	int half, len, errorCode;
} HH4Reader;

#define JITC_DSTLOG_SIZE	16

typedef struct _OsecpuJitc {
	int phase, dstLogIndex;
	Defines *defines;
	const Int32 *src, *src1;
	Int32 *dst, *dst1, *dstLog[JITC_DSTLOG_SIZE];
		// dstLog[]は命令を少しさかのぼりたいときに使う.
		//   たとえばdata(2E)が利用する.
	Int32 dr[4]; // Integer
	int errorCode;
	HH4Reader *hh4r;
	const unsigned char *hh4src1;
	Int32 *hh4dst, *hh4dst1;
} OsecpuJitc;

typedef struct _OsecpuVm {
	Int32 r[0x40]; // Integer
	int bit[0x40]; // Integer
	Int32 dr[4]; // Integer
	double f[0x40]; // Float
	int bitF[0x40]; // Float
	PReg p[0x40];
	const Int32 *ip, *ip1; /* instruction-pointer, program-counter */
	const Defines *defines;
	int errorCode;
} OsecpuVm;

// osecpu-vm.c
void osecpuInit(); // 初期化.
int instrLengthSimple(Int32 opecode); // 簡単な命令について命令長を返す.
void instrLengthSimpleInit();
void instrLengthSimpleInitTool(int *table, int ope0, int ope1); // 初期化支援.
int instrLength(const Int32 *src, const Int32 *src1); // 命令長を返す.
void jitcSetRetCode(int *pRC, int value);
int jitcStep(OsecpuJitc *jitc); // OSECPU命令を一つだけ検証する. エラーがなければ0を返す.
int jitcAll(OsecpuJitc *jitc);

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
#define JITC_BAD_FLIMM_MODE		11
#define JITC_BAD_FXX			12
#define JITC_LABEL_REDEFINED	13
#define JITC_BAD_LABEL_TYPE		14
#define JITC_LABEL_UNDEFINED	15

void jitcStep_checkBits32(int *pRC, int bits);
void jitcStep_checkRxx(int *pRC, int rxx);
void jitcStep_checkRxxNotR3F(int *pRC, int rxx);

int execStep(OsecpuVm *r); // 検証済みのOSECPU命令を一つだけ実行する.
int execAll(OsecpuVm *vm);

#define EXEC_BAD_BITS			1
#define EXEC_BITS_RANGE_OVER	2
#define EXEC_BAD_R2				3	// SBX, SHL, SARのr2が不適切.
#define EXEC_DIVISION_BY_ZERO	4
#define EXEC_SRC_OVERRUN		5
#define EXEC_ABORT_OPECODE_M1	0xffff

void execStep_checkBitsRange(Int32 value, int bits, OsecpuVm *vm);

// hh4.c : hh4関係.
void hh4Init(HH4Reader *hh4r, void *p, int half, void *p1);
int hh4Get4bit(HH4Reader *hh4r);
Int32 hh4GetUnsigned(HH4Reader *hh4r);
Int32 hh4GetSigned(HH4Reader *hh4r);
Int32 *hh4Decode(OsecpuJitc *jitc);
unsigned char *hh4StrToBin(unsigned char *src, unsigned char *src1, unsigned char *dst, unsigned char *dst1);

// integer.c : 整数命令
void osecpuInitInteger();
int instrLengthInteger(const Int32 *src, const Int32 *src1);
Int32 *hh4DecodeInteger(OsecpuJitc *jitc, Int32 opecode);
int jitcStepInteger(OsecpuJitc *jitc);
void execStepInteger(OsecpuVm *vm);

// pointer.c : ポインタ命令
void osecpuInitPointer();
int instrLengthPointer(const Int32 *src, const Int32 *src1);
Int32 *hh4DecodePointer(OsecpuJitc *jitc, Int32 opecode);
int jitcStepPointer(OsecpuJitc *jitc);
void execStepPointer(OsecpuVm *vm);

// float.c : 浮動小数点命令
void osecpuInitFloat();
int instrLengthFloat(const Int32 *src, const Int32 *src1);
Int32 *hh4DecodeFloat(OsecpuJitc *jitc, Int32 opecode);
int jitcStepFloat(OsecpuJitc *jitc);
void execStepFloat(OsecpuVm *vm);

// extend.c : 拡張命令関係.
void osecpuInitExtend();
int instrLengthExtend(const Int32 *src, const Int32 *src1);
Int32 *hh4DecodeExtend(OsecpuJitc *jitc, Int32 opecode);
int jitcStepExtend(OsecpuJitc *jitc);
void execStepExtend(OsecpuVm *vm);


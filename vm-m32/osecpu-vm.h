#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int Int32; // 32bit以上であればよい（64bit以上でもよい）.

#define DEFINES_MAXLABELS	4096

typedef struct _PReg {
	unsigned char *p;
	int typ;
} PReg;

typedef struct _Label {
	int typ, opt;
	Int32 *dst;
} Label;

typedef struct _Defines {
	// ラベルや構造体の定義.
	Label label[DEFINES_MAXLABELS];
} Defines;

#define JITC_DSTLOG_SIZE	16
#define PREFIX2F_SIZE		16

typedef struct _OsecpuJitc {
	int phase, dstLogIndex;
	Defines *defines;
	const Int32 *src, *src1;
	Int32 srcBuffer[16];
	Int32 *dst, *dst1, *dstLog[JITC_DSTLOG_SIZE];
		// dstLog[]は命令を少しさかのぼりたいときに使う.
		//   たとえばdata(2E)が利用する.
	int errorCode, instrLength;
	Int32 dr[4]; // Other
	Int32 *ope04; // Integer
	unsigned char prefix2f[PREFIX2F_SIZE];
} OsecpuJitc;

typedef struct _OsecpuVm {
	Int32 r[0x40]; // Integer
	Int32 dr[4]; // Other
	double f[0x40]; // Float
	PReg p[0x40]; // Pointer
	const Int32 *ip, *ip1; /* instruction-pointer, program-counter */
	const Defines *defines;
	int errorCode;
	unsigned char prefix2f[PREFIX2F_SIZE];
	char *stack0, *stack1, *stackTop;
} OsecpuVm;

// osecpu-vm.c

void definesInit(Defines *def);

void jitcInitDstLogSetPhase(OsecpuJitc *jitc, int phase);
void jitcSetRetCode(int *pRC, int value);
void jitcSetSrcBufferSimple(OsecpuJitc *jitc, int length);
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
#define JITC_BAD_TYPE			16
#define JITC_BAD_PREFIX			17
#define JITC_UNSUPPORTED		18
#define JITC_BAD_ENTER			19

int execStep(OsecpuVm *r); // 検証済みのOSECPU命令を一つだけ実行する.
int execAll(OsecpuVm *vm);

#define EXEC_BAD_BITS			 1+256
#define EXEC_BITS_RANGE_OVER	 2+256
#define EXEC_BAD_R2				 3+256	// SBX, SHL, SARのr2が不適切.
#define EXEC_DIVISION_BY_ZERO	 4+256
#define EXEC_SRC_OVERRUN		 5+256
#define EXEC_TYP_MISMATCH		 6+256
#define EXEC_PTR_RANGE_OVER		 7+256
#define EXEC_BAD_ACCESS			 8+256
#define EXEC_API_ERROR			 9+256
#define EXEC_STACK_ALLOC_ERROR	10+256
#define EXEC_STACK_FREE_ERROR	11+256
#define EXEC_EXIT				12+256
#define EXEC_MALLOC_ERROR		13+256
#define EXEC_MFREE_ERROR		14+256
#define EXEC_ABORT_OPECODE_M1	0xffff

#define EXEC_CMA_FLAG_SEEK		1
#define EXEC_CMA_FLAG_READ		2
#define EXEC_CMA_FLAG_WRITE		4
#define EXEC_CMA_FLAG_EXEC		8

#define PTR_TYP_NULL			0
#define PTR_TYP_CODE			-1
#define PTR_TYP_NATIVECODE		-2
#define PTR_TYP_INVALID			-3	// 不定値状態.

unsigned char *hh4StrToBin(unsigned char *src, unsigned char *src1, unsigned char *dst, unsigned char *dst1);

// integer.c : 整数命令.
void jitcInitInteger(OsecpuJitc *jitc);
int jitcStepInteger(OsecpuJitc *jitc);
int jitcAfterStepInteger(OsecpuJitc *jitc);
void execStepInteger(OsecpuVm *vm);

int getTypBitInteger(int typ);
void getTypInfoInteger(int typ, int *typSize0, int *typSize1, int *typSign);
void jitcStep_checkBits32(int *pRC, int bits);
Int32 execStep_signBitExtend(Int32 value, int bit);
Int32 execStep_getRxx(OsecpuVm *vm, int r, int bit);

// pointer.c : ポインタ命令.
void jitcInitPointer(OsecpuJitc *jitc);
int jitcStepPointer(OsecpuJitc *jitc);
int jitcAfterStepPointer(OsecpuJitc *jitc);
void execStepPointer(OsecpuVm *vm);

void getTypSize(int typ, int *typSize0, int *typSize1, int *typSign); // これは直すべき.
void execStep_checkMemAccess(OsecpuVm *vm, int p, int typ, int flag);
void execStep_plimm(OsecpuVm *vm, int p, int i);

// float.c : 浮動小数点命令
void jitcInitFloat(OsecpuJitc *jitc);
int jitcStepFloat(OsecpuJitc *jitc);
int jitcAfterStepFloat(OsecpuJitc *jitc);
void execStepFloat(OsecpuVm *vm);

// other.c : 雑命令.
void jitcInitOther(OsecpuJitc *jitc);
int jitcStepOther(OsecpuJitc *jitc);
int jitcAfterStepOther(OsecpuJitc *jitc);
void execStepOther(OsecpuVm *vm);
int osecpuVmStackInit(OsecpuVm *vm, int stackSize);

// extend.c : 拡張命令関係.
void jitcInitExtend(OsecpuJitc *jitc);
int jitcStepExtend(OsecpuJitc *jitc);
int jitcAfterStepExtend(OsecpuJitc *jitc);
void execStepExtend(OsecpuVm *vm);


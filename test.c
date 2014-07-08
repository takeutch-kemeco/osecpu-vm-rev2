#include "osecpu-vm.h"

void apiInit(OsecpuVm *vm);

#define BUFFER_SIZE		256

int main(int argc, const char **argv)
{
	Defines defs;
	OsecpuJitc jitc;
	OsecpuVm vm;
	unsigned char hh4src[BUFFER_SIZE], *hh4src1;
	Int32 i32buf[BUFFER_SIZE], j32buf[BUFFER_SIZE];
	int rc;
	jitc.defines = &defs;
	vm.defines = &defs;

	// hh4StrToBin()を使って、hh4src[]に16進数を4bit単位で書き込む. 
	hh4src1 = hh4StrToBin(
		"2 d85 0 a0"					// LIMM32(R00, -123);
		"c40 0 1 0 c40"					// FLIMM64(F00, 1);
		"c40 1 40490fdb 1 c40"			// FLIMM64(F01, (float) PI);
		"c40 2 400921fb54442d18 2 c40"  // FLIMM64(F02, PI);

		// R02 = 1 + 2 + 3 + 4 + ... + 10
		"2 0 1 a0"						// LIMM32(R01, 0);
		"2 0 2 a0"						// LIMM32(R02, 0);
		"1 0 0"							// LB0(0);
		"94 2 1 2 a0"					// ADD32(R02, R02, R01);
		"2 1 bf a0"						// LIMM32(R3F, 1);
		"94 1 bf 1 a0"					// ADD32(R01, R3F, R01);
		"2 8b bf a0"					// LIMM32(R3F, 11);
		"a1 1 bf a0 bf a0"				// CMPNE32_32(R3F, R01, R3F);
		"4 bf"							// CND(R3F);
		"3 0 bf"						// PLIMM(P3F, 0);

#if 0
		// F03 = 4 * (1 - 1/3 + 1/5 - 1/7 + ...)
		// テストなので速度とかを気にせず、極力Fxxを使ってみる.
		"c40 0 0 3 c40"					// FLIMM64(F03, 0);
		"c40 0 84 4 c40"				// FLIMM64(F04, 4);
		"c40 0 1 5 c40"					// FLIMM64(F05, 1);
		"1 1 0"							// LB0(1);
		"c53 4 5 6 c40"					// FDIV64(F06, F04, F05);
		"c50 3 6 3 c40"					// FADD64(F03, F06, F03);
		"c40 0 bf bf c40"				// FLIMM64(F3F, -1);
		"c52 4 bf 4 c40"				// FMUL64(F04, F3F, F04);
		"c40 0 2 bf c40"				// FLIMM64(F3F, 2);
		"c50 5 bf 5 c40"				// FADD64(F05, F3F, F05);
		"c40 0 767fffff bf c40"			// FLIMM64(F3F, 0x7fffff);
		"c49 5 bf c40 bf a0"			// FCMPNE32_64(R3F, F05, F3F);
		"4 bf"							// CND(R3F);
		"3 1 bf"						// PLIMM(P3F, 1);
#endif

#if 0
		"2 0 3 0"						// LIMM0(R03, 0);
		"90 3 3 3 a0"					// CP32(R03, R03); // ここでエラーコード1になる.
#endif

#if 1
		"1 2 1"							// LB1(2);
		"ae 2 84 01 23 45 67"			// data(UINT8, 4, 0x01, 0x23, 0x45, 0x67);
		"3 2 1"							// PLIMM(P01, 2);
		"2 84 3 a0"						// LIMM32(R03, 4);
		"2 2 bf a0"						// LIMM32(R3F, 2);
		"8e bf a0 1 2 1"				// PADD32(P01, UINT8, P01, R3F);
		"88 1 2 0 3 a0"					// LMEM32(P01, UINT8, 0, R03);
#endif
		"2 84 b0 a0"					// LIMM32(R30, 4);
		"3 3 b0"						// PLIMM(P30, 3);
		"9e a8 bf"						// PCP(P3F, P28);
		"1 3 1"							// LB1(3);

		, NULL, hh4src, &hh4src[BUFFER_SIZE]
	);

	if (hh4src1 == NULL) {
		printf("hh4src1 == NULL\n");
		return 1;
	}
	printf("hh4src1 - hh4src = %d\n", hh4src1 - hh4src);

	// jitcAll()を使って、hh4src[]内のプログラムをj32buf[]に変換する.
	hh4ReaderInit(&jitc.hh4r, hh4src, 0, hh4src1, 0);
	jitc.dst  =  j32buf;
	jitc.dst1 = &j32buf[BUFFER_SIZE];
	rc = jitcAll(&jitc);
	printf("jitcAll()=%d\n", rc); // 0なら成功.
	if (rc != 0) return 1;
	*jitc.dst = -1; // デバッグ用の特殊opecode.
	vm.ip  = j32buf;
	vm.ip1 = jitc.dst;
	// この時点で、i32buf[]は破棄しても良い.

	apiInit(&vm);
	// execAll()を使って、j32buf[]内の中間コードを実行する.
	printf("execAll()=%d\n", execAll(&vm)); // 65535なら成功(EXEC_ABORT_OPECODE_M1).
	printf("R00=%d\n", vm.r[0x00]);
	printf("R01=%d\n", vm.r[0x01]);
	printf("R02=%d\n", vm.r[0x02]);
	printf("R03=%d\n", vm.r[0x03]);
	printf("F00=%f\n", vm.f[0x00]);
	printf("F01=%.15f\n", vm.f[0x01]);
	printf("F02=%.15f\n", vm.f[0x02]);
	printf("F03=%f\n", vm.f[0x03]);
	printf("F04=%f\n", vm.f[0x04]);

	return 0;
}


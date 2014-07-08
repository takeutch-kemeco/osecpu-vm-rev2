#include "osecpu-vm.h"

int main(int argc, const char **argv)
{
	Defines defs;
	OsecpuJitc jitc;
	OsecpuVm vm;
	HH4Reader hh4r;
	unsigned char hh4src[256], *hh4src1;
	Int32 i32buf[256], j32buf[256];
	definesInit(&defs);
	jitc.defines = &defs;
	vm.defines = &defs;
	jitc.hh4r = &hh4r;

	osecpuInit();

	// hh4StrToBin()を使って、hh4src[]に16進数を4bit単位で書き込む. 
	hh4src1 = 	hh4StrToBin(
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

					// F03 = 4 * (1 - 1/3 + 1/5 - 1/7 + ...)
					// テストなので速度とかを気にせず、極力Fxxを使ってみる.
					"c40 0 0 3 c40"					// FLIMM64(F03, 0);
					"c40 0 1 4 c40"					// FLIMM64(F04, 1);
					"1 1 0"							// LB0(1);
					"c40 0 84 bf c40"				// FLIMM64(F3F, 4);
					"c53 bf 4 5 c40"				// FDIV64(F05, F3F, F04);
					"c50 3 5 3 c40"					// FADD64(F03, F05, F03);
					"c40 0 2 bf c40"				// FLIMM64(F3F, 2);
					"c50 4 bf 4 c40"				// FADD64(F04, F3F, F04);
					"c40 0 84 bf c40"				// FLIMM64(F3F, 4);
					"c53 bf 4 5 c40"				// FDIV64(F05, F3F, F04);
					"c51 3 5 3 c40"					// FSUB64(F03, F05, F03);
					"c40 0 2 bf c40"				// FLIMM64(F3F, 2);
					"c50 4 bf 4 c40"				// FADD64(F04, F3F, F04);
					"c40 0 767ffffd bf c40"			// FLIMM64(F3F, 0x7ffffd);
					"c49 4 bf c40 bf a0"			// FCMPNE32_64(R3F, F04, F3F);
					"4 bf"							// CND(R3F);
					"3 1 bf",						// PLIMM(P3F, 1);
					NULL, hh4src, &hh4src[256]
				);

	if (hh4src1 == NULL) {
		printf("hh4src1 == NULL\n");
		return 1;
	}

	// hh4Decode()を使って、hh4src[]内のプログラムをi32buf[]に展開する.
	hh4Init(&hh4r, hh4src, 0, hh4src1);
	jitc.hh4dst  =  i32buf;
	jitc.hh4dst1 = &i32buf[256];
	jitc.src     =  i32buf;
	jitc.src1    = hh4Decode(&jitc);
	// この時点で、hh4src[]は破棄しても良い.

	// jitcAll()を使って、i32buf[]内のプログラムをj32buf[]に変換する(実際には無変換).
	jitc.dst  =  j32buf;
	jitc.dst1 = &j32buf[256];
	printf("jitcAll()=%d\n", jitcAll(&jitc)); // 7なら成功(JITC_SRC_OVERRUN).
	*jitc.dst = -1; // デバッグ用の特殊opecode.
	vm.ip  = j32buf;
	vm.ip1 = jitc.dst;
	// この時点で、i32buf[]は破棄しても良い.

	// execAll()を使って、j32buf[]内の中間コードを実行する.
	printf("execAll()=%d\n", execAll(&vm)); // 65535なら成功(EXEC_ABORT_OPECODE_M1).
	printf("R00=%d\n", vm.r[0x00]);
	printf("F00=%f\n", vm.f[0x00]);
	printf("F01=%.15f\n", vm.f[0x01]);
	printf("F02=%.15f\n", vm.f[0x02]);
	printf("R01=%d\n", vm.r[0x01]);
	printf("R02=%d\n", vm.r[0x02]);
	printf("F03=%f\n", vm.f[0x03]);
	printf("F04=%f\n", vm.f[0x04]);

	return 0;
}


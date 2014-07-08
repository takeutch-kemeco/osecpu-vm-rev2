#include "osecpu-vm.h"

int main(int argc, const char **argv)
{
	Defines defs;
	OsecpuJitc jitc;
	OsecpuVm vm;
	HH4Reader hh4r;
	unsigned char hh4src[64], *hh4src1;
	Int32 i32buf[64], j32buf[64];
	jitc.defines = &defs;
	vm.defines = &defs;
	jitc.hh4r = &hh4r;

	OsecpuInit();

	// hh4StrToBin()を使って、hh4src[]に16進数を4bit単位で書き込む. 
	hh4src1 = 	hh4StrToBin(
					"2 d85 0 a0"					// LIMM32(R00, -123);
					"c40 0 1 0 c40"					// FLIMM64(F00, 1;
					"c40 1 40490fdb 1 c40"			// FLIMM64(F01, (float) PI);
					"c40 2 400921fb54442d18 2 c40", // FLIMM64(F02, PI);
					NULL, hh4src, &hh4src[64]
				);

	// hh4Decode()を使って、hh4src[]内のプログラムをi32buf[]に展開する.
	hh4Init(&hh4r, hh4src, 0, hh4src1);
	jitc.hh4dst  =  i32buf;
	jitc.hh4dst1 = &i32buf[64];
	jitc.src     =  i32buf;
	jitc.src1    = hh4Decode(&jitc);
	// この時点で、hh4src[]は破棄しても良い.

	// jitcAll()を使って、i32buf[]内のプログラムをj32buf[]に変換する(実際には無変換).
	jitc.dst  =  j32buf;
	jitc.dst1 = &j32buf[64];
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
 
	return 0;
}


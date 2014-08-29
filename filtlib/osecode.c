#include "osecode.h"

unsigned char *osecpuCode_backendTable[OsecpuCode_BackendTableLength] = {
	"0",		// 0000: NOP();
	"2uu",		// 0001: LB(uimm, opt);
	"3suu",		// 0002: LIMM(imm, r, bit);
	"2uu",		// 0003: PLIMM(uimm, p);
	"1u",		// 0004: CND(r);
	"",			// 0005:
	"",			// 0006:
	"",			// 0007:
	"5uuuuu",	// 0008: LMEM(p,typ,0,r,bit);
	"5uuuuu",	// 0009: SMEM(r,bit,p,typ,0);
	"",			// 000a:
	"",			// 000b:
	"",			// 000c:
	"",			// 000d:
	"",			// 000e:
	"",			// 000f:
	"4uuuu",	// 0010: OR(r1,r2,r0,bit);
	"4uuuu",	// 0011: XOR(r1,r2,r0,bit);
	"4uuuu",	// 0012: AND(r1,r2,r0,bit);
	"4uuuu",	// 0013: SBX(r1,r2,r0,bit);
	"4uuuu",	// 0014: ADD(r1,r2,r0,bit);
	"4uuuu",	// 0015: SUB(r1,r2,r0,bit);
	"4uuuu",	// 0016: MUL(r1,r2,r0,bit);
	"",			// 0017:
	"4uuuu",	// 0018: SHL(r1,r2,r0,bit);
	"4uuuu",	// 0019: SAR(r1,r2,r0,bit);
	"4uuuu",	// 001a: DIV(r1,r2,r0,bit);
	"4uuuu",	// 001b: MOD(r1,r2,r0,bit);
	"",			// 001c:
	"",			// 001d:
	"2uu",		// 001e: PCP(p1, p0);
	"",			// 001f:
	"5uuuuu",	// 0020: CMPE(r1, r2, bit1, r0, bit0);
	"5uuuuu",	// 0021: CMPNE(r1, r2, bit1, r0, bit0);
	"5uuuuu",	// 0022: CMPL(r1, r2, bit1, r0, bit0);
	"5uuuuu",	// 0023: CMPGE(r1, r2, bit1, r0, bit0);
	"5uuuuu",	// 0024: CMPLE(r1, r2, bit1, r0, bit0);
	"5uuuuu",	// 0025: CMPG(r1, r2, bit1, r0, bit0);
	"5uuuuu",	// 0026: TSTZ(r1, r2, bit1, r0, bit0);
	"5uuuuu",	// 0027: TSTNZ(r1, r2, bit1, r0, bit0);
};


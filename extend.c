#include "osecpu-vm.h"

// C0-DF‚É–½—ß‚ðŠg’£‚·‚é‚½‚ß‚Ì‚à‚Ì.

void osecpuInitExtend()
{
	return;
}

int instrLengthExtend(const Int32 *src, const Int32 *src1)
// instrLengthSimpleInitTool()‚Å“o˜^‚µ‚Ä‚¢‚È‚¢‚à‚Ì‚¾‚¯‚É”½‰ž‚·‚ê‚Î‚æ‚¢.
{
	return 0;
}

Int32 *hh4DecodeExtend(OsecpuJitc *jitc, Int32 opecode)
// instrLengthSimpleInitTool()‚Å“o˜^‚µ‚Ä‚¢‚È‚¢‚à‚Ì‚¾‚¯‚É”½‰ž‚·‚ê‚Î‚æ‚¢.
{
	return jitc->dst;
}

int jitcStepExtend(OsecpuJitc *jitc)
{
	return -1;
}

void execStepExtend(OsecpuVm *vm)
{
	return;
}

#include "osecpu-vm.h"

void jitcStep_checkPxx(int *pRC, int pxx)
{
	if (!(0x00 <= pxx && pxx <= 0x3f))
		jitcSetRetCode(pRC, JITC_BAD_PXX);
	return;
}

#if 0

jitcStep:

	if (opecode == 0x03) { /* PLIMM(imm, Pxx); */
		i = ip[1];
		if (!(0 <= i && i <= jitc->defines->maxLabels))
			jitcSetRetCode(pRC, JITC_BAD_LABEL);
		else {
			if (jitc->defines->label[i].typ == 1)
				; /* OK */
			else if (jitc->defines->label[i].typ == 0 && ip[2] == 0x3f)
				; /* OK */
			else
				jitcSetRetCode(pRC, JITC_BAD_LABEL);
		}
		jitcStep_checkPxx(pRC, ip[2]);
		goto fin;
	}
#endif

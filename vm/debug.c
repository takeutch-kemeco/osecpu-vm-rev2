#include "osecpu-vm.h"

char *debugJitcReport(OsecpuJitc *jitc, char *msg)
{
	int i = jitc->errorCode;
	if (i == 0)
		sprintf(msg, "JITC: No errors.");
	else {
		char *s = "Internal error";
		if (i == JITC_BAD_OPECODE)		s = "Bad opecode";
		if (i == JITC_BAD_BITS)			s = "Bad bit";
		if (i == JITC_BAD_RXX)			s = "Bad Rxx";
 		if (i == JITC_BAD_PXX)			s = "Bad Pxx";
 		if (i == JITC_BAD_CND)			s = "Bad CND";
 		if (i == JITC_BAD_LABEL)		s = "Bad label";
		sprintf(msg, "JITC: %s. (DR0=%06d, op=0x%02x, ec=%03d)", s, jitc->dr[0], jitc->hh4Buffer[0], i);
	}
	return msg;
}

void debugCheckBreakPoint(OsecpuVm *vm);
void debugMonitor(OsecpuVm *vm);

void execStepDebug(OsecpuVm *vm)
{
	debugCheckBreakPoint(vm);
	if (vm->toDebugMonitor == 1)
		debugMonitor(vm);
	return;
}

void debugCheckBreakPoint(OsecpuVm *vm)
{
	int i = vm->debugBreakPointIndex;
	Int32 v = vm->debugBreakPointValue;
	char t = 0;
	if (0 <= i && i <= 0x3f) {
		if (vm->r[i] == v)
			t = 1;
	}
	if (i == 0x40) {
		if (vm->dr[i - 0x40] == v)
			t = 1;
	}
	if (i == 0x41) {
		if (vm->ip[0] == v)
			t = 1;
	}
	if (t != 0 && vm->toDebugMonitor == 0)
		vm->toDebugMonitor = 1;
	if (t == 0 && vm->toDebugMonitor == 2)
		vm->toDebugMonitor = 0;
	return;
}

void drv_flshWin(int sx, int sy, int x0, int y0);
extern int *vram, v_xsiz, v_ysiz;

void debugMonitor(OsecpuVm *vm)
{
	char cmdlin[1024], *p;
	int i, j;
	i = vm->errorCode;
	if (i > 0) {
		p = "Internal error";
		if (i == EXEC_BAD_BITS)			p = "Bad bit";
		if (i == EXEC_BITS_RANGE_OVER)	p = "Bit range over";
		if (i == EXEC_SRC_OVERRUN)		p = "Code terminated";
		if (i == EXEC_TYP_MISMATCH)		p = "Type mismatch";
		if (i == EXEC_PTR_RANGE_OVER)	p = "Pointer range over";
		if (i == EXEC_BAD_ACCESS)		p = "Bad access type (read, write, exec, seek...)";
		if (i == EXEC_API_ERROR)		p = "Api error";
		if (i == EXEC_STACK_ALLOC_ERROR)	p = "Stack alloc error";
		if (i == EXEC_STACK_FREE_ERROR)	p = "Stack free error";
		if (i == EXEC_EXIT)				p = "Exit";
		if (i == EXEC_MALLOC_ERROR)		p = "Heap alloc error";
		if (i == EXEC_MFREE_ERROR)		p = "Heap free error";
		if (i == EXEC_EXECSTEP_OVER)	p = "Exec step over";
		if (i == EXEC_ALLOCLIMIT_OVER)	p = "Alloc limit over";
		printf("dbg: VM: %s. (ec=%03d)\n", p, i);
	}
	i = 0;
	if (vm->ip < vm->ip1)
		i = vm->ip[0];
	p = "unknown";
	static char *table[] = {
		"NOP", "LB", "LIMM", "PLIMM", "CND", 0, 0, 0, "LMEM", "SMEM", "PLMEM", "PSMEM", 0, 0, "PADD", "PDIF",
		"OR", "XOR", "AND", "SBX", "ADD", "SUB", "MUL", 0, "SHL", "SAR", "DIV", "MOD", 0, 0, "PCP", 0,
		"CMPE", "CMPNE", "CMPL", "CMPGE", "CMPLE", "CMPG", "TSTZ", "TSTNZ", "PCMPE", "PCMPNE", "PCMPL", "PCMPGE", "PCMPLE", "PCMPG", "DATA", "PRE_2F",
	};
	if (0 <= i && i <= 0x2f) {
		if (table[i] != 0)
			p = table[i];
	}
	if (i == 0xfd) p = "LIDR";
	if (i == 0xfe) p = "REM";
	j = vm->debugBreakPointValue;
	printf("dbg: DR0=%06d, next-op=0x%02x(%-6s), bp.i=0x%02x, bp.v=0x%08x=%010d", vm->dr[0], i, p, vm->debugBreakPointIndex, j, j);
	for (i = 0; i < vm->debugWatchs; i++) {
		j = vm->debugWatchIndex[i];
		printf(", R%02X=0x%08x=%010d(bit=%02d)", j, vm->r[j], vm->r[j], vm->bit[j]);
	}
	printf("\n");
	if (vm->debugAutoFlsh != 0 && vram != NULL)
		drv_flshWin(v_xsiz, v_ysiz, 0, 0);
	for (;;) {
		printf("dbg>");
		fgets(cmdlin, 1024 - 2, stdin);
		if (cmdlin[0] == '\n') continue;
		if (strcmp(cmdlin, "s\n") == 0) {	// step
			if (vm->errorCode == 0)
				break;
			continue;
		}
		if (strcmp(cmdlin, "c\n") == 0) {	// cont
			if (vm->errorCode == 0) {
				vm->toDebugMonitor = 2;
				break;
			}
			continue;
		}
		if (strcmp(cmdlin, "delete\n") == 0) {	// delete
			if (vm->errorCode == 0) {
				vm->debugBreakPointIndex = -1;
				break;
			}
			continue;
		}
		if (strncmp(cmdlin, "b DR0=0x", 8) == 0) {
			vm->debugBreakPointIndex = 0x40;
			sscanf(cmdlin + 8, "%x", &(vm->debugBreakPointValue));
			continue;
		}
		if (strncmp(cmdlin, "b DR0=", 6) == 0) {
			vm->debugBreakPointIndex = 0x40;
			sscanf(cmdlin + 6, "%d", &(vm->debugBreakPointValue));
			continue;
		}
		if (strncmp(cmdlin, "b op=0x", 7) == 0) {
			vm->debugBreakPointIndex = 0x41;
			sscanf(cmdlin + 7, "%x", &(vm->debugBreakPointValue));
			continue;
		}
		if (strncmp(cmdlin, "b op=", 5) == 0) {
			vm->debugBreakPointIndex = 0x41;
			sscanf(cmdlin + 5, "%d", &(vm->debugBreakPointValue));
			continue;
		}
		if (strncmp(cmdlin, "b R", 3) == 0 && cmdlin[5] == '=') {
			i = -1;
			sscanf(cmdlin + 3, "%x", &i);
			if (0 <= i && i <= 0x3f) {
				vm->debugBreakPointIndex = i;
				if (cmdlin[6] == '0' && cmdlin[7] == 'x')
					sscanf(cmdlin + 8, "%x", &(vm->debugBreakPointValue));
				else
					sscanf(cmdlin + 6, "%d", &(vm->debugBreakPointValue));
				continue;
			}
		}
		if (strncmp(cmdlin, "p R", 3) == 0) {
			i = -1;
			sscanf(cmdlin + 3, "%x", &i);
			if (0 <= i && i <= 0x3f) {
				printf("R%02X=0x%08x=%010d(bit=%02d)\n", i, vm->r[i], vm->r[i], vm->bit[i]);
				continue;
			}
		}
		if (strncmp(cmdlin, "watchs=", 7) == 0) {
			i = -1;
			sscanf(cmdlin + 7, "%d", &i);
			if (0 <= i && i <= 2) {
				vm->debugWatchs = i;
				continue;
			}
		}
		if (strncmp(cmdlin, "watch0=R", 8) == 0) {
			i = -1;
			sscanf(cmdlin + 8, "%x", &i);
			if (0 <= i && i <= 0x3f) {
				vm->debugWatchIndex[0] = i;
				continue;
			}
		}
		if (strncmp(cmdlin, "watch1=R", 8) == 0) {
			i = -1;
			sscanf(cmdlin + 8, "%x", &i);
			if (0 <= i && i <= 0x3f) {
				vm->debugWatchIndex[1] = i;
				continue;
			}
		}
		if (strcmp(cmdlin, "q\n") == 0 || strcmp(cmdlin, "quit\n") == 0)
			exit(1);
		if (strcmp(cmdlin, "info\n") == 0) {
			printf("execSteps1=%d, execSteps0=%09d\n", vm->execSteps1, vm->execSteps0);
			continue;
		}

		printf("debug command error.\n");
	}
	return;
}


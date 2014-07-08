/* OS依存関数 */
void *mallocRWE(int bytes); // 実行権付きメモリのmalloc.
void drv_openWin(int x, int y, unsigned char *buf, char *winClosed);
void drv_flshWin(int sx, int sy, int x0, int y0);
void drv_sleep(int msec);

int OsecpuMain(int argc, const unsigned char **argv);

#if (!defined(DRV_OSNUM))
	#if (defined(_WIN32))
		#define DRV_OSNUM			0x0001
	#endif
	#if (defined(__APPLE__))
		#define DRV_OSNUM			0x0002
	#endif
	#if (defined(__linux__))
		#define DRV_OSNUM			0x0003
	#endif
	/* 0001: win32-x86-32bit */
	/* 0002: MacOS-x86-32bit */
	/* 0003: linux-x86-32bit */
#endif

int *vram = 0, v_xsiz, v_ysiz;
char *toDebugMonitor = 0;

#define KEY_ENTER		'\n'
#define KEY_ESC			27
#define KEY_BACKSPACE	8
#define KEY_TAB			9
#define KEY_PAGEUP		0x1020
#define KEY_PAGEDWN		0x1021
#define	KEY_END			0x1022
#define	KEY_HOME		0x1023
#define KEY_LEFT		0x1024
#define KEY_UP			0x1025
#define KEY_RIGHT		0x1026
#define KEY_DOWN		0x1027
#define KEY_INS			0x1028
#define KEY_DEL			0x1029

#define KEYBUFSIZ		4096
int *keybuf, keybuf_r, keybuf_w, keybuf_c;

void putKeybuf(int i)
{
	if ((i & 0x003fffff) == ('D' | 0x00060000) || (i & 0x003fffff) == ('F' | 0x00060000)) {
		if (toDebugMonitor != 0)
			*toDebugMonitor = 1;
	} else {
		if (keybuf_c < KEYBUFSIZ) {
			keybuf[keybuf_w] = i;
			keybuf_c++;
			keybuf_w = (keybuf_w + 1) & (KEYBUFSIZ - 1);
		}
	}
	return;
}

/* OS依存部 */

#if (DRV_OSNUM == 0x0001)

#include <windows.h>
#include <setjmp.h>

#define TIMER_ID			 1
#define TIMER_INTERVAL		10

struct BLD_WORK {
	HINSTANCE hi;
	HWND hw;
	BITMAPINFO bmi;
	int tmcount1, tmcount2, flags, smp; /* bit0: 終了 */
	HANDLE mtx;
	char *winClosed;
};

struct BLD_WORK bld_work;

struct BL_WIN {
	int xsiz, ysiz, *buf;
};

struct BL_WORK {
	struct BL_WIN win;
	jmp_buf jb;
	int csiz_x, csiz_y, cx, cy, col0, col1, tabsiz, slctwin;
	int tmcount, tmcount0, mod, rand_seed;
	int *cbuf;
	unsigned char *ftyp;
	unsigned char **fptn;
	int *ccol, *cbak;
	int *kbuf, kbuf_rp, kbuf_wp, kbuf_c;
};

struct BL_WORK bl_work;

#define BL_SIZ_KBUF			8192

#define BL_WAITKEYF		0x00000001
#define BL_WAITKEYNF	0x00000002
#define BL_WAITKEY		0x00000003
#define BL_GETKEY		0x00000004
#define BL_CLEARREP		0x00000008
#define BL_DELFFF		0x00000010

#define	BL_KEYMODE		0x00000000	// 作りかけ, make/remake/breakが見えるかどうか

#define w	bl_work
#define dw	bld_work

void bld_openWin(int x, int y, char *winClosed);
void bld_flshWin(int sx, int sy, int x0, int y0);
LRESULT CALLBACK WndProc(HWND hw, unsigned int msg, WPARAM wp, LPARAM lp);
void bl_cls();
int bl_iCol(int i);
void bl_readyWin(int n);

static HANDLE threadhandle;

int main(int argc, const char **argv)
{
	return OsecpuMain(argc, (const unsigned char **) argv);
}

void *mallocRWE(int bytes)
{
	void *p = malloc(bytes);
	DWORD dmy;
	VirtualProtect(p, bytes, PAGE_EXECUTE_READWRITE, &dmy);
	return p;
}

static int winthread(void *dmy)
{
	WNDCLASSEX wc;
	RECT r;
	unsigned char *p, *p0, *p00;
	int i, x, y;
	MSG msg;

	x = dw.bmi.bmiHeader.biWidth;
	y = - dw.bmi.bmiHeader.biHeight;

	wc.cbSize = sizeof (WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = dw.hi;
	wc.hIcon = (HICON) LoadImage(NULL, MAKEINTRESOURCE(IDI_APPLICATION),
		IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(IDC_ARROW),
		IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wc.hbrBackground = (HBRUSH) COLOR_APPWORKSPACE;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "WinClass";
	if (RegisterClassEx(&wc) == 0)
		return 1;
	r.left = 0;
	r.top = 0;
	r.right = x;
	r.bottom = y;
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
	x = r.right - r.left;
	y = r.bottom - r.top;

#if 0
	static unsigned char t[32];
	p00 = p0 = p = GetCommandLineA();
	if (*p == 0x22) {
		p00 = p0 = ++p;
		while (*p != '\0' && *p != 0x22) {
			if (*p == '\\')
				p0 = p + 1;
			p++;
		}
	} else {
		while (*p > ' ') {
			if (*p == '\\')
				p0 = p + 1;
			p++;
		}
	}
	if (p - p0 > 4 && p[-4] == '.' && p[-3] == 'e' && p[-2] == 'x' && p[-1] == 'e')
		p -= 4;
	for (i = 0; i < 32 - 1; i++) {
		if (p <= &p0[i])
			break;
		t[i] = p0[i];
	}
	t[i] = '\0';
#endif
	char *t = "osecpu";

	dw.hw = CreateWindowA(wc.lpszClassName, t, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, x, y, NULL, NULL,	dw.hi, NULL);
	if (dw.hw == NULL)
		return 1;
	ShowWindow(dw.hw, SW_SHOW);
	UpdateWindow(dw.hw);
	SetTimer(dw.hw, TIMER_ID,     TIMER_INTERVAL,       NULL);
	SetTimer(dw.hw, TIMER_ID + 1, TIMER_INTERVAL * 10,  NULL);
	SetTimer(dw.hw, TIMER_ID + 2, TIMER_INTERVAL * 100, NULL);
	dw.flags |= 2 | 4;

	for (;;) {
		i = GetMessage(&msg, NULL, 0, 0);
		if (i == 0 || i == -1)	/* エラーもしくは終了メッセージ */
			break;
		/* そのほかはとりあえずデフォルト処理で */
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
//	PostQuitMessage(0);
	dw.flags |= 1; /* 終了, bld_waitNF()が見つける */
	if (dw.winClosed != NULL)
		*dw.winClosed = 1;
	return 0;
}

void bld_openWin(int sx, int sy, char *winClosed)
{
	static int i;

	dw.bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
	dw.bmi.bmiHeader.biWidth = sx;
	dw.bmi.bmiHeader.biHeight = - sy;
	dw.bmi.bmiHeader.biPlanes = 1;
	dw.bmi.bmiHeader.biBitCount = 32;
	dw.bmi.bmiHeader.biCompression = BI_RGB;
	dw.winClosed = winClosed;

	threadhandle = CreateThread(NULL, 0, (void *) &winthread, NULL, 0, (void *) &i);

	return;
}

void drv_flshWin(int sx, int sy, int x0, int y0)
{
	InvalidateRect(dw.hw, NULL, FALSE);
	UpdateWindow(dw.hw);
	return;
}

LRESULT CALLBACK WndProc(HWND hw, unsigned int msg, WPARAM wp, LPARAM lp)
{
	int i, j;
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(dw.hw, &ps);
		SetDIBitsToDevice(hdc, 0, 0, w.win.xsiz, w.win.ysiz,
			0, 0, 0, w.win.ysiz, w.win.buf, &dw.bmi, DIB_RGB_COLORS);
		EndPaint(dw.hw, &ps);
	}
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	if (msg == WM_TIMER && wp == TIMER_ID) {
		w.tmcount += TIMER_INTERVAL;
		return 0;
	}
	if (msg == WM_TIMER && wp == TIMER_ID + 1) {
		dw.tmcount1 += TIMER_INTERVAL * 10;
		w.tmcount = dw.tmcount1;
		return 0;
	}
	if (msg == WM_TIMER && wp == TIMER_ID + 2) {
		dw.tmcount2 += TIMER_INTERVAL * 100;
		w.tmcount = dw.tmcount1 = dw.tmcount2;
		return 0;
	}
	if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
		i = -1;
#if 0
		int s_sht = GetKeyState(VK_SHIFT);
		int s_ctl = GetKeyState(VK_CONTROL);
		int s_cap = GetKeyState(VK_CAPITAL);
		int s_num = GetKeyState(VK_NUMLOCK);
		if ('A' <= wp && wp <= 'Z') {
			i = wp;
			if (((s_sht < 0) ^ (s_cap & 1)) == 0)
				i += 'a' - 'A';
		}
		if (wp == VK_SPACE)		i = ' ';
#endif
		if (wp == VK_RETURN)	i = KEY_ENTER;
		if (wp == VK_ESCAPE)	i = KEY_ESC;
		if (wp == VK_BACK)		i = KEY_BACKSPACE;
		if (wp == VK_TAB)		i = KEY_TAB;
		if (wp == VK_PRIOR)		i = KEY_PAGEUP;
		if (wp == VK_NEXT)		i = KEY_PAGEDWN;
		if (wp == VK_END)		i = KEY_END;
		if (wp == VK_HOME)		i = KEY_HOME;
		if (wp == VK_LEFT)		i = KEY_LEFT;
		if (wp == VK_RIGHT)		i = KEY_RIGHT;
		if (wp == VK_UP)		i = KEY_UP;
		if (wp == VK_DOWN)		i = KEY_DOWN;
		if (wp == VK_INSERT)	i = KEY_INS;
		if (wp == VK_DELETE)	i = KEY_DEL;
		j &= 0;
		if ((GetKeyState(VK_LCONTROL) & (1 << 15)) != 0) j |= 1 << 17;
		if ((GetKeyState(VK_LMENU)    & (1 << 15)) != 0) j |= 1 << 18;
		if ((GetKeyState(VK_RCONTROL) & (1 << 15)) != 0) j |= 1 << 25;
		if ((GetKeyState(VK_RMENU)    & (1 << 15)) != 0) j |= 1 << 26;
		if ((GetKeyState(VK_RSHIFT)   & (1 << 15)) != 0) i |= 1 << 24;
		if ((GetKeyState(VK_LSHIFT)   & (1 << 15)) != 0) i |= 1 << 16;
		if ((GetKeyState(VK_NUMLOCK)  & (1 <<  0)) != 0) i |= 1 << 22;
		if ((GetKeyState(VK_CAPITAL)  & (1 <<  0)) != 0) i |= 1 << 23;
		if (j != 0) {
			if ('A' <= wp && wp <= 'Z') i = wp;
		}
		if (i != -1) {
			putKeybuf(i | j);
//			bl_putKeyB(1, &i);
			return 0;
		}
	}
	if (msg == WM_KEYUP) {
		i = 0xfff;
//		bl_putKeyB(1, &i);
	}
	if (msg == WM_CHAR) {
		i = 0;
		if (' ' <= wp && wp <= 0x7e) {
			i = wp;
			j &= 0;
			if ((GetKeyState(VK_LCONTROL) & (1 << 15)) != 0) j |= 1 << 17;
			if ((GetKeyState(VK_LMENU)    & (1 << 15)) != 0) j |= 1 << 18;
			if ((GetKeyState(VK_RCONTROL) & (1 << 15)) != 0) j |= 1 << 25;
			if ((GetKeyState(VK_RMENU)    & (1 << 15)) != 0) j |= 1 << 26;
			if ((GetKeyState(VK_RSHIFT)   & (1 << 15)) != 0) i |= 1 << 24;
			if ((GetKeyState(VK_LSHIFT)   & (1 << 15)) != 0) i |= 1 << 16;
			if ((GetKeyState(VK_NUMLOCK)  & (1 <<  0)) != 0) i |= 1 << 22;
			if ((GetKeyState(VK_CAPITAL)  & (1 <<  0)) != 0) i |= 1 << 23;
			if (('A' <= wp && wp <= 'Z') || ('a' <= wp && wp <= 'z')) {
				if (j != 0) {
					i |= j;
					i &= ~0x20;
				}
			}
			putKeybuf(i);
//			bl_putKeyB(1, &i);
			return 0;
		}
	}
	return DefWindowProc(hw, msg, wp, lp);
}

void drv_openWin(int sx, int sy, UCHAR *buf, char *winClosed)
{
	int i, x, y;
//	if (sx <= 0 || sy <= 0) return;
//	if (sx < 160) return;
	w.win.buf = (int *) buf;
	w.win.xsiz = sx;
	w.win.ysiz = sy;
	bld_openWin(sx, sy, winClosed);
	return;
}

void drv_sleep(int msec)
{
	Sleep(msec);
//	MsgWaitForMultipleObjects(1, &threadhandle, FALSE, msec, QS_ALLINPUT);
	/* 勉強不足でまだ書き方が分かりません! */
	return;
}

#endif

#if (DRV_OSNUM == 0x0002)

// by Liva, 2013.05.29-

#include <mach/mach.h>
#include <Cocoa/Cocoa.h>

typedef unsigned char UCHAR;

void *mallocRWE(int bytes)
{
	void *p = malloc(bytes);
	vm_protect(mach_task_self(), (vm_address_t) p, bytes, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
	return p;
}

NSApplication* app;

@interface OSECPUView : NSView {
  UCHAR *_buf;
  int _sx;
  int _sy;
	CGContextRef _context;
}

- (id)initWithFrame:(NSRect)frameRect
  buf : (UCHAR *) buf
  sx : (int) sx
  sy : (int) sy;

- (void)drawRect:(NSRect)rect;
@end

@implementation OSECPUView
- (id)initWithFrame:(NSRect)frameRect
  buf : (UCHAR *) buf
  sx : (int) sx
  sy : (int) sy
{
  self = [super initWithFrame:frameRect];
  if (self) {
    _buf = buf;
    _sx = sx;
    _sy = sy;
  }
  return self;
}
 
- (void)drawRect:(NSRect)rect {
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  _context = CGBitmapContextCreate (_buf, _sx, _sy, 8, 4 * _sx, colorSpace, (kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst));
  CGImageRef image = CGBitmapContextCreateImage(_context);
  CGContextRef currentContext = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
  CGContextDrawImage(currentContext, NSRectToCGRect(rect), image);
}
 
@end

@interface Main : NSObject<NSWindowDelegate> {
  int argc;
  const UCHAR **argv;
  char *winClosed;
  OSECPUView *_view;
}

- (void)runApp;
- (void)createThread : (int)_argc
args : (const char **)_argv;
- (BOOL)windowShouldClose:(id)sender;
- (void)openWin : (UCHAR *)buf
sx : (int) sx
sy : (int) sy
winClosed : (char *)_winClosed;
- (void)flushWin : (NSRect) rect;
@end

@implementation Main
- (void)runApp
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  OsecpuMain(argc, (const UCHAR **) argv);
  [NSApp terminate:self];
	[pool release];
}

- (void)createThread : (int)_argc
  args : (const char **)_argv
{
  argc = _argc;
  argv = _argv;
  NSThread *thread = [[[NSThread alloc] initWithTarget:self selector:@selector(runApp) object:nil] autorelease];
  [thread start];
}

- (BOOL)windowShouldClose:(id)sender
{
  *winClosed = 1;
  return YES;
}

- (void)openWin : (UCHAR *)buf
  sx : (int) sx
  sy : (int) sy
  winClosed : (char *)_winClosed
{

	NSWindow* window = [[NSWindow alloc] initWithContentRect: NSMakeRect(0, 0, sx, sy) styleMask: NSTitledWindowMask | NSMiniaturizableWindowMask | NSClosableWindowMask backing: NSBackingStoreBuffered defer: NO];
	[window setTitle: @"osecpu"];
	[window center];
	[window makeKeyAndOrderFront:nil];
	[window setReleasedWhenClosed:YES];
	window.delegate = self;
	winClosed = _winClosed;

	_view = [[OSECPUView alloc] initWithFrame:NSMakeRect(0,0,sx,sy) buf:buf sx:sx sy:sy];
	[window.contentView addSubview:_view];
}

- (void)flushWin : (NSRect) rect
  {
    [_view setNeedsDisplayInRect:rect]; // by ???, 2014.06.25
  }

@end

id objc_main;

int main(int argc, const char **argv)
{
	// by hikarupsp, 2014.06.12-

	// Main thread
	NSAutoreleasePool* pool;
	ProcessSerialNumber psn = {0, kCurrentProcess};
	// ForegroundApplicationになれるように設定(こうしないとキーイベントが取得できない)
	TransformProcessType(&psn, kProcessTransformToForegroundApplication);

	objc_main = [[Main alloc] init];
	NSApp = [NSApplication sharedApplication];
	pool = [[NSAutoreleasePool alloc] init];
	[objc_main createThread:argc args:argv];
	[NSApp run];
	[pool release];

	return 0;
}

void drv_openWin(int sx, int sy, UCHAR *buf, char *winClosed)
{
  [objc_main openWin:buf sx:sx sy:sy winClosed:winClosed];
}

void drv_flshWin(int sx, int sy, int x0, int y0)
{
  [objc_main flushWin:NSMakeRect(x0,y0,sx,sy)];
}

void drv_sleep(int msec)
{
  [NSThread sleepForTimeInterval:0.001*msec];
   return;
}

#endif

#if (DRV_OSNUM == 0x0003)
 
// by takeutch-kemeco, 2013.07.25-

typedef unsigned char UCHAR;
 
void __bl_openWin_attach_vram(int, int, int*);
void *__bld_mallocRWE(unsigned int);
void bld_flshWin(int, int, int, int);
void bl_wait(int);
void __bld_set_callback_key_press(void (*f)(void*, const int));
void __bld_set_callback_key_release(void (*f)(void*, const int));
 
extern int bl_argc;
extern const UCHAR** bl_argv;
 
static void __drv_bld_callback_key_press(void* a, const int keyval) { if (keyval != -1) putKeybuf(keyval); }
static void __drv_bld_callback_key_release(void* a, const int keyval) { /* putKeybuf(0x0fff); */ }
 
void *mallocRWE(int bytes) { return __bld_mallocRWE(bytes); }
void drv_openWin(int sx, int sy, UCHAR *buf, char *winClosed) { __bl_openWin_attach_vram(sx, sy, (int*) buf); }
void drv_flshWin(int sx, int sy, int x0, int y0) { bld_flshWin(sx, sy, x0, y0); }
void drv_sleep(int msec) { bl_wait(msec); }
 
blMain()
{
    __bld_set_callback_key_press(__drv_bld_callback_key_press);
    __bld_set_callback_key_release(__drv_bld_callback_key_release);
    OsecpuMain(bl_argc, bl_argv);
}
 
#endif /* (DRV_OSNUM == 0x0003) */



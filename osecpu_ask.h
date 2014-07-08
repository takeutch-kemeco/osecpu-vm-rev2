#if (!defined(REVISION))
	#define REVISION	2
#endif

#define api_drawPoint(mod, c, x, y)					DB(0xfc,0xfe,0x10); R30=0x0002; R31=mod; R32=c; R33=x; R34=y; PCALL(P2F) 
#define api_drawLine(mod, c, x0, y0, x1, y1)		DB(0xfc,0xfe,0x10); R30=0x0003; R31=mod; R32=c; R33=x0; R34=y0; R35=x1; R36=y1; PCALL(P2F) 
#define api_fillRect(mod, c, xsiz, ysiz, x0, y0)	DB(0xfc,0xfe,0x10); R30=0x0004; R31=mod;      R32=c; R33=xsiz; R34=ysiz; R35=x0; R36=y0; PCALL(P2F) 
#define api_drawRect(mod, c, xsiz, ysiz, x0, y0)	DB(0xfc,0xfe,0x10); R30=0x0004; R31=mod|0x20; R32=c; R33=xsiz; R34=ysiz; R35=x0; R36=y0; PCALL(P2F) 
#define api_fillOval(mod, c, xsiz, ysiz, x0, y0)	DB(0xfc,0xfe,0x10); R30=0x0005; R31=mod;      R32=c; R33=xsiz; R34=ysiz; R35=x0; R36=y0; PCALL(P2F) 
#define api_drawOval(mod, c, xsiz, ysiz, x0, y0)	DB(0xfc,0xfe,0x10); R30=0x0005; R31=mod|0x20; R32=c; R33=xsiz; R34=ysiz; R35=x0; R36=y0; PCALL(P2F) 
#define api_exit(i)									DB(0xfc,0xfe,0x10); R30=0x0008; R31=i; PCALL(P2F)
#define api_sleep(opt, msec)						DB(0xfc,0xfe,0x10); R30=0x0009; R31=opt; R32=msec; PCALL(P2F)
#define api_inkey(_i, mod)							DB(0xfc,0xfe,0x10); R30=0x000d; R31=mod; PCALL(P2F); _i=R30 
#define api_openWin(xsiz, ysiz)						DB(0xfc,0xfe,0x10); R30=0x0010; R31=xsiz; R32=ysiz; PCALL(P2F)

#define junkApi_flushWin(xsiz, ysiz, x0, y0)		DB(0xfe,0x05,0x01); DDBE(0x0011); R30=0xff41; R31=xsiz; R32=ysiz; R33=x0; R34=y0; PCALL(P2F)
#define junkApi_drawString0(mod, xsiz, ysiz, x0, y0, c, s)	DB(0xfe,0x05,0x01); DDBE(0x0006); R30=0xff48; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x00); DB%(s,0x00); PCALL(P2F) 
#define junkApi_drawString3(mod, xsiz, ysiz, x0, y0, c, s)	DB(0xfe,0x05,0x01); DDBE(0x0006); R30=0xff48; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x03); DB%(s,0x00); PCALL(P2F) 
#define junkApi_drawString5(mod, xsiz, ysiz, x0, y0, c, s)	DB(0xfe,0x05,0x01); DDBE(0x0006); R30=0xff48; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x05); DB%(s,0x00); PCALL(P2F) 
#define junkApi_drawString6(mod, xsiz, ysiz, x0, y0, c, s)	DB(0xfe,0x05,0x01); DDBE(0x0006); R30=0xff48; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x06); DB%(s,0x00); PCALL(P2F) 
#define junkApi_drawString(mod, xsiz, ysiz, x0, y0, c, s)	junkApi_drawString6(mod, xsiz, ysiz, x0, y0, c, s)
#define junkApi_drawString2(mod, xsiz, ysiz, x0, y0, c, len, s)		DB(0xfe,0x05,0x01); DDBE(0x0006); R30=0xff48; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x02,0x02); R37=len; P31=s; PCALL(P2F)
#define junkApi_rand(_r, range)						DB(0xfe,0x05,0x01); DDBE(0x0013); R30=0xff49; R31=range; PCALL(P2F); _r=R30
#define junkApi_putStringDec0(s, reg, width, opt)	DB(0xfe,0x05,0x01); DDBE(0x0016); R30=0xff09; DB(0xff,0x00,0x00); DB%(s,0x00); DB(0xff,0x03,0x01); DDBE(4,-1,width,opt,-16-(reg&0x3f)); PCALL(P2F)
#define junkApi_putStringDec6(s, reg, width, opt)	DB(0xfe,0x05,0x01); DDBE(0x0016); R30=0xff09; DB(0xff,0x00,0x06); DB%(s,0x00); DB(0xff,0x03,0x01); DDBE(4,-1,width,opt,-16-(reg&0x3f)); PCALL(P2F)
#define junkApi_drawStringDec0(mod, xsiz, ysiz, x0, y0, c, s, reg, width, opt)	DB(0xfe,0x05,0x01); DDBE(0x0017); R30=0xff4c; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x00); DB%(s,0x00); DB(0xff,0x03,0x01); DDBE(4,-1,width,opt,-16-(reg&0x3f)); PCALL(P2F) 
#define junkApi_drawStringDec6(mod, xsiz, ysiz, x0, y0, c, s, reg, width, opt)	DB(0xfe,0x05,0x01); DDBE(0x0017); R30=0xff4c; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x06); DB%(s,0x00); DB(0xff,0x03,0x01); DDBE(4,-1,width,opt,-16-(reg&0x3f)); PCALL(P2F) 
#define junkApi_putStringHex0(s, reg, width, opt)	DB(0xfe,0x05,0x01); DDBE(0x0016); R30=0xff09; DB(0xff,0x00,0x00); DB%(s,0x00); DB(0xff,0x03,0x01); DDBE(4,0,width,opt,-16-(reg&0x3f)); PCALL(P2F)
#define junkApi_putStringHex6(s, reg, width, opt)	DB(0xfe,0x05,0x01); DDBE(0x0016); R30=0xff09; DB(0xff,0x00,0x06); DB%(s,0x00); DB(0xff,0x03,0x01); DDBE(4,0,width,opt,-16-(reg&0x3f)); PCALL(P2F)
#define junkApi_drawStringHex0(mod, xsiz, ysiz, x0, y0, c, s, reg, width, opt)	DB(0xfe,0x05,0x01); DDBE(0x0017); R30=0xff4c; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x00); DB%(s,0x00); DB(0xff,0x03,0x01); DDBE(4,0,width,opt,-16-(reg&0x3f)); PCALL(P2F) 
#define junkApi_drawStringHex6(mod, xsiz, ysiz, x0, y0, c, s, reg, width, opt)	DB(0xfe,0x05,0x01); DDBE(0x0017); R30=0xff4c; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=c; DB(0xff,0x00,0x06); DB%(s,0x00); DB(0xff,0x03,0x01); DDBE(4,0,width,opt,-16-(reg&0x3f)); PCALL(P2F) 
#define junkApi_putStringDec(s, reg, width, opt)	junkApi_putStringDec6(s, reg, width, opt)
#define junkApi_drawStringDec(mod, xsiz, ysiz, x0, y0, c, s, reg, width, opt)	junkApi_drawStringDec6(mod, xsiz, ysiz, x0, y0, c, s, reg, width, opt)
#define junkApi_putStringHex(s, reg, width, opt)	junkApi_putStringHex6(s, reg, width, opt)
#define junkApi_drawStringHex(mod, xsiz, ysiz, x0, y0, c, s, reg, width, opt)	junkApi_drawStringHex6(mod, xsiz, ysiz, x0, y0, c, s, reg, width, opt) 

#define junkApi_fopenRead(_filesize, _p, arg)		DB(0xfe,0x05,0x01); DDBE(0x0740); R30=0xff01; R31=arg; PCALL(P2F); _filesize=R30; _p=P31
#define junkApi_fopenWrite(arg, filesize, p)		DB(0xfe,0x05,0x01); DDBE(0x0741); R30=0xff02; R31=arg; R32=filesize; P31=p; PCALL(P2F)
#define junkApi_allocBuf(_p)						DB(0xfe,0x05,0x01); DDBE(0x0742); R30=0xff03; PCALL(P2F); _p=P31
#define junkApi_writeStdout(len, p)					DB(0xfe,0x05,0x01); DDBE(0x0743); R30=0xff05; R31=len; P31=p; PCALL(P2F)
#define junkApi_putConstString0(s)					DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x00,0x00); DB%(s,0x00); PCALL(P2F)
#define junkApi_putConstString3(s)					DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x00,0x03); DB%(s,0x00); PCALL(P2F)
#define junkApi_putConstString5(s)					DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x00,0x05); DB%(s,0x00); PCALL(P2F)
#define junkApi_putConstString6(s)					DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x00,0x06); DB%(s,0x00); PCALL(P2F)
#define junkApi_putConstString(s)					junkApi_putConstString6(s)
#define junkApi_putString2(len, s)					DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x02,0x02); R31=len; P31=s; PCALL(P2F)
#define junkApi_putcharRxx(reg)						DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x01,0x04); DDBE(-16-(reg&0x3f),0); PCALL(P2F)
#define junkApi_putchar(c)							R38=c;                          DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x01,0x04); DDBE(-16-0x38,0); PCALL(P2F)
#define junkApi_putchar2(c0, c1)					R38=c0; R39=c1;                 DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x01,0x04); DDBE(-16-0x38,-16-0x39,0); PCALL(P2F)
#define junkApi_putchar3(c0, c1, c2)				R38=c0; R39=c1; R3A=c2;         DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x01,0x04); DDBE(-16-0x38,-16-0x39,-16-0x3a,0); PCALL(P2F)
#define junkApi_putchar4(c0, c1, c2, c3)			R38=c0; R39=c1; R3A=c2; R3B=c3; DB(0xfe,0x05,0x01); DDBE(0x0001); R30=0xff07; DB(0xff,0x01,0x04); DDBE(-16-0x38,-16-0x39,-16-0x3a,-16-0x3b,0); PCALL(P2F)
#define junkApi_drawChar(mod, xsiz, ysiz, x0, y0, col, ch)	R38=ch; DB(0xfe,0x05,0x01); DDBE(0x0006); R30=0xff48; R31=mod; R32=xsiz; R33=ysiz; R34=x0; R35=y0; R36=col; DB(0xff,0x01,0x04); DDBE(-16-0x38,0); PCALL(P2F) 
#define junkApi_malloc(_p, typ, len)				R38=typ; R39=len; DB(0x32,_p&0x3f,0x38,0x39)
#define junkApi_jitc2(_rc, _p, mod, lev, di1, len, s)	R30=0xff08; R31=mod; R32=lev; R33=di1; R34=len; P31=s; PCALL(P2F); _rc=R30; _p=P31

#define junkApi_bitblt(mod, xsiz, ysiz, xscale, yscale, x0, y0, lineskip, inv, p_buf, typ0, p_table, typ1)	DB(0xfe,0x05,0x01); DDBE(0x0018); R30=0xff4d; R31=mod; R32=xsiz; R33=ysiz; R34=xscale; R35=yscale; R36=x0; R37=y0; R38=lineskip; R39=inv; R3A=typ0; R3B=typ1; P31=p_buf; P32=p_table; PCALL(P2F)

#define	LMEM0(bit, reg, typ, preg)	LMEM(bit, reg, typ, preg, 0)
#define SMEM0(bit, reg, typ, preg)	SMEM(bit, reg, typ, preg, 0)
#define	PLMEM0(preg0, typ, preg1)	PLMEM(preg0, typ, preg1, 0)
#define PSMEM0(preg0, typ, preg1)	PSMEM(preg0, typ, preg1, 0)

#define	LMEM0PP(bit, reg, typ, preg)	LMEM(bit, reg, typ, preg, 0); PADDI(32, preg, typ, preg, 1)
#define SMEM0PP(bit, reg, typ, preg)	SMEM(bit, reg, typ, preg, 0); PADDI(32, preg, typ, preg, 1)

#define PALMEM(bit, reg0, typ32, preg0, reg1, mclen)	PADD(32, P3F, typ32, preg0, reg1); LMEM(bit, reg0, typ32, P3F, mclen)
#define PASMEM(bit, reg0, typ32, preg0, reg1, mclen)	PADD(32, P3F, typ32, preg0, reg1); SMEM(bit, reg0, typ32, P3F, mclen)
#define PALMEM0(bit, reg0, typ32, preg0, reg1)		PALMEM(bit, reg0, typ32, preg0, reg1, 0)
#define PASMEM0(bit, reg0, typ32, preg0, reg1)		PASMEM(bit, reg0, typ32, preg0, reg1, 0)
#define PAPLMEM(preg0, typ32, preg1, reg1, mclen)	PADD(32, P3F, typ32, preg1, reg1); PLMEM(preg0, typ32, P3F, mclen)
#define PAPSMEM(preg0, typ32, preg1, reg1, mclen)	PADD(32, P3F, typ32, preg1, reg1); PSMEM(preg0, typ32, P3F, mclen)
#define PAPLMEM0(preg0, typ32, preg1, reg1)		PAPLMEM(preg0, typ32, preg1, reg1, 0)
#define PAPSMEM0(preg0, typ32, preg1, reg1)		PAPSMEM(preg0, typ32, preg1, reg1, 0)

#define OPSWP(...)									DB(0xff, 0x00, 0xff, __VA_ARGS__, 0)	// 2å¬Ç∏Ç¬èëÇ≠. ëÂÇ´Ç¢ï˚Ç©ÇÁèëÇ≠.

#define beginFunc(label)							ELB(label); DB(0xbc, 0xf0)
	// 0x3c, opt, r1, p1, lenR|lenP, r0, p0 ... opt=0Ç»ÇÁí«â¡Ç»ÇµÇ≈Ç¢Ç¢ÇÒÇ∂Ç·Ç»Ç¢Ç©.
#define endFunc()									DB(0xbd, 0xf0); RET()

#define Int32s										SInt32
#define VoidPtr										VPtr
#define do											for (;0;)
#define PAD											PADDINGPREFIX

#define T_VPTR		0x01
#define T_SINT8     0x02 // 8bitÇÃïÑçÜïtÇ´, Ç¢ÇÌÇ‰ÇÈ signed char.
#define T_UINT8     0x03
#define T_SINT16    0x04 // 16bitÇÃïÑçÜïtÇ´, Ç¢ÇÌÇ‰ÇÈ short.
#define T_UINT16    0x05
#define T_SINT32    0x06
#define T_UINT32    0x07
#define T_SINT4     0x08
#define T_UINT4     0x09
#define T_SINT2     0x0a
#define T_UINT2     0x0b
#define T_SINT1     0x0c // ë„ì¸Ç≈Ç´ÇÈÇÃÇÕ0Ç©-1ÇÃÇ›.
#define T_UINT1     0x0d
#define T_SINT12    0x0e
#define T_UINT12    0x0f
#define T_SINT20    0x10
#define T_UINT20    0x11
#define T_SINT24    0x12
#define T_UINT24    0x13
#define T_SINT28    0x14
#define T_UINT28    0x15

#define MODE_COL3	0
#define MODE_COL9	1
#define MODE_COL15	2
#define MODE_COL24	3
#define MODE_PSET	0x00
#define MODE_OR		0x04
#define MODE_XOR	0x08
#define MODE_AND	0x0c

#define	DAT_SA(label, typ32, length)	aska_code(0); DAT_SA0(label, typ32, length)
#define DAT_END()						aska_code(1)

%include "osecpu_asm.h"
OSECPU_HEADER();

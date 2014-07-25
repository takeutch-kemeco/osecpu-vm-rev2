#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UCHAR;

#if (!defined(REVISION))
	#define REVISION	2
#endif

#define	BUFSIZE		2 * 1024 * 1024
#define	JBUFSIZE	64 * 1024

#define MAXLABELS	4096
#define MAXIDENTS	4096

struct Element {
	UCHAR typ, subTyp[3];
		// [0]:優先順位.
		// [1]:右結合フラグ.
		// [2]:各種フラグ.
	union {
		const void *pValue;
		int iValue;
	};
	int subValue;
};

struct VarName {
	int len;
	const UCHAR *p;
	struct Element ele;
};

struct Work {
	int argc, flags, sp, isiz;
	const UCHAR **argv, *argIn, *argOut;
	struct Element ele0[10000], ele1[10000];
	UCHAR ibuf[BUFSIZE];
	UCHAR obuf[BUFSIZE];
	UCHAR jbuf[JBUFSIZE];
};

const UCHAR *searchArg(struct Work *w, const UCHAR *tag);
int putBuf(const UCHAR *filename, const UCHAR *p0, const UCHAR *p1);
int getConstInt(const UCHAR **pp, char *errflg, int flags);
int getConstInt1(const UCHAR **pp);
int aska(struct Work *w);
int prepro(struct Work *w);
int lbstk(struct Work *w);
int db2bin(struct Work *w);
int disasm(struct Work *w);
int appack(struct Work *w);
int maklib(struct Work *w);
//int fcode(struct Work *w);
int b32(struct Work *w);
int dumpbk(struct Work *w);
int dumpfr(struct Work *w);

#define PUT_USAGE				-999
#define	FLAGS_PUT_MANY_INFO		1
#define FLAGS_NO_DEL_ARGOUT		2

int main(int argc, const UCHAR **argv)
{
	struct Work *w = malloc(sizeof (struct Work));
	w->argc = argc;
	w->argv = argv;
	w->argIn  = searchArg(w, "in:" );
	w->argOut = searchArg(w, "out:");
	w->flags = 0;
	int r;
	const UCHAR *cs = searchArg(w, "flags:");
	if (cs != NULL) {
		w->flags = getConstInt1(&cs);
	}
	if (w->argOut != NULL && (w->flags & FLAGS_NO_DEL_ARGOUT) == 0) {
		remove(w->argOut);
	}
	if (w->argIn != NULL) {
		FILE *fp = fopen(w->argIn, "rb");
		if (fp == NULL) {
			fprintf(stderr, "fopen error: '%s'\n", w->argIn);
			goto fin;
		}
		r = fread(w->ibuf, 1, BUFSIZE - 2, fp);
		fclose(fp);
		if (r >= BUFSIZE - 2) {
			fprintf(stderr, "too large infile: '%s'\n", w->argIn);
			goto fin;
		}
		w->ibuf[r] = '\0';
		w->ibuf[r + 1] = '\0';
		w->ibuf[r + 2] = '\0';
		w->ibuf[r + 3] = '\0';
		w->isiz = r;
	}

	cs = searchArg(w, "tool:");
	r = PUT_USAGE;
	if (cs != NULL && w->argIn != NULL) {
		if (strcmp(cs, "aska")   == 0) { r = aska(w);   }
		if (strcmp(cs, "prepro") == 0) { r = prepro(w); }
		if (strcmp(cs, "lbstk")  == 0) { r = lbstk(w);  }
		if (strcmp(cs, "db2bin") == 0) { r = db2bin(w); }
		if (strcmp(cs, "disasm") == 0) { r = disasm(w); }
		if (strcmp(cs, "appack") == 0) { r = appack(w); }
		if (strcmp(cs, "maklib") == 0) { r = maklib(w); }
	//	if (strcmp(cs, "fcode")  == 0) { r = fcode(w);  }
		if (strcmp(cs, "b32")    == 0) { r = b32(w);    }
		if (strcmp(cs, "dumpbk") == 0) { r = dumpbk(w); }
		if (strcmp(cs, "dumpfr") == 0) { r = dumpfr(w); }
	}
	if (cs != NULL && strcmp(cs, "getint") == 0) {
		cs = searchArg(w, "exp:");
		if (cs != NULL) {
			char errflg = 0;
			r = getConstInt(&cs, &errflg, w->flags);
			printf("getConstInt=%d, errflg=%d\n", r, errflg);
			r = 0;
			cs = NULL;
		}
	} 
	if (cs != NULL && strcmp(cs, "osastr") == 0) {
		cs = searchArg(w, "str:");
		if (cs != NULL) {
			r = strlen(cs);
			while (r > 0) {
				if (r >= 8) {
					printf("%02X ",  cs[0] | ((cs[7] >> 6) & 1) << 7);
					printf("%02X ",  cs[1] | ((cs[7] >> 5) & 1) << 7);
					printf("%02X ",  cs[2] | ((cs[7] >> 4) & 1) << 7);
					printf("%02X ",  cs[3] | ((cs[7] >> 3) & 1) << 7);
					printf("%02X ",  cs[4] | ((cs[7] >> 2) & 1) << 7);
					printf("%02X ",  cs[5] | ((cs[7] >> 1) & 1) << 7);
					printf("%02X\n", cs[6] | ( cs[7]       & 1) << 7);
					r -= 8;
					cs += 8;
					continue;
				}
				printf("%02X ", *cs);
				cs++;
				r--;
			}
			r = 0;
			cs = NULL;
		}
	}
	if (cs != NULL && strcmp(cs, "binstr") == 0) {
		cs = searchArg(w, "str:");
		if (cs != NULL) {
			r = strlen(cs);
			while (r > 0) {
				printf("%d%d%d%d%d%d%d%d ", (*cs >> 7) & 1, (*cs >> 6) & 1,
					(*cs >> 5) & 1, (*cs >> 4) & 1, (*cs >> 3) & 1, (*cs >> 2) & 1, (*cs >> 1) & 1, *cs & 1);
				cs++;
				r--;
			}
			r = 0;
			cs = NULL;
			putchar('\n');
		}
	}


	if (r == PUT_USAGE) {
		printf("target-revision: %d\n", REVISION);
		puts("usage>osectols tool:aska   in:file [out:file] [flags:#]\n"
			 "      osectols tool:prepro in:file [out:file] [flags:#]\n"
			 "      osectols tool:lbstk  in:file [out:file] [flags:#] [lst:debuginfo]\n"
			 "      osectols tool:db2bin in:file [out:file] [flags:#]\n"
			 "      osectols tool:disasm in:file [out:file] [flags:#]\n"
			 "      osectols tool:appack in:file [out:file] [flags:#]\n"
			 "      osectols tool:maklib in:file [out:file] [flags:#]\n"
			 "      osectols tool:getint exp:expression [flags:#]\n"
			 "      osectols tool:osastr str:string\n"
			 "      osectols tool:binstr str:string\n"
			 "  aska   ver.0.22\n"//123
			 "  prepro ver.0.01\n"
			 "  lbstk  ver.0.04\n"//117
			 "  db2bin ver.0.22\n"//123
			 "  disasm ver.0.02\n"
			 "  appack ver.0.25\n"//123
			 "  maklib ver.0.01\n"
			 "  getint ver.0.06\n"
			 "  osastr ver.0.00\n"
			 "  binstr ver.0.00\n"
		//	 "  fcode  ver.0.00\n"
			 "  b32    ver.0.01\n"  //118
			 "  dumpbk ver.0.03\n"  //123
			 "  dumpfr ver.0.02"  //123

		);
		r = 1;
	}
fin:
	return r;
}

int strncmp1(const UCHAR *s, const UCHAR *t)
{
	int r = 0;
	while (*s == *t) {
		s++;
		t++;
	}
	if (*t != '\0')
		r = *s - *t;
	return r;
}

UCHAR *strcpy1(UCHAR *q, const UCHAR *p)
{
	while (*p != '\0')
		*q++ = *p++;
	return q;
}

const UCHAR *searchArg(struct Work *w, const UCHAR *tag)
{
	int i, l;
	const UCHAR *r = NULL;
	for (i = 1; i < w->argc; i++) {
		if (strncmp1(w->argv[i], tag) == 0) {
			r = w->argv[i] + strlen(tag);
			break;
		}
	}
	return r;
}

int putBuf(const UCHAR *filename, const UCHAR *p0, const UCHAR *p1)
{
	int r = 0;
	if (filename != NULL && p1 != NULL) {
		FILE *fp = fopen(filename, "wb");
		if (fp == NULL) {
			fprintf(stderr, "fopen error: '%s'\n", filename);
			r = 1;
		} else {
			fwrite(p0, 1, p1 - p0, fp);
			fclose(fp);
		}
	}
	return r;
}

int hexChar(UCHAR c)
{
	int r = -1;
	if ('0' <= c && c <= '9') r = c - '0';
	if ('A' <= c && c <= 'F') r = c - ('A' - 10);
	if ('a' <= c && c <= 'f') r = c - ('a' - 10);
	return r;
}

int isSymbolChar(UCHAR c)
{
	int r = 0;
	if (c == '!') r = 1;
	if ('%' <= c && c <= '&') r = 1;
	if (0x28 <= c && c <= 0x2f) r = 1;
	if (0x3a <= c && c <= 0x3f) r = 1;
	if (0x5b <= c && c <= 0x5e) r = 1;
	if (0x7b <= c && c <= 0x7e) r = 1;
	// @や_は記号ではない.
	return r;
}

#define ELEMENT_TERMINATE		0
#define ELEMENT_ERROR			1
#define ELEMENT_REG				2
#define ELEMENT_PREG			3
#define ELEMENT_CONST			4
#define ELEMENT_DREG			7

	#define	ELEMENT_INT				1
#define ELEMENT_PLUS			8
#define ELEMENT_PLUS1			9
#define ELEMENT_PLUS2			10
#define ELEMENT_MINUS			12
#define ELEMENT_MINUS1			13
#define ELEMENT_MINUS2			14
#define ELEMENT_PPLUS			16
#define ELEMENT_PPLUS0			17
#define ELEMENT_PPLUS1			18
#define ELEMENT_MMINUS			20
#define ELEMENT_MMINS0			21
#define ELEMENT_MMINS1			22
#define ELEMENT_PRODCT			24
#define ELEMENT_PRDCT1			25
#define ELEMENT_PRDCT2			26
#define ELEMENT_COMMA			28
#define ELEMENT_COMMA0			29	// 引数セパレータ.
#define ELEMENT_COMMA1			30	// 演算子.
#define ELEMENT_PRNTH0			32	// かっこ.
#define ELEMENT_PRNTH1			33
#define ELEMENT_OR				34
#define ELEMENT_OROR			35
#define ELEMENT_AND				36
#define ELEMENT_ANDAND			37
#define ELEMENT_XOR				38
#define ELEMENT_SHL				40
#define ELEMENT_SAR				41
#define ELEMENT_SLASH			42
#define	ELEMENT_PERCNT			43
#define	ELEMENT_EQUAL			44
#define	ELEMENT_OREQUL			45
#define	ELEMENT_XOREQL			46
#define	ELEMENT_ANDEQL			47
#define	ELEMENT_PLUSEQ			48
#define	ELEMENT_MINUSE			49
#define	ELEMENT_PRDCTE			50
#define	ELEMENT_SHLEQL			52
#define	ELEMENT_SAREQL			53
#define ELEMENT_SLASHE			54
#define ELEMENT_PRCNTE			55
#define ELEMENT_SEMCLN			56
#define ELEMENT_COLON			57
#define ELEMENT_BRACE0			58
#define ELEMENT_BRACE1			59
#define	ELEMENT_EEQUAL			60
#define	ELEMENT_NOTEQL			61
#define	ELEMENT_LESS			62
#define	ELEMENT_GRTREQ			63
#define	ELEMENT_LESSEQ			64
#define	ELEMENT_GREATR			65
#define	ELEMENT_NOT				66
#define	ELEMENT_TILDE			67
#define	ELEMENT_DOT				68
#define	ELEMENT_ARROW			69
#define ELEMENT_BRCKT0			70
#define ELEMENT_BRCKT1			71

#define ELEMENT_IDENT			96
#define ELEMENT_FOR				97
#define ELEMENT_IF				98
#define ELEMENT_ELSE			99
#define ELEMENT_BREAK			100
#define ELEMENT_CONTIN			101
#define ELEMENT_CONTI0			102
#define ELEMENT_GOTO			103
#define ELEMENT_INT32S			104
#define ELEMENT_VDPTR			105

const UCHAR *stringToElements(struct Element *q, const UCHAR *p, int mode0)
// mode0: 1:コンマはコンマ演算子としては解釈しない、式の区切りとみなす.
// どこでやめるか:
//   forで始まっている場合は、nest中の;でも止まらない. → いや止まる.
//   {で止まる. {を含む. → いや含まない.
//   forで始まってなければ、;でも止まる. ;を含む. → いや含まない.
//	 }で止まる. }を含む. → いや含まない.
//   カンマ区切りやかっこの閉じ過ぎによる停止では、,や)を含まずに終わる。
{
	int i, j, k, l, nest = 0, stack[100];
	stack[0] = mode0;
	struct Element *q0 = q;
	q->typ = ELEMENT_TERMINATE; // とりあえずfor以外を入れておく.
	for (;;) {
		if (q - 1 > q0 && q[-2].typ == ELEMENT_MINUS && q[-1].typ == ELEMENT_CONST) {
			if ((q - 2 == q0) || (q - 2 > q0 &&
				q[-3].typ != ELEMENT_REG && q[-3].typ != ELEMENT_PREG &&
				q[-3].typ != ELEMENT_CONST && q[-3].typ != ELEMENT_IDENT &&
				q[-3].typ != ELEMENT_PRNTH1))
			{
				q[-2] = q[-1];
				q[-2].iValue *= -1;
				q--;
			}
		}
		if (*p == '\0' || *p == ';' || *p == '{' || *p == '}') break;
		if (*p == ',' && nest == 0 && (stack[0] & 1) != 0) break;
		if (*p == ')' && nest <= 0) break;
		if (*p == ' ' || *p == '\t') {
			p++;
			continue;
		}
		if (p[0] == '0' && p[1] == 'x') {
			i = 0;
			p += 2;
			for (;;) {
				if (*p != '_') {
					j = hexChar(*p);
					if (j < 0) break;
					i = i << 4 | j;
				}
				p++;
			}
			q->typ = ELEMENT_CONST;
			q->subTyp[0] = ELEMENT_INT;
			q->iValue = i;
			q++;
			continue;
		}
		if ('0' <= *p && *p <= '9') {
			i = 0;
			for (;;) {
				if (*p != '_') {
					if ('0' <= *p && *p <= '9')
						i = i * 10 + (*p - '0');
					else
						break;
				}
				p++;
			}
			q->typ = ELEMENT_CONST;
			q->subTyp[0] = ELEMENT_INT;
			q->iValue = i;
			q++;
			continue;
		}
		if (*p == '\'') {
			i = 0;
			p++;
			while (*p != '\'' && *p >= ' ') {
				if (*p != '\\') {
					j = *p++;
get8bit:
					i = i << 8 | j;
					continue;
				}
				if (p[1] == '.') {
					p += 2;
					continue;
				}
				if (p[1] == '\n') {
					p += 2;
					if (*p == '\r') p++;
					continue;
				}
				if (p[1] == '\r') {
					p += 2;
					if (*p == '\n') p++;
					continue;
				}
				if (p[1] == '\\' || p[1] == '\"' || p[1] == '\'') {
					j = p[1];
					p += 2;
					goto get8bit;
				}
				if (p[1] == 'n') { j = '\n'; p += 2; goto get8bit; }
				if (p[1] == 'r') { j = '\r'; p += 2; goto get8bit; }
				if (p[1] == 't') { j = '\t'; p += 2; goto get8bit; }
				if ('0' <= p[1] && p[1] <= '7') {
					// あえて2桁以上は受け入れない.
					// それは\oであるべきだと思うから.
					j = p[1] - '0';
					p += 2;
					if (*p == '.') p++;
					goto get8bit;
				}
				if (p[1] == 'x') {
					p += 2;
					j = 0;
					for (;;) {
						k = hexChar(*p);
						if (k < 0) break;
						j = j << 4 | k;
						p++;
					}
					if (*p == '.') p++;
					goto get8bit;
				}
				goto err;
			}
			if (*p != '\'') goto err;
			p++;
			q->typ = ELEMENT_CONST;
			q->subTyp[0] = ELEMENT_INT;
			q->iValue = i;
			q++;
			continue;
		}
		if (*p == '(') {
			p++;
			q->typ = ELEMENT_PRNTH0;
			q++;
			stack[++nest] = 0;
			continue;
		}
		if (*p == ')') {
			p++;
			q->typ = ELEMENT_PRNTH1;
			q++;
			nest--;
			continue;
		}
		static struct {
			UCHAR typ, s[3];
		} table_s[] = {
			{ ELEMENT_PLUS,   "+  " },
			{ ELEMENT_MINUS,  "-  " },
			{ ELEMENT_PPLUS,  "++ " },
			{ ELEMENT_MMINUS, "-- " },
			{ ELEMENT_PRODCT, "*  " },
			{ ELEMENT_COMMA,  ",  " },
			{ ELEMENT_OR,     "|  " },
			{ ELEMENT_OROR,   "|| " },
			{ ELEMENT_AND,    "&  " },
			{ ELEMENT_ANDAND, "&& " },
			{ ELEMENT_XOR,    "^  " },
			{ ELEMENT_SHL,    "<< " },
			{ ELEMENT_SAR,    ">> " },
			{ ELEMENT_SLASH,  "/  " },
			{ ELEMENT_PERCNT, "%  " },
			{ ELEMENT_EQUAL,  "=  " },
			{ ELEMENT_OREQUL, "|= " },
			{ ELEMENT_XOREQL, "^= " },
			{ ELEMENT_ANDEQL, "&= " },
			{ ELEMENT_PLUSEQ, "+= " },
			{ ELEMENT_MINUSE, "-= " },
			{ ELEMENT_PRDCTE, "*= " },
			{ ELEMENT_SHLEQL, "<<=" },
			{ ELEMENT_SAREQL, ">>=" },
			{ ELEMENT_SLASHE, "/= " },
			{ ELEMENT_PRCNTE, "%= " },
		//	{ ELEMENT_SEMCLN, ";  " },
			{ ELEMENT_COLON,  ":  " },
		//	{ ELEMENT_BRACE0, "{  " },
		//	{ ELEMENT_BRACE1, "}  " },
			{ ELEMENT_EEQUAL, "== " },
			{ ELEMENT_NOTEQL, "!= " },
			{ ELEMENT_LESS,   "<  " },
			{ ELEMENT_GRTREQ, ">= " },
			{ ELEMENT_LESSEQ, "<= " },
			{ ELEMENT_GREATR, ">  " },
			{ ELEMENT_NOT,    "!  " },
			{ ELEMENT_TILDE,  "~  " },
			{ ELEMENT_DOT,    ".  " },
			{ ELEMENT_ARROW,  "-> " },
			{ ELEMENT_BRCKT0, "[  " },
			{ ELEMENT_BRCKT1, "]  " },
			{ 0, 0, 0, 0 }
		};
		for (i = j = k = 0; table_s[i].typ != 0; i++) {
			l = 3;
			while (table_s[i].s[l - 1] <= ' ') l--;
			if (k >= l) continue;
			if (strncmp(p, table_s[i].s, l) == 0) {
				k = l;
				j = i;
			}
		}
		if (k > 0) {
			p += k;
			q->typ = table_s[j].typ;
			q++;
			continue;
		}

		if (!isSymbolChar(*p) && *p > ' ') {
			q->typ = ELEMENT_IDENT;
			q->pValue = p;
			do {
				p++;
			} while (!isSymbolChar(*p) && *p > ' ');
			q->subValue = p - (const UCHAR *) q->pValue;
			if (q->subValue == 3) {
				i = hexChar(p[-2]);
				j = hexChar(p[-1]);
				if (p[-3] == 'R' && 0 <= i && i <= 3 && j >= 0) {
					q->typ = ELEMENT_REG;
					q->iValue = i << 4 | j;
					q++;
					continue;
				}
				if (p[-3] == 'P' && 0 <= i && i <= 3 && j >= 0) {
					q->typ = ELEMENT_PREG;
					q->iValue = i << 4 | j;
					q++;
					continue;
				}
				if (p[-3] == 'D' && p[-2] == 'R' && '0' <= p[-1] && p[-1] <= '9') {
					q->typ = ELEMENT_DREG;
					q->iValue = p[-1] - '0';
					q++;
					continue;
				}
			}
			static struct {
				UCHAR typ, s[10];
			} table_k[] = {
				{ ELEMENT_FOR,    "3for"      },
				{ ELEMENT_IF,     "2if"       },
				{ ELEMENT_ELSE,   "4else"     },
				{ ELEMENT_BREAK,  "5break"    },
				{ ELEMENT_CONTIN, "8continue" },
				{ ELEMENT_CONTI0, "9continue0" },
				{ ELEMENT_GOTO,   "4goto"     },
				{ ELEMENT_INT32S, "6SInt32"   },
				{ ELEMENT_VDPTR,  "4VPtr"  },
				{ 0, "" }
			};
			for (i = 0; table_k[i].typ != 0; i++) {
				l = table_k[i].s[0] - '0';
				if (q->subValue != l) continue;
				if (strncmp((const UCHAR *) q->pValue, &table_k[i].s[1], l) == 0) break;
			}
			if (table_k[i].typ != 0) {
				q->typ = table_k[i].typ;
			}
			q++;
			continue;
		}
err:
		q->typ = ELEMENT_ERROR;
		q++;
		break;
	}
	q->typ = ELEMENT_TERMINATE;
	return p;
}

const struct Element *shuntingYard(struct Element *q, const struct Element *p)
{
	char isLastValue = 0, subTyp2[100];
	struct Element stack[100], *q0 = q;
	int sp = 0, sp0, nest = 0, i;
	subTyp2[0] = 0;
	for (;;) {
		stack[sp].subTyp[1] = 0; // デフォルトでは左結合.
		if (p->typ == ELEMENT_ERROR) goto err;
		if (p->typ == ELEMENT_CONST || p->typ == ELEMENT_REG || p->typ == ELEMENT_PREG || p->typ == ELEMENT_DREG) {
			if (isLastValue != 0) goto err;
			*q++ = *p++;
			isLastValue = 1;
			continue;
		}
		stack[sp].subTyp[2] = subTyp2[nest];
		if (isLastValue == 0) {
			static struct {
				int typ0, typ1;
				UCHAR subTyp0, subTyp1;
			} table[] = {
				{ ELEMENT_PLUS,   ELEMENT_PLUS1,   2, 0 }, 
				{ ELEMENT_MINUS,  ELEMENT_MINUS1,  2, 0 },
				{ ELEMENT_PRODCT, ELEMENT_PRDCT1,  2, 0 },
				{ ELEMENT_PPLUS,  ELEMENT_PPLUS0,  2, 0 },
				{ ELEMENT_MMINUS, ELEMENT_MMINS0,  2, 0 },
				{ ELEMENT_NOT,    ELEMENT_NOT,     2, 0 },
				{ ELEMENT_TILDE,  ELEMENT_TILDE,   2, 0 },
				{ 0, 0, 0, 0 }
			};
			for (i = 0; table[i].typ0 != 0; i++) {
				if (p->typ == table[i].typ0) break;
			}
			if (table[i].typ0 != 0) {
				p++;
				stack[sp].typ = table[i].typ1;
				stack[sp].subTyp[0] = table[i].subTyp0;
				stack[sp].subTyp[1] = table[i].subTyp1;
				sp++;
				continue;
			}
			if (p->typ == ELEMENT_PRNTH0) {
				p++;
				stack[sp].typ = ELEMENT_PRNTH0;
				stack[sp].subTyp[0] = 99;
				sp++;
				nest++;
				subTyp2[nest] = subTyp2[nest - 1];
				continue;
			}
			if (p->typ == ELEMENT_PERCNT && p[1].typ == ELEMENT_PRNTH0) {
				p += 2;
				stack[sp].typ = ELEMENT_PRNTH0;
				stack[sp].subTyp[0] = 99;
				sp++;
				nest++;
				subTyp2[nest] = 1;
				continue;
			}
		} else {
			if (p->typ == ELEMENT_PPLUS) {
				p++;
				stack[sp].typ = ELEMENT_PPLUS1;
				stack[sp].subTyp[0] = 1;
				sp++;
				continue;
			}
			if (p->typ == ELEMENT_MMINUS) {
				p++;
				stack[sp].typ = ELEMENT_MMINS1;
				stack[sp].subTyp[0] = 1;
				sp++;
				continue;
			}
			if (p->typ == ELEMENT_PRNTH1) {
				p++;
				while (stack[sp - 1].typ != ELEMENT_PRNTH0) {
					*q++ = stack[--sp];
				}
				sp--;
				nest--;
			//	isLastValue = 1;
				/* ここに関数呼び出しが必要かどうかの判定を書く */
				continue;
			}
			static struct {
				int typ0, typ1;
				UCHAR subTyp0, subTyp1;
			} table[] = {
				{ ELEMENT_PRODCT, ELEMENT_PRDCT2,  4, 0 },
				{ ELEMENT_SLASH,  ELEMENT_SLASH,   4, 0 },
				{ ELEMENT_PERCNT, ELEMENT_PERCNT,  4, 0 },
				{ ELEMENT_PLUS,   ELEMENT_PLUS2,   5, 0 },
 				{ ELEMENT_MINUS,  ELEMENT_MINUS2,  5, 0 },
				{ ELEMENT_SHL,    ELEMENT_SHL,     6, 0 },
				{ ELEMENT_SAR,    ELEMENT_SAR,     6, 0 },
				{ ELEMENT_LESS,   ELEMENT_LESS,    7, 0 },
				{ ELEMENT_GRTREQ, ELEMENT_GRTREQ,  7, 0 },
				{ ELEMENT_LESSEQ, ELEMENT_LESSEQ,  7, 0 },
				{ ELEMENT_GREATR, ELEMENT_GREATR,  7, 0 },
				{ ELEMENT_EEQUAL, ELEMENT_EEQUAL,  8, 0 },
				{ ELEMENT_NOTEQL, ELEMENT_NOTEQL,  8, 0 },
				{ ELEMENT_AND,    ELEMENT_AND,     9, 0 },
				{ ELEMENT_XOR,    ELEMENT_XOR,    10, 0 },
				{ ELEMENT_OR,     ELEMENT_OR,     11, 0 },
				{ ELEMENT_ANDAND, ELEMENT_ANDAND, 12, 0 },
				{ ELEMENT_OROR,   ELEMENT_OROR,   13, 0 },
				{ ELEMENT_EQUAL,  ELEMENT_EQUAL,  15, 1 }, // 右結合.
				{ ELEMENT_OREQUL, ELEMENT_OREQUL, 15, 1 }, // 右結合.
				{ ELEMENT_XOREQL, ELEMENT_XOREQL, 15, 1 }, // 右結合.
				{ ELEMENT_ANDEQL, ELEMENT_ANDEQL, 15, 1 }, // 右結合.
				{ ELEMENT_PLUSEQ, ELEMENT_PLUSEQ, 15, 1 }, // 右結合.
				{ ELEMENT_MINUSE, ELEMENT_MINUSE, 15, 1 }, // 右結合.
				{ ELEMENT_PRDCTE, ELEMENT_PRDCTE, 15, 1 }, // 右結合.
				{ ELEMENT_SHLEQL, ELEMENT_SHLEQL, 15, 1 }, // 右結合.
				{ ELEMENT_SAREQL, ELEMENT_SAREQL, 15, 1 }, // 右結合.
				{ ELEMENT_SLASHE, ELEMENT_SLASHE, 15, 1 }, // 右結合.
				{ ELEMENT_PRCNTE, ELEMENT_PRCNTE, 15, 1 }, // 右結合.
				{ ELEMENT_COMMA,  ELEMENT_COMMA1, 16, 0 },
				{ 0, 0, 0, 0 }
			};
			for (i = 0; table[i].typ0 != 0; i++) {
				if (p->typ == table[i].typ0) break;
			}
			if (table[i].typ0 != 0) {
				p++;
				stack[sp].typ = table[i].typ1;
				stack[sp].subTyp[0] = table[i].subTyp0;
				stack[sp].subTyp[1] = table[i].subTyp1;
				sp0 = sp;
				while (sp > 0 && (
						(stack[sp0].subTyp[1] == 0 && stack[sp0].subTyp[0] >= stack[sp - 1].subTyp[0])
						|| (stack[sp0].subTyp[0] > stack[sp - 1].subTyp[0])))
				{
					*q++ = stack[--sp];
				}
				stack[sp++] = stack[sp0];	
				isLastValue = 0;
				continue;
			}
		}
		if (p->typ == ELEMENT_TERMINATE || p->typ == ELEMENT_SEMCLN) break;
		goto err;
	}
	while (sp > 0) {
		sp--;
		if (stack[sp].typ == ELEMENT_PRNTH0) goto err;
		*q++ = stack[sp];
	}
	q->typ = ELEMENT_TERMINATE;
	goto fin;
err:
	q0->typ = ELEMENT_ERROR;
fin:
	return p;
}

void dumpElements(const struct Element *e)
{
	int i;
	if (e->typ != ELEMENT_ERROR) {
		for (i = 0; e[i].typ != ELEMENT_TERMINATE; i++) {
			if (e[i].typ == ELEMENT_CONST)	{ printf("%d ",    e[i].iValue); continue; }
			if (e[i].typ == ELEMENT_REG)	{ printf("R%02X ", e[i].iValue); continue; }
			if (e[i].typ == ELEMENT_PREG)	{ printf("P%02X ", e[i].iValue); continue; }
			if (e[i].typ == ELEMENT_MINUS1)	{ printf("-a ");    continue; }
			if (e[i].typ == ELEMENT_MINUS2)	{ printf("a-b ");   continue; }
			if (e[i].typ == ELEMENT_MMINS0)	{ printf("--a ");   continue; }
			if (e[i].typ == ELEMENT_MMINS1)	{ printf("a-- ");   continue; }
			if (e[i].typ == ELEMENT_PLUS1)	{ printf("+a ");    continue; }
			if (e[i].typ == ELEMENT_PLUS2)	{ printf("a+b ");   continue; }
			if (e[i].typ == ELEMENT_PPLUS0)	{ printf("++a ");   continue; }
			if (e[i].typ == ELEMENT_PPLUS1)	{ printf("a++ ");   continue; }
			if (e[i].typ == ELEMENT_PRDCT2)	{ printf("a*b ");   continue; }
			if (e[i].typ == ELEMENT_EQUAL)	{ printf("a=b ");   continue; }
			if (e[i].typ == ELEMENT_PLUSEQ)	{ printf("a+=b ");  continue; }
			if (e[i].typ == ELEMENT_MINUSE)	{ printf("a-=b ");  continue; }
			if (e[i].typ == ELEMENT_SHL)	{ printf("a<<b ");  continue; }
			if (e[i].typ == ELEMENT_SAR)	{ printf("a>>b ");  continue; }
			printf("?(%d) ", e[i].typ);
		}
		putchar('\n');
	}
	return;
}

char canEval(const UCHAR typ)
// 4:代入系, 8:可換系, 16:除算系, 32:比較系.
{
	char r = 0;
	if (typ == ELEMENT_PLUS1)  r = 1;
	if (typ == ELEMENT_PLUS2)  r = 2 + 8;
	if (typ == ELEMENT_MINUS1) r = 1;
	if (typ == ELEMENT_MINUS2) r = 2;
	if (typ == ELEMENT_PPLUS0) r = 1;
	if (typ == ELEMENT_PPLUS1) r = 1;
	if (typ == ELEMENT_MMINS0) r = 1;
	if (typ == ELEMENT_MMINS1) r = 1;
	if (typ == ELEMENT_PRDCT2) r = 2 + 8;
	if (typ == ELEMENT_COMMA1) r = 2;
	if (typ == ELEMENT_OR)     r = 2 + 8;
	if (typ == ELEMENT_OROR)   r = 2 + 8;
	if (typ == ELEMENT_AND)    r = 2 + 8;
	if (typ == ELEMENT_ANDAND) r = 2 + 8;
	if (typ == ELEMENT_XOR)    r = 2 + 8;
	if (typ == ELEMENT_SHL)    r = 2;
	if (typ == ELEMENT_SAR)    r = 2;
	if (typ == ELEMENT_SLASH)  r = 2 + 16;
	if (typ == ELEMENT_PERCNT) r = 2 + 16;
	if (typ == ELEMENT_EQUAL)  r = 2 + 4;
	if (typ == ELEMENT_OREQUL) r = 2 + 4;
	if (typ == ELEMENT_XOREQL) r = 2 + 4;
	if (typ == ELEMENT_ANDEQL) r = 2 + 4;
	if (typ == ELEMENT_PLUSEQ) r = 2 + 4;
	if (typ == ELEMENT_MINUSE) r = 2 + 4;
	if (typ == ELEMENT_PRDCTE) r = 2 + 4;
	if (typ == ELEMENT_SHLEQL) r = 2 + 4;
	if (typ == ELEMENT_SAREQL) r = 2 + 4;
	if (typ == ELEMENT_SLASHE) r = 2 + 4 + 16;
	if (typ == ELEMENT_PRCNTE) r = 2 + 4 + 16;
	if (typ == ELEMENT_EEQUAL) r = 2 + 8 + 32;
	if (typ == ELEMENT_NOTEQL) r = 2 + 8 + 32;
	if (typ == ELEMENT_LESS)   r = 2 + 32;
	if (typ == ELEMENT_GRTREQ) r = 2 + 32;
	if (typ == ELEMENT_LESSEQ) r = 2 + 32;
	if (typ == ELEMENT_GREATR) r = 2 + 32;
	if (typ == ELEMENT_NOT)    r = 1;
	if (typ == ELEMENT_TILDE)  r = 1;
	return r;
}

int evalConstInt1(const UCHAR typ, int a)
{
	// 何もしない: ELEMENT_PLUS1, ELEMENT_PPLUS1, ELEMENT_MMINS1
	if (typ == ELEMENT_MINUS1) a = - a;
	if (typ == ELEMENT_PPLUS0) a++;
	if (typ == ELEMENT_MMINS0) a--;
	if (typ == ELEMENT_NOT)    a = - ((a & 1) != 0);
	if (typ == ELEMENT_TILDE)  a = ~a;
	return a;
}

int evalConstInt2(const UCHAR typ, int a, const int b)
// 呼び出し元でゼロ割チェックをすること.
{
	if (typ == ELEMENT_PLUS2)  a +=  b;
	if (typ == ELEMENT_MINUS2) a -=  b;
	if (typ == ELEMENT_PRDCT2) a *=  b;
	if (typ == ELEMENT_COMMA1) a =   b;
	if (typ == ELEMENT_OR)     a |=  b;
	if (typ == ELEMENT_AND)    a &=  b;
	if (typ == ELEMENT_XOR)    a ^=  b;
	if (typ == ELEMENT_SHL)    a <<= b;
	if (typ == ELEMENT_SAR)    a >>= b;
	if (typ == ELEMENT_SLASH)  a /=  b;
	if (typ == ELEMENT_PERCNT) a %=  b;
	if (typ == ELEMENT_EQUAL)  a =   b;
	if (typ == ELEMENT_OREQUL) a |=  b;
	if (typ == ELEMENT_XOREQL) a ^=  b;
	if (typ == ELEMENT_ANDEQL) a &=  b;
	if (typ == ELEMENT_PLUSEQ) a +=  b;
	if (typ == ELEMENT_MINUSE) a -=  b;
	if (typ == ELEMENT_SHLEQL) a <<= b;
	if (typ == ELEMENT_SAREQL) a >>= b;
	if (typ == ELEMENT_SLASHE) a /=  b;
	if (typ == ELEMENT_PRCNTE) a %=  b;
	if (typ == ELEMENT_OROR)   a = - (((a | b) & 1) != 0);
	if (typ == ELEMENT_ANDAND) a = - (((a & b) & 1) != 0);
	if (typ == ELEMENT_EEQUAL) a = - (a == b); 
	if (typ == ELEMENT_NOTEQL) a = - (a != b);
	if (typ == ELEMENT_LESS)   a = - (a <  b);
	if (typ == ELEMENT_GRTREQ) a = - (a >= b);
	if (typ == ELEMENT_LESSEQ) a = - (a <= b);
	if (typ == ELEMENT_GREATR) a = - (a >  b);
	if (typ == ELEMENT_NOT)    a ^= -1;
	if (typ == ELEMENT_TILDE)  a ^= -1;
	return a;
}

int getConstInt(const UCHAR **pp, char *errflg, int flags)
// flags = 1:shuntingYardの結果を表示.
//         2:RxxやPxxを定数変換する.
//         4:stringToElementsの結果を表示.
{
	struct Element e[10000], f[10000];
	int r = 0, i, stk[100], sp = 0;
	*pp = stringToElements(e, *pp, flags >> 8);
	if ((flags & 128) != 0) dumpElements(e);
	shuntingYard(f, e);
	if ((flags & 1) != 0) dumpElements(f);

	if (f[0].typ == ELEMENT_ERROR) {
err:
		*errflg = 1;
	} else {
		for (i = 0; f[i].typ != ELEMENT_TERMINATE; i++) {
			if (f[i].typ == ELEMENT_CONST) {
				stk[sp++] = f[i].iValue;
				continue;
			}
			if ((flags & 2) != 0) {
				if (f[i].typ == ELEMENT_REG) {
					stk[sp++] = f[i].iValue;
					continue;
				}
				if (f[i].typ == ELEMENT_PREG) {
					stk[sp++] = f[i].iValue + 0x40;
					continue;
				}
				if (f[i].typ == ELEMENT_DREG) {
					stk[sp++] = f[i].iValue + 0x80;
					continue;
				}
			}
			char ce = canEval(f[i].typ) & 3;
			if (ce == 1 && sp >= 1) {
				stk[sp - 1] = evalConstInt1(f[i].typ, stk[sp - 1]);
				continue;
			}
			if (ce == 2 && sp >= 2) {
				stk[sp - 2] = evalConstInt2(f[i].typ, stk[sp - 2], stk[sp - 1]);
				sp--;
				continue;
			}
			goto err;
		}
	}
	if (sp != 1)
		*errflg = 1;
	r = stk[0];
	return r;
}

int getConstInt1(const UCHAR **pp)
// カンマで止まる.
{
	char errflg = 0;
	return getConstInt(pp, &errflg, 1 << 8);
}

char cmpBytes(const UCHAR *p, const char *t)
{
	char retCode = 0, c;
	const char *t0 = t;
	while (*t != '\0') {
		if (*t == '_' || *t == ' ' || *t == '.') {
			t++;
			continue;
		}
		if (*t == '#') {
			p += t[1] - '0';
			t += 2;
			continue;
		}
		if (('0' <= *t && *t <= '9') || ('a' <= *t && *t <= 'f') || *t == 'x') {
			if (!(('0' <= t[1] && t[1] <= '9') || ('a' <= t[1] && t[1] <= 'f') || t[1] == 'x')) {
				fprintf(stderr, "cmpBytes: error: \"%s\"\n", t0);
				exit(1);
			}
			if (*t != 'x') {
				c = *t - '0';
				if (*t >= 'a')
					c = *t - ('a' - 10);
				if (((*p >> 4) & 0xf) != c) break;
			}
			t++;
			if (*t != 'x') {
				c = *t - '0';
				if (*t >= 'a')
					c = *t - ('a' - 10);
				if ((*p & 0xf) != c) break;
			}
			t++;
			p++;
			continue;
		}
		if (*t == 'Y' && t[1] == 'x') {
			if (*p < 0x80 || 0xbf < *p) break;
			t += 2;
			p++;
			continue;
		}
	}
	if (*t == '\0')
		retCode = 1;
	return retCode;
}

/*** aska ***/

struct Ident {
	int len0, len1;
	const UCHAR *p0, *p1;
	int depth;
	struct Element ele;
};

struct Brace {
	UCHAR typ, flg0;
	const UCHAR *p0, *p1;
};

UCHAR *askaPass1(struct Work *w);
int idendef(const char *p, void *dmy);
const UCHAR *idenref(const UCHAR *p, UCHAR *q, void *iden);
int isRxx(const UCHAR *p);
int isPxx(const UCHAR *p);
int isConst(const UCHAR *p);
const UCHAR *copyConst(const UCHAR *p, UCHAR **qq);
const UCHAR *skipSpace(const UCHAR *p);

int aska(struct Work *w)
{
	return putBuf(w->argOut, w->obuf, askaPass1(w));
}

int askaAllocTemp(UCHAR *a, int i1)
{
	int i, j = -1;
	for (i = 0; i < i1; i++) {
		if (a[i] < 0xfe) {
			j = a[i];
			a[i] = 0xfe;
			break;
		}
	}
	return j;
}

const char *getOpecode(const UCHAR typ)
{
	const char *r = "?";
	if (typ == ELEMENT_PLUS2)  r = "ADD";
	if (typ == ELEMENT_MINUS2) r = "SUB";
	if (typ == ELEMENT_PPLUS0) r = "ADD";
	if (typ == ELEMENT_PPLUS1) r = "ADD";
	if (typ == ELEMENT_MMINS0) r = "SUB";
	if (typ == ELEMENT_MMINS1) r = "SUB";
	if (typ == ELEMENT_PRDCT2) r = "MUL";
	if (typ == ELEMENT_OR)     r = "OR";
	if (typ == ELEMENT_AND)    r = "AND";
	if (typ == ELEMENT_XOR)    r = "XOR";
	if (typ == ELEMENT_SHL)    r = "SHL";
	if (typ == ELEMENT_SAR)    r = "SAR";
	if (typ == ELEMENT_SLASH)  r = "DIV";
	if (typ == ELEMENT_PERCNT) r = "MOD";
	if (typ == ELEMENT_OREQUL) r = "OR";
	if (typ == ELEMENT_XOREQL) r = "XOR";
	if (typ == ELEMENT_ANDEQL) r = "AND";
	if (typ == ELEMENT_PLUSEQ) r = "ADD";
	if (typ == ELEMENT_MINUSE) r = "SUB";
	if (typ == ELEMENT_PRDCTE) r = "MUL";
	if (typ == ELEMENT_SHLEQL) r = "SHL";
	if (typ == ELEMENT_SAREQL) r = "SAR";
	if (typ == ELEMENT_SLASHE) r = "DIV";
	if (typ == ELEMENT_PRCNTE) r = "MOD";
	if (typ == ELEMENT_EEQUAL) r = "CMPE";
	if (typ == ELEMENT_NOTEQL) r = "CMPNE";
	if (typ == ELEMENT_LESS)   r = "CMPL";
	if (typ == ELEMENT_GRTREQ) r = "CMPGE";
	if (typ == ELEMENT_LESSEQ) r = "CMPLE";
	if (typ == ELEMENT_GREATR) r = "CMPG";
	return r;
}

int askaPass1AllocStk(UCHAR *tmpR, UCHAR *tmpP, int typ)
{

}

char askaPass1FreeStk(struct Element *ele, UCHAR *tmpR, UCHAR *tmpP)
{
	char r = 0;
	int i;
	if ((ele->subTyp[2] & 0x03) == 0x01) {
		ele->subTyp[2] |= 0x02;
		if (ele->typ == ELEMENT_REG) {
			if (tmpR == NULL)
				r = 1;
			else {
				for (i = 0; tmpR[i] < 0xfe; i++);
				tmpR[i] = ele->iValue;
			}
		}
		if (ele->typ == ELEMENT_PREG) {
			if (tmpP == NULL)
				r = 1;
			else {
				for (i = 0; tmpP[i] < 0xfe; i++);
				tmpP[i] = ele->iValue;
			}
		}
	}
	return r;
}

char askaPass1Optimize(struct Element **pstk, struct Element *stk0, struct Element **pele, UCHAR *tmpR, UCHAR *tmpP);

char askaPass1PrefechConst(struct Element **pstk, struct Element **pele)
{
	struct Element *stk = *pstk, *ele = *pele, *stk0 = stk;
	char r = askaPass1Optimize(&stk, stk, &ele, NULL, NULL);
	if (r == 0 && stk == stk0 + 1 && stk[-1].typ == ELEMENT_CONST && (canEval(ele->typ) & 3) == 2) {
		*pstk = stk;
		*pele = ele;
	} else
		r = 1;
	return r;
}

int askaLog2(int i)
{
	int r = -1;
	if (i > 0 && ((i - 1) & i) == 0) {
		r = 0;
		while (i >= 2) {
			i >>= 1;
			r++;
		}
	}
	return r;
}

char askaPass1Optimize(struct Element **pstk, struct Element *stk0, struct Element **pele, UCHAR *tmpR, UCHAR *tmpP)
// 方針: 演算子列(ele)を書き換えずに済む範囲でのみ最適化.
{
	struct Element *stk = *pstk, *ele = *pele, tmpEle, *tele, *tstk;
	char r = 0, rr;
	int i;
	for (;;) {
		if (r != 0) break;
		UCHAR et = ele->typ, ce = canEval(et);
		if (et == ELEMENT_CONST || et == ELEMENT_REG || et == ELEMENT_PREG) {
			*stk = *ele;
			ele++;
			stk->subTyp[2] = 0;
			stk++;
			continue;
		}
		if ((ce & 4) != 0 && stk[-2].typ == ELEMENT_CONST) break; /* 定数に代入はできない */
		if ((ce & 3) == 1 && &stk[-1] < stk0) break; /* 項が足りない */
		if ((ce & 3) == 2 && &stk[-2] < stk0) break; /* 項が足りない */
		if ((ce & 3) == 1 && stk[-1].typ == ELEMENT_CONST) {
			stk[-1].iValue = evalConstInt1(et, stk[-1].iValue);
			ele++;
			continue;
		}
		if ((ce & 3) == 2 && stk[-2].typ == ELEMENT_CONST && stk[-1].typ == ELEMENT_CONST) {
			if ((ce & 16) != 0 && stk[-1].iValue == 0) break;
			stk[-2].iValue = evalConstInt2(et, stk[-2].iValue, stk[-1].iValue);
			ele++;
			stk--;
			continue;
		}
		if ((ce & (3 | 8)) == (2 | 8)) {
			rr = 0; // 可換2項演算子.
			if (stk[-2].typ == ELEMENT_CONST && stk[-1].typ != ELEMENT_CONST)  rr = 1; // 定数項は右側へ.
			if (stk[-2].typ != ELEMENT_PREG  && stk[-1].typ == ELEMENT_PREG)   rr = 1; // ポインタレジスタが混ざっているときは左側へ.
			if (stk[-2].typ == stk[-1].typ && stk[-2].iValue > stk[-1].iValue) rr = 1; // 同型の場合はレジスタ番号が小さいほうを左側に.
			if (rr != 0) {
				tmpEle  = stk[-2];
				stk[-2] = stk[-1];
				stk[-1] = tmpEle;
				continue;
			}
		}
		if ((ce & 3) == 1) {
			if (stk[-1].typ != ELEMENT_PREG && et == ELEMENT_PRDCT1) break;
			if (stk[-1].typ == ELEMENT_PREG) {
				if (et == ELEMENT_MINUS1 || et == ELEMENT_NOT || et == ELEMENT_TILDE) break;
			}
		}
		if ((ce & 3) == 2 && et != ELEMENT_COMMA1 && stk[-2].typ == ELEMENT_PREG) {
			if (stk[-1].typ == ELEMENT_PREG) {
				if ((ce & 32) == 0 && et != ELEMENT_MINUS2 && et != ELEMENT_EQUAL) break;
			}
			if (stk[-1].typ != ELEMENT_PREG) {
				if (et != ELEMENT_PLUS2 && et != ELEMENT_MINUS2 && et != ELEMENT_PLUSEQ && et != ELEMENT_MINUSE) break;
			}
		}
		if ((ce & 3) == 2 && stk[-2].typ == stk[-1].typ && stk[-2].iValue == stk[-1].iValue) {
			// 左辺と右辺で同じものが来た場合.
			static struct {
				int typ, value;
			} table0[] = {
				{ ELEMENT_MINUS2,  0 },
				{ ELEMENT_XOR,     0 },
				{ ELEMENT_EEQUAL, -1 },
				{ ELEMENT_NOTEQL,  0 },
				{ ELEMENT_LESS,    0 },
				{ ELEMENT_GRTREQ, -1 },
				{ ELEMENT_LESSEQ, -1 },
				{ ELEMENT_GREATR,  0 },
				{ 0, 0 }
			};
			for (i = 0; table0[i].typ != 0; i++) {
				if (table0[i].typ == et)
					break;
			}
			if (table0[i].typ != 0) {
				// 結果が定数になるパターン.
				ele++;
				r |= askaPass1FreeStk(&stk[-2], tmpR, tmpP);
				r |= askaPass1FreeStk(&stk[-1], tmpR, tmpP);
				stk--;
				stk[-1].typ = ELEMENT_CONST;
				stk[-1].subTyp[0] = ELEMENT_INT;
				stk[-1].iValue = table0[i].value;
				continue;
			}
			rr = 0;
			if (et == ELEMENT_EQUAL || et == ELEMENT_OREQUL || et == ELEMENT_ANDEQL) rr = 1;
			if (et == ELEMENT_OR || et == ELEMENT_AND) rr = 1;
			if (rr != 0) {
				// そのまま結果になるパターン.
				ele++;
				r |= askaPass1FreeStk(&stk[-1], tmpR, tmpP);
				stk--;
				continue;
			}
		}
		if ((ce & 3) == 2 && stk[-1].typ == ELEMENT_CONST) {
			// 特定の定数との演算.
			static struct {
				int typ, value;
			} table0[] = {
				{ ELEMENT_PLUS2,   0 },
				{ ELEMENT_MINUS2,  0 },
				{ ELEMENT_PRDCT2,  1 },
				{ ELEMENT_OR,      0 },
				{ ELEMENT_AND,    -1 },
				{ ELEMENT_XOR,     0 },
				{ ELEMENT_SHL,     0 },
				{ ELEMENT_SAR,     0 },
				{ ELEMENT_SLASH,   1 },
				{ ELEMENT_OREQUL,  0 },
				{ ELEMENT_XOREQL,  0 },
				{ ELEMENT_ANDEQL, -1 },
				{ ELEMENT_PLUSEQ,  0 },
				{ ELEMENT_MINUSE,  0 },
				{ ELEMENT_PRDCTE,  1 },
				{ ELEMENT_SHLEQL,  0 },
				{ ELEMENT_SAREQL,  0 },
				{ ELEMENT_SLASHE,  1 },
				{ 0, 0 }
			};
			for (i = 0; table0[i].typ != 0; i++) {
				if (table0[i].typ == et && table0[i].value == stk[-1].iValue)
					break;
			}
			if (table0[i].typ != 0) {
				// 演算しても左側の値が不変になるパターン.
				ele++;
				stk--;
				continue;
			}
			static struct {
				int typ, v0, v1;
			} table1[] = {
				{ ELEMENT_PRDCT2,  0,  0 },
				{ ELEMENT_OR,     -1, -1 },
				{ ELEMENT_AND,     0,  0 },
				{ ELEMENT_PERCNT,  1,  0 },
				{ ELEMENT_PERCNT, -1,  0 },
				{ 0, 0, 0 }
			};
			for (i = 0; table1[i].typ != 0; i++) {
				if (table1[i].typ == et && table1[i].v0 == stk[-1].iValue)
					break;
			}
			if (table1[i].typ != 0) {
				// 結果が定数になるパターン.
				ele++;
				stk--;
				stk[-1].typ = ELEMENT_CONST;
				stk[-1].subTyp[0] = ELEMENT_INT;
				stk[-1].iValue = table1[i].v1;
				continue;
			}
			// 定数の連続演算の結合.
			tele = ele + 1;
			tstk = stk;
			rr = askaPass1PrefechConst(&tstk, &tele); // 後続を仮評価する.
			if (rr == 0) {
				if (et == ELEMENT_PLUS2  && tele->typ == ELEMENT_PLUS2 ) {
					rr = 1;
					stk[-1].iValue += stk[0].iValue;
				}
				if (et == ELEMENT_PLUS2  && tele->typ == ELEMENT_MINUS2) {
					rr = 1;
					stk[-1].iValue = - (stk[-1].iValue - stk[0].iValue);
				}
				if (et == ELEMENT_MINUS2 && tele->typ == ELEMENT_PLUS2 ) {
					rr = 1;
					stk[-1].iValue = - stk[-1].iValue + stk[0].iValue;
				}
				if (et == ELEMENT_MINUS2 && tele->typ == ELEMENT_MINUS2) {
					rr = 1;
					stk[-1].iValue += stk[0].iValue;
				}
				if (et == ELEMENT_PRDCT2 && tele->typ == ELEMENT_PRDCT2) {
					rr = 1;
					stk[-1].iValue *= stk[0].iValue;
				}
				if (et == ELEMENT_PRDCT2 && tele->typ == ELEMENT_SHL   ) {
					i = askaLog2(stk[-1].iValue);
					if (i >= 0) {
						rr = 1;
						stk[-1].iValue = i + stk[0].iValue;
					}
				}
				if (et == ELEMENT_OR     && tele->typ == ELEMENT_OR    ) {
					rr = 1;
					stk[-1].iValue |= stk[0].iValue;
				}
				if (et == ELEMENT_AND    && tele->typ == ELEMENT_AND   ) {
					rr = 1;
					stk[-1].iValue &= stk[0].iValue;
				}
				if (et == ELEMENT_XOR    && tele->typ == ELEMENT_XOR   ) {
					rr = 1;
					stk[-1].iValue |= stk[0].iValue;
				}
				if (et == ELEMENT_SHL    && tele->typ == ELEMENT_SHL   ) {
					if (stk[-1].iValue >= 0 && stk[0].iValue >= 0) {
						rr = 1;
						stk[-1].iValue += stk[0].iValue;
					}
				}
				if (et == ELEMENT_SHL    && tele->typ == ELEMENT_PRDCT2) {
					if (stk[-1].iValue >= 0) {
						rr = 1;
						stk[-1].iValue = (1 << stk[-1].iValue) * stk[0].iValue;
					}
				}
				if (et == ELEMENT_SAR    && tele->typ == ELEMENT_SAR   ) {
					if (tstk[-2].iValue >= 0 && tstk[-1].iValue >= 0) {
						rr = 1;
						tstk[-2].iValue += tstk[-1].iValue;
					}
				}
				if (et == ELEMENT_SAR    && tele->typ == ELEMENT_SLASH ) {
					if (stk[-1].iValue >= 0) {
						rr = 1;
						stk[-1].iValue = (1 << stk[-1].iValue) * stk[0].iValue;
					}
				}
				if (rr != 0) {
					ele = tele;
					continue;
				}
			}
		}
		if (et == ELEMENT_PLUS1) {
			ele++;
			continue;
		}
		if (et == ELEMENT_COMMA1) {
			ele++;
			r |= askaPass1FreeStk(&stk[-2], tmpR, tmpP);
			stk[-2] = stk[-1];
			stk--;
			continue;
		}
		if (et == ELEMENT_SLASH) {
			// 未完成: 右シフトとの併用.
		}
		if (et == ELEMENT_PERCNT) {
			// 未完成: &との併用
		}
		// まだ考えてない: ELEMENT_PRDCT1, ELEMENT_OROR, ELEMENT_ANDAND
		break;
	}
	if (r == 0) {
		*pstk = stk;
		*pele = ele;
	}
	return r;
}

void askaPass1Optimize1(struct Element *stk, struct Element *ele)
{
	if (stk[0].typ == ELEMENT_REG && stk[1].typ == ELEMENT_CONST) {
		if (stk[1].iValue == 0 && (ele->typ == ELEMENT_PRDCTE || ele->typ == ELEMENT_ANDEQL)) {
			ele->typ = ELEMENT_EQUAL;
		}
		if (stk[1].iValue == -1 && ele->typ == ELEMENT_OREQUL) {
			ele->typ = ELEMENT_EQUAL;
		}
		if (ele->typ == ELEMENT_PRCNTE && (stk[1].iValue == 1 || stk[1].iValue == -1)) {
			ele->typ = ELEMENT_EQUAL;
			stk[1].iValue = 0;
		}
	}
	if (stk[0].typ == ELEMENT_REG && stk[1].typ == ELEMENT_REG && stk[0].iValue == stk[1].iValue) {
		if (ele->typ == ELEMENT_XOREQL || ele->typ == ELEMENT_MINUSE) {
			ele->typ = ELEMENT_EQUAL;
			stk[1].typ = ELEMENT_CONST;
			stk[1].subTyp[0] = ELEMENT_INT;
			stk[1].iValue = 0;
		}
		if (ele->typ == ELEMENT_SLASHE) {
			ele->typ = ELEMENT_EQUAL;
			stk[1].typ = ELEMENT_CONST;
			stk[1].subTyp[0] = ELEMENT_INT;
			stk[1].iValue = 1;
		}
	}
	return;
}

void askaPass1OptimizeOpecode(UCHAR *q, struct Element *stk)
{
	if (stk->typ == ELEMENT_CONST) {
		int i = askaLog2(stk->iValue);
		if (strcmp(q, "SUB") == 0 && stk->iValue < 0) {
			strcpy(q, "ADD");
			stk->iValue *= -1;
		}
		if (strcmp(q, "ADD") == 0 && stk->iValue < 0) {
			strcpy(q, "SUB");
			stk->iValue *= -1;
		}
		if (strcmp(q, "MUL") == 0 && i >= 0) {
			strcpy(q, "SHL");
			stk->iValue = i;
		}
		if (strcmp(q, "DIV") == 0 && stk->iValue == -1) {
			strcpy(q, "MUL");
		}
		if (strcmp(q, "DIV") == 0 && i >= 0) {
			strcpy(q, "SAR");
			stk->iValue = i;
		}
		// MOD→AND変換はできない.
		// -123%8 と -123&7 は異なる結果なので、一般にx%8==x&7とは言えないから.
	}
}

int substitute(struct Element *ele, struct Ident *ident)
{
	int i, j;
	int r = -1;
	for (i = 0; ele[i].typ != ELEMENT_TERMINATE && ele[i].typ != ELEMENT_ERROR; i++) {
		if (ele[i].typ == ELEMENT_IDENT) {
			for (j = 0; j < MAXIDENTS; j++) {
				if (ident[j].len0 == ele[i].subValue && strncmp(ident[j].p0, ele[i].pValue, ele[i].subValue) == 0)
					break;
			}
			if (j >= MAXIDENTS) {
				r = i;
				break;
			}
			ele[i] = ident[j].ele;
		}
	}
	return r;
}

const UCHAR *askaPass1Sub(struct Work *w, const UCHAR *p, UCHAR **qq, struct Ident *ident, int flags)
{
	int i, j, dsp;
	UCHAR tmpR[16], tmpP[16];
	for (i = 0; i < 16; i++)
		tmpP[i] = tmpR[i] = 0xff;

	w->sp = -2;
	UCHAR *q = *qq;
	struct Element *ele0 = w->ele0, *ele1 = w->ele1, tmpEle, *prm0, *next;
	const UCHAR *r = stringToElements(ele0, p, flags);
	if (ele0[0].typ == ELEMENT_FOR || ele0[0].typ == ELEMENT_IF ||
		ele0[0].typ == ELEMENT_BREAK || ele0[0].typ == ELEMENT_CONTIN || ele0[0].typ == ELEMENT_CONTI0 || ele0[0].typ == ELEMENT_GOTO) goto fin;
	i = substitute(ele0, ident);
	if (i >= 0) {
		if (i == 0 && ele0[1].typ == ELEMENT_PRNTH0) goto fin;
		fprintf(stderr, "aska: undeclared: %.*s\n", ele0[i].subValue, ele0[i].pValue);
		goto exp_err;
	}
	j = 0;
	if (ele0[0].typ == ELEMENT_REG && ele0[1].typ == ELEMENT_EQUAL) {
		j = 1;
		for (i = 2; ele0[i].typ != ELEMENT_TERMINATE && ele0[i].typ != ELEMENT_ERROR; i++) {
			if (ele0[i].typ == ele0[0].typ && ele0[i].iValue == ele0[0].iValue)
				j = 0;
		}
	}
	for (i = 1; i <= 12; i++)
		tmpR[i] = tmpP[i] = 0x3c - i;
	tmpR[0] = 0xfe;
	if (j != 0)
		tmpR[0] = ele0[0].iValue;
	j = 0;
	if (ele0[0].typ == ELEMENT_PREG && ele0[1].typ == ELEMENT_EQUAL) {
		j = 1;
		for (i = 2; ele0[i].typ != ELEMENT_TERMINATE && ele0[i].typ != ELEMENT_ERROR; i++) {
			if (ele0[i].typ == ele0[0].typ && ele0[i].iValue == ele0[0].iValue)
				j = 0;
		}
	}
	tmpP[0] = 0xfe;
	if (j != 0)
		tmpP[0] = ele0[0].iValue;
	const struct Element *c_ele = shuntingYard(ele1, ele0);
	if ((flags & 256) != 0) {
		for (i = 0; ele1[i].typ != ELEMENT_TERMINATE && ele1[i].typ != ELEMENT_ERROR; i++);
		ele1[i + 1] = ele1[i];
		ele1[i].typ = ELEMENT_NOT;
	}
	if (ele1->typ != ELEMENT_ERROR && c_ele->typ == ELEMENT_TERMINATE) {
		int sp = 0;
		for (i = 0; ele1[i].typ != ELEMENT_TERMINATE; i++) {
			struct Element *tstk = &ele0[sp], *tele = &ele1[i];
			askaPass1Optimize(&tstk, ele0, &tele, tmpR, tmpP);
			sp = tstk - ele0;
			i = tele - ele1;
			if (tele->typ == ELEMENT_TERMINATE) break;
			char ce = canEval(ele1[i].typ);
			if (ce == 1 && sp > 0) {
				if (ele0[sp - 1].typ == ELEMENT_REG && ele1[i].typ == ELEMENT_PLUS1) continue;
				if (ele1[i].subTyp[2] != 0) goto exp_giveup;
				if (ele0[sp - 1].typ == ELEMENT_REG) {
					if (ele1[i].typ == ELEMENT_MINUS1) {
						askaPass1FreeStk(&ele0[sp - 1], tmpR, tmpP);
						j = askaAllocTemp(tmpR, 16);
						if (j < 0) goto exp_giveup;
						sprintf(q, "MULI(32,R%02X,R%02X,-1);", j, ele0[sp - 1].iValue);
						q += 17;
						ele0[sp - 1].subTyp[2] = 1;
						ele0[sp - 1].iValue = j;
						continue;
					}
					if (ele1[i].typ == ELEMENT_NOT || ele1[i].typ == ELEMENT_TILDE) {
						askaPass1FreeStk(&ele0[sp - 1], tmpR, tmpP);
						j = askaAllocTemp(tmpR, 16);
						if (j < 0) goto exp_giveup;
						sprintf(q, "XORI(32,R%02X,R%02X,-1);", j, ele0[sp - 1].iValue);
						q += 17;
						ele0[sp - 1].subTyp[2] = 1;
						ele0[sp - 1].iValue = j;
						continue;
					}
				}
			}
			if ((ce & 16) != 0 && ele0[sp - 1].typ == ELEMENT_CONST && ele0[sp - 1].iValue == 0) goto exp_div0;
			// OROR, ANDANDは処理が難しい.というかめんどくさい.
			if (sp >= 2)
				askaPass1Optimize1(&ele0[sp - 2], &ele1[i]);
			if ((ce & 3) == 2 && sp >= 2 && ele1[i].typ != ELEMENT_EQUAL) {
				prm0 = NULL;
				next = &ele1[i + 1];
				dsp = 1;
				if ((ce & 4) != 0 && ele0[sp - 2].typ != ELEMENT_CONST)
					prm0 = &ele0[sp - 2]; // 代入系.
				if ((ce & 32) != 0) {
					// 比較系
					if ((ce & 8) == 0) {
						j = 0;
						if (ele0[sp - 2].typ == ELEMENT_CONST && ele0[sp - 1].typ == ELEMENT_REG) j |= 1;
						if (ele0[sp - 2].typ == ELEMENT_REG   && ele0[sp - 1].typ == ELEMENT_REG  && ele0[sp - 2].iValue > ele0[sp - 1].iValue) j |= 1;
						if (ele0[sp - 2].typ == ELEMENT_PREG  && ele0[sp - 1].typ == ELEMENT_PREG && ele0[sp - 2].iValue > ele0[sp - 1].iValue) j |= 1;
						if (j != 0) {
							tmpEle = ele0[sp - 2];
							ele0[sp - 2] = ele0[sp - 1];
							ele0[sp - 1] = tmpEle;
							ele1[i].typ ^= 127;
						}
					}
					while (next->typ == ELEMENT_NOT) {
						next++;
						ele1[i].typ ^= 1;
					}
				}
				if (sp >= 3 && next->typ == ELEMENT_EQUAL && ele0[sp - 3].typ == ELEMENT_REG) {
					prm0 = &ele0[sp - 3];
					next++;
					dsp++;
				}
				if (prm0 == NULL) {
					if (ele1[i].subTyp[2] != 0) goto exp_giveup;
					askaPass1FreeStk(&ele0[sp - 2], tmpR, tmpP);
					askaPass1FreeStk(&ele0[sp - 1], tmpR, tmpP);
					j = askaAllocTemp(tmpR, 16);
					if (j < 0) goto exp_giveup;
					sp++;
					ele0[sp - 1] = ele0[sp - 2];
					ele0[sp - 2] = ele0[sp - 3];
					ele0[sp - 3].typ = ELEMENT_REG;
					ele0[sp - 3].subTyp[2] = 1;
					ele0[sp - 3].iValue = j;
					dsp++;
					prm0 = &ele0[sp - 3];
				}
				if (ele0[sp - 2].typ == ELEMENT_CONST) {
					sprintf(q, "LIMM(32,R3F,0x%X);", ele0[sp - 2].iValue);
					while (*q != 0) q++;
					ele0[sp - 2].typ = ELEMENT_REG;
					ele0[sp - 2].subTyp[2] = 0;
					ele0[sp - 2].iValue = 0x3f;
				}
				if (ele0[sp - 2].typ == ELEMENT_PREG) {
					*q = 'P';
					strcpy(q + 1, getOpecode(ele1[i].typ));
				} else {
					strcpy(q, getOpecode(ele1[i].typ));
					askaPass1OptimizeOpecode(q, &ele0[sp - 1]);
				}
				i &= 0;
				if (strncmp(q, "CMP", 3) == 0) i |= 1;
				while (*q != 0) q++;
				if (i == 0) {
					if (ele0[sp - 2].typ == ELEMENT_REG  && ele0[sp - 1].typ == ELEMENT_CONST)
						sprintf(q, "I(32,R%02X,R%02X,0x%X);", prm0->iValue, ele0[sp - 2].iValue, ele0[sp - 1].iValue);
					if (ele0[sp - 2].typ == ELEMENT_REG  && ele0[sp - 1].typ == ELEMENT_REG)
						sprintf(q, "(32,R%02X,R%02X,R%02X);", prm0->iValue, ele0[sp - 2].iValue, ele0[sp - 1].iValue);
					if (ele0[sp - 2].typ == ELEMENT_PREG && ele0[sp - 1].typ == ELEMENT_PREG)
						sprintf(q, "(32,R%02X,P%02X,P%02X);", prm0->iValue, ele0[sp - 2].iValue, ele0[sp - 1].iValue);
				} else {
					if (ele0[sp - 2].typ == ELEMENT_REG  && ele0[sp - 1].typ == ELEMENT_CONST)
						sprintf(q, "I(32,32,R%02X,R%02X,0x%X);", prm0->iValue, ele0[sp - 2].iValue, ele0[sp - 1].iValue);
					if (ele0[sp - 2].typ == ELEMENT_REG  && ele0[sp - 1].typ == ELEMENT_REG)
						sprintf(q, "(32,32,R%02X,R%02X,R%02X);", prm0->iValue, ele0[sp - 2].iValue, ele0[sp - 1].iValue);
				}
				while (*q != 0) q++;
				do {
					askaPass1FreeStk(&ele0[sp - 1], tmpR, tmpP);
					sp--;
				} while (--dsp != 0);
				i = next - ele1 - 1;
				continue;
			}
			if (ele1[i].typ == ELEMENT_EQUAL && sp >= 2) {
				if (ele0[sp - 2].typ == ELEMENT_REG && ele0[sp - 1].typ == ELEMENT_CONST) {
					sprintf(q, "LIMM(32,R%02X,0x%X);", ele0[sp - 2].iValue, ele0[sp - 1].iValue);
					while (*q != 0) q++;
					ele0[sp - 2] = ele0[sp - 1];
					sp--;
					continue;
				}
				if (ele0[sp - 2].typ == ELEMENT_REG && ele0[sp - 1].typ == ELEMENT_REG) {
					// 未完成: どっちを残すか、もっとうまく判断させたい...たとえばレジスタ番号の若い方とか.
					if (ele0[sp - 1].subTyp[2] != 0) tmpR[ele0[sp - 1].iValue]--;
					if (ele0[sp - 2].iValue != ele0[sp - 1].iValue) {
						sprintf(q, "CP(32,R%02X,R%02X);", ele0[sp - 2].iValue, ele0[sp - 1].iValue);
						q += 15;
						ele0[sp - 2] = ele0[sp - 1];
					}
					sp--;
					continue;
				}
				if (ele0[sp - 2].typ == ELEMENT_PREG && ele0[sp - 1].typ == ELEMENT_PREG) {
					// 未完成: どっちを残すか、もっとうまく判断させたい...たとえばレジスタ番号の若い方とか.
					if (ele0[sp - 1].subTyp[2] != 0) tmpP[ele0[sp - 1].iValue]--;
					sprintf(q, "PCP(P%02X,P%02X);", ele0[sp - 2].iValue, ele0[sp - 1].iValue);
					q += 13;
					ele0[sp - 2] = ele0[sp - 1];
					sp--;
					continue;
				}
			}
			if ((ele1[i].typ == ELEMENT_PPLUS0 || ele1[i].typ == ELEMENT_MMINS0) && sp > 0 && ele0[sp - 1].typ == ELEMENT_REG) {
pplus1:
				j = 1;
				if (ele1[i].typ == ELEMENT_MMINS0 || ele1[i].typ == ELEMENT_MMINS1) j = -1;
				sprintf(q, "ADDI(32,R%02X,R%02X,0x%08X);", ele0[sp - 1].iValue, ele0[sp - 1].iValue, j);
				q += 28;
				continue;
			}
			if ((ele1[i].typ == ELEMENT_PPLUS1 || ele1[i].typ == ELEMENT_MMINS1) && sp > 0 && ele0[sp - 1].typ == ELEMENT_REG) {
				if (ele1[i + 1].typ == ELEMENT_TERMINATE || ele1[i + 1].typ == ELEMENT_COMMA1) goto pplus1;
				/* テンポラリを使って処理 */
			}
			goto exp_err;
		}
		w->sp = sp;
		if (sp >= 2) {
			q = strcpy1(q, "DB(0xef);");
		}
		goto fin;
exp_giveup:
		/* giveupコードを出力 */
		q = strcpy1(q, "DB(0xed);");
		goto exp_skip;
exp_div0:
		q = strcpy1(q, "DB(0xee);");
		goto exp_skip;
exp_err:
		q = strcpy1(q, "DB(0xef);");
exp_skip:
		w->sp = -1;
	}
fin:
	*qq = q;
	return r;
}

void askaPass1UseR3F(struct Element *stk, UCHAR *q, UCHAR *q0)
{
	if (stk->typ == ELEMENT_REG && (stk->subTyp[2] & 1) != 0 && q - 17 >= q0 && q[-2] == ')' && q[-1] == ';') {	// CMP(R00,R00,R00);
		q -= 3;
		while (*q != '(' && q > q0) q--;
		if (*q == '(') {
			q--;
			while (q > q0 && *q != ';' && *q > ' ') q--;
			if (strncmp1(q + 1, "CMP") == 0) {
				while (*q != '(') q++;
				if (q[1+6] == 'R') {
					q[2+6] = '3';
					q[3+6] = 'F';
					stk->iValue = 0x3f;
				}
			}
			if (strncmp1(q + 1, "PCMP") == 0) {
				while (*q != '(') q++;
				if (q[1+3] == 'R') {
					q[2+3] = '3';
					q[3+3] = 'F';
					stk->iValue = 0x3f;
				}
			}
		}
	}
	return;
}

UCHAR *askaPass1(struct Work *w)
{
	char code = 0, underscore = 1, flag, isWaitBrase0 = 0;
	const UCHAR *p = w->ibuf, *r, *s, *t, *braceP;
	UCHAR *q = w->obuf, *qq;
	struct Ident *ident = malloc(MAXIDENTS * sizeof (struct Ident));
	struct Brace *brase = malloc(128 * sizeof (struct Brace));
	int i, j, k, braseDepth = 0;
	struct Element *ele0 = w->ele0;
	brase[0].typ = 0;
	for (i = 0; i < MAXIDENTS; i++) {
		ident[i].len0 = 0;
	}
	if (strstr(p, "/*") != NULL) {
		// これはpreproで処分させる.
		fputs("find: '/*'\n", stderr);
		goto err;
	}
	while (*p != '\0') {
		p = skipSpace(p);
		if (*p == '\n' || *p == '\r') {
			*q++ = *p++;
			underscore = 1;
			continue;
		}
		if (*p == '#') {
			do {
				while (*p != '\n' && *p != '\r' && *p != '\0')
					*q++ = *p++;
				flag = 0;
				if (p[-1] == '\\')
					flag = 1;
				while (*p == '\n' || *p == '\r')
					*q++ = *p++;
			} while (flag != 0 && *p != '\0');
			underscore = 1;
			continue;
		}
		if (p[0] == '/' && p[1] == '/') {
			while (*p != '\n' && *p != '\r' && *p != '\0')
				*q++ = *p++;
			while (*p == '\n' || *p == '\r')
				*q++ = *p++;
			underscore = 1;
			continue;
		}
		if (strncmp1(p, "OSECPU_HEADER();") == 0) {
			code = 1;
			memcpy(q, p, 16);
			p += 16;
			q += 16;
			continue;
		}
		if (strncmp1(p, "%include") == 0 || strncmp1(p, "%define") == 0 || strncmp1(p, "%undef") == 0) {
			*q++ = '#';
			p++;
			while (*p != '\n' && *p != '\r' && *p != '\0')
				*q++ = *p++;
			while (*p == '\n' || *p == '\r')
				*q++ = *p++;
			underscore = 1;
			continue;
		}
		if (strncmp1(p, "aska_code(1);") == 0) {
			code = 1;
			p += 13;
			continue;
		}
		if (strncmp1(p, "aska_code(0);") == 0) {
			code = 0;
			p += 13;
			continue;
		}
		if (code != 0 && underscore != 0) {
			*q++ = '_';
			*q++ = ' ';
			underscore = 0;
		}

		if (isWaitBrase0 != 0) {
			if (*p == '{')
				p++;
			else {
				q = strcpy1(q, "DB(0xef);");
			}
			isWaitBrase0 = 0;
			continue;
		}
		if (*p == '}') {
			p = skipSpace(p + 1);
			if (braseDepth > 0 && brase[braseDepth].typ == ELEMENT_FOR) {
				q = strcpy1(q, "LB0(CONTINUE);");
				if ((brase[braseDepth].flg0 & 2) != 0) { // using remark
					q = strcpy1(q, "REM03();");
				}
				if (brase[braseDepth].p1[0] != ')') {
					askaPass1Sub(w, brase[braseDepth].p1, &q, ident, 0);
				}
				askaPass1Sub(w, brase[braseDepth].p0, &q, ident, 0);
				if (w->sp == 0) {
					ele0[0].typ = ELEMENT_CONST;
					ele0[0].iValue = -1;
					w->sp++;
				}
				askaPass1UseR3F(&ele0[0], q, w->obuf);
				if (ele0[0].typ == ELEMENT_CONST) {
					if ((ele0[0].iValue & 1) != 0) {
						q = strcpy1(q, "JMP(CONTINUE0);");
					}
				} else if (ele0[0].typ == ELEMENT_REG) {
					sprintf(q, "CND(R%02X);JMP(CONTINUE0);", ele0[0].iValue);
					q += 24;
				} else {
					q = strcpy1(q, "DB(0xef);");
				}
				if ((brase[braseDepth].flg0 & 1) != 0)
					q = strcpy1(q, "ENDLOOP();");
				else
					q = strcpy1(q, "ENDLOOP0();");
			}
			if (braseDepth > 0 && brase[braseDepth].typ == ELEMENT_IF) {
				if (strncmp1(p, "else") == 0) {
					p += 4;
					braseDepth--;
					// 変数定義の破棄.
					for (i = 0; i < MAXIDENTS; i++) {
						if (ident[i].len0 > 0 && ident[i].depth > braseDepth)
							ident[i].len0 = 0;
					}
					braseDepth++;
					brase[braseDepth].typ = ELEMENT_ELSE;
					isWaitBrase0 = 1;
					q = strcpy1(q, "JMP(lbstk1(1,1));LB(0,lbstk1(1,0));");
					continue;
				} else {
					q = strcpy1(q, "LB(0,lbstk1(1,0));lbstk3(1);");
				}
			}
			if (braseDepth > 0 && brase[braseDepth].typ == ELEMENT_ELSE) {
				q = strcpy1(q, "LB(0,lbstk1(1,1));lbstk3(1);");
			}
			if (braseDepth > 0) {
				braseDepth--;
				// 変数定義の破棄.
				for (i = 0; i < MAXIDENTS; i++) {
					if (ident[i].len0 > 0 && ident[i].depth > braseDepth)
						ident[i].len0 = 0;
				}
			} else {
				q = strcpy1(q, "DB(0xef);");
			}
			continue;
		}

		flag = 0;
#if 1
		for (r = p; *r != '\0' && *r != '\r' && *r != '\n' && *r != ';'; r++) {
			if (*r == '\"')
				flag = 1;
		}
		if (/* *r != ';' || */ flag != 0) {
			while (p < r)
				*q++ = *p++;
			continue;
		}
#endif

#if 0
		if (strncmp1(p, "int") == 0 && p[3] <= ' ') {
			i = idendef(p + 4, iden);
			if (i != 0) goto err;
			r = p;
			continue;
		}
#endif

		if (strncmp1(p, "SInt32") == 0 && p[6] <= ' ') {
			p += 6;
			k = 'R';
identLoop:
			for (;;) {
				p = skipSpace(p);
				r = p;
				for (;;) {
					if (*p <= ' ' || isSymbolChar(*p)) break;
					p++;
				}
				j = p - r;
				p = skipSpace(p);
				if (*p != ':') goto errSkip;
				p = skipSpace(p + 1);
				if (j == 0) goto errSkip;
				if (*p != k || hexChar(p[1]) < 0 || hexChar(p[2]) < 0 || (!isSymbolChar(p[3]) && p[3] > ' ')) goto errSkip;
				for (i = 0; i < MAXIDENTS; i++) {
					if (ident[i].len0 == j && strncmp(ident[i].p0, r, j) == 0) break;
				}
				if (i < MAXIDENTS) {
					fprintf(stderr, "aska: SInt32/VPtr: redefine: %.*s\n", j, r);
					goto identSkip;
				} else {
					for (i = 0; i < MAXIDENTS; i++) {
						if (ident[i].len0 == 0) break;
					}
					if (i >= MAXIDENTS) {
						fputs("aska: SInt32/VPtr: too many idents\n", stderr);
						exit(1);
					}
					ident[i].len0 = j;
					ident[i].p0 = r;
					ident[i].ele.iValue = hexChar(p[1]) << 4 | hexChar(p[2]);
					ident[i].depth = braseDepth;
					if (*p == 'R')
						ident[i].ele.typ = ELEMENT_REG;
					if (*p == 'P')
						ident[i].ele.typ = ELEMENT_PREG;
				}
identSkip:
				p = skipSpace(p + 3);
				if (*p == ';') break;
				if (*p != ',') goto errSkip;
				p++;
			}
			continue;
		}
		if (strncmp1(p, "VPtr") == 0 && p[4] <= ' ') {
			p += 4;
			k = 'P';
			goto identLoop;
		}
		static UCHAR *table_a[] = {
			"PLIMM", "CND", "LMEM", "SMEM", "PADD", "PADDI", "PDIF",
			"DB", "DDBE", "PJMP", "PCALL", "PLMEM", "PSMEM", NULL
			// もっと追加してもいいが、とりあえず試験的にここまでとする.
		};
		for (j = 0; (r = table_a[j]) != NULL; j++) {
			k = strlen(r);
			if (strncmp(p, r, k) == 0 && isSymbolChar(p[k]) != 0) break;
		}
		if (r != NULL) {
			if (p[k] == '%' && p[k + 1] == '(') {
				strcpy(q, r);
				q += k;
				p += k + 1;
				goto through;
			}
			if (p[k] == '(') {
				strcpy(q, r);
				q += k;
				p += k;
				j = 0;
				while (*p != ';' && *p != '\n' && *p != '\r' && *p != '\0') {
					if (j == 1 && isSymbolChar(*p) == 0 && *p > ' ') {
						for (i = 0; i < MAXIDENTS; i++) {
							k = ident[i].len0;
							if (k == 0) continue;
							if (strncmp(ident[i].p0, p, k) == 0 && (isSymbolChar(p[k]) != 0 || p[k] <= ' ')) break;
						}
						if (i < MAXIDENTS && k > 0) {
							p += k;
							if (ident[i].ele.typ == ELEMENT_REG) {
								sprintf(q, "R%02X", ident[i].ele.iValue);
								q += 3;
							}
							if (ident[i].ele.typ == ELEMENT_PREG) {
								sprintf(q, "P%02X", ident[i].ele.iValue);
								q += 3;
							}
							j = 0;
							continue;
						}
					}
					if (isSymbolChar(*p) != 0) j = 1;
					if (isSymbolChar(*p) == 0 && *p > ' ') j = 0;
					*q++ = *p++;
				}
				if (*p == ';') *q++ = *p++;
				continue;
			}
		}
		r = askaPass1Sub(w, p, &q, ident, 0);
		if (w->sp <= -2) {
			s = p;
			if (w->ele0[0].typ == ELEMENT_BREAK && strncmp1(p, "break") == 0) {
				p = skipSpace(p + 5);
				if (*p != ';') goto errSkip;
				p++;
				q = strcpy1(q, "JMP(BREAK);");
				for (j = braseDepth; j >= 0 && brase[j].typ != ELEMENT_FOR; j--);
				if (j >= 0) brase[j].flg0 |= 1; // using break
				continue;
			}
			if (w->ele0[0].typ == ELEMENT_CONTIN && strncmp1(p, "continue") == 0) {
				p = skipSpace(p + 8);
				if (*p != ';') goto errSkip;
				p++;
				q = strcpy1(q, "JMP(CONTINUE);");
				continue;
			}
			if (w->ele0[0].typ == ELEMENT_CONTI0 && strncmp1(p, "continue0") == 0) {
				p = skipSpace(p + 9);
				if (*p != ';') goto errSkip;
				p++;
				q = strcpy1(q, "JMP(CONTINUE0);");
				continue;
			}
			if (w->ele0[0].typ == ELEMENT_GOTO && strncmp1(p, "goto") == 0) {
				p = skipSpace(p + 4);
				q = strcpy1(q, "JMP(L_");
				while (*p > ' ' && !isSymbolChar(*p)) *q++ = *p++;
				if (*p != ';') goto errSkip;
				p++;
				*q++ = ')';
				*q++ = ';';
				continue;
			}
			if (w->ele0[0].typ == ELEMENT_IF && strncmp1(p, "if") == 0) {
				p = skipSpace(p + 2);
				if (*p != '(') goto errSkip;
				t = skipSpace(p + 1);
				p = stringToElements(w->ele0, t, 0);
				if (*p != ')') goto errSkip;
				p = skipSpace(p + 1);
				j = 0;
				if (strncmp1(p, "continue")  == 0) { j = 1; p += 8; }
				if (strncmp1(p, "continue0") == 0) { j = 4; p += 9; }
				if (strncmp1(p, "break")     == 0) { j = 2; p += 5; }
				if (strncmp1(p, "goto")      == 0) { j = 3; p += 4; }
				if (j > 0) {
					askaPass1Sub(w, t, &q, ident, 0);
					if (w->sp <= 0) goto errSkip;
					askaPass1UseR3F(&ele0[0], q, w->obuf);
					if (ele0[0].typ == ELEMENT_CONST) {
						if ((ele0[0].iValue & 1) == 0)
							j |= 8;
					} else if (ele0[0].typ == ELEMENT_REG) {
						sprintf(q, "CND(R%02X);", ele0[0].iValue);
						q += 9;
					} else
						goto errSkip;
					p = skipSpace(p);
					if (j == 1) q = strcpy1(q, "JMP(CONTINUE");
					if (j == 2) q = strcpy1(q, "JMP(BREAK");
					if (j == 3) q = strcpy1(q, "JMP(L_");
					if (j == 4) q = strcpy1(q, "JMP(CONTINUE0");
					while (*p > ' ' && !isSymbolChar(*p)) *q++ = *p++;
					if (*p != ';') goto errSkip;
					p++;
					*q++ = ')';
					*q++ = ';';
					if (j == 2) {
						for (j = braseDepth; j >= 0 && brase[j].typ != ELEMENT_FOR; j--);
						if (j >= 0) brase[j].flg0 |= 1; // using break
					}
					continue;
				}
				q = strcpy1(q, "lbstk2(1,2);");
				askaPass1Sub(w, t, &q, ident, 256); // 256:否定型にする.
				if (w->sp <= 0) goto errSkip;
				askaPass1UseR3F(&ele0[0], q, w->obuf);
				if (ele0[0].typ == ELEMENT_CONST) {
					if ((ele0[0].iValue & 1) == 0) {
						strcpy(q, "JMP(lbstk1(1,0));");
						q += 17;
					}
				} else if (ele0[0].typ == ELEMENT_REG) {
					sprintf(q, "CND(R%02X);JMP(lbstk1(1,0));", ele0[0].iValue);
					q += 9+17;
				} else
					goto errSkip;
				brase[braseDepth + 1].typ = ELEMENT_IF;
				isWaitBrase0 = 1;
				braseDepth++;
				continue;
			}
			if (w->ele0[0].typ == ELEMENT_FOR && strncmp1(p, "for") == 0) {
				p = skipSpace(p + 3);
				if (*p != '(') goto errSkip;
				p = skipSpace(p + 1);
				qq = q;
				p = askaPass1Sub(w, p, &q, ident, 0);
				if (*p != ';') goto errSkip;
				i = -1;
				j = 0;
				if (w->ele1[0].typ == ELEMENT_REG && w->ele1[1].typ == ELEMENT_CONST && w->ele1[2].typ == ELEMENT_EQUAL && w->ele1[3].typ == ELEMENT_TERMINATE) {
					i = w->ele1[0].iValue;
					j = w->ele1[1].iValue;
				}
				p = skipSpace(p + 1);
				brase[braseDepth + 1].typ = ELEMENT_FOR;
				brase[braseDepth + 1].flg0 = 0;
				brase[braseDepth + 1].p0 = p;
				p = stringToElements(w->ele0, p, 0);
				substitute(w->ele0, ident);
				if (*p != ';') goto errSkip;
				k = 0;
				if (i >= 0) {
					if (w->ele0[0].typ == ELEMENT_REG && w->ele0[0].iValue == i &&
						w->ele0[1].typ == ELEMENT_NOTEQL && w->ele0[2].typ == ELEMENT_CONST && w->ele0[3].typ == ELEMENT_TERMINATE)
					{
						k = w->ele0[2].iValue;
						j = k - j;
					} else if (w->ele0[0].typ == ELEMENT_REG && w->ele0[0].iValue == i &&
						w->ele0[1].typ == ELEMENT_NOTEQL && w->ele0[2].typ == ELEMENT_REG && w->ele0[3].typ == ELEMENT_TERMINATE)
					{
						k = w->ele0[2].iValue | 0x80520000;
						j = 1;
					} else
						i = -1;
				}
				p = skipSpace(p + 1);
				brase[braseDepth + 1].p1 = p;
				p = stringToElements(w->ele0, p, 0);
				substitute(w->ele0, ident);
				if (*p != ')') goto errSkip;
				p = skipSpace(p + 1);
				if (i >= 0) {
					if (j > 0) {
						if (w->ele0[0].typ == ELEMENT_REG && w->ele0[0].iValue == i && w->ele0[1].typ == ELEMENT_PPLUS)
							;
						else if (w->ele0[1].typ == ELEMENT_PPLUS && w->ele0[1].typ == ELEMENT_REG && w->ele0[1].iValue == i)
							;
						else
							i = -1;
					}
					if (j < 0) {
						if (w->ele0[0].typ == ELEMENT_REG && w->ele0[0].iValue == i && w->ele0[1].typ == ELEMENT_MMINUS)
							;
						else if (w->ele0[1].typ == ELEMENT_MMINUS && w->ele0[1].typ == ELEMENT_REG && w->ele0[1].iValue == i)
							;
						else
							i = -1;
					}
				}
				if (i >= 0) {
					brase[braseDepth + 1].flg0 |= 2; // using remark
					for (j = q - qq; j >= 0; j--)
						qq[j + 46*0+18] = qq[j];
					char tmpStr[52];
					sprintf(tmpStr, "REM02(0x%08X);", k);
					// printf("strlen=%d\n", strlen(tmpStr));
				//	memcpy(qq, tmpStr, 46); q += 46;
					memcpy(qq, tmpStr, 18); q += 18;
					q = strcpy1(q, "lbstk2(0,3); LB(2,lbstk1(0,0));");
				} else
					q = strcpy1(q, "LOOP();");
				isWaitBrase0 = 1;
				braseDepth++;
				continue;
			}
#if 0
			if (strncmp1(p, "askaDirect") == 0) {
				p = skipSpace(p + 10);
				if (*p != '(') goto errSkip;
				p = skipSpace(p + 1);
				i = 0;
				while ('0' <= *p && *p <= '9') {
					i = i * 10 + (*p - '0');
					p++;
				}
				p = skipSpace(p);
				if (p != ',') goto errSkip;
				p = askaPass1Sub(w, p, &q, ident, 0);
				if (*p != ')') goto errSkip;


				if (strncmp1(p, "32") == 0) { i = 32; p += 2; }
				else if (strncmp1(p, "32") == 0) { i = 32; p += 2; }

			}
#endif

errSkip:
			w->sp = -2;
			p = s;
		}
		if (w->sp <= -2 || p >= r) {
through:
			while (*p != ';' && *p != '\n' && *p != '\r' && *p != '\0')
				*q++ = *p++;
			if (*p == ';') *q++ = *p++;
			continue;
#if 0
			if (!(q - 9 >= w->obuf && strncmp1(q - 9, "DB(0xef);") == 0)) {
				strcpy(q, "DB(0xef);");
				q += 9;
			}
			r++; // 突破できない文字列を突破させる
#endif
		}
		p = r;
		if (*p == ';')
			p++;
		else {
			q = strcpy1(q, "DB(0xef);");
		}
	}
	if (braseDepth != 0)
		q = strcpy1(q, "DB(0xef);\n");
	goto fin;
err:
	q = NULL;
fin:
	return q;
}

int idendef(const char *p, void *dmy)
{
	return 1;
}

const UCHAR *idenref(const UCHAR *p, UCHAR *q, void *iden)
{
	while (*p != ';') {
		if (*p <= ' ') {
			p++;
			continue;
		}
		if ('0' <= *p && *p <= '9') {
			while (*p > ' ' && !isSymbolChar(*p))
				*q++ = *p++;
			continue;
		}
		if (isSymbolChar(*p)) {
			*q++ = *p++;
			if (*p == ';') {
				*q++ = ' ';
				*q++ = ' ';
				break;
			}
			if (isSymbolChar(*p))
				*q++ = *p++;
			else
				*q++ = ' ';
			if (*p == ';') {
				*q++ = ' ';
				break;
			}
			if (isSymbolChar(*p))
				*q++ = *p++;
			else
				*q++ = ' ';
			continue;
		}
		while (*p > ' ' && !isSymbolChar(*p))
			*q++ = *p++;
		continue;
	}
	*q++ = ';';
	*q++ = ' ';
	*q++ = ' ';
	*q = '\0';
	return p + 1;
}

const UCHAR *skipSpace(const UCHAR *p)
{
	while (*p == ' ' || *p == '\t')
		p++;
	return p;
}

/*** prepro ***/

int prepro(struct Work *w)
{
	return 0;
}

/*** lbstk ***/

#define	LBSTK_LINFOSIZE	1024

struct LbstkLineInfo {
	unsigned int maxline, line0;
	UCHAR file[1024];
};

int lbstkPass0(struct Work *w, struct LbstkLineInfo *linfo);
UCHAR *lbstkPass1(struct Work *w, struct LbstkLineInfo *linfo);
const UCHAR *skip0(const UCHAR *p);
const UCHAR *skip1(const UCHAR *p);
const UCHAR *skip2(const UCHAR *p);

int lbstk(struct Work *w)
{
	struct LbstkLineInfo *lineInfo = malloc(LBSTK_LINFOSIZE * sizeof (struct LbstkLineInfo));
	const UCHAR *cs;
	int i, r;

	for (i = 0; i < LBSTK_LINFOSIZE; i++) {
		lineInfo[i].maxline = 0;
		lineInfo[i].line0 = 0;
		lineInfo[i].file[0] = '\0';
	}
	lineInfo[LBSTK_LINFOSIZE - 1].line0 = 0xffffffff;

	r = lbstkPass0(w, lineInfo);
	if (r != 0) goto fin;
	cs = searchArg(w, "lst:" );
	if (cs != NULL) {
		FILE *fp = fopen(cs, "w");
		for (i = 0; lineInfo[i].file[0] != '\0'; i++) {
			fprintf(fp, "%6u - %6u : \"", lineInfo[i].line0, lineInfo[i].line0 + lineInfo[i].maxline);
			cs = strchr(lineInfo[i].file, '\"');
			fwrite(lineInfo[i].file, 1, cs - lineInfo[i].file, fp);
			fputs("\"\n", fp);
		}
		fclose(fp);
	}
	r = putBuf(w->argOut, w->obuf, lbstkPass1(w, lineInfo));
fin:
	return r;
}

int lbstkPass0(struct Work *w, struct LbstkLineInfo *linfo)
{
	const UCHAR *p = w->ibuf, *q;
	int i, r = 0;
	unsigned int line;
	for (;;) {
		for (;;) {
			p = strstr(p, "lbstk");
			if (p == NULL) break;
			if (p[6] == '(' && (p[5] == '6' || p[5] == '7')) break;
			p++;
		}
	//	p = strstr(p, "lbstk6(");
		if (p == NULL) break;
		p = strchr(p + 7, '\"');
		if (p == NULL) goto err1;
		p++;
		q = strchr(p, '\"');
		if (q == NULL) goto err1;
		q++;
		if (q - p > 1024) goto err2;
		for (i = 0; linfo[i].line0 != 0xffffffff; i++) {
			if (linfo[i].file[0] == '\0') {
				memcpy(linfo[i].file, p, q - p);
				break;
			}
			if (memcmp(linfo[i].file, p, q - p) == 0)
				break;
		}
		if (linfo[i].line0 == 0xffffffff) goto err3;
		p = strchr(q, ',');
		if (p == NULL) goto err1;
		p++;
		line = getConstInt1(&p);
		if (linfo[i].maxline < line)
			linfo[i].maxline = line;
	}
	line = 0;
	for (i = 0; linfo[i].file[0] != '\0'; i++) {
		linfo[i].line0 = line;
		line += linfo[i].maxline + 100;
		line = ((line + 999) / 1000) * 1000; /* 1000単位に切り上げる */
	}
	goto fin;

err1:
	r = 1;
	fputs("lbstk5: syntax error\n", stderr);
	goto fin;

err2:
	r = 2;
	fputs("lbstk5: too long filename\n", stderr);
	goto fin;

err3:
	r = 3;
	fputs("lbstk5: too many filenames\n", stderr);
	goto fin;

fin:
	return r;
}

UCHAR *lbstkPass1(struct Work *w, struct LbstkLineInfo *linfo)
{
	int *stk[4], stkp[4], i;
	int nextlb, nowlb0, nowlbsz;
	const UCHAR *p, *r;
	for (i = 0; i < 4; i++) {
		stk[i] = malloc(65536 * sizeof (int));
		stkp[i] = 0;
	}

	nextlb = 0;
	p = strstr(w->ibuf, "lbstk0(");
	if (p != NULL) {
		p += 7;
		nextlb = getConstInt1(&p);
	}
	nowlb0 = -1;
	nowlbsz = -1;
	if ((w->flags & FLAGS_PUT_MANY_INFO) != 0)
		printf("lbstk0=%d, flags=%d\n", nextlb, w->flags);

	p = w->ibuf;
	UCHAR *q = w->obuf;
	int j, sz, ofs;
	unsigned int line;
	while (*p != '\0') {
		if (*p == 'l' && p[1] == 'b') {
			if (strncmp1(p, "lbstk0(") == 0) {
				p = skip2(p + 7);
				continue;
			}
			if (strncmp1(p, "lbstk2(") == 0) {
				p += 7;
				j = getConstInt1(&p); if (*p == ',') p++;
				sz = getConstInt1(&p); p = skip2(p);
				stk[j][(stkp[j])++] = nextlb;
				nextlb += sz;
				continue;
			}
			if (strncmp1(p, "lbstk1(") == 0) {
				p += 7;
				j = getConstInt1(&p); if (*p == ',') p++;
				ofs = getConstInt1(&p); p = skip1(p);
				if (stkp[j] <= 0) {
					fprintf(stderr, "lbstk1: error: stkp[%d] < 0\n", j);
					goto err;
				}
				i = stk[j][stkp[j] - 1];
				sprintf(q, "%d", i + ofs);
				while (*q != '\0') q++;
				continue;
			}
			if (strncmp1(p, "lbstk3(") == 0) {
				p += 7;
				j = getConstInt1(&p); p = skip1(p);
				if (stkp[j] <= 0) {
					fprintf(stderr, "lbstk3: error: stkp[%d] < 0\n", j);
					goto err;
				}
				i = (stkp[j])--;
				continue;
			}
			if (strncmp1(p, "lbstk4(") == 0) {
				p += 7;
				nowlbsz = getConstInt1(&p);
				nowlb0 = nextlb;
				nextlb += nowlbsz;
				p = skip2(p);
				continue;
			}
			if (strncmp1(p, "lbstk5(") == 0) {
				p += 7;
				i = getConstInt1(&p);
				p = skip1(p);
				if (i >= nowlbsz) {
					fprintf(stderr, "lbstk5: error: i=%d (sz=%d)\n", i, nowlbsz);
					goto err;
				}
				sprintf(q, "%d", nowlb0 + i);
				while (*q != '\0') q++;
				continue;
			}
			if (strncmp1(p, "lbstk6(") == 0) {
				p = strchr(p + 7, '\"') + 1;
				r = strchr(p, '\"') + 1;
				for (i = 0; memcmp(linfo[i].file, p, r - p) != 0; i++);
				p = strchr(r, ',') + 1;
				line = getConstInt1(&p) + linfo[i].line0;
				sprintf(q, "DB(0x%02X,0x%02X,0x%02X,0x%02X",
					(line >> 24) & 0xff,
					(line >> 16) & 0xff,
					(line >>  8) & 0xff,
					 line        & 0xff
				);
				while (*q != '\0') q++;
				continue;
			}
			if (strncmp1(p, "lbstk7(") == 0) {
				p = strchr(p + 7, '\"') + 1;
				r = strchr(p, '\"') + 1;
				for (i = 0; memcmp(linfo[i].file, p, r - p) != 0; i++);
				p = strchr(r, ',') + 1;
				line = getConstInt1(&p) + linfo[i].line0;
				sprintf(q, "0x%08X", line);
				while (*q != '\0') q++;
				p = strchr(p, ')') + 1;
				continue;
			}
		}
		*q++ = *p++;
	}
	for (i = 0; i < 4; i++) {
		if (stkp[i] != 0) {
			fprintf(stderr, "lbstk: error: stkp[%d] != 0\n", i);
			goto err;
		}
	}
	goto fin;

err:
	q = NULL;
fin:
	return q;
}


const UCHAR *skip0(const UCHAR *p)
{
	while (*p != '\0' && *p != ',') p++;
	if (*p == ',') p++;
	return p;
}

const UCHAR *skip1(const UCHAR *p)
{
	while (*p != '\0' && *p != ')') p++;
	if (*p == ')') p++;
	return p;
}

const UCHAR *skip2(const UCHAR *p)
{
	p = skip1(p);
	while (*p != '\0' && *p != ';') p++;
	if (*p == ';') p++;
	return p;
}

/*** db2bin ***/

int db2binSub0(struct Work *w, const UCHAR **pp, UCHAR **qq);
UCHAR *db2binSub1(const UCHAR **pp, UCHAR *q, int width);
UCHAR *db2binSub2(UCHAR *p0, UCHAR *p1); // ラベル番号の付け直し.
UCHAR *db2binSub3(UCHAR *p0, UCHAR *p1, UCHAR *p2); // ARRAY_PARAMの展開.

#define DB2BIN_HEADER_LENGTH	3

int db2bin(struct Work *w)
{
	const UCHAR *pi = w->ibuf;
	UCHAR *q = w->obuf, *pj = w->jbuf + JBUFSIZE - 1;
	int r = 0, i = 0, j;
	*pj = '\0';
	while ((*pi != '\0' || i > 0) && q != NULL) {
		if (i > 0) {
			pj -= i;
			for (j = 0; j < i; j++)
				pj[j] = w->jbuf[j];
		}
		if (*pj != '\0')
			i = db2binSub0(w, (const UCHAR **) &pj, &q);
		else
			i = db2binSub0(w, &pi, &q);
	}
	putchar(0); // これがないとGCCがおかしくなる.
	if ((w->flags & 1) == 0 && q != NULL) {
		q = db2binSub2(w->obuf + DB2BIN_HEADER_LENGTH, q);
		q = db2binSub3(w->obuf + DB2BIN_HEADER_LENGTH, q, w->obuf + BUFSIZE);
	}
#if 0
	if (*pi == '\0' && q != NULL /* && (w->obuf[4] & 0xf0) != 0 */) {
	//	fputs("osecpu binary first byte error.\n", stderr);
		for (r = q - w->obuf - 1; r >= DB2BIN_HEADER_LENGTH; r--)
			w->obuf[r + 1] = w->obuf[r];
		w->obuf[2] = 0x00;
		q++;
	}
#endif
	r = 1;
	if (*pi == '\0' && q != NULL /* && (w->obuf[4] & 0xf0) == 0 */)
		r = putBuf(w->argOut, w->obuf, q);
fin:
	return r;
}

int db2binSub0(struct Work *w, const UCHAR **pp, UCHAR **qq)
{
	const UCHAR *p = *pp, *r;
	UCHAR *qj = w->jbuf, *q = *qq;
	int i, j;
	while (*p != '\0' && q != NULL) {
		if (*p <= ' ') {
			p++;
			continue;
		}
		if (qj > w->jbuf) break;
		
		static UCHAR *table0[] = {
			" 08DB(", " 16DWBE(", " 32DDBE(", "-32DDLE(", "-32DD(",
			" 01D1B(", " 02D2B(", " 04D4B(", " 08D8B(", " 12D12B(",
			" 16D16B(", " 20D20B(", " 24D24B(", " 28D28B(", " 32D32B(",
			NULL
		};
		for (i = 0; table0[i] != NULL; i++) {
			if (strncmp1(p, table0[i] + 3) == 0) break;
		}
		if (table0[i] != NULL) {
			p += strlen(table0[i] + 3);
			j = (table0[i][1] - '0') * 10 + (table0[i][2] - '0');
			if (table0[i][0] == '-')
				j = - j;
			q = db2binSub1(&p, q, j);
			continue;
		}

		static UCHAR *table1[] = {
			"0OSECPU_HEADER(",		"DB(0x05,0xe2,0x00);",

			"1bit(",				"DB(0xf7,0x88); DDBE(%0);", // 6bytes.
			"1imm(",				"DB(0xf7,0x88); DDBE(%0);", // 6bytes.
			"1typ(",				"DB(0xf7,0x88); DDBE(%0);", // 6bytes.
			"1r(",					"DB(%0+0x80);", // 1byte.
			"1p(",					"DB((%0&0x3f)+0x80);", // 1byte.
			"1dr(",					"DB((%0&0x3f)+0x80);", // 1byte.

			"0NOP(",				"DB(0xf0);",
			"2LB(",					"DB(0xf1); imm(%1); imm(%0);",
			"3LIMM(",				"DB(0xf2); imm(%2); r(%1); bit(%0);",
			"2PLIMM(",				"DB(0xf3); imm(%1); p(%0);",
			"1CND(",				"DB(0xf4); r(%0);",
			"5LMEM(",				"DB(0x88); p(%3); typ(%2); imm(%4); r(%1); bit(%0);",
			"5SMEM(",				"DB(0x89); r(%1); bit(%0); p(%3); typ(%2); imm(%4);",
		//	"4PLMEM(",				"DB(0x0a,%0-0x40); DDBE(%1); DB(%2-0x40,%3);",
		//	"4PSMEM(",				"DB(0x0b,%0-0x40); DDBE(%1); DB(%2-0x40,%3);",
			"5PADD(",				"DB(0x8e); p(%3); typ(%2); r(%4); bit(%0); p(%1);",
		//	"4PDIF(",				"DB(0x0f,%0); DDBE(%1); DB(%2-0x40,%3-0x40);",
			"3CP(",					"DB(0x90); r(%2); r(%2); r(%1); bit(%0);",
			"4OR(",					"DB(0x90); r(%2); r(%3); r(%1); bit(%0);",
			"4XOR(",				"DB(0x91); r(%2); r(%3); r(%1); bit(%0);",
			"4AND(",				"DB(0x92); r(%2); r(%3); r(%1); bit(%0);",
			"4SBX(",				"DB(0x93); r(%2); r(%3); r(%1); bit(%0);",
			"4ADD(",				"DB(0x94); r(%2); r(%3); r(%1); bit(%0);",
			"4SUB(",				"DB(0x95); r(%2); r(%3); r(%1); bit(%0);",
			"4MUL(",				"DB(0x96); r(%2); r(%3); r(%1); bit(%0);",
			"4SHL(",				"DB(0x98); r(%2); r(%3); r(%1); bit(%0);",
			"4SAR(",				"DB(0x99); r(%2); r(%3); r(%1); bit(%0);",
			"4DIV(",				"DB(0x9a); r(%2); r(%3); r(%1); bit(%0);",
			"4MOD(",				"DB(0x9b); r(%2); r(%3); r(%1); bit(%0);",
			"2PCP(",				"DB(0x9e); p(%1); p(%0);",
			"5CMPE(",				"DB(0xa0); r(%3); r(%4); bit(%1); r(%2); bit(%0);",
			"5CMPNE(",				"DB(0xa1); r(%3); r(%4); bit(%1); r(%2); bit(%0);",
			"5CMPL(",				"DB(0xa2); r(%3); r(%4); bit(%1); r(%2); bit(%0);",
			"5CMPGE(",				"DB(0xa3); r(%3); r(%4); bit(%1); r(%2); bit(%0);",
			"5CMPLE(",				"DB(0xa4); r(%3); r(%4); bit(%1); r(%2); bit(%0);",
			"5CMPG(",				"DB(0xa5); r(%3); r(%4); bit(%1); r(%2); bit(%0);",
			"5TSTZ(",				"DB(0xa6); r(%3); r(%4); bit(%1); r(%2); bit(%0);",
			"5TSTNZ(",				"DB(0xa7); r(%3); r(%4); bit(%1); r(%2); bit(%0);",
		//	"3PCMPE(",				"DB(0x28,%0,%1-0x40,%2-0x40);",
		//	"3PCMPNE(",				"DB(0x29,%0,%1-0x40,%2-0x40);",
		//	"3PCMPL(",				"DB(0x2a,%0,%1-0x40,%2-0x40);",
		//	"3PCMPGE(",				"DB(0x2b,%0,%1-0x40,%2-0x40);",
		//	"3PCMPLE(",				"DB(0x2c,%0,%1-0x40,%2-0x40);",
		//	"3PCMPG(",				"DB(0x2d,%0,%1-0x40,%2-0x40);",
		//	"2EXT(",				"DB(0x2f,%0); DWBE(%1);",

			"2LIDR(",				"DB(0xfc,0xfd); imm(%1); dr(%0);",
			"0REM00(",				"DB(0xfc,0xfe,0x00);",
			"0REM01(",				"DB(0xfc,0xfe,0x10);",
			"1REM02(",				"DB(0xfc,0xfe,0x21); imm(%0);",
			"0REM03(",				"DB(0xfc,0xfe,0x30);",
			"0REM04(",				"DB(0xfc,0xfe,0x40);",
			"0REM05(",				"DB(0xfc,0xfe,0x50);",
			"0REM06(",				"DB(0xfc,0xfe,0x60);",
			"0REM07(",				"DB(0xfc,0xfe,0x87,0xf0);",
			"0REM34(",				"DB(0xfc,0xfe,0xb4,0xf0);",
			"1REM35(",				"DB(0xfc,0xfe,0xb5,0xf1); imm(%0);",
			"0REM36(",				"DB(0xfc,0xfe,0xb6,0xf0);",
			"1REM37(",				"DB(0xfc,0xfe,0xb7,0xf1); imm(%0);",
			"5REM38(",				"DB(0xfc,0xfe,0xb8,0x85,%0+0x80,%1+0x80); typ(%2); r(%3); p(%4);",
			"1REM09(",				"DB(0xfc,0xfe,0x89,0xf1); imm(%0);",
		//	"1DBGINFO(",			"DBGINFO0(%0);",
			"1JMP(",				"PLIMM(P3F, %0);",
			"1PJMP(",				"PCP(P3F, %0);",
			"5PADDI(",				"LIMM(%0, R3F, %4); PADD(%0, %1, %2, %3, R3F);",
			"4ORI(",				"LIMM(%0, R3F, %3); OR( %0, %1, %2, R3F);",
			"4XORI(",				"LIMM(%0, R3F, %3); XOR(%0, %1, %2, R3F);",
			"4ANDI(",				"LIMM(%0, R3F, %3); AND(%0, %1, %2, R3F);",
			"4SBXI(",				"LIMM(%0, R3F, %3); SBX(%0, %1, %2, R3F);",
			"4ADDI(",				"LIMM(%0, R3F, %3); ADD(%0, %1, %2, R3F);",
			"4SUBI(",				"LIMM(%0, R3F, %3); SUB(%0, %1, %2, R3F);",
			"4MULI(",				"LIMM(%0, R3F, %3); MUL(%0, %1, %2, R3F);",
			"4SHLI(",				"LIMM(%0, R3F, %3); SHL(%0, %1, %2, R3F);",
			"4SARI(",				"LIMM(%0, R3F, %3); SAR(%0, %1, %2, R3F);",
			"4DIVI(",				"LIMM(%0, R3F, %3); DIV(%0, %1, %2, R3F);",
			"4MODI(",				"LIMM(%0, R3F, %3); MOD(%0, %1, %2, R3F);",
			"5CMPEI(",				"LIMM(%1, R3F, %4); CMPE( %0, %1, %2, %3, R3F);",
			"5CMPNEI(",				"LIMM(%1, R3F, %4); CMPNE(%0, %1, %2, %3, R3F);",
			"5CMPLI(",				"LIMM(%1, R3F, %4); CMPL( %0, %1, %2, %3, R3F);",
			"5CMPGEI(",				"LIMM(%1, R3F, %4); CMPGE(%0, %1, %2, %3, R3F);",
			"5CMPLEI(",				"LIMM(%1, R3F, %4); CMPLE(%0, %1, %2, %3, R3F);",
			"5CMPGI(",				"LIMM(%1, R3F, %4); CMPG( %0, %1, %2, %3, R3F);",
			"0DBGINFO1(",			"REM00();",
			NULL
		};
		for (i = 0; table1[i] != NULL; i += 2) {
			if (strncmp1(p, table1[i] + 1) == 0) break;
		}
		if (table1[i] != NULL) {
			const UCHAR *p0[10], *p1[10], *p00 = p;
			p += strlen(table1[i] + 1);
			for (j = 0; j < table1[i][0] - '0'; j++) {
				while(*p == ' ' || *p == '\t') p++;
				if (*p == ')') { p = p00; goto err; }
				p0[j] = p;
				while (*p != ',' && *p != ')' && *p != '(' && *p != '\'' && *p != '\"' && *p != '/') p++;
				if (*p != ',' && *p != ')') {
					p = stringToElements(w->ele0, p0[j], 1);
				}
				p1[j] = p;
				if (*p == ',') p++;
			}
			while(*p == ' ' || *p == '\t') p++;
			if (*p != ')') { p = p00; goto err; }
			p++;
			if (*p != ';') { p = p00; goto err; }
			p++;
			for (r = table1[i + 1]; *r != '\0'; ) {
				if (*r != '%') {
					*qj++ = *r++;
					continue;
				}
				j = r[1] - '0';
				r += 2;
				memcpy(qj, p0[j], p1[j] - p0[j]);
				qj += p1[j] - p0[j];
			}
			continue;
		}
		if (*p == ';') {
			p++;
			continue;
		}
err:
		fputs("db2bin: error: ", stderr);
		for (i = 0; p[i] != '\0'; i++) {
			if (i < 40) fputc(p[i], stderr);
			if (p[i] == ';') break;
		}
		fputc('\n', stderr);
		q = NULL;
		break;
	}
	*pp = p;
	*qq = q;
	return qj - w->jbuf;
}

UCHAR *db2binSub1(const UCHAR **pp, UCHAR *q, int width)
{
	const UCHAR *p = *pp;
	char errflg = 0;
	int i, j;
	while (*p != '\0' && *p != ')') {
		if (*p <= ' ' || *p == ',') { p++; continue; }
		if (width == 8 && *p == '\'') {
			p++;
			while (*p != '\0' && *p != '\'') {
				if (*p == '\\') {
					p++;
					if (*p == 'n') { p++; *q++ = '\n'; continue; }
					if (*p == 't') { p++; *q++ = '\t'; continue; }
					if (*p == 'r') { p++; *q++ = '\r'; continue; }
					if ('0' <= *p && *p <= '7') { p++; *q++ = p[-1] - '0'; continue; } 
				}
				*q++ = *p++;
			}
			if (*p == '\'') p++;
			continue;
		}
		static UCHAR *table[] = {
			"02T_SINT8", "03T_UINT8", "04T_SINT16", "05T_UINT16", "06T_SINT32", "07T_UINT32",
			"08T_SINT4", "09T_UINT4", "10T_SINT2", "11T_UINT2", "12T_SINT1", "13T_UINT1",
			"14T_SINT12", "15T_UINT12", "16T_SINT20", "17T_UINT20", "18T_SINT24", "19T_UINT24",
			"20T_SINT28", "21T_UINT28", NULL
		};
		for (i = 0; table[i] != NULL; i++) {
			if (strncmp1(p, table[i] + 2) == 0) {
				j = strlen(table[i] + 2);
				if (p[j] <= ' ' || isSymbolChar(p[j])) {
					p += j;
					i = (table[i][0] - '0') * 10 + (table[i][1] - '0');
					goto put_i;
				}
			}
		}
		*pp = p;
		i = getConstInt(&p, &errflg, 1 << 8 | 2); // カンマで止まる, R00を数字に展開.
		if (errflg != 0) {
			fprintf(stderr, "db2binSub: error: %.20s\n", *pp);
			exit(1);
			break;
		}
put_i:
		if (width == -32) {
			*q++ =  i        & 0xff;
			*q++ = (i >>  8) & 0xff;
			*q++ = (i >> 16) & 0xff;
			*q++ = (i >> 24) & 0xff;
		}
		if (width > 0) {
			static int buf = 1;
			for (j = width - 1; j >= 0; j--) {
				buf = buf << 1 | ((i >> j) & 1);
				if (buf >= 0x100) {
					*q++ = buf & 0xff;
					buf = 1;
				}
			}
		}
	}
	if (*p == ')') p++;
	*pp = p;
	return q;
}

int db2binDataWidth(int i)
{
	int r = -1;
	i >>= 1;
	if (i == 0x0002 / 2) r =  8;
	if (i == 0x0004 / 2) r = 16;
	if (i == 0x0006 / 2) r = 32;
	if (i == 0x0008 / 2) r =  4;
	if (i == 0x000a / 2) r =  2;
	if (i == 0x000c / 2) r =  1;
	if (i == 0x000e / 2) r = 12;
	if (i == 0x0010 / 2) r = 20;
	if (i == 0x0012 / 2) r = 24;
	if (i == 0x0014 / 2) r = 28;
	return r;
}

int db2binSub2Len(const UCHAR *src)
{
	int i = 0, j;
	if (*src == 0xf0) i = 1; // NOP
	if (src[0] == 0xf1 && src[1] == 0xf7 && src[2] == 0x88 && src[7] == 0xf7 && src[8] == 0x88) i = 13; // LB
	if (src[0] == 0xf2 && src[1] == 0xf7 && src[2] == 0x88) i = 14; // LIMM
	if (src[0] == 0xf3 && src[1] == 0xf7 && src[2] == 0x88) i = 8; // PLIMM
	if (src[0] == 0xf4) i = 2; // CND
	if (src[0] == 0x88 && src[2] == 0xf7 && src[3] == 0x88 && src[8] == 0xf7 && src[9] == 0x88 && src[15] == 0xf7 && src[16] == 0x88) i = 21;
	if (src[0] == 0x89 && src[2] == 0xf7 && src[3] == 0x88 && src[9] == 0xf7 && src[10] == 0x88 && src[15] == 0xf7 && src[16] == 0x88) i = 21;
	if (src[0] == 0x8e && src[2] == 0xf7 && src[3] == 0x88 && src[9] == 0xf7 && src[10] == 0x88) i = 16;
	if (0x90 <= src[0] && src[0] <= 0x9b && src[0] != 0x97 && src[4] == 0xf7 && src[5] == 0x88) i = 10; // OR...MOD
	if (src[0] == 0x9e) i = 3; // PCP
	if (0xa0 <= src[0] && src[0] <= 0xa7 && src[3] == 0xf7 && src[4] == 0x88 && src[10] == 0xf7 && src[11] == 0x88) i = 16; // CMPcc
	if (src[0] == 0xae && src[1] == 0xf7 && src[2] == 0x88 && src[7] == 0xf7 && src[8] == 0x88) {
		j = src[ 3] << 24 | src[ 4] << 16 | src[ 5] << 8 | src[ 6];
		i = src[ 9] << 24 | src[10] << 16 | src[11] << 8 | src[12];
		j = db2binDataWidth(j);
		if (j <= 0)
			fputs("db2binSub2Len: error: F0\n", stderr);
		if (((i * j) & 7) != 0)
			fprintf(stderr, "db2binSub2Len: error: F0 (align 8bit) bit=%d len=%d\n", j, i);
		i = 13 + ((i * j) >> 3);
	}
	if (0xb0 <= src[0] && src[0] <= 0xb3) i = 16;
	if (src[0] == 0xbc && src[1] == 0xf7 && src[2] == 0x88) i = 37; // ENTER
	if (src[0] == 0xbd && src[1] == 0xf7 && src[2] == 0x88) i = 37; // LEAVE
	if (src[0] == 0xfc && src[1] == 0xfd && src[2] == 0xf7 && src[3] == 0x88 && src[8] == 0xf0) i = 9; // LIDR
	if (src[0] == 0xfc && src[1] == 0xfd && src[2] == 0xf7 && src[3] == 0x88 && src[8] == 0x80) i = 9; // LIDR
	if (src[0] == 0xfc && src[1] == 0xfe) {
		if (src[2] == 0x00) i = 3; // remark-0
		if (src[2] == 0x10) i = 3; // remark-1
		if (src[2] == 0x21 && src[3] == 0xf7 && src[4] == 0x88) i = 9; // remark-2
		if (src[2] == 0x30) i = 3; // remark-3
		if (src[2] == 0x50) i = 3; // remark-5
		if (src[2] == 0x88 && src[3] == 0x85) i = 14; // remark-8
		if (src[2] == 0xb4 && src[3] == 0xf0) i = 4; // remark-34
		if (src[2] == 0xb5 && src[3] == 0xf1) i = 10; // remark-35
		if (src[2] == 0xb6 && src[3] == 0xf0) i = 4; // remark-36
		if (src[2] == 0xb7 && src[3] == 0xf1) i = 10; // remark-37
		if (src[2] == 0xb8 && src[3] == 0x85) {	// remark-38
			if (src[5] == 0x80) {	// src-mode:0	DB%(s,0x00);
				for (i = 14; src[i] != 0x00; i++);
				i++;
			}
			if (src[5] == 0x81) {	// src-mode:1	DDBE(...,0);
				for (i = 14; (src[i] | src[i + 1] | src[i + 2] | src[i + 3]) != 0; i += 4);
				i += 4;
			}
			if (src[5] == 0x82) {	// src-mode:2	R31=len; P31=s;
				i = 14;
			}
			if (src[5] == 0x83) {	// src-mode:3	DDBE(len,...);
				i = 18 + (src[14] << 24 | src[15] << 16 | src[16] << 8 | src[17]) * 4;
			}
		}
		if (src[2] == 0x89 && src[3] == 0xf1 && src[4] == 0xf7 && src[5] == 0x88) i = 10; // remark-9
	}
#if 0
	if (*src == 0x00) i = 1;
	if (0x01 <= *src && *src < 0x04) i = 6;
	if (*src == 0x04) i = 2;
	if (0x08 <= *src && *src < 0x0d) i = 8 + src[7] * 4;
	if (0x0e <= *src && *src < 0x10) i = 8;
	if (0x10 <= *src && *src < 0x2e) i = 4;
	if (0x1c <= *src && *src < 0x1f) i = 3;
	if (*src == 0x1f) i = 11;
	if (*src == 0x2f) i = 4 + src[1];
	if (0x3c <= *src && *src <= 0x3d) i = 7;
	if (*src == 0xf0 || *src == 0x34) {
		j = src[1] << 24 | src[2] << 16 | src[3] << 8 | src[4];
		i = src[5] << 24 | src[6] << 16 | src[7] << 8 | src[8];
		j = db2binDataWidth(j);
		if (j <= 0)
			fputs("db2binSub2Len: error: F0\n", stderr);
		if (((i * j) & 7) != 0)
			fprintf(stderr, "db2binSub2Len: error: F0 (align 8bit) bit=%d len=%d\n", j, i);
		i = 9 + ((i * j) >> 3);
	}
	if (0xf4 <= *src && *src <= 0xf7 || 0x30 <= *src && *src <= 0x33) i = 4; // 暫定処置.
	if (0xec <= *src && *src <= 0xef) i = 1;
	if (*src == 0xfe) i = 2 + src[1];
	if (*src == 0xff) {
		if (src[1] == 0x00) { i = strlen(src + 3) + 4; }
		if (src[1] == 0x01) {
			i = 3;
			while ((src[i + 0] | src[i + 1] | src[i + 2] | src[i + 3]) != 0) i += 4;
			i += 4;
		}
		if (src[1] == 0x02) { i = 3; }
		if (src[1] == 0x03) { i = 7 + (src[i + 3] | src[i + 4] | src[i + 5] | src[i + 6]) * 4; }
	}
#endif
	if (i == 0) {
		fprintf(stderr, "db2binSub2Len: unknown opecode: %02X-%02X-%02X\n", src[0], src[1], src[2]);
		exit(1);
	}
	return i;
}

UCHAR *db2binSub2(UCHAR *p0, UCHAR *p1)
{
	int t[MAXLABELS], i, j;
	UCHAR *p, *q, *r;

	// (1) ラベルマージ.

	for (j = 0; j < MAXLABELS; j++)
		t[j] = -1;

	for (p = p0; p < p1; ) {
		if (*p == 0xf1) {
			j = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			if (j >= MAXLABELS || j < 0) {
err:
				fputs("label number over error. check LMEM/SMEM/PLMEM/PSMEM.\n", stderr);
				exit(1);
			}
			t[j] = j;
			q = p + db2binSub2Len(p);
			while (q < p1 && cmpBytes(q, "fcfd_f788#480") != 0)
				q += db2binSub2Len(q);
			if (q < p1 && *q == 0xf1 && p[9] == q[9] && p[10] == q[10] && p[11] == q[11] && p[12] == q[12]) {
				i = q[3] << 24 | q[4] << 16 | q[5] << 8 | q[6];
				t[j] = i;
			}
		}
		p += db2binSub2Len(p);
	}

	for (p = p0; p < p1; ) {
		if (*p == 0xf3) {
			j = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			if (j >= MAXLABELS || j < 0) goto err;
			if (t[j] < 0) {
				fputs("use undefined label number error. check LMEM/SMEM/PLMEM/PSMEM.\n", stderr);
				exit(1);
			}
			if (t[j] != j) {
//printf("%d->", j);
				while (t[j] != j)
					j = t[j];
//printf("%d\n", j);
				p[3] = (j >> 24) & 0xff;
				p[4] = (j >> 16) & 0xff;
				p[5] = (j >>  8) & 0xff;
				p[6] =  j        & 0xff;
			}
		}
		p += db2binSub2Len(p);
	}

	// (2) 不要ラベルの消去.

	for (j = 0; j < MAXLABELS; j++)
		t[j] = -1;

	for (p = p0; p < p1; ) {
		if (*p == 0xf1 || *p == 0xf3) {
			j = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			if (*p == 0xf3)
				t[j]--; // このラベルは参照されているので消さない.
			if (*p == 0xf1 && p[12] != 0x00) // bugfix: hinted by yao, 2013.09.17. thanks!
				t[j]--; // データラベルの暗黙代入がありえるのでこのラベルは消せない.
		}
		p += db2binSub2Len(p);
	}
	i = 0;
	for (p = p0; p < p1; ) {
		if (*p == 0xf1) {
			j = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			if (t[j] >= 0) {
//printf("redefine label:%06X at %06X\n", j, p - p0);
			}
			if (t[j] < -1) {
//printf("LB:%06X->%06X at %06X\n", j, i, p - p0);
				t[j] = i;
				i++;
			}
		}
		p += db2binSub2Len(p);
	}
	for (p = p0; p < p1; ) {
		if (*p == 0xf1 || *p == 0xf3) {
			j = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			i = t[j];
			if (i >= 0 || *p == 0xf3) {
				p[3] = (i >> 24) & 0xff;
				p[4] = (i >> 16) & 0xff;
				p[5] = (i >>  8) & 0xff;
				p[6] =  i        & 0xff;
			} else {
				// 参照されないラベル命令は処分する.
//printf("delete label:%06X at %06X\n", j, p - p0);
				q = p;
				r = p + 13;
				while (r < p1)
					*q++ = *r++;
				p1 = q;
				continue;
			}
		}
		p += db2binSub2Len(p);
	}
	return p1;
}

void db2binSub3_check(UCHAR *q, UCHAR *q1)
{
	if (q > q1) {
		fprintf(stderr, "db2binSub3_check: buffer over.\n");
		exit(1);
	}
	return;
}

UCHAR *db2binSub3_put32(UCHAR *q, int i)
{
	q[ 0] = 0xf7;
	q[ 1] = 0x88;
	q[ 2] = (i >> 24) & 0xff;
	q[ 3] = (i >> 16) & 0xff;
	q[ 4] = (i >>  8) & 0xff;
	q[ 5] =  i        & 0xff;
	return q + 6;
}

UCHAR *db2binSub3_LIMM(UCHAR *q, UCHAR *q1, int bit, int r, int imm)
{
	q[ 0] = 0xf2;
	db2binSub3_put32(q +  1, imm);
	q[ 7] = (r & 0x3f) | 0x80;
	db2binSub3_put32(q +  8, bit);
	q += 14;
	db2binSub3_check(q, q1);
	return q;
}

UCHAR *db2binSub3(UCHAR *p0, UCHAR *p1, UCHAR *p2)
// ARRAY_PARAMの展開.
{
	int i, j = p1 - p0, k, sp;
	UCHAR *p = p2 - j;
	int tfreeBufTyp[4], tfreeBufLen[4];
	UCHAR tfreeBufR[4], tfreeBufP[4];
	for (i = 0; i < j; i++)
		p[i] = p0[i];
	for (i = 0; i < 4; i++)
		tfreeBufTyp[i] = 0;
	sp = -1;
	while (p < p2) {
		if (p[0] == 0xfc && p[1] == 0xfe && p[2] == 0xb8 && p[3] == 0x85) {
			UCHAR srcMode = p[5];
			if (srcMode == 0x80) {	// src-mode:0	DB(s, 0);
				sp++;
				tfreeBufTyp[sp] = p[8] << 24 | p[9] << 16 | p[10] << 8 | p[11];
				tfreeBufR[sp] = p[12];
				tfreeBufP[sp] = p[13];
				p0[0] = 0xfc;
				p0[1] = 0xfe;
				p0[2] = 0x88;
				p0[3] = 0x85;
				p0[4] = p[4];
				p0[5] = 0x80;
				db2binSub3_put32(p0 + 6, tfreeBufTyp[sp]);
				p0[12] = tfreeBufR[sp];
				p0[13] = tfreeBufP[sp];
				p += 14;
				p0 += 14;
				for (i = 0; p[i] != 0; i++);
				tfreeBufLen[sp] = i;	// len;
				p0 = db2binSub3_LIMM(p0, p, 32, tfreeBufR[sp], tfreeBufLen[sp]);
				p0 = db2binSub3_LIMM(p0, p, 32, 0x3b, tfreeBufTyp[sp]);
				p0[0] = 0xb0;	// talloc(32,32,P3B,R3B,tfreeBufR[sp]);
				p0[1] = 0xbb;
				db2binSub3_put32(p0 + 2, 32);
				p0[8] = tfreeBufR[sp];
				db2binSub3_put32(p0 + 9, 32);
				p0[15] = 0xbb;
				p0 += 16;
				p0[0] = 0x9e;	// PCP(tfreeBufP[sp],P3B);
				p0[1] = 0xbb;
				p0[2] = tfreeBufP[sp];
				p0 += 3;
				db2binSub3_check(p0, p);
				for (i = 0; i < tfreeBufLen[sp]; i++) {
					j = p[0];
					p++;
					k = 0xbb;
					p0 = db2binSub3_LIMM(p0, p, 32, k, j);
					p0[0] = 0x89;	// SMEM(32, m, tfreeBufTyp[sp], P3B, 0);
					p0[1] = k;
					db2binSub3_put32(p0 + 2, 32);
					p0[8] = 0xbb;
					db2binSub3_put32(p0 + 9, tfreeBufTyp[sp]);
					db2binSub3_put32(p0 + 15, 0);
					p0 += 21;
					p0 = db2binSub3_LIMM(p0, p, 32, 0x3f, 1);
					p0[0] = 0x8e;
					p0[1] = 0xbb;
					db2binSub3_put32(p0 + 2, tfreeBufTyp[sp]);
					p0[8] = 0xbf;
					db2binSub3_put32(p0 + 9, 32);
					p0[15] = 0xbb;
					p0 += 16;
					db2binSub3_check(p0, p);
				}
				p++;
				srcMode = 0xff;
			}
			if (srcMode == 0x81) {	// src-mode:1	DDBE(...,0);
				sp++;
				tfreeBufTyp[sp] = p[8] << 24 | p[9] << 16 | p[10] << 8 | p[11];
				tfreeBufR[sp] = p[12];
				tfreeBufP[sp] = p[13];
				p0[0] = 0xfc;
				p0[1] = 0xfe;
				p0[2] = 0x88;
				p0[3] = 0x85;
				p0[4] = p[4];
				p0[5] = 0x81;
				db2binSub3_put32(p0 + 6, tfreeBufTyp[sp]);
				p0[12] = tfreeBufR[sp];
				p0[13] = tfreeBufP[sp];
				p += 14;
				p0 += 14;
				for (i = 0; (p[i] | p[i + 1] | p[i + 2] | p[i + 3]) != 0; i += 4);
				tfreeBufLen[sp] = i / 4;	// len;
				p0 = db2binSub3_LIMM(p0, p, 32, tfreeBufR[sp], tfreeBufLen[sp]);
				p0 = db2binSub3_LIMM(p0, p, 32, 0x3b, tfreeBufTyp[sp]);
				p0[0] = 0xb0;	// talloc(32,32,P3B,R3B,tfreeBufR[sp]);
				p0[1] = 0xbb;
				db2binSub3_put32(p0 + 2, 32);
				p0[8] = tfreeBufR[sp];
				db2binSub3_put32(p0 + 9, 32);
				p0[15] = 0xbb;
				p0 += 16;
				p0[0] = 0x9e;	// PCP(tfreeBufP[sp],P3B);
				p0[1] = 0xbb;
				p0[2] = tfreeBufP[sp];
				p0 += 3;
				db2binSub3_check(p0, p);
				for (i = 0; i < tfreeBufLen[sp]; i++) {
					j = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
					p += 4;
					k = 0xbb;
					if (((int) 0x80520000) <= j && j <= ((int) 0x8052003f)) {
						k = (j & 0x3f) | 0x80;
					} else {
						p0 = db2binSub3_LIMM(p0, p, 32, k, j);
					}
					p0[0] = 0x89;	// SMEM(32, m, tfreeBufTyp[sp], P3B, 0);
					p0[1] = k;
					db2binSub3_put32(p0 + 2, 32);
					p0[8] = 0xbb;
					db2binSub3_put32(p0 + 9, tfreeBufTyp[sp]);
					db2binSub3_put32(p0 + 15, 0);
					p0 += 21;
					p0 = db2binSub3_LIMM(p0, p, 32, 0x3f, 1);
					p0[0] = 0x8e;
					p0[1] = 0xbb;
					db2binSub3_put32(p0 + 2, tfreeBufTyp[sp]);
					p0[8] = 0xbf;
					db2binSub3_put32(p0 + 9, 32);
					p0[15] = 0xbb;
					p0 += 16;
					db2binSub3_check(p0, p);
				}
				p += 4;
				srcMode = 0xff;
			}
			if (srcMode == 0x82) {	// src-mode:2	R31=len; P31=s;
				p0[0] = 0xfc;
				p0[1] = 0xfe;
				p0[2] = 0x88;
				p0[3] = 0x85;
				p0[4] = p[4];
				p0[5] = 0x82;
				p0[6] = 0xf7;
				p0[7] = 0x88;
				p0[8] = p[8];
				p0[9] = p[9];
				p0[10] = p[10];
				p0[11] = p[11];
				p0[12] = p[12];
				p0[13] = p[13];
				p0 += 14;
				p += 14;
				srcMode = 0xff;
			}
			if (srcMode == 0x83) {	// src-mode:3	DDBE(len,...);
				sp++;
				tfreeBufTyp[sp] = p[8] << 24 | p[9] << 16 | p[10] << 8 | p[11];
				tfreeBufR[sp] = p[12];
				tfreeBufP[sp] = p[13];
				p0[0] = 0xfc;
				p0[1] = 0xfe;
				p0[2] = 0x88;
				p0[3] = 0x85;
				p0[4] = p[4];
				p0[5] = 0x83;
				db2binSub3_put32(p0 + 6, tfreeBufTyp[sp]);
				p0[12] = tfreeBufR[sp];
				p0[13] = tfreeBufP[sp];
				p += 14;
				p0 += 14;
				tfreeBufLen[sp] = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
				p += 4;
				p0 = db2binSub3_LIMM(p0, p, 32, tfreeBufR[sp], tfreeBufLen[sp]);
				p0 = db2binSub3_LIMM(p0, p, 32, 0x3b, tfreeBufTyp[sp]);
				p0[0] = 0xb0;	// talloc(32,32,P3B,R3B,tfreeBufR[sp]);
				p0[1] = 0xbb;
				db2binSub3_put32(p0 + 2, 32);
				p0[8] = tfreeBufR[sp];
				db2binSub3_put32(p0 + 9, 32);
				p0[15] = 0xbb;
				p0 += 16;
				p0[0] = 0x9e;	// PCP(tfreeBufP[sp],P3B);
				p0[1] = 0xbb;
				p0[2] = tfreeBufP[sp];
				p0 += 3;
				db2binSub3_check(p0, p);
				for (i = 0; i < tfreeBufLen[sp]; i++) {
					j = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
					p += 4;
					k = 0xbb;
					if (((int) 0x80520000) <= j && j <= ((int) 0x8052003f)) {
						k = (j & 0x3f) | 0x80;
					} else {
						p0 = db2binSub3_LIMM(p0, p, 32, k, j);
					}
					p0[0] = 0x89;	// SMEM(32, m, tfreeBufTyp[sp], P3B, 0);
					p0[1] = k;
					db2binSub3_put32(p0 + 2, 32);
					p0[8] = 0xbb;
					db2binSub3_put32(p0 + 9, tfreeBufTyp[sp]);
					db2binSub3_put32(p0 + 15, 0);
					p0 += 21;
					p0 = db2binSub3_LIMM(p0, p, 32, 0x3f, 1);
					p0[0] = 0x8e;
					p0[1] = 0xbb;
					db2binSub3_put32(p0 + 2, tfreeBufTyp[sp]);
					p0[8] = 0xbf;
					db2binSub3_put32(p0 + 9, 32);
					p0[15] = 0xbb;
					p0 += 16;
					db2binSub3_check(p0, p);
				}
				srcMode = 0xff;
			}
			if (srcMode != 0xff) {
				fprintf(stderr, "db2binSub3: unknown srcMode=0x%02x\n", srcMode);
				exit(1);
			}
		} else if (cmpBytes(p, "f3_f788#4_b0 9e_af_bf f1_f788#4_f78800000003 fcfe00") != 0) {	// 8+3+13+3=27
			for (i = 0; i < 27; i++)
				p0[i] = p[i];
			p0 += 27;
			p += 27;
			while (sp >= 0) {
				p0 = db2binSub3_LIMM(p0, p, 32, 0x3a, tfreeBufLen[sp]);
				p0 = db2binSub3_LIMM(p0, p, 32, 0x3b, tfreeBufTyp[sp]);
				p0[0] = 0xb1;	// tfree(32,32,P3F,R3B,R3A);
				p0[1] = 0xbf;	// チェックをしない.
				p0[2] = 0xbb;
				db2binSub3_put32(p0 + 3, 32);
				p0[9] = 0xba;
				db2binSub3_put32(p0 + 10, 32);
				p0 += 16;
				sp--;
			}
		} else {
			j = db2binSub2Len(p);
			for (i = 0; i < j; i++)
				p0[i] = p[i];
			p0 += i;
			p += i;
		}
	}
	return p0;
}

int disasm0(struct Work *w)
{
	const UCHAR *p = w->ibuf + 4;
	UCHAR *q = w->obuf;
	while (*p != 0x00) {
		UCHAR c = *p;
		sprintf(q, "%06X: ", p - w->ibuf);
		q += 8;
		sprintf(q, "error: %02X\n", c);
		if (c == 0x01) {
			sprintf(q, "LB(%02X, %08X)", p[1], p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
			p += 6;
		}
		if (c == 0x02) {
			sprintf(q, "LIMM(R%02X, %08X)", p[1], p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
			p += 6;
		}
		if (c == 0x03) {
			sprintf(q, "PLIMM(P%02X, %08X)", p[1], p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
			p += 6;
		}
		if (c == 0x1e) {
			sprintf(q, "PCP(P%02X, P%02X)", p[1], p[2]);
			p += 3;
		}
		if (c == 0x2f) {
			int i = p[1];
			sprintf(q, "EXT(%02X, %04X, ...)", p[1], p[2] << 8 | p[3]);
			p += 4 + i;
		}

		while (*q != '\0') q++;
		if (q[-1] != ')')
			break;
		*q++ = ';';
		*q++ = '\n';
	}
	return putBuf(w->argOut, w->obuf, q);
}

int disasm1(struct Work *w)
// http://ref.x86asm.net/coder32.html
{
	const UCHAR *p = w->ibuf;
	UCHAR *q = w->obuf;
	while (*p != 0x00) {
		UCHAR c = *p, b = 0, f = 0;
		if (c <= 0x3f && c != 0x0f) {
			if ((c & 7) <= 3) { b = 1; f = 1; }
			if ((c & 7) == 4) { b = 1; f = 4; }
			if ((c & 7) == 5) { b = 1; f = 2; }
			if ((c & 7) >= 6) b = 1;
		}
		if (0x40 <= c && c <= 0x61)   b = 1;
		if (0x70 <= c && c <= 0x7f) { b = 1; f = 4; }
		if (0x88 <= c && c <= 0x8b) { b = 1; f = 1; }
		if (0x90 <= c && c <= 0x9a)   b = 1;
		if (0xb8 <= c && c <= 0xbf) { b = 1; f = 2; }
		if (c == 0xff) { b = 1; f = 1; }
		if (c == 0x0f) {
			c = p[1];
			if (0x80 <= c && c <= 0x8f) { b = 2; f = 2; }
			if (0xb6 <= c && c <= 0xb7) { b = 2; f = 1; }
		}
		sprintf(q, "%06X: ", p - w->ibuf);
		q += 8;
		if (b == 0) {
			sprintf(q, "error: %02X\n", c, p[1]);
			q += 13;
			break;
		}
		do {
			sprintf(q, "%02X ", *p++);
			q += 3;
		} while (--b != 0);
		if ((f & 1) != 0) { /* mod-nnn-r/m */
			c = *p & 0xc7;
			sprintf(q, "%02X ", *p++);
			q += 3;
			if (c < 0xc0 && (c & 7) == 4) {	// sib
				sprintf(q, "sib:%02X ", *p++);
				q += 7;
			} else if (c == 5) { // disp32
				goto disp32;
			}
			if ((c & 0xc0) == 0x40) {
				sprintf(q, "disp:%02X ", *p++);
				q += 8;
			}
			if ((c & 0xc0) == 0x80) {
disp32:
				sprintf(q, "disp:%08X ", p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24);
				p += 4;
				q += 14;
			}
		}
		if ((f & 2) != 0) { /* imm32 */
			sprintf(q, "imm:%08X ", p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24);
			p += 4;
			q += 13;
		}
		if ((f & 4) != 0) { /* imm8 */
			sprintf(q, "imm:%02X ", *p++);
			q += 7;
		}
		*q++ = '\n';
	}
	return putBuf(w->argOut, w->obuf, q);
}

int disasm(struct Work *w)
{
	int r = 0;
	if (w->flags == 0) r = disasm0(w);
	if (w->flags == 1) r = disasm1(w);
}

int appackEncodeConst(int i)
{
	if (i <= -0x11) i -= 0x50; /* -0x61以下へ */
	if (0x100 <= i && i <= 0x10000 && (i & (i - 1)) == 0) {
		i = - askaLog2(i) - 0x48; /* -0x50から-0x58へ (-0x59から-0x5fはリザーブ) */
	}
	return i;
}

#if 0

char fe0501a(const UCHAR *p, int lastLabel, char mod, int p3f)
{
	char r = 0;
	if (mod == 1) {
		if (p[0] == 0x03 && p[1] == 0x30 && (p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]) == lastLabel + 1 &&
			p[6] == 0x1e && p[7] == 0x3f && p[8] == p3f && p[9] == 0x01 && p[10] == 0x01 &&
			(p[11] << 24 | p[12] << 16 | p[13] << 8 | p[14]) == lastLabel + 1 &&
			p[15] == 0xfe && p[16] == 0x01 && p[17] == 0x00)
		{
			r = 1;
		}
	}
	return r;
}

const UCHAR *fe0501b(UCHAR **qq, char *pof, int b0, int b1, const UCHAR *p)
{
	int r3f;
	if (b0 >= 0x30) {
		while (b0 <= b1) {
			if (*p == 0x02 && p[1] == b0) {
				r3f = appackEncodeConst(p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
				appackSub1(qq, pof, r3f, 0);
				p += 6;
			} else if (*p == 0x10 && p[1] == b0 && p[3] == 0xff) {
				appackSub1(qq, pof, - p[2] - 16, 0);
				p += 4;
			} else {
				appackSub1(qq, pof, - b0 - 16, 0);
			}
			b0++;
		}
	}
	return p;
}

const UCHAR *fe0501c(UCHAR **qq, char *pof, int c1, const UCHAR *p)
{
	int c0 = 0x31;
	while (c0 <= c1) {
		if (*p == 0x1e && p[1] == c0) {
			appackSub1(qq, pof, - p[2] - 16, 0);
			p += 3;
		} else {
			appackSub1(qq, pof, - c0 - 16, 0);
		}
		c0++;
	}
	return p;
}

const UCHAR *fe0501d(UCHAR **qq, char *pof, int d, const UCHAR *p)
{
	int i, j, len;
	UCHAR f;
	if ((d & 1) != 0) {
		if (*p != 0xff) goto skip;
		if (p[1] == 0x00 && p[2] == 0x00) {	// len+UINT8
			appackSub1(qq, pof, 0, 1);
			p += 3;
			len = strlen(p);
			appackSub1(qq, pof, len, 1);
			do {
				i = *p++;
				appackSub0(qq, pof, (i >> 4) & 0x0f);
				appackSub0(qq, pof,  i       & 0x0f);
			} while (--len > 0);
			p++;
		} else if (p[1] == 0x02 && p[2] == 0x02) {	// len+Pxx
			appackSub1(qq, pof, 2, 1);
			p += 3;
			if (*p == 0x10) { // CP
				appackSub1(qq, pof, - p[2] - 16, 0);
				p += 4;
			} else {	// LIMM
				int r3f = appackEncodeConst(p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
				appackSub1(qq, pof, r3f, 0);
				p += 6;
			}
			// PCP
			appackSub1(qq, pof, - p[2] - 16, 0);
			p += 3;
		} else if (p[1] == 0x00 && p[2] == 0x03) {	// UINT8+term(zero)
			appackSub1(qq, pof, 3, 1);
			p += 3;
			do {
				i = *p++;
				appackSub0(qq, pof, (i >> 4) & 0x0f);
				appackSub0(qq, pof,  i       & 0x0f);
			} while (i != 0);
		} else if (p[1] == 0x01 && p[2] == 0x04) {	// hh4(signed)+term(zero)
			appackSub1(qq, pof, 4, 1);
			p += 3;
			do {
				i = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
				p += 4;
				appackSub1(qq, pof, i, 0);
			} while (i != 0);
		} else if (p[1] == 0x00 && p[2] == 0x05) {	// special: len(-4)+UINT7
			appackSub1(qq, pof, 5, 1);
			p += 3;
			int len = strlen(p);
			if (len < 4 || 19 < len) { fprintf(stderr, "fe0501d: mode=5, len=%d\n", len); exit(1); }
			appackSub0(qq, pof, len - 4);
			f = 1;
			do {
				i = *p++;
				for (j = 6; j >= 0; j--) {
					f = f << 1 | ((i >> j) & 1);
					if (f >= 0x10) {
						appackSub0(qq, pof, f & 0x0f);
						f = 1;
					}
				}
			} while (--len > 0);
			p++;
			while (f > 1) {
				f <<= 1;
				if (f >= 0x10) {
					appackSub0(qq, pof, f & 0x0f);
					f = 1;
				}
			}
		} else if (p[1] == 0x00 && p[2] == 0x06) {	// special: UINT7+term(zero)
			appackSub1(qq, pof, 6, 1);
			p += 3;
			f = 1;
			do {
				i = *p++;
				for (j = 6; j >= 0; j--) {
					f = f << 1 | ((i >> j) & 1);
					if (f >= 0x10) {
						appackSub0(qq, pof, f & 0x0f);
						f = 1;
					}
				}
			} while (i != 0);
			while (f > 1) {
				f <<= 1;
				if (f >= 0x10) {
					appackSub0(qq, pof, f & 0x0f);
					f = 1;
				}
			}
		} else
			goto skip;
	}
	if ((d & 2) != 0) {
		if (*p != 0xff) goto skip;
		if (p[1] == 0x03 && p[2] == 0x01) {	// len+hh4(signed)
			appackSub1(qq, pof, 1, 1);
			p += 3;
			len = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
			p += 4;
			appackSub1(qq, pof, len, 1);
			while (len > 0) {
				i = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
				p += 4;
				appackSub1(qq, pof, i, 0);
				len--;
			}
		} else if (p[1] == 0x02 && p[2] == 0x02) {	// len+Pxx
			appackSub1(qq, pof, 2, 1);
			p += 3;
			if (*p == 0x10) { // CP
				appackSub1(qq, pof, - p[2] - 16, 0);
				p += 4;
			} else {	// LIMM
				int r3f = appackEncodeConst(p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]);
				appackSub1(qq, pof, r3f, 0);
				p += 6;
			}
			// PCP
			appackSub1(qq, pof, - p[2] - 16, 0);
			p += 3;

		} else
			goto skip;

	}
skip:
	return p;
}

const UCHAR *fe0501e(UCHAR **qq, char *pof, int e1, const UCHAR *p)
{
	int e0 = 0x30;
	while (e0 <= e1) {
		if (*p == 0x10 && p[2] == e0 && p[3] == 0xff) {
			appackSub1(qq, pof, p[1], 1);
			p += 4;
		} else {
			appackSub1(qq, pof, e0, 1);
		}
		e0++;
	}
	return p;
}

const UCHAR *fe0501f(UCHAR **qq, char *pof, int f1, const UCHAR *p, int *pxxTyp)
{
	int f0 = 0x31;
	while (f0 <= f1) {
		if (*p == 0x1e && p[2] == f0) {
			appackSub1(qq, pof, p[1], 1);
			pxxTyp[p[1]] = -2; // もっと頑張るべきかもしれないけど、とりあえず.
			p += 3;
		} else {
			appackSub1(qq, pof, f0, 1);
		}
		f0++;
	}
	return p;
}

const UCHAR *fe0501bb(int b0, int b1, const UCHAR *p)
{
	int r3f, rxx = 0;
	if (b0 >= 0x30) {
		while (b0 <= b1) {
			if (*p == 0x10 && p[1] == b0 && p[2] == rxx && p[3] == 0xff) {
				p += 4;
			} else {
				p = NULL;
				break;
			}
			b0++;
			rxx++;
		}
	}
	return p;
}

const UCHAR *fe0501cc(int c0, int c1, const UCHAR *p)
{
	int pxx = 1;
	while (c0 <= c1) {
		if (*p == 0x1e && p[1] == c0 && p[2] == pxx) {
			p += 3;
		} else {
			p = NULL;
			break;
		}
		c0++;
		pxx++;
	}
	return p;
}

#endif

#define TYP_CODE		0
#define TYP_UNKNOWN		-1

#define MODE_REG_LC3	1	// osecpu120dから試験的に導入.

typedef int Int32;

typedef struct _Appack0Label {
	signed char opt;
	int typ, abs;
} Appack0Label;

typedef struct _AppackWork {
//	const unsigned char *p;
	unsigned char *q;
	char qHalf;
	int rep[4][0x40], immR3f;
	int wait1, wait7, wait3d, waitEnd, waitLb0;
	int pxxTyp[64];
	int hist[0x40];
	unsigned char opTbl[256];
	Appack0Label label[MAXLABELS];
	int lastLabel;
	int dr[4];
	int vecPrfx, vecPrfxMode;

	// for param.
	Int32 prm_r[0x40], prm_p[0x40], prm_f[0x40];
	double prm_fd[0x40];

	// for simple=0.
	int v_xsiz, v_ysiz;
} AppackWork;

void appack_initWork(AppackWork *aw)
{
	int i, j;
	for (i = 0; i < 0x40; i++)
		aw->pxxTyp[i] = TYP_UNKNOWN;
	for (i = 0; i < 0x40; i++)
		aw->hist[i] = 0;
	for (i = 0; i < 256; i++)
		aw->opTbl[i] = i;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 8; j++)
			aw->rep[i][j] = j;
	}
	for (j = 0; j < 8; j++)
		aw->rep[3][j] = 2 << j;

	aw->opTbl[0x14] = 0x00;
	aw->opTbl[0x00] = 0x14;
	aw->immR3f = 0;
	aw->wait1 = 0;
	aw->wait7 = 0;
	aw->wait3d = 0;
	aw->waitEnd = 0;
	aw->v_xsiz = 640;
	aw->v_ysiz = 480;
	aw->lastLabel = -1;
	aw->dr[0] = 0;
	return;
}

void appackSub0(AppackWork *aw, int i)
// 4bit出力.
{
	UCHAR *q = aw->q;
	if (aw->qHalf == 0) {
		*q++ = (i << 4) & 0xf0;
		aw->q = q;
	} else {
		q[-1] &= ~0x0f;
		q[-1] |= i & 0x0f;
	}
	aw->qHalf ^= 1;
	return;
}

void appackSub1u(AppackWork *aw, int i)
{
	if (i <= 0x6)
		appackSub0(aw, i);
	if (0x7 <= i && i <= 0x3f) {
		appackSub0(aw, i >> 4 | 0x8);
		goto bit4;
	}
	if (0x40 <= i && i <= 0x1ff) {
		appackSub0(aw, i >> 8 | 0xc);
		goto bit8;
	}
	if (0x200 <= i && i <= 0xfff) {
		appackSub0(aw, 0xe);
		goto bit12;
	}
	if (0x1000 <= i && i <= 0xffff) {
		appackSub0(aw, 0x7);
		appackSub0(aw, 0x4);
		goto bit16;
	}
	if (0x10000 <= i && i <= 0xfffff) {
		appackSub0(aw, 0x7);
		appackSub0(aw, 0x5);
		goto bit20;
	}
	if (0x100000 <= i && i <= 0xffffff) {
		appackSub0(aw, 0x7);
		appackSub0(aw, 0x6);
		goto bit24;
	}
	if (0x1000000 <= i && i <= 0xfffffff) {
		appackSub0(aw, 0x7);
		appackSub0(aw, 0x8);
		appackSub0(aw, 0x7);
		goto bit28;
	}
	if (0x10000000 <= i) {
		appackSub0(aw, 0x7);
		appackSub0(aw, 0x8);
		appackSub0(aw, 0x8);
		appackSub0(aw, i >> 28);
bit28:
		appackSub0(aw, i >> 24);
bit24:
		appackSub0(aw, i >> 20);
bit20:
		appackSub0(aw, i >> 16);
bit16:
		appackSub0(aw, i >> 12);
bit12:
		appackSub0(aw, i >> 8);
bit8:
		appackSub0(aw, i >> 4);
bit4:
		appackSub0(aw, i);
	}
	return;
}

void appackSub1s(AppackWork *aw, int i, char len0)
// 定数専用, { 0, 1, 2, 3, 4, 5, -1 }
{
	if (len0 <= 3) {
		if (i == -1) {
			appackSub0(aw, 0x6);
			goto fin;
		}
		if (0 <= i && i <= 0x5) {
			appackSub0(aw, i);
			goto fin;
		}
	}
	if (len0 <= 6) {
		if (-0x20 <= i && i <= 0x1f) {
			i &= 0x3f;
			appackSub0(aw, i >> 4 | 0x8);
			goto bit4;
		}
	}
	if (len0 <= 9) {
		if (-0x100 <= i && i <= 0x0ff) {
			i &= 0x1ff;
			appackSub0(aw, i >> 8 | 0xc);
			goto bit8;
		}
	}
	if (len0 <= 12) {
		if (-0x800 <= i && i <= 0x7ff) {
			i &= 0xfff;
			appackSub0(aw, 0xe);
			goto bit12;
		}
	}
	if (-0x8000 <= i && i <= 0x7fff) {
		i &= 0xffff;
		appackSub0(aw, 0x7);
		appackSub0(aw, 0x4);
		goto bit16;
	}
	if (-0x80000 <= i && i <= 0x7ffff) {
		i &= 0xfffff;
		appackSub0(aw, 0x7);
		appackSub0(aw, 0x5);
		goto bit20;
	}
	if (-0x800000 <= i && i <= 0x7fffff) {
		i &= 0xffffff;
		appackSub0(aw, 0x7);
		appackSub0(aw, 0x6);
		goto bit24;
	}
	if (-0x8000000 <= i && i <= 0x7ffffff) {
		i &= 0xfffffff;
		appackSub0(aw, 0x7);
		appackSub0(aw, 0x8);
		appackSub0(aw, 0x7);
		goto bit28;
	}

	appackSub0(aw, 0x7);
	appackSub0(aw, 0x8);
	appackSub0(aw, 0x8);
	appackSub0(aw, i >> 28);
bit28:
	appackSub0(aw, i >> 24);
bit24:
	appackSub0(aw, i >> 20);
bit20:
	appackSub0(aw, i >> 16);
bit16:
	appackSub0(aw, i >> 12);
bit12:
	appackSub0(aw, i >> 8);
bit8:
	appackSub0(aw, i >> 4);
bit4:
	appackSub0(aw, i);

//	goto fin;

//	fprintf(stderr, "appackSub1s: error: i=0x%x\n", i);
//	exit(1);
fin:
	return;
}

static int len3table0[7] = { -1, 0, 1, 2, 3, 4, -0x10+0 /* rep0 */ };
//static int len3table2[7] = { -1, 0, 1, -0x10+0, -0x10+1, -0x10+2, -0x10+3 };
//static int len3table2[7] = { -1, 0, 1, 2, -0x10+0, -0x10+1, -0x10+2 };
//static int len3table2[7] = { -1, 0, 1, 2, 3, -0x10+0, -0x10+1 };
static int len3table2[7] = { -1, 0, 1, 2, 3, 4, -0x10+0 };
static int len3table0f[7] = { -0xfff, -4, -3, -2, -1, 0, 1 }; // -0xfffはリザーブ.
static int len3table14[7] = { -1, -0x10+1 /* rep1 */, 1,  2,  3,  4, -0x10+0 /* rep0 */ }; // ADD

#if 0

void appackSub1i(AppackWork *aw, int i, int *t)
// 定数および整数レジスタ, リピートもあり.
// fcode_getIntegerに対応している.
{
	int j;
	if ((i & 0xffffffc0) == 0x80520000) {
		i &= 0x3f;
		for (j = 0; j < 8; j++) {
			if (aw->rep[0][j] == i) break;
		}
		if (j < 8)
			i = j | (-0x10);
		else {
			if (i == 0x3f) i = -0x11;
			if (0x00 <= i && i <= 0x0e) i |= -0x20; // -0x20 ... -0x12
			if (i == 0x0f) i = -0x21;
			if (0x10 <= i && i <= 0x3e) i -= 0x60; // -0x50 ... -0x22
		}
	} else {
		if (i <= -5) i -= 0x71 - 5; // 特殊変換のためにずらす.
		if (i ==  0x20) i = -0x08;
		if (i ==  0x40) i = -0x07;
		if (i ==  0x80) i = -0x06;
		if (i == 0x100) i = -0x05;
		for (j = 0; j <= 22; j++) {
			if (i == (1 << (j + 9)))
				i = j + (-0x68);	// -0x68 ... -0x52 (-0x51は1<<32)
		}
	}
	for (j = 0; j < 7; j++) {
		if (t[j] == i) break;
	}
	if (j < 7)
		appackSub1s(aw, j - 1, 0);
	else
		appackSub1s(aw, i, 1);
	return;
}

#define APPACK_SUB1R_PRM	5	// ここは使ってない.

void appackSub1r(AppackWork *aw, int i, char mode)
// 整数レジスタのみ, リピートもあり.
// fcode_getRegに対応している.
{
	int j;
	if (mode == 1 && i <= 6) {
		appackSub0(aw, i);
		goto fin;
	}
	for (j = 0; j < 8; j++) {
		if (aw->rep[0][j] == i) break;
	}
	if (j >= APPACK_SUB1R_PRM && i < 7 - APPACK_SUB1R_PRM)
		j = 8;
	if (j < 8) {
		if (j < APPACK_SUB1R_PRM && mode == 0)
			appackSub0(aw, (7 - APPACK_SUB1R_PRM) + j);
		else {
			appackSub0(aw, 0x9);
			appackSub0(aw, 0x8 + j);
		}
	} else {
		if (i < 7 - APPACK_SUB1R_PRM && mode == 0)
			appackSub0(aw, i);
		else if (i <= 0x17) {
			appackSub0(aw, 0x8 | i >> 4);
			appackSub0(aw, i & 0xf);
		} else if (i <= 0x20) { // R18-R1F
			appackSub0(aw, 0xc);
			appackSub0(aw, 0x4);
			appackSub0(aw, i & 0x7);
		} else {	// R20-R3F
			appackSub0(aw, 0x8 | i >> 4);
			appackSub0(aw, i & 0xf);
		}
	}
fin:
	return;
}

void appackSub1p(AppackWork *aw, int i, char mode)
// ポインタレジスタのみ, リピートもあり.
// fcode_getRegに対応している.
{
	int j;
	if (mode == 1 && i <= 6) {
		appackSub0(aw, i);
		goto fin;
	}
	for (j = 0; j < 8; j++) {
		if (aw->rep[1][j] == i) break;
	}
	if (j >= APPACK_SUB1R_PRM && i < 7 - APPACK_SUB1R_PRM)
		j = 8;
	if (j < 8) {
		if (j < APPACK_SUB1R_PRM && mode == 0)
			appackSub0(aw, (7 - APPACK_SUB1R_PRM) + j);
		else {
			appackSub0(aw, 0x9);
			appackSub0(aw, 0x8 + j);
		}
	} else {
		if (i < 7 - APPACK_SUB1R_PRM && mode == 0)
			appackSub0(aw, i);
		else if (i <= 0x17) {
			appackSub0(aw, 0x8 | i >> 4);
			appackSub0(aw, i & 0xf);
		} else if (i <= 0x20) { // P18-P1F
			appackSub0(aw, 0xc);
			appackSub0(aw, 0x4);
			appackSub0(aw, i & 0x7);
		} else {	// P20-P3F
			appackSub0(aw, 0x8 | i >> 4);
			appackSub0(aw, i & 0xf);
		}
	}
fin:
	return;
}

#endif

#define APPACK_SUB1R_PRM	6

int appack_getRep(const int *repRawList, const int *len3table, int mode, int *rep)
{
	int i, j = 0, k;
	if (mode == 0) {
		// for fcode_getInteger.
		k = -0x11;
		for (i = 0; i < 7; i++) {
			if (len3table[i] <= -0x10+7) {
				if (k < len3table[i])
					k = len3table[i];
			}
		}
		k += 0x11;
		for (i = 0; i < 0x40; i++) {
			if (i < k) {
				// 4ビットで書けるもの.
				rep[j] = repRawList[i];
				j++;
			} else {
				// 8ビット以上.
				if (0x0f <= repRawList[i] && repRawList[i] <= 0x3e) {
					rep[j] = repRawList[i];
					j++;
				}
			}
		}
	}
	if (mode == 1) {
		// for fcode_getReg (mode=0).
		for (i = 0; i < 0x40; i++) {
			if (i < APPACK_SUB1R_PRM) {
				// 4ビットで書けるもの.
				if (repRawList[i] >= 7 - APPACK_SUB1R_PRM) {
					rep[j] = repRawList[i];
					j++;
				}
			} else {
				// 8ビット以上.
				if (0x20 <= repRawList[i] && repRawList[i] <= 0x27) {
					rep[j] = repRawList[i];
					j++;
				}
			}
		}
	}
	if (mode >= 2) {
		fprintf(stderr, "appack_getRep: error: mode=%d\n", mode);
		exit(1);
	}
	return j;
}

void appackSub1i(AppackWork *aw, int i, int *t)
// 定数および整数レジスタ, リピートもあり.
// fcode_getIntegerに対応している.
{
	int j, k, rep[0x40];
	if ((i & 0xffffffc0) == 0x80520000) {
		i &= 0x3f;
		k = appack_getRep(aw->rep[0], t, 0, rep);
		for (j = 0; j < k; j++) {
			if (rep[j] == i) break;
		}
		if (j < k && j <= 7) {
			// rep
			for (i = 0; i < 7; i++) {
				if (t[i] == -0x10 + j) break;
			}
			if (i < 7)
				appackSub1s(aw, i - 1, 3);
			else
				appackSub1s(aw, -0x10 + j, 6);
		} else {
			if (i <= 0x0e)
				appackSub1s(aw, -0x20 + i, 6);
			else if (i == 0x3f)
				appackSub1s(aw, -0x20 + 0x0f, 6);
			else
				appackSub1s(aw, -0x100 + i, 9);
		}
	} else {
		for (j = 0; j < 7; j++) {
			if (t[j] >= -0x10 + 8 && t[j] == i) break;
		}
		if (j < 7) {
			appackSub1s(aw, j - 1, 3);
			goto fin;
		}
		if (-0x8 <= i && i <= 0x16) {
			appackSub1s(aw, i, 6);
			goto fin;
		}
		if (0x18 <= i && i <= 0x100) {
			static int table[9] = {
				0x20, 0x18, 0x40, 0x80, 0x100, 0xff, 0x7f, 0x3f, 0x1f
			};
			for (j = 0; j < 9; j++) {
				if (table[j] == i) break;
			}
			if (j < 9) {
				appackSub1s(aw, j + 0x17, 6);
				goto fin;
			}
		}
		if (-0x90 <= i && i <= 0xf0) {
			appackSub1s(aw, i, 9);
			goto fin;
		}
		if (0xf0 <= i && i <= 0x8000) {
			static int table[17] = {
				0x200, 0xf0, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0x10000,
				0xffff, 0x7fff, 0x3fff, 0x1fff, 0xfff, 0x7ff, 0x3ff, 0x1ff
			};
			for (j = 0; j < 17; j++) {
				if (table[j] == i) break;
			}
			if (j < 17) {
				appackSub1s(aw, j + 0xef, 9);
				goto fin;
			}
		}
		if (i >= 0x20000 && (i & (i - 1)) == 0) {
			static int table[14] = {
				0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000, 0x1000000,
				0x2000000, 0x4000000, 0x8000000, 0x10000000, 0x2000000, 0x40000000
			};
			for (j = 0; j < 14; j++) {
				if (table[j] == i) break;
			}
			if (j < 14) {
				appackSub1s(aw, -0xa0 + j, 9);
				goto fin;
			}
		}
		if (-0x620 <= i && i <= 0x7de) {
			appackSub1s(aw, i, 12);
			goto fin;
		}
		appackSub1s(aw, i, 16);
	}
fin:
	return;
}

void appackSub1rpf(AppackWork *aw, int i, char mode, int typ)
// fcode_getRegに対応している.
{
	int j, k, rep[0x40];
	if (mode == 1) {
		appackSub1u(aw, i);
		goto fin;
	}
	k = appack_getRep(aw->rep[typ], NULL, 1, rep);
	for (j = 0; j < k; j++) {
		if (rep[j] == i) break;
	}
	if (j < k && j <= 7) {
		// rep
		if (j < APPACK_SUB1R_PRM)
			appackSub1u(aw, (7 - APPACK_SUB1R_PRM) + j);
		else
			appackSub1u(aw, 0x20 + j);
		goto fin;
	}
	if (i < 7 - APPACK_SUB1R_PRM) {
		appackSub1u(aw, i);
		goto fin;
	}
	if (0x20 <= i && i <= 0x27) {
		appackSub0(aw, 0xc);
		appackSub0(aw, i >> 4);
		appackSub0(aw, i & 0xf);
	} else {
		appackSub0(aw, 0x8 | i >> 4);
		appackSub0(aw, i & 0xf);
	}
fin:
	return;
}

void appackSub1r(AppackWork *aw, int i, char mode) { appackSub1rpf(aw, i, mode, 0); }
void appackSub1p(AppackWork *aw, int i, char mode) { appackSub1rpf(aw, i, mode, 1); }
void appackSub1f(AppackWork *aw, int i, char mode) { appackSub1rpf(aw, i, mode, 2); }

void appackSub1op(AppackWork *aw, int i)
{
	if (i < 256)
		i = aw->opTbl[i];
	appackSub1u(aw, i);
	if (i < 0x40)
		aw->hist[i]++;
	return;
}

void appack_updateRep(AppackWork *aw, int typ, int r)
{
	int tmp0 = r, tmp1, i;
	for (i = 0; i < 0x40; i++) {
		tmp1 = aw->rep[typ][i];
		aw->rep[typ][i] = tmp0;
		if (tmp1 == r) break;
		tmp0 = tmp1;
	}
	return;
}

void appack_initLabel(struct Work *w, AppackWork *aw)
{
	int i, j, k, buf[16], bp = 0;
	const unsigned char *p, *pp;

	for (i = 0; i < MAXLABELS; i++) {
		aw->label[i].opt = -1;	// undefined
		aw->label[i].typ = TYP_UNKNOWN;
		aw->label[i].abs = -1;
	}

	for (p = w->ibuf + DB2BIN_HEADER_LENGTH; p < w->ibuf + w->isiz; ) {
		if (cmpBytes(p, "f1_f788#4_f788") != 0) {
			i = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			if (aw->label[i].opt >= 0) {
				fputs("appack_initLabel: label redefined.\n", stderr);
				exit(1);
			}
			aw->label[i].opt = p[9] << 24 | p[10] << 16 | p[11] << 8 | p[12];
			aw->label[i].typ = TYP_CODE;
			if (bp >= 16) {	// 16以上のLB命令の連続には対応できない.
				fputs("appack_initLabel: internal error.\n", stderr);
				exit(1);
			}
			buf[bp++] = i; // LB命令が連続したらバッファにためる.
		}
		if (cmpBytes(p, "ae_f788#4_f788") != 0) {
			j = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			for (i = 0; i < bp; i++)
				aw->label[buf[i]].typ = j;
		}
		if (*p != 0xf1)
			bp = 0;
		p += db2binSub2Len(p);
	}

	// opt=0,1について、abs[]を設定.
	// todo: この計算方法はおかしい.
	j = k = 0;
	for (i = 0; i < MAXLABELS; i++) {
		if (aw->label[i].opt == 0) aw->label[i].abs = j++;
		if (aw->label[i].opt == 1) aw->label[i].abs = k++;
	}

	j = 1;
	for (i = 0; i < MAXLABELS; i++) {
		if (j > 4) break;
		if (aw->label[i].opt != 1) continue;
		if (aw->label[i].typ == TYP_CODE) continue;
		aw->pxxTyp[j] = aw->label[i].typ;
		j++;
	}
	return;
}

const UCHAR *appack0_param0(const UCHAR *p, AppackWork *aw)
{
	int i;
	for (i = 0; i < 0x40; i++) {
		aw->prm_r[i] = 0x8052003f;
		aw->prm_p[i] = 0x8052003f;
 		aw->prm_f[i] = 0x8052003f;
		aw->prm_fd[i] = 0.0;
	}
	for (;;) {
		if (cmpBytes(p, "f2_f788#4_bx_f78800000020") != 0) {
			// LIMM(R30-R3B).
			i = p[7] & 0x3f;
			if (i <= 0x3b) {
				aw->prm_r[i] = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
				p += 14;
				continue;
			}
		}
		if (cmpBytes(p, "90YxYxbx_f78800000020") != 0 && p[1] == p[2]) {
			// CP(R30-R3B).
			i = p[3] & 0x3f;
			if (i <= 0x3b) {
				aw->prm_r[i] = 0x80520000 | (p[1] & 0x3f);
				p += 10;
				continue;
			}
		}
		break;
	}
	if (cmpBytes(p, "f3_f788#4_b0 9e_af_bf f1_f788#4_f78800000003 fcfe00") == 0)	// 8+3+13+3=27
		p = NULL;
	else
		p += 27;
	return p;
}

char appack0_param1(AppackWork *aw, int n, int r0)
{
	char retCode = 0;
	int i;
	for (i = 0; i < n; i++) {
		if (aw->prm_r[r0 + i] != (0x80520000 | i))
			break;
	}
	if (i >= n)
		retCode = 1;
	return retCode;
}

void appack0_waitFlush(AppackWork *aw)
{
	if (aw->wait1 != 0) {
		appackSub1op(aw, 0x01);
		aw->wait1 = 0;
	}
	while (aw->wait7 > 0) {
		appackSub1op(aw, 0x07);
		aw->wait7--;
	}
	if (aw->wait3d > 0) {
		appackSub1op(aw, 0x3d);
		aw->wait3d = 0;
	}
	if (aw->waitEnd > 0) {
		appackSub1op(aw, 0x05);
		appackSub1s(aw, 0x0009, 0);
		appackSub1i(aw, 0, len3table0);
		appackSub1i(aw, -1, len3table0);
		aw->waitEnd = 0;
	}
	return;
}

int appackSub2a(AppackWork *aw, char mode, int *pofs)
// 相対指定の原点を探す.
{
	*pofs = 0;
	int i = aw->lastLabel;
	if (mode == 0) {	// TYP_CODEで、opt=0-1.
		for (;;) {
			if (i < 0) break;
			if (0 <= aw->label[i].opt && aw->label[i].opt <= 1 && aw->label[i].typ == TYP_CODE) break;
			i--;
		}
		if (i < 0) {
			*pofs = 1;
			for (i = 0; i < MAXLABELS; i++) {
				if (0 <= aw->label[i].opt && aw->label[i].opt <= 1 && aw->label[i].typ == TYP_CODE) break;
			}
		}
	}
	if (mode == 1) {	// opt=1のみ.
		for (;;) {
			if (i < 0) break;
			if (aw->label[i].opt == 1) break;
			i--;
		}
		if (i < 0) {
			*pofs = 1;
			for (i = 0; i < MAXLABELS; i++) {
				if (aw->label[i].opt == 1) break;
			}
		}
	}
	if (i >= MAXLABELS) {
		fprintf(stderr, "appackSub2a: error. lastLabel=%d, mode=%d\n", aw->lastLabel, mode);
		exit(1);
	}
	return i;
}

int appackSub2b(AppackWork *aw, char mode, int j)
{
	int retCode;
	int i = appackSub2a(aw, mode, &retCode);
	if (i == j) goto fin;
	if (mode == 0) {	// TYP_CODEで、opt=0-1.
		if (!(0 <= aw->label[j].opt && aw->label[j].opt <= 1 && aw->label[j].typ == TYP_CODE)) {
			fprintf(stderr, "appackSub2b: PLIMM type error. lastLabel=%d, mode=%d, j=%d (DR0=%d)\n", aw->lastLabel, mode, j, aw->dr[0]);
			exit(1);
		}
		if (i > j) {
			while (i > j) {
				i--;
				retCode--;
				for (;;) {
					if (0 <= aw->label[i].opt && aw->label[i].opt <= 1 && aw->label[i].typ == TYP_CODE) break;
					i--;
				}
			}
		} else {
			while (i < j) {
				i++;
				retCode++;
				for (;;) {
					if (0 <= aw->label[i].opt && aw->label[i].opt <= 1 && aw->label[i].typ == TYP_CODE) break;
					i++;
				}
			}
		}
	}
	if (mode == 1) {	// opt=1.
		if (aw->label[j].opt != 1) {
			fprintf(stderr, "appackSub2b: PLIMM opt error. lastLabel=%d, mode=%d, j=%d\n", aw->lastLabel, mode, j);
			exit(1);
		}
		if (i > j) {
			while (i > j) {
				i--;
				retCode--;
				for (;;) {
					if (aw->label[i].opt == 1) break;
					i--;
				}
			}
		} else {
			while (i < j) {
				i++;
				retCode++;
				for (;;) {
					if (aw->label[i].opt == 1) break;
					i++;
				}
			}
		}
	}
fin:
	return retCode;
}

#define R0	-0x10+0 /* rep0 */
#define R1	-0x10+1 /* rep1 */

int appack0(struct Work *w, char flags)
{
	const UCHAR *p = w->ibuf, *pp, *p1;
	UCHAR *q = w->obuf, *qq;
//	int r3f = 0, lastLabel = -1, i, j, wait7 = 0, firstDataLabel = -1;
	int i = 0, j, k, l, m, n;
	AppackWork aw;
	char simple = flags & 1, encLidr = flags & 2, flg4, flgD;
	*q++ = *p++;
	*q++ = *p++;
	if (*p != 0x00) {
		fprintf(stderr, "appack: input-file format error.\n");
		exit(1);
	}
	p++;
	qq = db2binSub2(w->ibuf + DB2BIN_HEADER_LENGTH, w->ibuf + w->isiz); // ラベル番号の付け直し.
	w->isiz = qq - w->ibuf;
	appack_initWork(&aw);
	appack_initLabel(w, &aw);
	p1 = w->ibuf + w->isiz;
	aw.q = q;
	aw.qHalf = 0;
	aw.vecPrfx = -1;
	aw.vecPrfxMode = 0;
	while (p < p1) {
//		fprintf(stderr, "appack0: op=%02x-%02x-%02x-%02x (DR0=0x%08x)\n", *p, p[1], p[2], p[3], aw.dr[0]);

		if (*p == 0xf0) {
			p++;
			continue;
		}

		if (cmpBytes(p,"f1_f788#4_f78800000001 ae_f788#4f788") != 0) {
		//	appack0_waitFlush(&aw);
			i = p[16] << 24 | p[17] << 16 | p[18] << 8 | p[19];
			j = p[22] << 24 | p[23] << 16 | p[24] << 8 | p[25];
			appackSub1op(&aw, 0x2e);
			appackSub1u(&aw, i);
			appackSub1u(&aw, j);
			j *= db2binDataWidth(i);
			j >>= 3; // j /= 8;
			p += 26;
			for (i = 0; i < j; i++) {
				appackSub0(&aw, (*p >> 4) & 0x0f);
				appackSub0(&aw,  *p       & 0x0f);
				p++;
			}
			aw.lastLabel++;
			continue;
		}
		if (cmpBytes(p,"f1_f788#4_f78800000001 fcfe00 fcfd_f788#4_80 bc_f788#4_f788#4_f788#4_f788#4_f788#4_f788") != 0) {
			p += 13 + 3 + 9 + 37;
			goto appack0_bc;
		}
		if (cmpBytes(p,"f1_f788#4_f78800000001 fcfe00 bc_f788#4_f788#4_f788#4_f788#4_f788#4_f788") != 0) {
			p += 13 + 3 + 37;
appack0_bc:
			i = p[-34] << 24 | p[-33] << 16 | p[-32] << 8 | p[-31];
			j = p[-28] << 24 | p[-27] << 16 | p[-26] << 8 | p[-25];
			k = p[-22] << 24 | p[-21] << 16 | p[-20] << 8 | p[-19];
			l = p[-16] << 24 | p[-15] << 16 | p[-14] << 8 | p[-13];
			m = p[-10] << 24 | p[ -9] << 16 | p[ -8] << 8 | p[ -7];
			n = p[ -4] << 24 | p[ -3] << 16 | p[ -2] << 8 | p[ -1];
			if (i == 32 && j == 32 && k == 16 && l == 16 && m == 64 && n == 0) {
				aw.wait1 = 0;
				aw.wait7 = 0;
				aw.wait3d = 0;
				aw.waitEnd = 0;
				aw.waitLb0 = 0;
				appackSub1op(&aw, 0x3c);
#if 0
				appack_updateRep(&aw, 0, 0x33);
				appack_updateRep(&aw, 0, 0x32);
				appack_updateRep(&aw, 0, 0x31);
				appack_updateRep(&aw, 0, 0x30);
				appack_updateRep(&aw, 1, 0x32);
				appack_updateRep(&aw, 1, 0x31);
				appack_updateRep(&aw, 2, 0x31);
				appack_updateRep(&aw, 2, 0x30);
#endif
				aw.lastLabel++;
				continue;
			}
			fprintf(stderr, "appack0: error: op=bc (DR0=0x%08x)\n", *p, aw.dr[0]);
			exit(1);
		}
		if (cmpBytes(p,"f1_f788#4_f78800000000") != 0) {
			appack0_waitFlush(&aw);
			if (aw.waitLb0 > 0) {
				aw.wait1 = 1;
				aw.waitLb0--;
			} else
				appackSub1op(&aw, 0x01);
			aw.lastLabel++;
			p += 13;
			continue;
		}
		if (cmpBytes(p, "f2_f788#4_bb_f78800000020 f2_f788#4_ba_f78800000020 b2_bb_f78800000020_ba_f78800000020_Yx") != 0) {	// malloc
			appack0_waitFlush(&aw);
			i = p[ 3] << 24 | p[ 4] << 16 | p[ 5] << 8 | p[ 6]; // typ
			j = p[17] << 24 | p[18] << 16 | p[19] << 8 | p[20]; // len
			k = p[43] & 0x3f;
			if (cmpBytes(p + 44, "9e_Yx_bb f2_f788#4_b9_f78800000020 f2_f78800000000_bb_f78800000020 "
					"f1_f788#4_f78800000002 89_b9_f78800000020_bb_f788#4_f78800000000 f2_f78800000001_bf_f78800000020 "
					"8e_bb_f788#4_bf_f78800000020_bb f2_f78800000001_bf_f78800000020 94_bb_bf_bb_f78800000020 "
					"a0_ba_bb_f78800000020_bf_f78800000020 f4_bf f3_f788#4_bf") != 0) {
				// malloc_initInt
				l = p[50] << 24 | p[51] << 16 | p[52] << 8 | p[53];
				if (l != 0)
					appackSub1op(&aw, 0x04);
				appackSub1op(&aw, 0x32);
				appackSub1i(&aw, i, len3table0);
				appackSub1i(&aw, j, len3table0);
				appackSub1r(&aw, k, MODE_REG_LC3);
				if (l != 0)
					appackSub1i(&aw, l, len3table0);
				aw.pxxTyp[k] = i;
				appack_updateRep(&aw, 1, k);
				p += 189;
				aw.lastLabel++;
				continue;
			}
		}
		if (cmpBytes(p, "f2_f788#4_bf_f78800000020 96_Yx_bf_be_f78800000020 94_be_Yx_Yx_f78800000020") != 0) {	// AFFINE
			i = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];	// a
			for (j = 0; j < 6; j++) {
				if (aw.rep[3][j] == i) break;
			}
			if (i < 0) { j = 6; i *= -1; }
			if (i < 2) {
				fprintf(stderr, "appack0: error: AFFINE a=%d (DR0=0x%08x)\n", i, aw.dr[0]);
				exit(1);
			}
			appack_updateRep(&aw, 3, i);
			if (j < 6)
				i = j - 4;
			j = p[15] & 0x3f;
			k = p[26] & 0x3f;
			l = p[27] & 0x3f;
			appackSub1op(&aw, 0x0f);
			appackSub1i(&aw, i, len3table0f);
			appackSub1r(&aw, j, 0);
			appackSub1i(&aw, k | 0x80520000, len3table14);
			appackSub1r(&aw, l, MODE_REG_LC3);
			appack_updateRep(&aw, 0, j);
			appack_updateRep(&aw, 0, k);
			appack_updateRep(&aw, 0, l);
			p += 34;
			continue;
		}
		if (cmpBytes(p, "f2_f788#4_bf_f78800000020 96_Yx_bf_be_f78800000020 f2_f788#4_bf_f78800000020 94_be_bf_Yx_f78800000020") != 0) {	// AFFINEI
			i = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];	// a
			for (j = 0; j < 6; j++) {
				if (aw.rep[3][j] == i) break;
			}
			if (i < 0) { j = 6; i *= -1; }
			if (i < 2) {
				fprintf(stderr, "appack0: error: AFFINE a=%d (DR0=0x%08x)\n", i, aw.dr[0]);
				exit(1);
			}
			appack_updateRep(&aw, 3, i);
			if (j < 6)
				i = j - 4;
			j = p[15] & 0x3f;
			k = p[27] << 24 | p[28] << 16 | p[29] << 8 | p[30]; // b
			l = p[27 + 14] & 0x3f;
			appackSub1op(&aw, 0x0f);
			appackSub1i(&aw, i, len3table0f);
			appackSub1r(&aw, j, 0);
			appackSub1i(&aw, k, len3table14);
			appackSub1r(&aw, l, MODE_REG_LC3);
			appack_updateRep(&aw, 0, j);
			appack_updateRep(&aw, 0, l);
			p += 34 + 14;
			continue;
		}
		if (cmpBytes(p, "f2_f788#4_Yx_f78800000020") != 0) {	// LIMM
			appack0_waitFlush(&aw);
			i = p[7] & 0x3f;
			j = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			if (i == 0x3f)
				aw.immR3f = j;
			else {
				appackSub1op(&aw, 0x02);
				appackSub1i(&aw, j, len3table2);
				appackSub1r(&aw, i, MODE_REG_LC3);
				appack_updateRep(&aw, 0, i);
			}
			p += 14;
			if (aw.vecPrfx != -1 && i != 0x3f) {
				for (n = 1; n < aw.vecPrfx; n++) {
					while (cmpBytes(p, "fcfd_f788#4_80") != 0)
						p += 9;
					p += 14;
					appack_updateRep(&aw, 0, i + n);
				}
				appack_updateRep(&aw, 0, i);
				aw.vecPrfx = -1;
			}
			continue;
		}
		if (cmpBytes(p, "f3_f788#4_b0 f3_f788#4_bf f1_f788#4_f78800000003 fcfe00") != 0) {	// CALL
			i = p[ 3] << 24 | p[ 4] << 16 | p[ 5] << 8 | p[ 6];
			j = p[19] << 24 | p[20] << 16 | p[21] << 8 | p[22];
			k = p[11] << 24 | p[12] << 16 | p[13] << 8 | p[14];
			if (i == j) {
				appack0_waitFlush(&aw);
				aw.lastLabel++;
				appackSub1op(&aw, 0x3e);
				k = appackSub2b(&aw, 0, k);
				appackSub1s(&aw, k, 0);
				for (i = 0x30; i < 0x40; i++)
					aw.pxxTyp[i] = TYP_UNKNOWN;
				p += 32;
				continue;
			}
		}
		if (cmpBytes(p, "f3_f788#4_Yx") != 0) {
			appack0_waitFlush(&aw);
			i = p[7] & 0x3f;
			j = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			flg4 = 0;
			if (i != 0x3f)
				flg4 = 1;
			if (flg4 != 0)
				appackSub1op(&aw, 0x04);
			appackSub1op(&aw, 0x03);
			k = appackSub2b(&aw, flg4, j);
			appackSub1s(&aw, k, 0);
			if (flg4 != 0) {
				appackSub1p(&aw, i, MODE_REG_LC3);
				appack_updateRep(&aw, 1, i);
				aw.pxxTyp[i] = aw.label[j].typ;
			} else {
				if (aw.waitLb0 < k)
					aw.waitLb0 = k;
			}
			p += 8;
			continue;
		}
		if (cmpBytes(p, "88_Yx_f788#4_f78800000000_Yx_f78800000020") != 0) {
			appack0_waitFlush(&aw);
			i = p[1]  & 0x3f;
			j = p[14] & 0x3f;
			k = p[4] << 24 | p[5] << 16 | p[6] << 8 | p[7];
			p += 21;
			flgD = 0;
			if (aw.pxxTyp[i] != k) {
				flgD = 1;
				aw.pxxTyp[i] = k;
			}
			flg4 = 1;
			if (cmpBytes(p, "f2_f78800000001_bf_f78800000020 8e_Yx_f788#4_bf_f78800000020_Yx") != 0 && p[1-21] == p[15] && p[15] == p[29]) {
				flg4 = 0;
				p += 30;
			}
			if (flg4 != 0)
				appackSub1op(&aw, 0x04);
			if (flgD != 0)
				appackSub1op(&aw, 0x0d);
			appackSub1op(&aw, 0x08);
			appackSub1p(&aw, i, 0);
			if (flgD != 0)
				appackSub1u(&aw, k);
			appackSub1u(&aw, 0);
			appackSub1r(&aw, j, MODE_REG_LC3);
			appack_updateRep(&aw, 1, i);
			appack_updateRep(&aw, 0, j);
			if (aw.vecPrfx != -1) {
				for (n = 1; n < aw.vecPrfx; n++) {
					while (cmpBytes(p, "fcfd_f788#4_80") != 0)
						p += 9;
					p += 21;
					if (flg4 == 0)
						p += 30;
					appack_updateRep(&aw, 0, j + n);
				}
				appack_updateRep(&aw, 0, j);
				aw.vecPrfx = -1;
			}
			continue;
		}
		if (cmpBytes(p, "89_Yx_f78800000020_Yx_f788#4_f78800000000") != 0) {
			appack0_waitFlush(&aw);
			i = p[1] & 0x3f;	// r
			j = p[8] & 0x3f;	// p
			k = p[11] << 24 | p[12] << 16 | p[13] << 8 | p[14];
			p += 21;
			flgD = 0;
			if (aw.pxxTyp[j] != k) {
				flgD = 1;
				aw.pxxTyp[j] = k;
			}
			flg4 = 1;
			if (cmpBytes(p, "f2_f78800000001_bf_f78800000020 8e_Yx_f788#4_bf_f78800000020_Yx") != 0 && p[2-21] == p[15] && p[15] == p[29]) {
				flg4 = 0;
				p += 30;
			}
			if (flg4 != 0)
				appackSub1op(&aw, 0x04);
			if (flgD != 0)
				appackSub1op(&aw, 0x0d);
			appackSub1op(&aw, 0x09);
			appackSub1i(&aw, j | 0x80520000, len3table0);
			appackSub1p(&aw, j, 0);
			if (flgD != 0)
				appackSub1u(&aw, k);
			appackSub1u(&aw, 0);
			appack_updateRep(&aw, 1, j);
			appack_updateRep(&aw, 0, i);
			continue;
		}
		if (cmpBytes(p, "8e_Yx_f788#4_Yx_f78800000020_bf 88_bf_f788#4_f78800000000_Yx_f78800000020") != 0) {
			// PALMEM0
			i = p[1] & 0x3f; // p
			j = p[4] << 24 | p[5] << 16 | p[6] << 8 | p[7];	// typ
			k = p[8] & 0x3f; // offset-r
			l = p[30] & 0x3f; // data-r
			p += 37;
			flgD = 0;
			if (aw.pxxTyp[i] != j) {
				flgD = 1;
				aw.pxxTyp[i] = j;
			}
			if (flgD != 0)
				appackSub1op(&aw, 0x0d);
			appackSub1op(&aw, 0x08);
			appackSub1p(&aw, i, 0);
			if (flgD != 0)
				appackSub1u(&aw, j);
			appackSub1u(&aw, 1);
			appackSub1i(&aw, k | 0x80520000, len3table14);
			appackSub1r(&aw, l, MODE_REG_LC3);
			appack_updateRep(&aw, 1, i);
			appack_updateRep(&aw, 0, k);
			appack_updateRep(&aw, 0, l);
			continue;
		}
		if (cmpBytes(p, "8e_Yx_f788#4_Yx_f78800000020_bf 89_Yx_f78800000020_bf_f788#4_f78800000000") != 0) {
			// PASMEM0
			i = p[1] & 0x3f; // p
			j = p[4] << 24 | p[5] << 16 | p[6] << 8 | p[7];	// typ
			k = p[8] & 0x3f; // offset-r
			l = p[17] & 0x3f; // data-r
			p += 37;
			flgD = 0;
			if (aw.pxxTyp[i] != j) {
				flgD = 1;
				aw.pxxTyp[i] = j;
			}
			if (flgD != 0)
				appackSub1op(&aw, 0x0d);
			appackSub1op(&aw, 0x09);
			appackSub1i(&aw, l | 0x80520000, len3table0);
			appackSub1p(&aw, i, 0);
			if (flgD != 0)
				appackSub1u(&aw, j);
			appackSub1u(&aw, 1);
			appackSub1i(&aw, k | 0x80520000, len3table14);
			appack_updateRep(&aw, 1, i);
			appack_updateRep(&aw, 0, k);
			appack_updateRep(&aw, 0, l);
			continue;
		}
		if (cmpBytes(p, "8e_Yx_f788#4_Yx_f78800000020_Yx") != 0) {
			i = p[1] & 0x3f; // p1
			j = p[4] << 24 | p[5] << 16 | p[6] << 8 | p[7];	// typ
			k = p[8] & 0x3f; // r
			l = p[15] & 0x3f; // p0
			p += 16;
			flg4 = flgD = 0;
			if (i != l) flg4 = 1;
			if (aw.pxxTyp[i] != j) {
				flgD = 1;
				aw.pxxTyp[i] = j;
			}
			if (flg4 != 0)
				appackSub1op(&aw, 0x04);
			if (flgD != 0)
				appackSub1op(&aw, 0x0d);
			appackSub1op(&aw, 0x0e);
			appackSub1p(&aw, i, 0);
			if (flgD != 0)
				appackSub1u(&aw, j);
			if (k != 0x3f) {
				k |= 80520000;
				appack_updateRep(&aw, 0, k);
			} else
				k = aw.immR3f;
			appackSub1i(&aw, k, len3table14);
			if (flg4 != 0)
				appackSub1p(&aw, l, MODE_REG_LC3);
			appack_updateRep(&aw, 1, i);
			appack_updateRep(&aw, 1, l);
			continue;
		}
		if (cmpBytes(p, "90YxYxYx_f788") != 0 && p[2] == p[1]) {
			// CP
			appack0_waitFlush(&aw);
			i = p[1] & 0x3f;
			j = p[3] & 0x3f;
			appackSub1op(&aw, 0x02);
			appackSub1i(&aw, i | 0x80520000, len3table2);
			appackSub1r(&aw, j, MODE_REG_LC3);
			appack_updateRep(&aw, 0, i);
			appack_updateRep(&aw, 0, j);
			p += 10;
			if (aw.vecPrfx != -1) {
				for (n = 1; n < aw.vecPrfx; n++) {
					while (cmpBytes(p, "fcfd_f788#4_80") != 0)
						p += 9;
					p += 10;
					if (aw.vecPrfxMode == 0)
						appack_updateRep(&aw, 0, i);
					if (aw.vecPrfxMode == 1)
						appack_updateRep(&aw, 0, i + n);
					appack_updateRep(&aw, 0, j + n);
				}
				appack_updateRep(&aw, 0, i);
				appack_updateRep(&aw, 0, j);
				aw.vecPrfx = -1;
			}
			continue;
		} 
		if (cmpBytes(p, "YxYxYxYx_f788") != 0 && 0x90 <= *p && *p <= 0x9b && *p != 0x97) {
			appack0_waitFlush(&aw);
			static int len3table[12][7] = {
			//	{ -1,  0,  1,  2,  3,  4, R0 }, // general
				{ R1, 16,  1,  2,  8,  4, R0 }, // OR
				{ -1, R1,  1,  2,  3,  4, R0 }, // XOR
				{ R1, 15,  1,  7,  3,  5, R0 }, // AND
				{ 64, 16,  1,  2,  8,  4, 32 }, // SBX
				{ -1, R1,  1,  2,  3,  4, R0 }, // ADD
				{ R1, R0,  1,  2,  3,  4,  5 }, // SUB
				{ 10, R0, R1,  7,  3,  6,  5 }, // MUL
				{ -1,  0,  1,  2,  3,  4, R0 }, // SBR
				{ R1, R0,  1,  2,  3,  4,  5 }, // SHL
				{ R1, R0,  1,  2,  3,  4,  5 }, // SAR
				{ 10, R0, R1,  7,  3,  6,  5 }, // DIV
				{ 10, R0, R1,  7,  3,  6,  5 }, // MOD
			};
			k = *p & 0x0f;
			m = p[1];
			l = p[2];
			if (simple == 0) {
				if (cmpBytes(p, "98YxbfYx") != 0 && aw.immR3f == 1) {
					for (i = 0; i < 8; i++) {
						if (aw.rep[0][i] == (p[1] & 0x3f)) break;
					}
				//	if (i <= 1 || (0x80 <= p[1] && p[1] <= 0x84)) {
					if (i <= 1 || (0x80 <= p[1] && p[1] <= 0x80 + (6 - APPACK_SUB1R_PRM)) || i < APPACK_SUB1R_PRM) {
						k = 0x04;	// SHL -> ADD
						l = p[1];
					}
				}
				if (cmpBytes(p, "96YxbfYx") != 0 && aw.immR3f == -1) {
					k = 0x07; // MUL -> SBR
					aw.immR3f = 0;
				}
				if (k == 0x05) {
					if (m == 0xbf || l == p[3]) {
						k = 0x07;
						i = m;
						m = l;
						l = i;
					}
				}
			}
			flg4 = flgD = 0;
			if (m == p[3]) {
				i = m & 0x3f;
				j = aw.immR3f;
				if (l != 0xbf)
					j = 0x80520000 | (l & 0x3f);
				appackSub1op(&aw, k | 0x10);
				appackSub1r(&aw, i, 0);
				appackSub1i(&aw, j, len3table[k]);
			} else if (m != 0xbf) {
				flg4 = 1;
				i = p[3] & 0x3f;
				j = aw.immR3f;
				if (l != 0xbf)
					j = 0x80520000 | (l & 0x3f);
				appackSub1op(&aw, 0x04);
				appackSub1op(&aw, k | 0x10);
				appackSub1r(&aw, m & 0x3f, 0);
				appackSub1i(&aw, j, len3table[k]);
				appackSub1r(&aw, i, MODE_REG_LC3);
			} else {
				flgD = 1;
				i = p[3] & 0x3f;
				j = aw.immR3f;
				if (l == p[3]) {
					appackSub1op(&aw, 0x0d);
					appackSub1op(&aw, k | 0x10);
					appackSub1r(&aw, i, 0);
					appackSub1i(&aw, j, len3table[k]);
				} else {
					flg4 = 1;
					appackSub1op(&aw, 0x04);
					appackSub1op(&aw, 0x0d);
					appackSub1op(&aw, k | 0x10);
					appackSub1r(&aw, l & 0x3f, 0);
					appackSub1i(&aw, j, len3table[k]);
					appackSub1r(&aw, i, MODE_REG_LC3);
				}
			}
			if (m != 0xbf) appack_updateRep(&aw, 0, m & 0x3f);
			if (l != 0xbf) appack_updateRep(&aw, 0, l & 0x3f);
			appack_updateRep(&aw, 0, i);
			p += 10;
			if (aw.vecPrfx != -1) {
				for (n = 1; n < aw.vecPrfx; n++) {
					while (cmpBytes(p, "fcfd_f788#4_80") != 0)
						p += 9;
					if (cmpBytes(p, "f2_f788#4_bf_f78800000020") != 0)
						p += 14; // R3F=?; をスキップ.
					if (cmpBytes(p, "YxYxYxYx_f788") != 0 && 0x90 <= *p && *p <= 0x9b && *p != 0x97)
						p += 10;
					else {
						fprintf(stderr, "appack0: alu: vecPrfx: error:\n");
						exit(1);
					}
					if (aw.vecPrfxMode == 0) {
						if (flg4 == 0) {
							if (flgD == 0) {
								appack_updateRep(&aw, 0, (m & 0x3f) + n);
								if (l != 0xbf) appack_updateRep(&aw, 0, l & 0x3f);
							} else {
								if (m != 0xbf) appack_updateRep(&aw, 0, m & 0x3f);
								appack_updateRep(&aw, 0, (l & 0x3f) + n);
							}
						}
						if (flg4 != 0) {
							if (m != 0xbf) appack_updateRep(&aw, 0, m & 0x3f);
							if (l != 0xbf) appack_updateRep(&aw, 0, l & 0x3f);
						}
					}
					if (aw.vecPrfxMode == 1) {
						if (m != 0xbf) appack_updateRep(&aw, 0, (m & 0x3f) + n);
						if (l != 0xbf) appack_updateRep(&aw, 0, (l & 0x3f) + n);
					}
					appack_updateRep(&aw, 0, i + n);
				}
				if (m != 0xbf) appack_updateRep(&aw, 0, m & 0x3f);
				if (l != 0xbf) appack_updateRep(&aw, 0, l & 0x3f);
				appack_updateRep(&aw, 0, i);
				aw.vecPrfx = -1;
			}
			continue;
		}
		if (cmpBytes(p, "9e_Yx_Yx") != 0) {
			i = p[1] & 0x3f;
			j = p[2] & 0x3f;
			if (i != 0x3f && j != 0x3f) {
				appackSub1op(&aw, 0x1e);
				appackSub1p(&aw, i, 0);
				appackSub1p(&aw, j, MODE_REG_LC3);
				appack_updateRep(&aw, 1, i);
				appack_updateRep(&aw, 1, j);
				p += 3;
				continue;
			}
		}
		if (cmpBytes(p, "YxYxYx_f78800000020_bf_f78800000020 f4_bf f3_f788#4_bf") != 0 && 0xa0 <= *p && *p <= 0xa7) {
			appack0_waitFlush(&aw);
			i = p[21] << 24 | p[22] << 16 | p[23] << 8 | p[24];
			i = appackSub2b(&aw, 0, i);
			if (i == 1) {
				appackSub1op(&aw, *p & 0x3f);
				appackSub1r(&aw, p[1] & 0x3f, 0);
				if (p[2] == 0xbf)
					appackSub1i(&aw, aw.immR3f, len3table0);
				else
					appackSub1i(&aw, 0x80520000 | (p[2] & 0x3f), len3table0);
			} else {
				appackSub1op(&aw, 0x04);
				appackSub1op(&aw, *p & 0x3f);
				appackSub1r(&aw, p[1] & 0x3f, 0);
				if (p[2] == 0xbf)
					appackSub1i(&aw, aw.immR3f, len3table0);
				else
					appackSub1i(&aw, 0x80520000 | (p[2] & 0x3f), len3table0);
				appackSub1s(&aw, i, 0);
			}
			if (aw.waitLb0 < i)
				aw.waitLb0 = i;
			appack_updateRep(&aw, 0, p[1] & 0x3f);
			if (p[2] != 0xbf)
				appack_updateRep(&aw, 0, p[2] & 0x3f);
			p += 26;
			continue;
		}
		if (cmpBytes(p, "YxYxYx_f78800000020_Yx_f78800000020") != 0 && 0xa0 <= *p && *p <= 0xa7) {
			appack0_waitFlush(&aw);
			appackSub1op(&aw, 0x0d);
			appackSub1op(&aw, *p & 0x3f);
			appackSub1r(&aw, p[1] & 0x3f, 0);
			if (p[2] == 0xbf)
				appackSub1i(&aw, aw.immR3f, len3table0);
			else
				appackSub1i(&aw, 0x80520000 | (p[2] & 0x3f), len3table0);
			appackSub1r(&aw, p[9] & 0x3f, 0);
			appack_updateRep(&aw, 0, p[1] & 0x3f);
			if (p[2] != 0xbf)
				appack_updateRep(&aw, 0, p[2] & 0x3f);
			appack_updateRep(&aw, 0, p[9] & 0x3f);
			p += 16;
			continue;
		}
		if (cmpBytes(p, "bd_f788#4_f788#4_f788#4_f788#4_f788#4_f788#4 9e_b0_bf") != 0) {
			if (aw.wait3d + aw.waitEnd > 0)
				appack0_waitFlush(&aw);
			aw.wait3d = 1;
			p += 37 + 3;
			continue;
		}

		if (cmpBytes(p, "fcfd_f788#4_80") != 0) {
			i = p[4] << 24 | p[5] << 16 | p[6] << 8 | p[7];
			aw.dr[0] = i;
			if (encLidr != 0 /* && aw.wait1 != 1 && aw.wait7 == 0 && aw.wait3d == 0 && aw.waitEnd == 0 */) {
				appack0_waitFlush(&aw);
				appackSub1op(&aw, 0xfd);
				appackSub1s(&aw, i, 0);
				appackSub1u(&aw, 0);
			}
			p += 9;
			continue;
		}
		if (cmpBytes(p, "fcfe10 f2_f78800000009_b0_f78800000020 f2_f78800000000_b1_f78800000020 f2_f788ffffffff_b2_f78800000020 f3_f788#4_b0 9e_af_bf f1_f788#4_f78800000003 fcfe00") != 0) {
			aw.waitEnd = 1;
//printf("waitEnd = 1\n");
			p += 3 + 14 * 3 + 8 + 3 + 13 + 3;
			aw.lastLabel++;
			while (cmpBytes(p, "fcfd_f788#4_80") != 0)
				p += 9;
			continue;
		}
		if (cmpBytes(p, "fcfe10") != 0) {
			// REM01() : API呼び出しマクロヘッダ.
			appack0_waitFlush(&aw);
			pp = appack0_param0(p + 3, &aw);
			if (pp != NULL) {
				if (aw.prm_r[0x30] == 0x0002) { // api_drawPoint
					if (appack0_param1(&aw, 3, 0x32) != 0) {
						appackSub1op(&aw, 0x04);
						appackSub1op(&aw, 0x05);
						appackSub1s(&aw, 0x0002, 0);
						appackSub1i(&aw, aw.prm_r[0x31], len3table0); // mode
						p = pp;
						aw.lastLabel++;
						continue;
					}
					appackSub1op(&aw, 0x05);
					appackSub1s(&aw, 0x0002, 0);
					for (i = 0x31; i <= 0x34; i++)
						appackSub1i(&aw, aw.prm_r[i], len3table0);
					p = pp;
					aw.lastLabel++;
					continue;
				}
				if (aw.prm_r[0x30] == 0x0003) { // api_drawLine
					if (appack0_param1(&aw, 5, 0x32) != 0) {
						appackSub1op(&aw, 0x04);
						appackSub1op(&aw, 0x05);
						appackSub1s(&aw, 0x0003, 0);
						appackSub1i(&aw, aw.prm_r[0x31], len3table0); // mode
						p = pp;
						aw.lastLabel++;
						continue;
					}
					appackSub1op(&aw, 0x05);
					appackSub1s(&aw, 0x0003, 0);
					if (simple == 0) {
						if ((aw.prm_r[0x31] & 3) == 0 && aw.prm_r[0x32] == 7) aw.prm_r[0x32] = -1;
						if (aw.prm_r[0x33] == aw.v_xsiz - 1 && aw.v_xsiz > 0) aw.prm_r[0x33] = -1;
						if (aw.prm_r[0x34] == aw.v_ysiz - 1 && aw.v_ysiz > 0) aw.prm_r[0x34] = -1;
						if (aw.prm_r[0x35] == aw.v_xsiz - 1 && aw.v_xsiz > 0) aw.prm_r[0x35] = -1;
						if (aw.prm_r[0x36] == aw.v_ysiz - 1 && aw.v_ysiz > 0) aw.prm_r[0x36] = -1;
					}
					for (i = 0x31; i <= 0x36; i++)
						appackSub1i(&aw, aw.prm_r[i], len3table0);
					p = pp;
					aw.lastLabel++;
					continue;
				}
				if (aw.prm_r[0x30] == 0x0004) { // api_fillRect
					if (appack0_param1(&aw, 5, 0x32) != 0) {
						appackSub1op(&aw, 0x04);
						appackSub1op(&aw, 0x05);
						appackSub1s(&aw, 0x0004, 0);
						appackSub1i(&aw, aw.prm_r[0x31], len3table0); // mode
						p = pp;
						aw.lastLabel++;
						continue;
					}
					appackSub1op(&aw, 0x05);
					appackSub1s(&aw, 0x0004, 0);
					if (simple == 0) {
						if ((aw.prm_r[0x31] & 3) == 0 && aw.prm_r[0x32] == 7) aw.prm_r[0x32] = -1;
						if (aw.prm_r[0x33] == aw.v_xsiz) aw.prm_r[0x33] = -1;
						if (aw.prm_r[0x34] == aw.v_ysiz) aw.prm_r[0x34] = -1;
						if (aw.prm_r[0x34] == aw.prm_r[0x33] && aw.prm_r[0x34] > 0)
							aw.prm_r[0x34] = 0;
					}
					for (i = 0x31; i <= 0x36; i++)
						appackSub1i(&aw, aw.prm_r[i], len3table0);
					p = pp;
					aw.lastLabel++;
					continue;
				}
				if (aw.prm_r[0x30] == 0x0005) { // api_fillOval
					if (appack0_param1(&aw, 5, 0x32) != 0) {
						appackSub1op(&aw, 0x04);
						appackSub1op(&aw, 0x05);
						appackSub1s(&aw, 0x0005, 0);
						appackSub1i(&aw, aw.prm_r[0x31], len3table0); // mode
						p = pp;
						aw.lastLabel++;
						continue;
					}
					appackSub1op(&aw, 0x05);
					appackSub1s(&aw, 0x0005, 0);
					if (simple == 0) {
						if ((aw.prm_r[0x31] & 3) == 0 && aw.prm_r[0x32] == 7) aw.prm_r[0x32] = -1;
						if (aw.prm_r[0x33] == aw.v_xsiz) aw.prm_r[0x33] = -1;
						if (aw.prm_r[0x34] == aw.v_ysiz) aw.prm_r[0x34] = -1;
						if (aw.prm_r[0x34] == aw.prm_r[0x33] && aw.prm_r[0x34] > 0)
							aw.prm_r[0x34] = 0;
					}
					for (i = 0x31; i <= 0x36; i++)
						appackSub1i(&aw, aw.prm_r[i], len3table0);
					p = pp;
					aw.lastLabel++;
					continue;
				}
				if (aw.prm_r[0x30] == 0x0009) { // api_sleep
					appackSub1op(&aw, 0x05);
					appackSub1s(&aw, 0x0009, 0);
					appackSub1i(&aw, aw.prm_r[0x31], len3table0);
					appackSub1i(&aw, aw.prm_r[0x32], len3table0);
					p = pp;
					aw.lastLabel++;
					continue;
				}
				if (aw.prm_r[0x30] == 0x000d) { // api_inkey
					j = 0;
					if (cmpBytes(pp, "fcfd_f788#4_80") != 0) {
						j = pp[4] << 24 | pp[5] << 16 | pp[6] << 8 | pp[7];
						aw.dr[0] = j;
						pp += 9;
					}
					if (cmpBytes(pp, "90_b0_b0_Yx_f788") != 0) {
						i = pp[3] & 0x3f;
						appackSub1op(&aw, 0x05);
						appackSub1s(&aw, 0x000d, 0);
						appackSub1i(&aw, aw.prm_r[0x31], len3table0);
						appackSub1r(&aw, i, MODE_REG_LC3);
						appack_updateRep(&aw, 0, 0x32);
						appack_updateRep(&aw, 0, 0x31);
						appack_updateRep(&aw, 0, i);
						p = pp + 10;
						aw.lastLabel++;
						if (j != 0 && encLidr != 0) {
							appackSub1op(&aw, 0xfd);
							appackSub1s(&aw, j, 0);
							appackSub1u(&aw, 0);
						}
						continue;
					}
				}
				if (aw.prm_r[0x30] == 0x0010) { // api_openWin
					flg4 = 0;
					if (aw.prm_r[0x32] != 0) {
						flg4 = 1;
						appackSub1op(&aw, 0x04);
					}
					appackSub1op(&aw, 0x05);
					appackSub1s(&aw, 0x0010, 0);
					if (aw.prm_r[0x33] > 0)
						aw.v_xsiz = aw.prm_r[0x33];
					else
						aw.v_xsiz = -1;
					if (aw.prm_r[0x34] > 0)
						aw.v_ysiz = aw.prm_r[0x34];
					else if (aw.prm_r[0x34] == 0 && aw.v_xsiz > 0)
						aw.v_ysiz = aw.v_xsiz;
					else
						aw.v_ysiz = -1;
					if (simple == 0) {
						if (aw.prm_r[0x34] == aw.prm_r[0x33] && aw.prm_r[0x34] > 0)
							aw.prm_r[0x34] = 0;
					}
					if (flg4 != 0) {
						appackSub1i(&aw, aw.prm_r[0x31], len3table0);
						appackSub1i(&aw, aw.prm_r[0x32], len3table0);
					}
					appackSub1i(&aw, aw.prm_r[0x33], len3table0);
					appackSub1i(&aw, aw.prm_r[0x34], len3table0);
					p = pp;
					aw.lastLabel++;
					continue;
				}
				fprintf(stderr, "appack0: fcfe10: error: R30=0x%04x\n", aw.prm_r[0x30]);
				exit(1);
			}
		}
		if (cmpBytes(p, "fcfe21_f788") != 0) {
			// REM02() : for構文.
			appack0_waitFlush(&aw);
			aw.prm_r[0x31] = p[5] << 24 | p[6] << 16 | p[7] << 8 | p[8];
			if (cmpBytes(p + 9, "f2_f788#4_Yx_f78800000020 f1_f788#4_f78800000002") != 0) {
				i = p[16] & 0x3f;
				aw.prm_r[0x30] = p[12] << 24 | p[13] << 16 | p[14] << 8 | p[15];
				p += 36;
			} else {
				goto err;
			}
			if (aw.prm_r[0x30] != 0)
				appackSub1op(&aw, 0x04);
			appackSub1op(&aw, 0x06);
			if (aw.prm_r[0x30] != 0)
				appackSub1i(&aw, aw.prm_r[0x30], len3table0);
			appackSub1i(&aw, aw.prm_r[0x31], len3table0);
			appackSub1r(&aw, i, 1);
			appack_updateRep(&aw, 0, i);
			aw.lastLabel++;
			if (aw.vecPrfx != -1) {
				for (n = 1; n < aw.vecPrfx; n++) {
					while (cmpBytes(p, "fcfd_f788#4_80") != 0)
						p += 9;
					p += 36;
					appack_updateRep(&aw, 0, i + n);
					aw.lastLabel++;
				}
				appack_updateRep(&aw, 0, i);
				aw.vecPrfx = -1;
			}
			continue;
		}
		if (cmpBytes(p, "fcfe30") != 0) {
			// REM03() : next構文.
			if (aw.wait3d + aw.waitEnd > 0)
				appack0_waitFlush(&aw);
			while (*p != 0xf3)
				p += db2binSub2Len(p);
			p += db2binSub2Len(p);
			aw.wait7++;
			continue;
		}
		if (cmpBytes(p, "fcfeb4f0") != 0) {
			// REM34() : semi-vector-prefix.
			appack0_waitFlush(&aw);
			appackSub1op(&aw, 0x34);
			aw.vecPrfx = 2;
			aw.vecPrfxMode = 0;
			p += 4;
			continue;
		}
		if (cmpBytes(p, "fcfeb5f1_f788#4") != 0) {
			// REM35() : semi-vector-prefix.
			i = p[6] << 24 | p[7] << 16 | p[8] << 8 | p[9];
			appack0_waitFlush(&aw);
			appackSub1op(&aw, 0x35);
			if (3 <= i && i <= 9)
				appackSub1u(&aw, i - 3);
			else {
				fprintf(stderr, "appack0: REM35: error: i=%d\n", i);
				exit(1);
			}
			aw.vecPrfx = i;
			aw.vecPrfxMode = 0;
			p += 10;
			continue;
		}
		if (cmpBytes(p, "fcfeb6f0") != 0) {
			// REM36() : full-vector-prefix.
			appack0_waitFlush(&aw);
			appackSub1op(&aw, 0x36);
			aw.vecPrfx = 2;
			aw.vecPrfxMode = 1;
			p += 4;
			continue;
		}
		if (cmpBytes(p, "fcfeb7f1_f788#4") != 0) {
			// REM37() : semi-vector-prefix.
			i = p[6] << 24 | p[7] << 16 | p[8] << 8 | p[9];
			appack0_waitFlush(&aw);
			appackSub1op(&aw, 0x37);
			if (3 <= i && i <= 9)
				appackSub1u(&aw, i - 3);
			else {
				fprintf(stderr, "appack0: REM37: error: i=%d\n", i);
				exit(1);
			}
			aw.vecPrfx = i;
			aw.vecPrfxMode = 1;
			p += 10;
			continue;
		}
err:
		fprintf(stderr, "appack0: error: op=%02x-%02x-%02x-%02x (DR0=0x%08x)\n", *p, p[1], p[2], p[3], aw.dr[0]);
		exit(1);
	}
	while (aw.q[-1] == 0x00)
		aw.q--;
	return putBuf(w->argOut, w->obuf, aw.q);




#if 0
	*qq = '\0';
	p = w->ibuf + 2;
	while (*p != 0x00 || p == &w->ibuf[2]) {
//printf("%02X at %06X\n", *p, p - w->ibuf);
		if (*p == 0x00) {
			p++;
		} else if (*p == 0x01) {
			f = 0;
			i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			lastLabel++;
			if (p[1] == 0x01 && i == lastLabel && (p[6] == 0x34 || p[6] == 0xf0)) {
				appackSub1op(&q, &of, 0x2e, hist, opTbl);
				p += 6;
				goto op_f0;
			}
			if (p[1] == 0x01 && i == lastLabel &&
				p[6] == 0xfe && p[7] == 0x01 && p[8] == 0x00 &&
				p[9] == 0x3c && p[10] == 0x00 && p[11] == 0x20 && p[12] == 0x20 && p[13] == 0x00 && p[14] == 0x00 && p[15] == 0x00)
			{
				appackSub1op(&q, &of, 0x3c, hist, opTbl);
				p += 6 + 3 + 7;
				continue;
			}
			if (p[1] != 0x00 || i != lastLabel) {
				f = 1;
				appackSub1op(&q, &of, 0x04, hist, opTbl);
			}
			appackSub1op(&q, &of, 0x01, hist, opTbl);
			if (f != 0) {
				appackSub1(&q, &of, p[1] | (i - lastLabel) << 8, 0);
				lastLabel = i;
			}
			p += 6;
		} else if (*p == 0x02) {
			if (p[1] != 0x3f) {
				appackSub1op(&q, &of, 0x02, hist, opTbl);
				appackSub1(&q, &of, p[1], 1);
				appackSub1(&q, &of, appackEncodeConst(p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]), 0);
			} else {
				r3f = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			}
			p += 6;
		} else if (*p == 0x03) {
			if (p[1] == 0x3f) {
				appackSub1op(&q, &of, 0x03, hist, opTbl);
				appackSub1(&q, &of, (p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]) - lastLabel, 0);
				p += 6;
			} else if (p[1] == 0x30 && p[6] == 0x03 && p[7] == 0x3f && p[12] == 0x01 && p[13] == 0x01 &&
				p[14] == p[2] && p[15] == p[3] && p[16] == p[4] && p[17] == p[5] &&
				p[18] == 0xfe && p[19] == 0x01 && p[20] == 0x00)
			{
				lastLabel++;
				i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
				j = p[8] << 24 | p[9] << 16 | p[10] << 8 | p[11];
				if (i != lastLabel) {
					appackSub1op(&q, &of, 0x04, hist, opTbl);
				}
				if (fe0103 != 0) {
					appackSub1op(&q, &of, 0x3e, hist, opTbl);
					if (i != lastLabel) {
						// ここに0をおけば絶対指定モードに変更できる.
						appackSub1(&q, &of, i - lastLabel, 0);
						lastLabel = i;
					}
					appackSub1(&q, &of, j - lastLabel, 0);
				} else {
					appackSub1op(&q, &of, 0x3f, hist, opTbl);
					if (i != lastLabel) {
						// ここに0をおけば絶対指定モードに変更できる.
						appackSub1(&q, &of, i - lastLabel, 0);
						lastLabel = i;
					}
					appackSub1(&q, &of, j + 0x40, 1);
				}
				fe0103 = 0;
				pxxTyp[0x30] = TYP_CODE;
				for (i = 0x31; i <= 0x3e; i++)
					pxxTyp[i] = TYP_UNKNOWN;
				p += 21;
			} else if (p[1] == 0x30 && p[6] == 0x1e && p[7] == 0x3f && p[9] == 0x01 && p[10] == 0x01 &&
				p[11] == p[2] && p[12] == p[3] && p[13] == p[4] && p[14] == p[5] &&
				p[15] == 0xfe && p[16] == 0x01 && p[17] == 0x00)
			{
				lastLabel++;
				i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
				if (i != lastLabel) {
					appackSub1op(&q, &of, 0x04, hist, opTbl);
				}
				appackSub1op(&q, &of, 0x3f, hist, opTbl);
				if (i != lastLabel) {
					// ここに0をおけば絶対指定モードに変更できる.
					appackSub1(&q, &of, i - lastLabel, 0);
					lastLabel = i;
				}
				appackSub1(&q, &of, p[8], 1);
				lastLabel = i;
				pxxTyp[0x30] = TYP_CODE;
				for (i = 0x31; i <= 0x3e; i++)
					pxxTyp[i] = TYP_UNKNOWN;
				p += 18;
			} else {
				i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
				pxxTyp[p[1]] = labelTyp[i];
				if (p == &w->ibuf[3] && p[1] == 1 && firstDataLabel == i) {
					p += 6;
					continue;
				}
				appackSub1op(&q, &of, 0x04, hist, opTbl);
				appackSub1op(&q, &of, 0x03, hist, opTbl);
				appackSub1(&q, &of, p[1], 1);
				appackSub1(&q, &of, i - lastLabel, 0);
				p += 6;
			}
		} else if (*p == 0x04) {
			appackSub1op(&q, &of, 0x04, hist, opTbl);
			appackSub1op(&q, &of, 0x04, hist, opTbl);
			appackSub1(&q, &of, p[1], 1);
			p += 2;
		} else if (0x08 <= *p && *p <= 0x0b) {
			static UCHAR bytes[] = { 0x02, 0x3f, 0x00, 0x00, 0x00, 0x01, 0x0e };
			f = ff = 0;
			if (p[7] == 0x00 && memcmp(&p[8], bytes, 7) == 0 && p[15] == p[6] && p[20] == p[6] && p[21] == 0x3f)
				f = 1;
			if (f == 0) {
				appackSub1op(&q, &of, 0x04, hist, opTbl);
			}
			if (pxxTyp[p[6]] != TYP_UNKNOWN && (p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]) != pxxTyp[p[6]]) {
				ff = 1;
				appackSub1op(&q, &of, 0x0d, hist, opTbl);
			}
			appackSub1op(&q, &of, *p, hist, opTbl);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[6], 1);
			if (ff != 0 || pxxTyp[p[6]] == TYP_UNKNOWN) {
				appackSub1(&q, &of, p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5], 1);
				pxxTyp[p[6]] = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			}
			if (*p == 0x0a)
				pxxTyp[p[1]] = TYP_UNKNOWN;
			appackSub1(&q, &of, p[7], 1); // 0を仮定.
			p += 8;
			if (f != 0) p += 6 + 8;
		} else if (*p == 0x0e) {
			f = ff = 0;
			if (pxxTyp[p[6]] != TYP_UNKNOWN && (p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]) != pxxTyp[p[6]])
				ff = 1;
			if (p[1] == 0x3f && 0x08 <= p[8] && p[8] <= 0x0b && p[14] == 0x3f && p[15] == 0) {
				if (ff != 0) {
					appackSub1op(&q, &of, 0x0d, hist, opTbl);
				}
				appackSub1op(&q, &of, p[8] + 0x30, hist, opTbl);
				appackSub1(&q, &of, p[9], 1);
				appackSub1(&q, &of, p[6], 1);
				if (ff != 0 || pxxTyp[p[6]] == TYP_UNKNOWN) {
					appackSub1(&q, &of, p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5], 1);
					pxxTyp[p[6]] = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
				}
				appackSub1(&q, &of, p[7], 1);
				appackSub1(&q, &of, 0, 1);
				p += 16;
				continue;
			}
			if (p[1] != p[6]) {
				f = 1;
				appackSub1op(&q, &of, 0x04, hist, opTbl);
			}
			if (ff != 0) {
				appackSub1op(&q, &of, 0x0d, hist, opTbl);
			}
			appackSub1op(&q, &of, 0x0e, hist, opTbl);
			appackSub1(&q, &of, p[1], 1);
			if (f != 0)
				appackSub1(&q, &of, p[6], 1);
			if (ff != 0 || pxxTyp[p[6]] == TYP_UNKNOWN) {
				appackSub1(&q, &of, p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5], 1);
				pxxTyp[p[6]] = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			}
			pxxTyp[p[1]] = pxxTyp[p[6]];
			if (p[7] != 0x3f)
				appackSub1(&q, &of, - p[7] - 16, 0);
			else {
				appackSub1(&q, &of, appackEncodeConst(r3f), 0);
			}
			p += 8;
		} else if (*p == 0x0f) {
			ff = 0;
			i = pxxTyp[p[6]];
			if (i == TYP_UNKNOWN)
				i = pxxTyp[p[7]];
		//	if (pxxTyp[p[6]] != TYP_UNKNOWN && pxxTyp[p[7]] != -1 && pxxTyp[p[6]] != pxxTyp[p[7]])
		//		i = -1;
			if (i != TYP_UNKNOWN && (p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5]) != i) {
				ff = 1;
				appackSub1op(&q, &of, 0x0d, hist, opTbl);
			}
			appackSub1op(&q, &of, 0x0f, hist, opTbl);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[6], 1);
			appackSub1(&q, &of, p[7], 1);
			if (ff != 0 || i == TYP_UNKNOWN) {
				appackSub1(&q, &of, p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5], 1);
				i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			}
			pxxTyp[p[6]] = pxxTyp[p[7]] = i;
			p += 8;
		} else if (0x10 <= *p && *p <= 0x1b && *p != 0x17) {
			if (*p == 0x10 && p[3] == 0xff) {
#if 0
				appackSub1op(&q, &of, 0x17, hist, opTbl);
				appackSub1(&q, &of, p[1], 1);
				appackSub1(&q, &of, p[2], 1);
#else
				appackSub1op(&q, &of, 0x02, hist, opTbl);
				appackSub1(&q, &of, p[1], 1);
				appackSub1(&q, &of, - p[2] - 16, 0);
#endif
			} else {
				f = 0;
				if (p[1] != p[2]) {
					f = 1;
					appackSub1op(&q, &of, 0x04, hist, opTbl);
				}
				appackSub1op(&q, &of, *p, hist, opTbl);
				appackSub1(&q, &of, p[1], 1);
				if (f != 0)
					appackSub1(&q, &of, p[2], 1);
				if (p[3] != 0x3f)
					appackSub1(&q, &of, - p[3] - 16, 0);
				else {
					appackSub1(&q, &of, appackEncodeConst(r3f), 0);
				}
				if (p[2] == 0x3f)
					appackSub1(&q, &of, r3f, 0);
			}
			p += 4;
		} else if (0x1c <= *p && *p <= 0x1e) {
			appackSub1op(&q, &of, *p, hist, opTbl);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[2], 1);
			if (*p == 0x1e) {
				pxxTyp[p[1]] = pxxTyp[p[2]];
			}
			p += 3;
		} else if (*p == 0x1f) {
			appackSub1op(&q, &of, 0x0d, hist, opTbl);	// 手抜き.
			appackSub1op(&q, &of, 0x1f, hist, opTbl);
			appackSub1(&q, &of, p[1], 1);
			i = p[2] << 24 | p[3] << 16 | p[4] << 8 | p[5];
			appackSub1(&q, &of, i, 1);
			if (i == 0)
				i = TYP_UNKNOWN;
			pxxTyp[p[1]] = i;
			appackSub1(&q, &of, p[6], 1);
			i = p[7] << 24 | p[8] << 16 | p[9] << 8 | p[10];
			appackSub1(&q, &of, i, 1);
			p += 11;
		} else if (0x20 <= *p && *p <= 0x27) {
			f = 0;
			if (p[1] != 0x3f) {
				f = 1;
				appackSub1op(&q, &of, 0x04, hist, opTbl);
			}
			appackSub1op(&q, &of, *p, hist, opTbl);
			if (f != 0)
				appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[2], 1);
			if (p[3] != 0x3f)
				appackSub1(&q, &of, - p[3] - 16, 0);
			else {
				appackSub1(&q, &of, appackEncodeConst(r3f), 0);
			}
			if (p[2] == 0x3f)
				appackSub1(&q, &of, r3f, 0);
			p += 4;
			if (f == 0) {
				p += 2 + 6;
				appackSub1(&q, &of, (p[-4] << 24 | p[-3] << 16 | p[-2] << 8 | p[-1]) - lastLabel, 0);
			}
		} else if (0x28 <= *p && *p <= 0x2d) {
			f = 0;
			if (p[1] != 0x3f) {
				f = 1;
				appackSub1op(&q, &of, 0x04, hist, opTbl);
			}
			appackSub1op(&q, &of, *p, hist, opTbl);
			if (f != 0)
				appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[2], 1);
			if (p[3] != 0x3f)
				appackSub1(&q, &of, - p[3] - 16, 0);
			else {
				fputs("appack: internal error: 28-2D: p[3] == P3F\n", stderr);
				exit(1);
			}
			p += 4;
			if (f == 0) {
				p += 2 + 6;
				appackSub1(&q, &of, (p[-4] << 24 | p[-3] << 16 | p[-2] << 8 | p[-1]) - lastLabel, 0);
			}
		} else if (0x30 <= *p && *p <= 0x33 || 0xf4 <= *p && *p <= 0xf7) {
			if ((*p & 1) == 0) {
				appackSub1op(&q, &of, 0x04, hist, opTbl);
				appackSub1op(&q, &of, (*p & 3) | 0x30, hist, opTbl);
				appackSub1(&q, &of, p[1], 1);
				appackSub1(&q, &of, p[2], 1);
				appackSub1(&q, &of, p[3], 1);
				pxxTyp[p[1]] = TYP_UNKNOWN;
			} else {
				appackSub1op(&q, &of, (*p & 3) | 0x30, hist, opTbl);
				if ((*p & 3) == 0x03)
					appackSub1(&q, &of, p[1], 1);
			}
			p += 4;
		} else if (*p == 0x3c) {
			f = 1;
	//		if (p[1] == 0x00 && p[2] == 0x20 && p[3] == 0x20 && p[4] == 0xcb && p[5] == 0x04 && p[6] == 0x04)
	//			f = 0;
			if (f != 0) {
				appackSub1op(&q, &of, 0x04, hist, opTbl);
			}
			appackSub1op(&q, &of, *p, hist, opTbl);
			if (f != 0) {
				appackSub1(&q, &of, p[1], 1);
				appackSub1(&q, &of, p[2], 1);
				appackSub1(&q, &of, p[3], 1);
				appackSub1(&q, &of, (p[4] >> 4) & 0x0f, 1);
				appackSub1(&q, &of,  p[4]       & 0x0f, 1);
				appackSub1(&q, &of, p[5], 1);
				appackSub1(&q, &of, p[6], 1);
			}
			p += 7;
		} else if (*p == 0x3d) {
			// bugfix: hinted by Iris, 2013.07.05. thanks!
			if (p[7] == 0x1e && p[8] == 0x3f && p[9] == 0x30 && p[10] == 0x00) { p += 7 + 3; wait7 = 0; continue; }
			f = 0;
			if (p[7] == 0x1e && p[8] == 0x3f && p[9] == 0x30) {
				f = 1;
				appackSub1op(&q, &of, 0x04, hist, opTbl);
				wait7 = 0;
			}
			appackSub1op(&q, &of, 0x3d, hist, opTbl);
			p += 7;
			if (f != 0) p += 3;
		} else if (*p == 0xf0 || *p == 0x34) {
			appackSub1op(&q, &of, 0x34, hist, opTbl);
op_f0:
			i = p[1] << 24 | p[2] << 16 | p[3] << 8 | p[4];
			int len = p[5] << 24 | p[6] << 16 | p[7] << 8 | p[8];
			appackSub1(&q, &of, i, 1);
			appackSub1(&q, &of, len, 1);
			p += 9;
			len *= db2binDataWidth(i);
			while (len > 0) {
				appackSub0(&q, &of, *p >> 4);
				appackSub0(&q, &of, *p);
				p++;
				len -= 8;
			}
		} else if (*p == 0xfe && p[1] == 0x05 && p[2] == 0x02 && p[7] == 0x02 &&
			p[13] == 0x01 && p[14] == 0x00 && (p[15] << 24 | p[16] << 16 | p[17] << 8 | p[18]) == lastLabel + 1)
		{
			lastLabel++;
			appackSub1op(&q, &of, 0x06, hist, opTbl);
			appackSub1(&q, &of, p[8], 1);
			appackSub1(&q, &of, appackEncodeConst(p[9] << 24 | p[10] << 16 | p[11] << 8 | p[12]), 0);
			appackSub1(&q, &of, appackEncodeConst(p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6]), 0);
			p += 19;
			/* 本来なら対応するfe0101のために、エンコードしたのかどうかをスタックに記録するべき */
		} else if (*p == 0xfe && p[1] == 0x01 && p[2] == 0x01) {
			wait7++;
			if (p[31] != 0x00 && !(p[31] == 0xfe && p[32] == 0x01 && p[33] == 0x01) && p[31] != 0x3d) {
				do {
					appackSub1op(&q, &of, 0x07, hist, opTbl);
				} while (--wait7 > 0);
			}
			p += 31;
		} else if (*p == 0xfe && p[1] == 0x01 && p[2] == 0x03) {
			fe0103 = 1;
			p += 3;
		} else if (*p == 0xfe && p[1] == 0x01 && p[2] == 0xfc) {	// ALIGNPREFIX0
			if (of != 0)
				appackSub0(&q, &of, 0x0f);
			p += 3;
		} else if (*p == 0xfe && p[1] == 0x01 && p[2] == 0xfd) {	// ALIGNPREFIX1
			if (of == 0)
				appackSub0(&q, &of, 0x0f);
			p += 3;
		} else if (*p == 0xfe && p[1] == 0x01 && p[2] == 0xff) {
			appackSub0(&q, &of, 0x0f);
			p += 3;
		} else if (*p == 0xfe && p[1] == 0x01) {
			appackSub1op(&q, &of, 0xfe, hist, opTbl);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[2], 1);
			p += 3;
		} else if (*p == 0xfe && p[1] == 0x05) {
			i = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			if (p[2] == 0x01) {
				qq = q; oof = of;
				static struct {
					int i, skip0, b0, b1, c1, d, mod, p3f, e1, f1, b2, c2, e2, f2;
				} table_fe0501[] = {
					{ 0x0010, 13, 0x31, 0x32, 0,    0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_openWin
					{ 0x0011, 13, 0x31, 0x34, 0,    0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_flushWin
					{ 0x0009, 13, 0x31, 0x32, 0,    0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_sleep
					{ 0x000d, 13, 0x31, 0x31, 0,    0, 1, 0x28, 0x30, 0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_inkey
					{ 0x0002, 13, 0x31, 0x34, 0,    0, 1, 0x28, 0,    0,    3, 0, 0, 0 }, 	// junkApi_drawPoint
					{ 0x0003, 13, 0x31, 0x36, 0,    0, 1, 0x28, 0,    0,    5, 0, 0, 0 }, 	// junkApi_drawLine
					{ 0x0004, 13, 0x31, 0x36, 0,    0, 1, 0x28, 0,    0,    5, 0, 0, 0 }, 	// junkApi_fillRect/drawRect
					{ 0x0005, 13, 0x31, 0x36, 0,    0, 1, 0x28, 0,    0,    5, 0, 0, 0 }, 	// junkApi_fillOval/draeOval
					{ 0x0006, 13, 0x31, 0x36, 0,    1, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f }, 	// junkApi_drawString
					{ 0x0740, 13, 0x31, 0x31, 0,    0, 1, 0x28, 0x30, 0x31, 0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_fopenRead
					{ 0x0741, 13, 0x31, 0x32, 0x31, 0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_fopenWrite
					{ 0x0742, 13, 0x31, 0,    0,    0, 1, 0x28, 0,    0x31, 0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_allocBuf
					{ 0x0743, 13, 0x31, 0x31, 0x31, 0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_writeStdout
					{ 0x0001, 13, 0x31, 0,    0,    1, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f }, 	//
					{ 0x0013, 13, 0x31, 0x31, 0,    0, 1, 0x28, 0x30, 0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_rand
					{ 0x0014, 13, 0x31, 0x31, 0,    0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_randSeed
					{ 0x0015, 13, 0x31, 0,    0,    0, 1, 0x28, 0x30, 0,    0x3f, 0x3f, 0x3f, 0x3f },	// junkApi_getSeed
					{ 0x0016, 13, 0x31, 0,    0,    3, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f }, 	//
					{ 0x0017, 13, 0x31, 0x36, 0,    3, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f }, 	//
					{ 0x0018, 13, 0x31, 0x3b, 0x32, 0, 1, 0x28, 0,    0,    0x3f, 0x3f, 0x3f, 0x3f }, 	// junkApi_bitblt
					{ 0,      0,  0,    0,    0,    0, 0, 0,    0,    0,    0x3f, 0x3f, 0x3f, 0x3f }
				};
				for (j = 0; table_fe0501[j].skip0 > 0; j++) {
					if (i == table_fe0501[j].i) break;
				}
				if (table_fe0501[j].skip0 > 0) {
					appackSub1op(&qq, &oof, 0x04, hist, opTbl);
					appackSub1op(&qq, &oof, 0x05, hist, opTbl);
					appackSub1(&qq, &oof, i, 0);
					pp = fe0501b(&qq, &oof, table_fe0501[j].b0, table_fe0501[j].b1 - table_fe0501[j].b2, p + table_fe0501[j].skip0);
					pp = fe0501bb(table_fe0501[j].b1 - table_fe0501[j].b2 + 1, table_fe0501[j].b1, pp);
					if (pp != NULL) {
						pp = fe0501c(&qq, &oof, table_fe0501[j].c1 - table_fe0501[j].c2, pp);
						pp = fe0501cc(table_fe0501[j].c1 - table_fe0501[j].c2 + 1, table_fe0501[j].c1, pp);
						if (pp != NULL) {
							if (fe0501a(pp, lastLabel, table_fe0501[j].mod, table_fe0501[j].p3f) != 0) {
								lastLabel++;
								if (table_fe0501[j].mod == 1) pp += 18;
								pp = fe0501e(&qq, &oof, table_fe0501[j].e1, pp);
								pp = fe0501f(&qq, &oof, table_fe0501[j].f1, pp, pxxTyp);
								q = qq;
								of = oof;
								p = pp;
								pxxTyp[0x30] = TYP_CODE;
								for (i = 0x31; i <= 0x3e; i++)
									pxxTyp[i] = -2;	// ここはもっと頑張るべき.
								continue;
							}
						}
					}
					hist[opTbl[0x04]]--;
					hist[opTbl[0x05]]--;
					qq = q; oof = of;
					appackSub1op(&qq, &oof, 0x05, hist, opTbl);
					appackSub1(&qq, &oof, i, 0);
					pp = fe0501b(&qq, &oof, table_fe0501[j].b0, table_fe0501[j].b1, p + table_fe0501[j].skip0);
					pp = fe0501c(&qq, &oof, table_fe0501[j].c1, pp);
					pp = fe0501d(&qq, &oof, table_fe0501[j].d, pp);
					if (fe0501a(pp, lastLabel, table_fe0501[j].mod, table_fe0501[j].p3f) != 0) {
						lastLabel++;
						q = qq;
						of = oof;
						p = pp;
						if (table_fe0501[j].mod == 1) p += 18;
						p = fe0501e(&q, &of, table_fe0501[j].e1, p);
						p = fe0501f(&q, &of, table_fe0501[j].f1, p, pxxTyp);
						pxxTyp[0x30] = TYP_CODE;
						for (i = 0x31; i <= 0x3e; i++)
							pxxTyp[i] = -2;	// ここはもっと頑張るべき.
						continue;
					}
					hist[opTbl[0x05]]--;
				}
			}
			appackSub1op(&q, &of, 0xfe, hist, opTbl);
			appackSub1(&q, &of, p[1], 1);
			appackSub1(&q, &of, p[2], 1);
			appackSub1(&q, &of, i, 0);
			p += 7;
		} else if (*p == 0xff && p[1] == 0x00 && p[2] == 0xff) {
			appackSub1op(&q, &of, 0x0d, hist, opTbl);
			appackSub1op(&q, &of, 0x01, hist, opTbl);
			p += 3;
			for (;;) {
				i = *p++;
				appackSub1(&q, &of, i, 1);
				if (i == 0) break;
				if (i == 1) {
					for (j = 0; j < 256; j++)
						opTbl[j] = j;
					continue;
				}
				j = *p++;
				appackSub1(&q, &of, j, 1);
				opTbl[i] = j;
				opTbl[j] = i;
			}
		} else {
			printf("appack: error: %02X (at %06X)\n", *p, p - w->ibuf);
			break;
		}
	}
	for (i = 0; i < 4; i++)
		appackSub1(&q, &of, 0, 1); // 適当に4個くらいダミーの0を出力.

	for (i = 0; i < 4; i++) {	// ダミーも含めて余計な00をカット.
		if (q[-1] == 0x00)
			q--;
	}
	if ((w->flags & FLAGS_PUT_MANY_INFO) != 0) {
		printf("size=%d\n", q - w->obuf);
#if 0
		for (i = 0; i < 0x40; i++) {
			printf("%02X:%05d ", i, hist[i]);
			if ((i & 0x07) == 0x07) putchar('\n');
		}
#endif
	}
#endif
	return putBuf(w->argOut, w->obuf, q);
}

#undef R0
#undef R1

int appackSub2(const UCHAR **pp, char *pif)
{
	int r = 0;
	const UCHAR *p = *pp;
	if (*pif == 0) {
		r = *p >> 4;
	} else {
		r = *p & 0x0f;
		p++;
		*pp = p;
	}
	*pif ^= 1;
	return r;
}

int appackSub3(const UCHAR **pp, char *pif, char uf)
{
	int r = 0, i, j;
	if (uf != 0) {
		do {
			r = appackSub2(pp, pif);
		} while (r == 0xf);
		if (0x8 <= r && r <= 0xb) {
			r = r << 4 | appackSub2(pp, pif);
			r &= 0x3f;
		} else if (0xc <= r && r <= 0xd) {
			r = r << 4 | appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
			r &= 0x1ff;
		} else if (r == 0xe) {
			r = appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
		} else if (r == 0x7) {
			i = appackSub3(pp, pif, 1);
			r = 0;
			while (i > 0) {
				r = r << 4 | appackSub2(pp, pif);
				i--;
			}
		}
	} else {
		do {
			r = appackSub2(pp, pif);
		} while (r == 0xf);
		if (r <= 6) {
			#if (REVISION == 1)
				if (r >= 4) r -= 7;
			#elif (REVISION == 2)
				if (r >= 6) r -= 7;
			#endif
		} else if (0x8 <= r && r <= 0xb) {
			r = r << 4 | appackSub2(pp, pif);
			r &= 0x3f;
			if (r >= 0x20) r -= 0x40;
		} else if (0xc <= r && r <= 0xd) {
			r = r << 4 | appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
			r &= 0x1ff;
			if (r >= 0x100) r -= 0x200;
		} else if (r == 0xe) {
			r = appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
			r = r << 4 | appackSub2(pp, pif);
			if (r >= 0x800) r -= 0x1000;
		} else if (r == 0x7) {
			i = appackSub3(pp, pif, 1) - 1;
			r = appackSub2(pp, pif);
			j = 0x8;
			while (i > 0) {
				r = r << 4 | appackSub2(pp, pif);
				j <<= 4;
				i--;
			}
			if (r >= j) r -= j + j;
		}
	}
	return r;
}

UCHAR *appackSub4(UCHAR *q, int i)
{
	q[0] = (i >> 24) & 0xff;
	q[1] = (i >> 16) & 0xff;
	q[2] = (i >>  8) & 0xff;
	q[3] =  i        & 0xff;
	return q + 4;
}

char appackSub5(const UCHAR **pp, char *pif, UCHAR *preg, int *pr3f)
{
	char f = 0;
	int i = appackSub3(pp, pif, 0);
	if (-0x4f <= i && i <= -0x10) {
		f = 1;
		*preg = -0x10 - i;
	} else if (i >= -0x0f) {
		*pr3f = i;
		*preg = 0x3f;
	} else if (i <= -0x50) {
		*pr3f = i + 0x40;
		*preg = 0x3f;
	}
	return f;
}

int appack1(struct Work *w)
{
	const UCHAR *p = w->ibuf;
	UCHAR *q = w->obuf;
	*q++ = *p++;
	*q++ = *p++;
//	*q++ = *p++;
//	*q++ = *p++;
	char inf = 0, f = 0;
	int r3f = 0, lastLabel = -1, i, j;
	for (;;) {
		*q = appackSub3(&p, &inf, 1);
		if (*q == 0x00) break;
		if (*q == 0x01) {
			lastLabel++;
			q[1] = 0x00;
			if (f != 0) {
				i = appackSub3(&p, &inf, 0);
				q[1] = i & 0xff;
				lastLabel += i >> 8;
			}
			q = appackSub4(q + 2, lastLabel);
			f = 0;
		} else if (*q == 0x02) {
			q[1] = appackSub3(&p, &inf, 1);
			i = appackSub3(&p, &inf, 0);
			q = appackSub4(q + 2, i);
			f = 0;
		} else if (*q == 0x06) {
			f = 1;
		} else if (0x10 <= *q && *q <= 0x1b && *q != 0x17) {
			q[1] = appackSub3(&p, &inf, 1);
			if (f == 0)
				q[2] = q[1];
			else
				q[2] = appackSub3(&p, &inf, 1);
			f = appackSub5(&p, &inf, &q[3], &r3f);
			if (f != 0 && (q[2] == 0x3f || q[3] == 0x3f)) {
				f = 0;
				r3f = appackSub3(&p, &inf, 0);
			}
			if (f == 0) {
				q[6] = *q;
				q[7] = q[1];
				q[8] = q[2];
				q[9] = q[3];
				*q = 0x02;
				q[1] = 0x3f;
				q = appackSub4(q + 2, r3f);
			}
			q += 4;
			f = 0;
		} else if (0x20 <= *q && *q <= 0x27) {
			q[1] = 0x3f;
			if (f != 0)
				q[1] = appackSub3(&p, &inf, 1);
			q[2] = appackSub3(&p, &inf, 1);
			f = appackSub5(&p, &inf, &q[3], &r3f);
			if (f != 0 && (q[2] == 0x3f || q[3] == 0x3f)) {
				f = 0;
				r3f = appackSub3(&p, &inf, 0);
			}
			if (f != 0) {
				q[6] = *q;
				q[7] = q[1];
				q[8] = q[2];
				q[9] = q[3];
				*q = 0x02;
				q[1] = 0x3f;
				q = appackSub4(q + 2, r3f);
			}
			if (q[1] == 0x3f) {
				q[4] = 0x04;
				q[5] = 0x3f;
				q[6] = 0x03;
				q[7] = 0x00;
				i = appackSub3(&p, &inf, 0) + lastLabel;
				q = appackSub4(q + 8, i);
			} else
				q += 4;
			f = 0;
		} else {
			printf("err: %02X (at %06X, %06X)\n", *q, q - w->obuf, p - w->ibuf);
			break;
		}
	}
	return putBuf(w->argOut, w->obuf, q);
}

int appack2(struct Work *w, char level)
{
	FILE *fp;
	int i = 0;
	UCHAR *p, *q = NULL, *pp;
	fp = fopen("$tmp.org", "wb");
	fwrite(w->ibuf + 2, 1, w->isiz - 2, fp);
	fclose(fp);
	w->obuf[0] = w->ibuf[0];
	w->obuf[1] = w->ibuf[1];
	if (level == 0) {
		i = system("bim2bin -osacmp in:$tmp.org out:$tmp.tk5");
	} else {
		i = system("bim2bin -osacmp in:$tmp.org out:$tmp.tk5 eopt:@ eprm:@");
	}
	if (i == 0) {
		printf("org:%d ", w->isiz);
		fp = fopen("$tmp.tk5", "rb");
		w->isiz = fread(w->ibuf, 1, BUFSIZE - 4, fp);
		fclose(fp);
		p = w->ibuf + 16;
		i = 0;
		do {
			i = i << 7 | *p++ >> 1;
		} while ((p[-1] & 1) == 0);
		//printf("org.size=%d\n", i);
		AppackWork aw;
		aw.q = &w->obuf[3];
		aw.qHalf = 0;
		w->obuf[2] = 0x02;
		char flag8 = 0;
	//	if (0x000000 <= i && i <= 0x000006) flag8 = 0;	// x
		if (0x000007 <= i && i <= 0x00003f) flag8 = 1;	// 8x
	//	if (0x000040 <= i && i <= 0x0001ff) flag8 = 0;	// cxx
		if (0x000200 <= i && i <= 0x000fff) flag8 = 1;	// exxx
		if (0x001000 <= i && i <= 0x00ffff) flag8 = 1;	// 74xxxx
	//	if (0x010000 <= i && i <= 0x0fffff) flag8 = 0;	// 75xxxxx
		if (0x100000 <= i && i <= 0xffffff) flag8 = 1;	// 76xxxxxx
		if (i >= 0x01000000) { fprintf(stderr, "appack2: too large file.\n"); exit(1); }
		if (flag8 != 0)
			appackSub0(&aw, 0x08);
		if (*p == 0x15)      { appackSub1u(&aw, 1); p++; }
		else if (*p == 0x19) { appackSub1u(&aw, 2); p++; }
		else if (*p == 0x21) { appackSub1u(&aw, 3); p++; }
		else                 { appackSub1u(&aw, 0);      }
		appackSub1u(&aw, i);
		q = aw.q;
		while (p < w->ibuf + w->isiz)
			*q++ = *p++;
		printf("tk5:%d\n", q - w->obuf);
	} else {
		fputs("appack2: bim2bin error\n", stderr);
	}
	remove("$tmp.org");
	remove("$tmp.tk5");
	return putBuf(w->argOut, w->obuf, q);
}

int appack(struct Work *w)
{
	int r = 0, f = w->flags >> 2;
	if (f == 0) r = appack0(w, 0);
	if (f == 1) r = appack0(w, 2);	// encLidr=1
	if (f == 2) r = appack0(w, 3);	// simple=1, encLidr=1
	if (f == 4) r = appack2(w, 0);	// tek5 fast
	if (f == 5) r = appack2(w, 1);	// tek5 best (very slow)

//	if (f == 1) r = appack1(w);
//	if (f == 2) r = appack2(w, 0);
//	if (f == 3) r = appack2(w, 1);
	return r;
}

int maklib0(struct Work *w)
{
	UCHAR *q = w->obuf;
	q[0] = 0x05;
	q[1] = 0x1b;
	q[2] = 0x00;
	q[3] = 0x00;
	q[4] = 0;
	q[5] = 0;
	q[6] = 0;
	q[7] = 24;
	int i, j;
	for (j = 0; j < 1; j++) {
		for (i = 0; i < 16; i++) {
			q[8 + j * 20 + 4 + i] = 0;
		}
	}
	strcpy(&q[8 + 20 * 0 + 4], "decoder");
	q[8 + 20 * 0 + 0] = 0;
	q[8 + 20 * 0 + 1] = 0;
	q[8 + 20 * 0 + 2] = 0;
	q[8 + 20 * 0 + 3] = 32;
	q[8 + 20 * 1 + 0] = 0xff;
	q[8 + 20 * 1 + 1] = 0xff;
	q[8 + 20 * 1 + 2] = 0xff;
	q[8 + 20 * 1 + 3] = 0xff;
	FILE *fp = fopen("decoder.ose", "rb");
	if (fp == NULL) {
		fputs("fopen error: decoder.ose", fp);
		return 1;
	}
	i = fread(&q[8 + 20 * 1 + 4], 1, 1024 * 1024 - 256, fp);
	fclose(fp);
	q[8 + 20 * 1 + 4 + i] = 0xff;
	return putBuf(w->argOut, w->obuf, q + 8 + 20 * 1 + 4 + i);
}

int maklib1(struct Work *w)
{
	int i;
	if (w->ibuf[0] == 0x05 && w->ibuf[1] == 0x1b) {
		for (i = 2; i < w->isiz; i++)
			w->obuf[i - 2] = w->ibuf[i];
		return putBuf(w->argOut, w->obuf, w->obuf + w->isiz - 2);
	}
	return 1; // error.
}

int maklib2(struct Work *w)
{
	static UCHAR header[] = { 
		0x81, 0xfc, 0x78, 0x78, 0x77, 0x02, 0xcd, 0x20, 0xb9, 0x78, 0x78, 0xbe, 0x78, 0x78, 0xbf, 0x78,
		0x78, 0xbb, 0x00, 0x80, 0xfd, 0xf3, 0xa4, 0xfc, 0x87, 0xf7, 0x83, 0xee, 0xc7, 0x19, 0xed, 0x57,
		0xe9, 0x78, 0x78, 0x55, 0x50, 0x58, 0x21
	};
	static UCHAR footer[] = {
		0xa4, 0xe8, 0x3a, 0x00, 0x72, 0xfa, 0x41, 0xe8, 0x2f, 0x00, 0xe3, 0x3a, 0x73, 0xf9, 0x83, 0xe9,
		0x03, 0x72, 0x06, 0x88, 0xcc, 0xac, 0xf7, 0xd0, 0x95, 0x31, 0xc9, 0xe8, 0x1b, 0x00, 0x11, 0xc9,
		0x75, 0x08, 0x41, 0xe8, 0x13, 0x00, 0x73, 0xfb, 0x41, 0x41, 0x81, 0xfd, 0x00, 0xf3, 0x83, 0xd1,
		0x01, 0x8d, 0x03, 0x96, 0xf3, 0xa4, 0x96, 0xeb, 0xc8, 0xe8, 0x02, 0x00, 0x11, 0xc9, 0x01, 0xdb,
		0x75, 0x04, 0xad, 0x11, 0xc0, 0x93, 0xc3
	};

	int i;
	UCHAR uc = 0, *p = w->ibuf + w->isiz - sizeof footer;
	for (i = 0; i < sizeof header; i++) {
		if (header[i] != 0x78)
			uc |= w->ibuf[i] ^ header[i];
	}
	if (uc != 0) {
		fputs("maklib2: header error", stderr);
		return 1;
	}
	for (i = 0; i < sizeof footer; i++) {
		if (footer[i] != 0x78)
			uc |= p[i] ^ footer[i];
	}
	if (uc != 0) {
		fputs("maklib2: footer error", stderr);
		return 1;
	}
	w->obuf[0] = 0x05;
	w->obuf[1] = 0xc1; /* Compressed Library */
	for (i = 0; i < w->isiz - 0x39 - sizeof footer; i++)
		w->obuf[i + 2] = w->ibuf[i + 0x39];
	return putBuf(w->argOut, w->obuf, &w->obuf[i + 2]);
}

struct ComLib_Str {
	const UCHAR *p;
	int bitBuf, bitBufLen;
	int tmp;
};

int ComLib_getBit(struct ComLib_Str *s)
{
	if (s->bitBufLen == 0) {
		s->bitBuf = s->p[0] | s->p[1] << 8;
		s->p += 2;
		s->bitBufLen |= 16;
	}
	s->bitBufLen--;
	return (s->bitBuf >> s->bitBufLen) & 1;
}

int ComLib_getTmpBit(struct ComLib_Str *s)
{
	s->tmp = (s->tmp << 1 | ComLib_getBit(s)) & 0xffff;
	return ComLib_getBit(s);
}

UCHAR *ComLib_main(const UCHAR *p, UCHAR *q)
{
	struct ComLib_Str s;
	int i, dis;
	dis |= -1;
	s.p = p;
	s.bitBufLen &= 0;
	goto l1;
l0:
	*q++ = *s.p++;
l1:
	i = ComLib_getBit(&s);
	if (i != 0) goto l0;
	s.tmp = 1;
	do {
		i = ComLib_getTmpBit(&s);
		if (s.tmp == 0) goto fin;
	} while (i == 0);
	if (s.tmp >= 3)
		dis = ~((s.tmp - 3) << 8 | *s.p++);
	s.tmp &= 0;
	i = ComLib_getTmpBit(&s);
	s.tmp = s.tmp << 1 | i;
	if (s.tmp == 0) {
		s.tmp |= 1;
		do {
			i = ComLib_getTmpBit(&s);
		} while (i == 0);
		s.tmp += 2;
	}
	s.tmp++;
	if (dis < -0xd00) s.tmp++;
	for (i = 0; i < s.tmp; i++)
		q[i] = q[i + dis];
	q += s.tmp;
	goto l1;
fin:
	return q;
}

int maklib3(struct Work *w)
{
	if (w->ibuf[0] == 0x05 && w->ibuf[1] == 0xc1) {
		w->obuf[0] = 0x05;
		w->obuf[1] = 0x1b;
		return putBuf(w->argOut, w->obuf, ComLib_main(w->ibuf + 2, w->obuf + 2));
	}
	return 1;
}

int maklib(struct Work *w)
{
	int r = 0, f = w->flags >> 2;
	if (f == 0) r = maklib0(w);
	if (f == 1) r = maklib1(w);
	if (f == 2) r = maklib2(w);
	if (f == 3) r = maklib3(w);
	return r;
}

void fc_sub0(const UCHAR **pp, char *pi, const UCHAR *p1, char i1)
{
	int c = 0;
	while (*pp != p1 || *pi != i1) {
		if (c < 256) {
			if (*pi == 0)
				printf("%X", **pp >> 4);
			else
				printf("%X", **pp & 0xf);
		}
		c++;
		if (c == 256)
			printf("...");
		*pi ^= 1;
		if (*pi == 0)
			(*pp)++;
	}
	putchar(' ');
	return;
}

struct fc_str0 {
	const UCHAR **pp;
	char *pi;
	UCHAR bitbuf, buflen;
};

void fc_sub1(struct fc_str0 *bb, const UCHAR **pp, char *pi)
{
	bb->pp = pp;
	bb->pi = pi;
	bb->buflen = 0;
	return;
}

int fc_sub2(struct fc_str0 *bb)
{
	if (bb->buflen <= 0) {
		bb->bitbuf = appackSub2(bb->pp, bb->pi);
		bb->buflen = 4;
	}
	bb->buflen--;
	return (bb->bitbuf >> bb->buflen) & 1;
}

void fc_sub3(const UCHAR **pp, char *pi, const UCHAR **p1, char *i1)
{
	int i = appackSub3(p1, i1, 1); fc_sub0(pp, pi, *p1, *i1); // mode.
	int j, k;
	struct fc_str0 bb;
	if (i == 0) {
		j = appackSub3(p1, i1, 1); fc_sub0(pp, pi, *p1, *i1); // len
		while (j > 0) {
			appackSub2(p1, i1); appackSub2(p1, i1); fc_sub0(pp, pi, *p1, *i1); // imm. or reg.
			j--;
		}
	} else if (i == 4) {
		do {
			j = appackSub3(p1, i1, 0); fc_sub0(pp, pi, *p1, *i1);
		} while (j != 0);
	} else if (i == 5) {
		j = appackSub2(p1, i1) + 4; fc_sub0(pp, pi, *p1, *i1); // len
		fc_sub1(&bb, p1, i1);
		while (j > 0) {
			for (k = 0; k < 7; k++)
				fc_sub2(&bb);
			j--;
		}
		fc_sub0(pp, pi, *p1, *i1);
	} else if (i == 6) {
		fc_sub1(&bb, p1, i1);
		do {
			j = 0;
			for (k = 0; k < 7; k++)
				j = j << 1 | fc_sub2(&bb);
		} while (j != 0);
		fc_sub0(pp, pi, *p1, *i1);
	} else {
		printf("-- array0-mod: %02X\n", i);
		exit(1);
	}
	return;
}

void fc_sub4(const UCHAR **pp, char *pi, const UCHAR **p1, char *i1)
{
	int i = appackSub3(p1, i1, 1); fc_sub0(pp, pi, *p1, *i1); // mode.
	int j;
	if (i == 1) {
		j = appackSub3(p1, i1, 1); fc_sub0(pp, pi, *p1, *i1); // len
		while (j > 0) {
			appackSub3(p1, i1, 0); fc_sub0(pp, pi, *p1, *i1); // imm. or reg.
			j--;
		}
	} else {
		printf("-- array1-mod: %02X\n", i);
		exit(1);
	}
	return;
}

#if 0

int fcode(struct Work *w)
{
	const UCHAR *p = w->ibuf + 2, *p0 = p;
	char inf = 0, f4 = 0, inf0 = inf, fd = 0;
	int op, *hist = malloc(256 * sizeof (int));
	int r3f = 0, lastLabel = -1, i, j, k = 0;
	char *pregTypeFlag = malloc(64);
	UCHAR *opTbl = malloc(256);
	for (op = 0; op < 256; op++) {
		hist[op] = 0;
		opTbl[op] = op;
	}
	for (i = 0; i < 64; i++)
		pregTypeFlag[i] = 0; /* unknown */
	pregTypeFlag[1] |= 1;
	for (;;) {
		op = appackSub3(&p, &inf, 1);
		if (op == 0x00) break;
		if ((f4 | fd) == 0)
			printf("#%04d %04X.%d: ", k++, p0 - w->ibuf, inf0 << 3);
		fc_sub0(&p0, &inf0, p, inf);
		if (0 <= op && op <= 255)
			hist[op]++;
		op = opTbl[op];
		if (op == 0x04 && f4 == 0) {
			f4 |= 1;
			continue;
		}
		if (op == 0x0d) {
			fd |= 1;
			continue;
		}
		if (op == 0x01) {
			if (fd != 0) {
				for (;;) {
					i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // imm.
					if (i == 0) break;
					if (i == 1) {
						for (i = 0; i < 256; i++)
							opTbl[i] = i;
						continue;
					}
					j = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // imm.
					opTbl[i] = j;
					opTbl[j] = i;
				}
			} else {
				if (f4 != 0)
					appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm.
			}
			goto end_op;
		}
		if (op == 0x02) {
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm.
end_op:
			putchar('\n');
			f4 = fd = 0;
			continue;
		}
		if (op == 0x03) {
			i = 0x3f;
			if (f4 != 0) {
				i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
				pregTypeFlag[i] |= 1;
			}
			appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm.
			goto end_op;
		}
		if (op == 0x04) {
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			goto end_op;
		}
		if (op == 0x05) {
			i = appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm.
			if (i == 0x0001) {
				fc_sub3(&p0, &inf0, &p, &inf);
				goto end_op;
			}
			if (i == 0x0002) {
				if (f4 == 0) {
					for (j = 0; j < 4; j++) {
						appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
					}
				} else {
					appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
				}
				goto end_op;
			}
			if (i == 0x0003 || i == 0x0004 || i == 0x0005) {
				if (f4 == 0) {
					for (j = 0; j < 6; j++) {
						appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
					}
				} else {
					appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
				}
				goto end_op;
			}
			if (i == 0x0006) {
				for (j = 0; j < 6; j++) {
					appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
				}
				fc_sub3(&p0, &inf0, &p, &inf);
				goto end_op;
			}
			if (i == 0x0009 || i == 0x0010) {
				for (j = 0; j < 2; j++) {
					appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
				}
				goto end_op;
			}
			if (i == 0x000d || i == 0x0013) {
				appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
				appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
				goto end_op;
			}
			if (i == 0x0016) {
				fc_sub3(&p0, &inf0, &p, &inf);
				fc_sub4(&p0, &inf0, &p, &inf);
				goto end_op;
			}
			if (i == 0x0017) {
				for (j = 0; j < 6; j++) {
					appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
				}
				fc_sub3(&p0, &inf0, &p, &inf);
				fc_sub4(&p0, &inf0, &p, &inf);
				goto end_op;
			}
			if (i == 0x0740) {
				appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
				appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
				appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
				goto end_op;
			}
			if (i == 0x0741) {
				for (j = 0; j < 3; j++) {
					appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
				}
				goto end_op;
			}
			printf("-- unknown function code: %04X\n", i);
			break;
		}
		if (op == 0x06) {
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm.
			appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm.
			goto end_op;
		}
		if (op == 0x07)
			goto end_op;
		if (0x08 <= op && op <= 0x0b) {
			j = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // preg-no.
			if (fd != 0 || pregTypeFlag[i] == 0) {
				appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // imm.
				pregTypeFlag[i] |= 1;
			}
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // imm.
			if (op == 0x0a)
				pregTypeFlag[j] &= 0;
			goto end_op;
		}
		if (op == 0x0e) {
			j = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // preg-no.
			i = j;
			if (f4 != 0) {
				i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // preg-no.
			}
			if (fd != 0 || pregTypeFlag[i] == 0) {
				appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // imm.
			}
			pregTypeFlag[j] |= 1;
			pregTypeFlag[i] |= 1;
			appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
			goto end_op;
		}
		if (op == 0x17) {
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			goto end_op;
		}
		if (0x10 <= op && op <= 0x1b) {
			i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			if (f4 != 0) {
				i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			}
			appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
			if (i == 0x3f)
				appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm.
			goto end_op;
		}
		if (op == 0x1e) {
			i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			j = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			pregTypeFlag[i] = pregTypeFlag[j];
			goto end_op;
		}
		if (0x20 <= op && op <= 0x27) {
			i = 0x3f;
			if (f4 != 0) {
				i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			}
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
			if (i == 0x3f) {
				appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm.
			}
			goto end_op;
		}
		if (0x28 <= op && op <= 0x2d) {
			i = 0x3f;
			if (f4 != 0) {
				i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			}
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm. or reg.
			if (i == 0x3f) {
				appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm.
			}
			goto end_op;
		}
		if (op == 0x2e) {
			i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // typ.
			j = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // len.
			j *= db2binDataWidth(i);
			j >>= 2;
			while (j > 0) {
				appackSub2(&p, &inf);
				j--;
			}
			fc_sub0(&p0, &inf0, p, inf);
			goto end_op;
		}
		if (op == 0x30 || op == 0x32) {
			if (f4 != 0) {
				i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // preg-no.
				appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
				appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
				pregTypeFlag[i] &= 0; /* unknown */
				goto end_op;
			}
		}
		if (0x38 <= op && op <= 0x3b) {
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // preg-no.
			if (fd != 0 || pregTypeFlag[i] == 0) {
				appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // imm.
				pregTypeFlag[i] |= 1;
			}
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // reg-no.
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // imm.
			goto end_op;
		}
		if (op == 0x3c) {
			if (f4 == 0)
	 			goto end_op;
		}
		if (op == 0x3d)
			goto end_op;
		if (op == 0x3e) {
			if (f4 != 0) {
				appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // imm.
			}
			appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // imm.
			pregTypeFlag[0x30] |= 1;
			for (i = 0x31; i <= 0x3e; i++)
				pregTypeFlag[i] &= 0;
			goto end_op;
		}
		if (op == 0x3f) {
			if (f4 != 0) {
				appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf); // imm.
			}
			appackSub3(&p, &inf, 0); fc_sub0(&p0, &inf0, p, inf); // imm.
			pregTypeFlag[0x30] |= 1;
			for (i = 0x31; i <= 0x3e; i++)
				pregTypeFlag[i] &= 0;
			goto end_op;
		}
		if (op == 0xfe) {
			i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf);
			if (i == 1) {
				i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf);
			} else if (i == 5) {
				i = appackSub3(&p, &inf, 1); fc_sub0(&p0, &inf0, p, inf);
			}
			goto end_op;
		}

		printf("-- unknown opecode: %02X\n", op);
		break;
	}
	puts("freq[]:");
	for (op = 0; op < 64; op++) {
		printf("%02X: %04d  ", op, hist[op]);
		if ((op & 7) == 7)
			putchar('\n');
	}
	return 0;
}

#endif

int b32(struct Work *w)
{
	int i, j, k, l, *qi, *qi1;
	char inf, buf4[4];
	const UCHAR *p;
	UCHAR *q;
	if (w->ibuf[0] != 0x05 || w->ibuf[1] != 0xe2 || w->ibuf[2] != 0x00) {
		fputs("input file format error.\n", stderr);
		exit(1);
	}
	p = &(w->ibuf[3]);
	inf = 0;
	qi = (int *) w->obuf;
	*qi++ = 0x05e200cf;
	*qi++ = 0xee7ff188;
	for (;;) {
		if (p >= &(w->ibuf[w->isiz])) break;
		i = appackSub3(&p, &inf, 1);
		*qi++ = i | 0x76000000;
		if (i == 0x01) {	// LB(uimm,opt);
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			continue;
		}
		if (i == 0x02) {	// LIMM(imm,r,bit);
			*qi++ = 0xfffff788;
			*qi++ = appackSub3(&p, &inf, 0);
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1);
			*qi++ = i | 0x76000000;
			if (i > 32) {
err_bit32:
				fputs("bit > 32 error.\n", stderr);
				exit(1);
			}
			continue;
		}
		if (i == 0x03) {	// PLIMM(uimm,p);
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			continue;
		}
		if (i == 0x04) {	// CND(r);
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			continue;
		}
		if (i == 0x08) {	// LMEM(p,typ,0,r,bit);
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1);
			if (!(0x02 <= i && i <= 0x15)) {
				fputs("LMEM typ error.\n", stderr);
				exit(1);
			}
			*qi++ = 0x76000006; // typ6 (SInt32)
			i = appackSub3(&p, &inf, 1);
			if (i != 0) {
				fputs("LMEM addressing error.\n", stderr);
				exit(1);
			}
			*qi++ = 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1); *qi++ = i | 0x76000000; if (i > 32) goto err_bit32;
			continue;
		}
		if (i == 0x09) {	// SMEM(r,bit,p,typ,0);
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1); *qi++ = i | 0x76000000; if (i > 32) goto err_bit32;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1);
			if (!(0x02 <= i && i <= 0x15)) {
				fputs("SMEM typ error.\n", stderr);
				exit(1);
			}
			*qi++ = 0x76000006; // typ6 (SInt32)
			i = appackSub3(&p, &inf, 1);
			if (i != 0) {
				fputs("SMEM addressing error.\n", stderr);
				exit(1);
			}
			*qi++ = 0x76000000;
			continue;
		}
		if (i == 0x0e) {	// PADD(p1,typ,r,bit,p0);
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1);
			if (!(0x02 <= i && i <= 0x15)) {
				fputs("LMEM typ error.\n", stderr);
				exit(1);
			}
			*qi++ = 0x76000006; // typ6 (SInt32)
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1); *qi++ = i | 0x76000000; if (i > 32) goto err_bit32;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			continue;
		}
		if (0x10 <= i && i <= 0x16) {	// OR(r1,r2,r0,bit);など.
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1); *qi++ = i | 0x76000000; if (i > 32) goto err_bit32;
			continue;
		}
		if (0x18 <= i && i <= 0x1b) {	// SHL(r1,r2,r0,bit);など.
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1); *qi++ = i | 0x76000000; if (i > 32) goto err_bit32;
			continue;
		}
		if (i == 0x1e) {	// PCP(p1,p0);
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			continue;
		}
		if (0x20 <= i && i <= 0x27) {	// CMPcc(r1,r2,bit1,r0,bit0);など.
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1); *qi++ = i | 0x76000000; if (i > 32) goto err_bit32;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1); *qi++ = i | 0x76000000; if (i > 32) goto err_bit32;
			continue;
		}
		if (i == 0x2e) {	// data(typ, len, ...);
			int usgn = 0, bitlen = 0, bitbuf = 0, bitbuflen;
			i = appackSub3(&p, &inf, 1);
			j = appackSub3(&p, &inf, 1);
			if (0x02 <= i && i <= 0x15) {
				usgn = i & 1;
				bitlen = db2binDataWidth(i);
			}
			if (bitlen > 0) {
				*qi++ = 0x76000006; // typ6 (SInt32)
				*qi++ = j | 0x76000000;
				bitbuflen = 0;
				for (k = 0; k < j; k++) {
					i = 0;
					for (l = 0; l < bitlen; l++) {
						if (bitbuflen <= 0) {
							bitbuf = appackSub2(&p, &inf);
							bitbuflen = 4;
						}
						i = i << 1 | ((bitbuf >> 3) & 1);
						bitbuf <<= 1;
						bitbuflen--;
					}
					if (usgn == 0 && bitlen < 32) {
						if ((i & (1 << (bitlen - 1))) != 0)
							i -= 1 << bitlen;
					}
					*qi++ = i;
				}
				continue;
			}
			fprintf(stderr, "internal error: op=0x2e, typ=0x%02x\n", i);
			exit(1);
		}
		if (0x30 <= i && i <= 0x33) {
			if ((i & 1) == 0) {
				j = appackSub3(&p, &inf, 1) | 0x76000000;
				if (qi[-6] == 0x76000002 && qi[-5] == 0xfffff788 && qi[-3] == j) {
					// 直前のLIMM命令でtypを設定していた場合は修正する.
					k = qi[-4];
					if (0x02 <= k && k <= 0x15)
						k = 0x0006;
					qi[-4] = k;
				}
				*qi++ = j;
				*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			} else {
				*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
				j = appackSub3(&p, &inf, 1) | 0x76000000;
				if (qi[-7] == 0x76000002 && qi[-6] == 0xfffff788 && qi[-4] == j) {
					// 直前のLIMM命令でtypを設定していた場合は修正する.
					k = qi[-5];
					if (0x02 <= k && k <= 0x15)
						k = 0x0006;
					qi[-5] = k;
				}
				*qi++ = j;
			}
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			continue;
		}
		if (0x3c <= i && i <= 0x3d) {	// ENTER(rn,bit0,pn,fn,bit1,0);など.
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1);
			if (i != 0) {
				fputs("ENTER/LEAVE typ error.\n", stderr);
				exit(1);
			}
			*qi++ = 0x76000000;
			continue;
		}
		if (i == 0xfd) {	// LIDR(imm,r);
			*qi++ = 0xfffff788;
			*qi++ = appackSub3(&p, &inf, 0);
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			continue;
		}
		if (i == 0xfe) {
			*qi++ = appackSub3(&p, &inf, 1) | 0x76000000;
			i = appackSub3(&p, &inf, 1);
			*qi++ = i | 0x76000000;
			for (j = 0; j < i; j++) {
				*qi++ = 0xfffff788;
				*qi++ = appackSub3(&p, &inf, 0);
			}
			continue;
		}
		fprintf(stderr, "internal error: op=0x%02x\n", i);
		exit(1);
	}
	qi1 = qi;
	for (qi = (int *) w->obuf; qi < qi1; qi++) {
		q = (UCHAR *) qi;
		buf4[0] = q[0];
		buf4[1] = q[1];
		buf4[2] = q[2];
		buf4[3] = q[3];
		q[0] = buf4[3];
		q[1] = buf4[2];
		q[2] = buf4[1];
		q[3] = buf4[0];
	}
	return putBuf(w->argOut, w->obuf, (UCHAR *) qi1);
}

int dumpbk(struct Work *w)
{
	const UCHAR *p = &(w->ibuf[3]), *p1 = &(w->ibuf[w->isiz]);
	int i, j, k;
	if (w->ibuf[0] != 0x05 || w->ibuf[1] != 0xe2 || w->ibuf[2] != 0x00) {
		fputs("input file format error.\n", stderr);
		exit(1);
	}
	printf("0000: 05e2 0_0\n");
	while (p < p1) {
		printf("%04x: ", p - w->ibuf);
		if (cmpBytes(p, "f1_f788#4_f788#4") != 0) {
			i = p[ 3] << 24 | p[ 4] << 16 | p[ 5] << 8 | p[ 6];
			j = p[ 9] << 24 | p[10] << 16 | p[11] << 8 | p[12];
			printf("  f1_f788%08x_f788%08x\n", i, j);
			p += 13;
			continue;
		}
		if (cmpBytes(p, "f2_f788#4_Yx_f788#4") != 0) {
			i = p[ 3] << 24 | p[ 4] << 16 | p[ 5] << 8 | p[ 6];
			j = p[10] << 24 | p[11] << 16 | p[12] << 8 | p[13];
			printf("  f2_f788%08x_%02x_f788%08x\n", i, p[7], j);
			p += 14;
			continue;
		}
		if (cmpBytes(p, "f3_f788#4_Yx") != 0) {
			i = p[ 3] << 24 | p[ 4] << 16 | p[ 5] << 8 | p[ 6];
			printf("  f3_f788%08x_%02x\n", i, p[7]);
			p += 8;
			continue;
		}
		if (cmpBytes(p, "f4_Yx") != 0) {
			printf("  f4_%02x\n", p[1]);
			p += 2;
			continue;
		}
		if (cmpBytes(p, "88_Yx_f788#4_f788#4_Yx_f788#4") != 0) {
			i = p[ 4] << 24 | p[ 5] << 16 | p[ 6] << 8 | p[ 7];
			j = p[10] << 24 | p[11] << 16 | p[12] << 8 | p[13];
			k = p[17] << 24 | p[18] << 16 | p[19] << 8 | p[20];
			printf("  88_%02x_f788%08x_f788%08x_%02x_f788%08x\n", p[1], i, j, p[14], k);
			p += 21;
			continue;
		}
		if (cmpBytes(p, "89_Yx_f788#4_Yx_f788#4_f788#4") != 0) {
			i = p[ 4] << 24 | p[ 5] << 16 | p[ 6] << 8 | p[ 7];
			j = p[11] << 24 | p[12] << 16 | p[13] << 8 | p[14];
			k = p[17] << 24 | p[18] << 16 | p[19] << 8 | p[20];
			printf("  89_%02x_f788%08x_%02x_f788%08x_f788%08x\n", p[1], i, p[8], j, k);
			p += 21;
			continue;
		}
		if (cmpBytes(p, "8e_Yx_f788#4_Yx_f788#4_Yx") != 0) {
			i = p[ 4] << 24 | p[ 5] << 16 | p[ 6] << 8 | p[ 7];
			j = p[11] << 24 | p[12] << 16 | p[13] << 8 | p[14];
			printf("  8e_%02x_f788%08x_%02x_f788%08x_%02x\n", p[1], i, p[8], j, p[15]);
			p += 16;
			continue;
		}
		if (cmpBytes(p, "9x_Yx_Yx_Yx_f788#4") != 0 && *p <= 0x9b && *p != 0x97) {
			i = p[ 6] << 24 | p[ 7] << 16 | p[ 8] << 8 | p[ 9];
			printf("  %02x_%02x_%02x_%02x_f788%08x\n", *p, p[1], p[2], p[3], i);
			p += 10;
			continue;
		}
		if (cmpBytes(p, "9e_Yx_Yx") != 0) {
			printf("  9e_%02x_%02x\n", p[1], p[2]);
			p += 3;
			continue;
		}
		if (cmpBytes(p, "ax_Yx_Yx_f788#4_Yx_f788#4") != 0 && *p <= 0xa7) {
			i = p[ 5] << 24 | p[ 6] << 16 | p[ 7] << 8 | p[ 8];
			j = p[12] << 24 | p[13] << 16 | p[14] << 8 | p[15];
			printf("  %02x_%02x_%02x_f788%08x_%02x_f788%08x\n", *p, p[1], p[2], i, p[9], j);
			p += 16;
			continue;
		}
		if (cmpBytes(p, "ae_f788#4_f788#4") != 0) {
			i = p[ 3] << 24 | p[ 4] << 16 | p[ 5] << 8 | p[ 6];
			j = p[ 9] << 24 | p[10] << 16 | p[11] << 8 | p[12];
			p += 13;
			printf("  ae_f788%08x_f788%08x\n", i, j);
			printf("%04x:     ", p - w->ibuf);
			i = db2binDataWidth(i) * j / 8;
			for (j = 0; j < i; j++)
				printf("%02x", *p++);
			printf("\n");
			continue;
		}
		if (cmpBytes(p, "b2_Yx_f788#4_Yx_f788#4_Yx") != 0) {
			i = p[ 4] << 24 | p[ 5] << 16 | p[ 6] << 8 | p[ 7];
			j = p[11] << 24 | p[12] << 16 | p[13] << 8 | p[14];
			printf("  b2_%02x_f788%08x_%02x_f788%08x_%02x\n", p[1], i, p[8], j, p[15]);
			p += 16;
			continue;
		}
		if (cmpBytes(p, "bx_f788#4_f788#4_f788#4_f788#4_f788#4_f788#4") != 0 && 0xbc <= *p && *p <= 0xbd) {
			i = p[ 3] << 24 | p[ 4] << 16 | p[ 5] << 8 | p[ 6];
			j = p[ 9] << 24 | p[10] << 16 | p[11] << 8 | p[12];
			k = p[15] << 24 | p[16] << 16 | p[17] << 8 | p[18];
			printf("  %02x_f788%08x_f788%08x_f788%08x_", *p, i, j, k);
			i = p[21] << 24 | p[22] << 16 | p[23] << 8 | p[24];
			j = p[27] << 24 | p[28] << 16 | p[29] << 8 | p[30];
			k = p[33] << 24 | p[34] << 16 | p[35] << 8 | p[36];
			printf("f788%08x_f788%08x_f788%08x\n", i, j, k);
			p += 37;
			continue;
		}
		if (cmpBytes(p, "fcfdf788#480") != 0) {
			i = p[ 4] << 24 | p[ 5] << 16 | p[ 6] << 8 | p[ 7];
			printf("fcfd_f788%08x_80 (%d)\n", i, i);
			p += 9;
			continue;
		}
		if (cmpBytes(p, "fcfe_x0") != 0 && p[2] <= 0x60) {
			printf("  fcfe_%x_0\n", p[2] >> 4);
			p += 3;
			continue;
		}
		if (cmpBytes(p, "fcfe_bxf0") != 0) {
			printf("  fcfe_%x_f0\n", p[2]);
			p += 4;
			continue;
		}
		if (cmpBytes(p, "fcfe_bxf1_f788#4") != 0) {
			i = p[ 6] << 24 | p[ 7] << 16 | p[ 8] << 8 | p[ 9];
			printf("  fcfe_%x_f1_f788%08x\n", p[2], i);
			p += 10;
			continue;
		}
		if (cmpBytes(p, "fcfe_21_f788") != 0) {
			i = p[ 5] << 24 | p[ 6] << 16 | p[ 7] << 8 | p[ 8];
			printf("  fcfe_2_1_f788%08x\n", i);
			p += 9;
			continue;
		}

		printf("\ndumpbk: unknown opecode: %02x-%02x-%02x-%02x\n", *p, p[1], p[2], p[3]);
		exit(1);
	}
	return 0;
}

// hh4関係.

typedef struct _Hh4ReaderPointer {
	const unsigned char *p;
	int half;
} Hh4ReaderPointer;

typedef struct _Hh4Reader {
	Hh4ReaderPointer p, p1;
	int length, errorCode;
} Hh4Reader;


void hh4ReaderInit(Hh4Reader *hh4r, void *p, int half, void *p1, int half1)
{
	hh4r->p.p = p;
	hh4r->p.half = half;
	hh4r->p1.p = p1;
	hh4r->p1.half = half1;
	hh4r->errorCode = 0;
	return;
}

int hh4ReaderEnd(Hh4Reader *hh4r)
{
	int retCode = 0;
	if (hh4r->p.p > hh4r->p1.p)
		retCode = 1;
	if (hh4r->p.p == hh4r->p1.p && hh4r->p.half >= hh4r->p1.half)
		retCode = 1;
	return retCode;
}

int hh4ReaderGet4bit(Hh4Reader *hh4r)
{
	int value = 0;
	if (hh4ReaderEnd(hh4r) != 0) {
		hh4r->errorCode = 1;
		goto fin;
	}
	value = *hh4r->p.p;
	if (hh4r->p.half == 0) {
		value >>= 4;
		hh4r->p.half = 1;
	} else {
		value &= 0xf;
		hh4r->p.p++;
		hh4r->p.half = 0;
	}
fin:
	printf("%x", value);
	return value;
}

Int32 hh4ReaderGetUnsigned(Hh4Reader *hh4r)
{
	int i = hh4ReaderGet4bit(hh4r), len = 3;
	if (i <= 0x6)
		;	// 0xxx型
	else if (i == 0x7) {
		len = hh4ReaderGetUnsigned(hh4r) * 4;
		if (len > 32) {
			hh4r->errorCode = 1;
			len = 0;
		}
		int j;
		i = 0;
		for (j = len; j > 0; j -= 4) {
			i <<= 4;
			i |= hh4ReaderGet4bit(hh4r);
		}
	} else if (i <= 0xb) {	// 10xx_xxxx型
		i = i << 4 | hh4ReaderGet4bit(hh4r);
		len = 6;
		i &= 0x3f;
	} else if (i <= 0xd) {	// 110x_xxxx_xxxx型
		i = i << 8 | hh4ReaderGet4bit(hh4r) << 4;
		i |= hh4ReaderGet4bit(hh4r);
		len = 9;
		i &= 0x1ff;
	} else if (i == 0xe) {	// 1110_xxxx_xxxx_xxxx型
		i = hh4ReaderGet4bit(hh4r) << 8;
		i |= hh4ReaderGet4bit(hh4r) << 4;
		i |= hh4ReaderGet4bit(hh4r);
		len = 12;
	} else { // 0x0fは読み飛ばす.
		i = hh4ReaderGetUnsigned(hh4r);
		len = hh4r->length;
	}
	hh4r->length = len;
	return i;
}

Int32 hh4ReaderGetSigned(Hh4Reader *hh4r)
{
	Int32 i = hh4ReaderGetUnsigned(hh4r);
	int len = hh4r->length;
	if (0 < len && len <= 31 && i >= (1 << (len - 1)))
		i -= 1 << len; // MSBが1なら引き算して負数にする.
	return i;
}

Int32 hh4ReaderGet4Nbit(Hh4Reader *hh4r, int n)
{
	int i;
	Int32 value = 0;
	for (i = 0; i < n; i++)
		value = value << 4 | hh4ReaderGet4bit(hh4r);
	return value;
}

int dumpfr_getUnsigned(Hh4Reader *hh4r)
{
	return hh4ReaderGetUnsigned(hh4r);
}

int dumpfr_getSigned(Hh4Reader *hh4r)
{
	int i = hh4ReaderGetSigned(hh4r);
	if (hh4r->length == 3) {	// 0,1,2,3,4,5,-1.
		i &= 7;
		if (i == 6)
			i = -1;
	}
	return i;
}

void dumpfr_skipHh4(Hh4Reader *hh4r, int i)
{
	// 手抜きなのでビット指定コードがあった場合のことを考えていない.
	int j;
	for (j = 0; j < i; j++) {
		printf("_");
		dumpfr_getUnsigned(hh4r);
	}
	return;
}

int dumpfr_getReg(Hh4Reader *hh4r, int *rep, char mode)
{
	int i = dumpfr_getUnsigned(hh4r);
	if (hh4r->length == 3 && mode == 0) {
		if (i >= 5)
			i += 0x18 - 5;
	}
	if (0x18 <= i && i <= 0x1f)
		i = rep[i & 7];
	else if (0x40 <= i && i <= 0x47)
		i += 0x18 - 0x40;
	return i;
}

void dumpfr_updateRep(int *rep, int r)
{
	int tmp0 = r, tmp1, i;
	for (i = 0; i < 8; i++) {
		tmp1 = rep[i];
		rep[i] = tmp0;
		if (tmp1 == r) break;
		tmp0 = tmp1;
	}
	return;
}

int dumpfr(struct Work *w)
{
	Hh4Reader hh4r;
	char flag4 = 0, flagD = 0, flag14 = 0;
	int op, *hist = malloc(256 * sizeof (int));
	int r3f = 0, lastLabel = -1, i, j, k = 0, repP[8];
	char *pregTypeFlag = malloc(64);
	UCHAR *opTbl = malloc(256);
	if (w->ibuf[0] != 0x05 || w->ibuf[1] != 0xe2 || w->ibuf[2] < 0x10) {
		fputs("input file format error.\n", stderr);
		exit(1);
	}
	hh4ReaderInit(&hh4r, w->ibuf + 2, 0, w->ibuf + w->isiz, 0);
	for (op = 0; op < 256; op++) {
		hist[op] = 0;
		opTbl[op] = op;
	}
	opTbl[0x00] = 0x14;
	opTbl[0x14] = 0x00;
	for (i = 0; i < 64; i++)
		pregTypeFlag[i] = 0; /* 推測不. */
	for (i = 1; i <= 4; i++)
		pregTypeFlag[i] = 1; // 推測可能.
	for (i = 0; i < 8; i++)
		repP[i] = 0x30 + i;
	printf("0000: 05e2");
	for (;;) {
		if (hh4ReaderEnd(&hh4r) != 0) break;
		if (flag4 == 0 && flagD == 0)
			printf("\n%04x: ", (hh4r.p.p - w->ibuf) * 2 + hh4r.p.half);
		op = dumpfr_getUnsigned(&hh4r);
		if (hh4ReaderEnd(&hh4r) != 0 && op == 0) {
			// 末尾の0.5バイトを埋めていた0を発見.
			continue;
		}
		if (op < 256)
			hist[op]++;
		op = opTbl[op];
		if (op == 0x1) {
			if (flag4 == 0 && flagD == 0)
				continue;
		}
		if (op == 0x4) {
			if (flag4 == 0) {
				flag4 = 1;
				printf("_");
				continue;
			}
		}
		if (op == 0x2) {
			dumpfr_skipHh4(&hh4r, 2);
			continue;
		}
		if (op == 0x3) {
			if (flag4 == 0) {
				dumpfr_skipHh4(&hh4r, 1);
				continue;
			}
			dumpfr_skipHh4(&hh4r, 1);
			printf("_");
			i = dumpfr_getReg(&hh4r, repP, 1);
			pregTypeFlag[i] = 1;
			dumpfr_updateRep(repP, i);
			flag4 = 0;
			continue;
		}
		if (op == 0x5) {
			printf("_");
			i = dumpfr_getSigned(&hh4r);
			if (i == 0x0002) {
				j = 4;
				if (flag4 != 0) j = 1;
				dumpfr_skipHh4(&hh4r, j);
				flag4 = 0;
				continue;
			}
			if (i == 0x0003) {
				j = 6;
				if (flag4 != 0) j = 1;
				dumpfr_skipHh4(&hh4r, j);
				flag4 = 0;
				continue;
			}
			if (i == 0x0004) {
				j = 6;
				if (flag4 != 0) j = 1;
				dumpfr_skipHh4(&hh4r, j);
				flag4 = 0;
				continue;
			}
			if (i == 0x0005) {
				j = 6;
				if (flag4 != 0) j = 1;
				dumpfr_skipHh4(&hh4r, j);
				flag4 = 0;
				continue;
			}
			if (i == 0x0009) {	// api_sleep
				dumpfr_skipHh4(&hh4r, 2);
				continue;
			}
			if (i == 0x000d) {	// api_inkey
				dumpfr_skipHh4(&hh4r, 2);
				continue;
			}
			if (i == 0x0010) {
				j = 2;
				if (flag4 != 0) j = 4;
				dumpfr_skipHh4(&hh4r, j);
				flag4 = 0;
				continue;
			}
			printf("\ndumpfr: unknown function number : R30=0x%04x\n", i);
			exit(1);
		}
		if (op == 0x6) {
			j = 2;
			if (flag4 != 0) j++;
			if (flagD != 0) j++;
			dumpfr_skipHh4(&hh4r, j);
			flag4 = flagD = 0;
			continue;
		}
		if (op == 0x07) {
			if (flag4 == 0 && flagD == 0)
				continue;
		}
		if (op == 0x08) {
			printf("_");
			i = dumpfr_getReg(&hh4r, repP, 0);
			if (pregTypeFlag[i] == 0)
				flagD = 1;
			pregTypeFlag[i] = 1;
			if (flagD != 0)
				dumpfr_skipHh4(&hh4r, 1);
			printf("_");
			j = dumpfr_getUnsigned(&hh4r);
			dumpfr_skipHh4(&hh4r, j + 1);
			dumpfr_updateRep(repP, i);
			flag4 = flagD = 0;
			continue;
		}
		if (op == 0x09) {
			dumpfr_skipHh4(&hh4r, 1);
			printf("_");
			i = dumpfr_getReg(&hh4r, repP, 0);
			if (pregTypeFlag[i] == 0)
				flagD = 1;
			pregTypeFlag[i] = 1;
			if (flagD != 0)
				dumpfr_skipHh4(&hh4r, 1);
			printf("_");
			j = dumpfr_getUnsigned(&hh4r);
			dumpfr_skipHh4(&hh4r, j);
			dumpfr_updateRep(repP, i);
			flag4 = flagD = 0;
			continue;
		}
		if (op == 0x0d) {
			if (flagD == 0) {
				flagD = 1;
				printf("_");
				continue;
			}
		}
		if (op == 0x0f) {
			dumpfr_skipHh4(&hh4r, 4);
			continue;
		}
		if (0x10 <= op && op <= 0x1b /* && op != 0x17 */) {
			j = 2;
			if (flag4 != 0) j++;
			dumpfr_skipHh4(&hh4r, j);
			flag4 = flagD = 0;
			continue;
		}
		if (0x20 <= op && op <= 0x27) {
			if (flagD == 0) {
				j = 2;
				if (flag4 != 0) j++;
				dumpfr_skipHh4(&hh4r, j);
				flag4 = 0;
				continue;
			}
			dumpfr_skipHh4(&hh4r, 3);
			flagD = 0;
			continue;
		}
		if (op == 0x2e) {
			printf("_");
			i = dumpfr_getUnsigned(&hh4r);
			printf("_");
			j = dumpfr_getUnsigned(&hh4r);
			printf("_");
			j = db2binDataWidth(i) * j / 4;
			for (i = 0; i < j; i++)
				hh4ReaderGet4bit(&hh4r);
			continue;
		}
		if (op == 0x32) {
			printf("_");
			i = dumpfr_getUnsigned(&hh4r);
			printf("_");
			j = dumpfr_getUnsigned(&hh4r);
			printf("_");
			k = dumpfr_getReg(&hh4r, repP, 1);
			if (flagD == 0) {
				if (flag4 != 0) {
					dumpfr_skipHh4(&hh4r, 1);
					flag4 = 0;
				}
				dumpfr_updateRep(repP, k);
				pregTypeFlag[k] = 1;
				continue;
			}
		}
		if (op == 0x34) {
			if (flag4 == 0 && flagD == 0)
				continue;
		}
		if (op == 0x35) {
			if (flag4 == 0 && flagD == 0) {
				dumpfr_skipHh4(&hh4r, 1);
				continue;
			}
		}
		if (op == 0x36) {
			if (flag4 == 0 && flagD == 0)
				continue;
		}
		if (op == 0x37) {
			if (flag4 == 0 && flagD == 0) {
				dumpfr_skipHh4(&hh4r, 1);
				continue;
			}
		}
		if (op == 0x3c) {
			if (flag4 == 0 && flagD == 0)
				continue;
		}
		if (op == 0x3d) {
			if (flag4 == 0 && flagD == 0)
				continue;
		}
		if (op == 0x3e) {
			dumpfr_skipHh4(&hh4r, 1);
			continue;
		}
		if (op == 0xfd) {
			printf("_");
			i = dumpfr_getUnsigned(&hh4r);
			printf("_");
			j = dumpfr_getUnsigned(&hh4r);
			if (j == 0)
				printf(" (%d)", i);
			continue;
		}
		printf("\ndumpfr: unknown opecode: %02x\n", op);
		exit(1);
	}
	printf("\n\n");
	for (i = 0; i < 0x40; i++) {
		printf("%02X:%05d ", i, hist[i]);
		if ((i & 0x07) == 0x07) putchar('\n');
	}

	return 0;
}



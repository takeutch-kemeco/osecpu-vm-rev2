#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char UCHAR;

#if (!defined(REVISION))
	#define REVISION	2
#endif

#define	BUFSIZE		2 * 1024 * 1024
#define	JBUFSIZE	64 * 1024

#define MAXLABEL	4096
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
int fcode(struct Work *w);

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
		if (strcmp(cs, "fcode")  == 0) { r = fcode(w);  }
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
			 "  aska   ver.0.20\n"//108
			 "  prepro ver.0.01\n"
			 "  lbstk  ver.0.03\n"
			 "  db2bin ver.0.17\n"//113
			 "  disasm ver.0.02\n"
			 "  appack ver.0.20\n"//110
			 "  maklib ver.0.01\n"
			 "  getint ver.0.06\n"
			 "  osastr ver.0.00\n"
			 "  binstr ver.0.00\n"
			 "  fcode  ver.0.00"
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
#define ELEMENT_GOTO			102
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
			}
			static struct {
				UCHAR typ, s[10];
			} table_k[] = {
				{ ELEMENT_FOR,    "3for"      },
				{ ELEMENT_IF,     "2if"       },
				{ ELEMENT_ELSE,   "4else"     },
				{ ELEMENT_BREAK,  "5break"    },
				{ ELEMENT_CONTIN, "8continue" },
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
		if (p->typ == ELEMENT_CONST || p->typ == ELEMENT_REG || p->typ == ELEMENT_PREG) {
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
		ele0[0].typ == ELEMENT_BREAK || ele0[0].typ == ELEMENT_CONTIN || ele0[0].typ == ELEMENT_GOTO) goto fin;
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
						sprintf(q, "MULI(R%02X,R%02X,-1);", j, ele0[sp - 1].iValue);
						q += 17;
						ele0[sp - 1].subTyp[2] = 1;
						ele0[sp - 1].iValue = j;
						continue;
					}
					if (ele1[i].typ == ELEMENT_NOT || ele1[i].typ == ELEMENT_TILDE) {
						askaPass1FreeStk(&ele0[sp - 1], tmpR, tmpP);
						j = askaAllocTemp(tmpR, 16);
						if (j < 0) goto exp_giveup;
						sprintf(q, "XORI(R%02X,R%02X,-1);", j, ele0[sp - 1].iValue);
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
				if ((brase[braseDepth].flg0 & 2) != 0) { // using remark
					q = strcpy1(q, "DB(0xfc,0xfe,0x30);");
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
						q = strcpy1(q, "JMP(CONTINUE);");
					}
				} else if (ele0[0].typ == ELEMENT_REG) {
					sprintf(q, "CND(R%02X);JMP(CONTINUE);", ele0[0].iValue);
					q += 23;
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
				if (strncmp1(p, "continue") == 0) { j = 1; p += 8; }
				if (strncmp1(p, "break")    == 0) { j = 2; p += 5; }
				if (strncmp1(p, "goto")     == 0) { j = 3; p += 4; }
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
						qq[j + 46] = qq[j];
					char tmpStr[52];
					sprintf(tmpStr, "DB(0xfc,0xfe,0x21,0xf7,0x88);DDBE(0x%08X);", k);
					// printf("strlen=%d\n", strlen(tmpStr));
					memcpy(qq, tmpStr, 46);
					q += 46;
				}
				isWaitBrase0 = 1;
				braseDepth++;
				q = strcpy1(q, "LOOP();");
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
		p = strstr(p, "lbstk6(");
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
	if ((w->flags & 1) == 0 && q != NULL)
		q = db2binSub2(w->obuf + DB2BIN_HEADER_LENGTH, q);
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
			"1p(",					"DB(%0-0x40+0x80);", // 1byte.

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

		//	"1DBGINFO0(",			"DB(0xfe,0x05,0x00); DDBE(%0);",
			"0DBGINFO1(",			"DB(0xfc,0xfe,0x00);",
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
	if (src[0] == 0xbc && src[1] == 0xf7 && src[2] == 0x88) i = 37; // ENTER
	if (src[0] == 0xbd && src[1] == 0xf7 && src[2] == 0x88) i = 37; // LEAVE
	if (src[0] == 0xfc && src[1] == 0xfd && src[2] == 0xf7 && src[3] == 0x88 && src[8] == 0xf0) i = 9; // LIDR
	if (src[0] == 0xfc && src[1] == 0xfe && src[2] == 0x00) i = 3; // remark-0
	if (src[0] == 0xfc && src[1] == 0xfe && src[2] == 0x10) i = 3; // remark-1
	if (src[0] == 0xfc && src[1] == 0xfe && src[2] == 0x21 && src[3] == 0xf7 && src[4] == 0x88) i = 9; // remark-2
	if (src[0] == 0xfc && src[1] == 0xfe && src[2] == 0x30) i = 3; // remark-3
	if (src[0] == 0xfc && src[1] == 0xfe && src[2] == 0x50) i = 3; // remark-5
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
	int t[MAXLABEL], i, j;
	UCHAR *p, *q, *r;
	for (j = 0; j < MAXLABEL; j++)
		t[j] = -1;

	for (p = p0; p < p1; ) {
		if (*p == 0xf1 || *p == 0xf3) {
			j = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			if (j >= MAXLABEL || j < 0) {
				fputs("label number over error. check LMEM/SMEM/PLMEM/PSMEM.\n", stderr);
				exit(1);
			}
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

void appackSub0(UCHAR **qq, char *pof, int i)
// 4bit出力.
{
	UCHAR *q = *qq;
	if (*pof == 0) {
		*q &= ~0xf0;
		*q++ |= (i << 4) & 0xf0;
		*qq = q;
	} else {
		q[-1] &= ~0x0f;
		q[-1] |= i & 0x0f;
	}
	*pof ^= 1;
	return;
}

void appackSub1(UCHAR **qq, char *pof, int i, char uf)
{
	if (uf != 0) {
		if (i <= 0x6)
			appackSub0(qq, pof, i);
		if (0x7 <= i && i <= 0x3f) {
			appackSub0(qq, pof, i >> 4 | 0x8);
			appackSub0(qq, pof, i);
		}
		if (0x40 <= i && i <= 0x1ff) {
			appackSub0(qq, pof, i >> 8 | 0xc);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x200 <= i && i <= 0xfff) {
			appackSub0(qq, pof, 0xe);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x1000 <= i && i <= 0xffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x4);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x10000 <= i && i <= 0xfffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x5);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x100000 <= i && i <= 0xffffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x6);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x1000000 <= i && i <= 0xfffffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, i >> 24);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
		if (0x10000000 <= i) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, i >> 28);
			appackSub0(qq, pof, i >> 24);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
	} else {
#if (REVISION == 1)
		if (-0x3 <= i && i <= 0x3) {
			// -3, -2, -1, 0, 1, 2, 3
#elif (REVISION == 2)
		if (-0x1 <= i && i <= 0x5) {
			// 4, 5, -1, 0, 1, 2, 3
#endif
			if (i < 0) i--;
			i &= 0x7;
			appackSub0(qq, pof, i);
		} else if (-0x20 <= i && i <= 0x1f) {
			i &= 0x3f;
			appackSub0(qq, pof, i >> 4 | 0x8);
			appackSub0(qq, pof, i);
		} else if (-0x100 <= i && i <= 0xff) {
			i &= 0x1ff;
			appackSub0(qq, pof, i >> 8 | 0xc);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else if (-0x800 <= i && i <= 0x7ff) {
			appackSub0(qq, pof, 0xe);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else if (-0x8000 <= i && i <= 0x7fff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x4);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else if (-0x80000 <= i && i <= 0x7ffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x5);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else if (-0x800000 <= i && i <= 0x7fffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x6);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else if (-0x8000000 <= i && i <= 0x7ffffff) {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, i >> 24);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		} else {
			appackSub0(qq, pof, 0x7);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, 0x8);
			appackSub0(qq, pof, i >> 28);
			appackSub0(qq, pof, i >> 24);
			appackSub0(qq, pof, i >> 20);
			appackSub0(qq, pof, i >> 16);
			appackSub0(qq, pof, i >> 12);
			appackSub0(qq, pof, i >> 8);
			appackSub0(qq, pof, i >> 4);
			appackSub0(qq, pof, i);
		}
	}
	return;
}

int appackEncodeConst(int i)
{
	if (i <= -0x11) i -= 0x50; /* -0x61以下へ */
	if (0x100 <= i && i <= 0x10000 && (i & (i - 1)) == 0) {
		i = - askaLog2(i) - 0x48; /* -0x50から-0x58へ (-0x59から-0x5fはリザーブ) */
	}
	return i;
}

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

#define TYP_CODE		0
#define TYP_UNKNOWN		-1

void appackSub1op(UCHAR **qq, char *pof, int i, int *hist, UCHAR *opTbl)
{
	if (i < 256)
		i = opTbl[i];
	appackSub1(qq, pof, i, 1);
	if (i < 0x40)
		hist[i]++;
	return;
}

typedef struct _AppackWork {
	const unsigned char *p;
	unsigned char *q;
	char pHalf;
	int rep0, rep1, rep0p, rep1p, rep0f, rep1f;
	int pxxTyp[64], labelTyp[MAXLABEL];
	int hist[0x40];
	unsigned char opTbl[256];
} AppackWork;

void appack_setRep(AppackWork *aw, int reg, int typ)
{
	if (typ == 0) {
		aw->rep1 = aw->rep0;
		aw->rep0 = reg;
	}
	if (typ == 1) {
		aw->rep1p = aw->rep0p;
		aw->rep0p = reg;
	}
	if (typ == 2) {
		aw->rep1f = aw->rep0f;
		aw->rep0f = reg;
	}
	return;
}

void appack_initWork(AppackWork *aw)
{
	int i;
	for (i = 0; i < 0x40; i++)
		aw->pxxTyp[i] = TYP_UNKNOWN;
	for (i = 0; i < MAXLABEL; i++)
		aw->labelTyp[i] = TYP_UNKNOWN;
	for (i = 0; i < 0x40; i++)
		aw->hist[i] = 0;
	for (i = 0; i < 256; i++)
		aw->opTbl[i] = i;
	return;
}

void appack_initLabelTyp(struct Work *w, AppackWork *aw)
{
	int i;
	const unsigned char *p, *pp;
	for (p = w->ibuf + DB2BIN_HEADER_LENGTH; p < w->ibuf + w->isiz; ) {
		if (*p == 0xf1) {
			i = p[3] << 24 | p[4] << 16 | p[5] << 8 | p[6];
			aw->labelTyp[i] = TYP_CODE;
			pp = p;
			for (;;) {
				if (pp >= w->ibuf + w->isiz) break;
				if (*pp != 0xf1) break;
				pp += db2binSub2Len(pp);
			}
			if (pp < w->ibuf + w->isiz && *pp == 0xae) {
				aw->labelTyp[i] = pp[3] << 24 | pp[4] << 16 | pp[5] << 8 | pp[6];
				if (aw->pxxTyp[4] == TYP_UNKNOWN) {
					if (aw->pxxTyp[1] == TYP_UNKNOWN)
						aw->pxxTyp[1] = aw->labelTyp[i];
					else if (aw->pxxTyp[2] == TYP_UNKNOWN)
						aw->pxxTyp[2] = aw->labelTyp[i];
					else if (aw->pxxTyp[3] == TYP_UNKNOWN)
						aw->pxxTyp[3] = aw->labelTyp[i];
					else /* if (pxxTyp[4] == TYP_UNKNOWN) */
						aw->pxxTyp[4] = aw->labelTyp[i];
				}
			}
		}
		p += db2binSub2Len(p);
	}
	return;
}

int appack0(struct Work *w)
{
	const UCHAR *p = w->ibuf, *pp;
	UCHAR *q = w->obuf, *qq;
	*q++ = *p++;
	*q++ = *p++;
	char of = 0, f, oof, ff, fe0103 = 0;
	int r3f = 0, lastLabel = -1, i, j, wait7 = 0, firstDataLabel = -1;
	AppackWork aw;
	qq = db2binSub2(w->ibuf + DB2BIN_HEADER_LENGTH, w->ibuf + w->isiz); // ラベル番号の付け直し.
	w->isiz = qq - w->ibuf;
	appack_initWork(&aw);
	appack_initLabelTyp(w, &aw);

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
		w->obuf[2] = 0xf0;
		q = &w->obuf[3];
		char of = 0;
		pp = p;
		for (;;) {
			if (*p == 0x15)      { appackSub1(&q, &of, 1, 1); p++; }
			else if (*p == 0x19) { appackSub1(&q, &of, 2, 1); p++; }
			else if (*p == 0x21) { appackSub1(&q, &of, 3, 1); p++; }
			else                 { appackSub1(&q, &of, 0, 1);      }
			appackSub1(&q, &of, i >> 8, 1);
			if (of == 0) break;
			q = &w->obuf[3];
			of = 0;
			appackSub0(&q, &of, 0x08);
			p = pp;
		}
		*q++ = i & 0xff;
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
	if (f == 0) r = appack0(w);
	if (f == 1) r = appack1(w);
	if (f == 2) r = appack2(w, 0);
	if (f == 3) r = appack2(w, 1);
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



/*
	(C) 1999-2005  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/

#ifndef karelH
#define karelH

#ifdef __BORLANDC__
#pragma option -K
#endif

#include "lang.h"

enum {
	clnull,
	cliden, clkey, clsym, clnum, clkom,
clbrkp, clrun, clsel, clscroll, clscrollbk,
clstatus, clresult, clchyb, clkarel, clznacka,
clznak, clzed, clstatusFindProc, clNinstr, clspeed,
clprik, clprik1, clstatusRun, clwatch, clFieldHelp,
Ncl
};

enum { MODE_TEXT, MODE_FIELD, MODE_FINDPROC, MODE_QUIT };

#define Dkeyw 41 //number of keywords
#define Nlang 2

//config
#define persistblok cfg[0]
#define overwrblok cfg[1]
#define indent cfg[2]
#define unindent cfg[3]
#define foncur cfg[4]
#define entern cfg[5]
#define upcasekeys cfg[6]
#define tabs cfg[7]
#define groupUndo cfg[8]
#define undoSave cfg[9]
#define langKey cfg[10] //obsolete
extern int cfg[11];

//max. 9 marks
#define maxznacek 265
//field size
#define PXMAX 42
#define PYMAX 22
//max fields
#define PLMAX 9
//field name length in menu
#define MMNAME 15

typedef unsigned char *uchar;

struct Tvar
{
	Tvar *nxt, *nxthash;
	char *jmeno;
	int offset;  //offset from BP
	int typ;
};

//procedure list item
struct Tslovo
{
	Tslovo() :nxt(0), pre(0), u(0xffff), Nparam(0), Nvar(0), var(0), varend(&var){};
	~Tslovo();
	Tslovo *nxt, *pre, *nxthash;
	char *jmeno; //name; pointer to source text !
	size_t u;    //offset to compiled code
	int r;       // >0 => line number,  <0 => keyword
	int result;  //result type
	int Nparam;  //number of parameters
	int Nvar;    //number of local variables
	Tvar *var, **varend; //local variables
};

class Tplocha
{
public:
	int xm, ym; //size without border + 1
	int karelx, karely, karelsmer;
	//backup
	int xm2, ym2;
	int karelx2, karely2, karelsmer2;
	//allocated memory
	unsigned *pole1, *pole2;
	size_t Dpole1, Dpole2;
	char jmeno[256]; //file name
	char mname[MMNAME]; //name in menu

	void aktiv();
	void deaktiv();
	void smaz();
	void zaloha();
	void pojmenuj();
};
//------------------------------------------------------------------------
#define sizeA(a) (sizeof(a)/sizeof(*a))
#define endA(a) (a+(sizeof(a)/sizeof(*a)))

template <class T> void dellist(T* &list)
{
	T *a, *a1;
	for(a=list; a;){
		a1=a->nxt;
		delete a;
		a=a1;
	}
	list=0;
}

#if !defined(__MINMAX_DEFINED) && !defined(min)
#define __MINMAX_DEFINED
template <class T> inline T min(T a, T b){ return a<b ? a:b; }
template <class T> inline T max(T a, T b){ return a>b ? a:b; }
#endif
template <class T> inline void amin(T &x, int m){ if(x<m) x=m; }
template <class T> inline void amax(T &x, int m){ if(x>m) x=m; }

template <class T> inline void aminmax(T &x, int l, int h){
	if(x<l) x=l;
	if(x>h) x=h;
}

//------------------------------------------------------------------------
class Ttext;
extern Ttext text;
extern bool delreg;

extern char *EXT, *EXTPL;
extern int fsmer, fod, fcase, fword;
extern int scrollDelay;

extern int X0, Y0, xmax, ymax;
extern int charWidth, charHeight, squareWidth, squareHeight;

extern int rprg;
extern bool tracing, lastTracing;
extern int klavesa;
extern size_t Dpreklad;
extern int Dprocedur;
extern unsigned char *preklad, **tab;
extern Tslovo *slovnik, *slovnikend;
extern unsigned long Cinstr, Ckroky;
extern int kax, kay, smer, plx, ply, plxm, plym;
extern Tplocha *plocha;
extern int pole[PXMAX][PYMAX];
extern int Nfields;
extern Tplocha plochy[PLMAX];
extern char keywordLang[256];

extern int mode;
extern int isBusy;
extern int jechyba, isWatch;
extern int statusColor;
extern bool ctrlp, ctrlk, ctrlq;

typedef char Tascii[256];
extern Tascii uppism, downpism, neoddel, pism;

extern char **keywords[];
extern Tslovo *keywS[];

int skipslovo(char *&u);
int backslovo(char *&u);
bool prikcmp(char *s1, char *s2);

void kompilace();
bool instrukce(bool beh);
bool mkslov(bool preloz);

bool jebeh();
void chyba(char *s, ...);
char *formatStr(const char *s, ...);
void smaz25();
void tiskCinstr();
void delay1(int ms);
void smazbreakpointy();
void delpreklad();
void prgreset();
void showprg();
void showRychlost();
void notrunr();
void plaktiv(int which);
void pltisk();
int findDlg();
int replaceDlg();
void updwkeys();
void najit();
void nahradit();
bool freemem();
bool freemem1();
void startup(char *args);
void repaintAll();
void beforeWait();
void beforeCmd();
void afterCmd();
void writeini();
void deleteini();
void wm_command(int cmd);
void wm_char(int c);
void wm_mouse(int mx, int my, int c);
void wm_timer(int id);
int onCloseQuery();
void lpsetcur(int y);
void runCmd(int c);
void zmenitadr();
void changekeys();
void changecolors();
void loadKeywords();
//------------------------------------------------------------------------
void tiskpole1(int c);
void tiskka();
void tiskpole(int i, int j);
void tiskzn();

void readKeyboard();
void waitKeyboard();
int keyPressed();
int isShift();
void sleep(int ms);
void setTimer(int id, int ms);
void killTimer(int id);

void textColor(int cl);
void putCh(char ch);
void putS(char *s);
void gotoXY(int x, int y);
void gotoNI();
void goto25();
void gotoSQ(int x, int y);
void clrEol();
void endOfStatus();
void showCursor(int ins);
void hideCursor();
void quit();
void cls();
void plResize();
void plInvalidate();
void setScroll(int top, int bottom, int pos, int page);
void showScrollBar(bool show);
void clipboardCopy(char *s, size_t n);
char *clipboardPaste(size_t &len, bool &del);

void statusMsg(int color, const char *s, ...);
void cprintF(const char *s, ...);
void printNinstr();
int oknoint(char *name, char *text, int &var);
void oknotxt(char *name, char *text, int w, int h);
void oknochyb(char *text);
int oknofile(char *name, char *ext, char *var, int action);
int oknoano(char *name, char *text);

void oprogramu();
void moznosti();
void plMenu();
void paintStatus();

bool lzezpet();
bool lzevpred();
bool jeblok();
bool jeffind();
bool jebreakp();

//------------------------------------------------------------------------
#endif


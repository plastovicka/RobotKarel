/*
	(C) 1999-2007  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include "hdr.h"
#pragma hdrstop

#include "karel.h"
#include "editor.h"
#include "compile.h"
/*
USEUNIT("Run.cpp");
USEUNIT("Editor.cpp");
USEUNIT("Compile.cpp");
USERC("Karel.rc");
USEUNIT("win.cpp");
//---------------------------------------------------------------------------
*/
#if 'Ø'<0
#error Option Unsigned characters must be on
#endif

char *EXT=".krl", *EXTPL=".kpl",
*SLOVNIK="karel.krl";

int cfg[]=
{0, 1, 1, 1, 1, 1, 0, 2, 1, 1, 0};

int rychlost=100;  //tracing delay (ms)
int maxstack0=4096;//stack size
int rprg;          //current executed line, it is highlighted in editor
int jechyba;       //is syntax or runtime error
int isWatch;
int isBusy;
int fast;

int xp, yp;
int Nfields=0;
int mode=MODE_TEXT;
bool prRun;
bool tracing, lastTracing;
bool delreg;        //settings were deleted
int scrpole[PXMAX][PYMAX]; //field which is visible
Tplocha plochy[PLMAX];
Tplocha *plocha=0;  //field object
static int plpoc=1; //new field number
Ttext text;         //source file object
Tbreakp breakp;     //breakpoint list sorted by lines
Tslovo *slovnik=0, *slovnikend; //procedure list
char *memmak=0, *memlng=0;
char keywordLang[256];

struct TintStr {
	int i;
	char *s;
}
plHelp[21]={
	{656, "Modify field"},
	{657, "-------------"},
	{658, "Insert  - build wall"},
	{659, "Delete  - delete square"},
	{660, "Alt+num - insert mark"},
	{661, "F4      - move Karel"},
	{0, ""},
	{662, "F7      - set width"},
	{663, "F8      - set height"},
	{0, ""},
	{664, "F5      - restore"},
	{665, "Shift+F5- generate randomly"},
	{666, "Ctrl+Del- delete all"},
	{0, ""},
	{667, "Left mouse button -"},
	{668, "  move cursor or insert mark"},
	{669, "Right mouse button -"},
	{670, "  build or destroy wall"},
	{0, ""},
	{671, "ENTER - exit and save field to the buffer"},
	{672, "ESC   - exit"}
};
//-------------------------------------------------------------------------
bool jebeh()
{
	return uprg!=0;
}

bool lzezpet()
{
	return text.lzezpet();
}

bool lzevpred()
{
	return text.lzevpred();
}

bool jeblok()
{
	return text.jeblok();
}

bool jeffind()
{
	return ffind[0]!='\0';
}

bool jebreakp()
{
	return breakp.nxt!=0;
}

void prgreset()
{
	if(jebeh()){
		uprg=0;
		delete[] stack;
		stack=0;
		notrunr();
	}
}

void delpreklad()
{
	if(preklad){
		prgreset();
		delete[] preklad;
		preklad=0;
		delete[] tab;
		tab=0;
	}
	dellist(slovnik);
}

Tslovo::~Tslovo()
{
	dellist(var);
}

bool freemem1()
{
	return text.freeUndo();
}

bool freemem()
{
	if(preklad){ delpreklad(); return false; }
	return freemem1();
}

int dbgrad(uchar u)
{
	int l, r, m;

	l=1; r=text.rpoc;
	while(l<r){
		m=(l+r+1)/2;
		if(u<tab[m]) r=m-1;
		else l=m;
	}
	return l;
}

int koncovka(long i)
{
	if(!_stricmp(lang, "Czech")){
		if(i>4 || i<=0) return 'ù';
		if(i>1) return 'y';
	}
	else if(!_stricmp(lang, "English")){
		if(i>1 || i<0) return 's';
	}
	return ' ';
}

void printNinstr()
{
	textColor(clNinstr);
	gotoNI();
	if(Cinstr && mode!=MODE_FIELD){
		cprintF(lng(650, "%9lu step%c%10lu instr. "),
			Ckroky, koncovka(Ckroky), Cinstr, koncovka(Cinstr));
	}
	else{
		putS("                                 ");
	}
	if(mode==MODE_FIELD){
		gotoSQ(0, plym+1);
		putS(plocha->mname);
	}
}

void tiskCinstr()
{
	int i, j;

	for(j=0; j<plym; j++)
	 for(i=0; i<plxm; i++){
		 if(pole[i][j]!=scrpole[i][j]){
			 scrpole[i][j]=pole[i][j];
			 tiskpole(i, j);
		 }
	 }
	tiskka();
	scrpole[kax][kay]=0x7fff;
	tiskzn();
	printNinstr();
}

void resetCinstr()
{
	Cinstr=0;
	Ckroky=0;
	tiskCinstr();
}

//initialize stack and registers
int init(Tslovo *p)
{
	Tvar *var;
	int i;
	static int parametr;
	char msg[32], *s1, *s2;

	if(maxstack0<100) maxstack0=100;
	if(maxstack0>10000) maxstack0=10000;
	maxstack= maxstack0;
	while(((stack=new Tstack[maxstack]))==0)
	 if(freemem1()){
		 jechyba=0;
		 chyba(lng(776, "Not enough memory for stack !"));
		 return 0;
	 }

	fast=0;
	uprg= preklad + p->u;
	sp=stack;
	if(p->result!=T_VOID){
		*sp++=0; //function result
	}
	for(i=0, var=p->var; i<p->Nparam; i++, var=var->nxt){
		if(var->typ!=T_INT){
			chyba(lng(777, "Cannot run function with non integer parameter"));
			prgreset();
			return 0;
		}
		strcpy(msg, lng(211, "Parameter "));
		for(s1=var->jmeno, s2=msg+10; neoddel[*s1] && s2-msg<sizeof(msg)-2; s1++, s2++){
			OemToCharBuff(s1, s2, 1);
		}
		*s2++=':';
		*s2=0;
		if(!oknoint(lng(26, "Run"), msg, parametr)){
			prgreset();
			return 0;
		}
		*sp++=parametr;
	}
	*sp++=0;
	*sp++=0; //return address
	bp=sp;
	resetCinstr();
	return 1;
}

//set rprg and print current line
void showprg()
{
	int i;
	Tslovo *f, *f1=0;
	Tvar *var;
	char *s;
	int b;
	char buf[256], *dest;
	const int x=33;

	tiskCinstr();
	if(jebeh()){
		rprg=dbgrad(uprg);
		text.gotor(rprg);
		text.lst();   //refresh window
		//print local variables
		for(f=slovnik; f && f->u<=unsigned(uprg-preklad); f=f->nxt)  f1=f;
		b=0;
		dest=buf;
		for(var=f1->var; var; var=var->nxt){
			if(var->typ==T_INT) i=bp[var->offset];
			else if(var->typ==T_REF) i=stack[bp[var->offset]];
			else continue;
			if((*uprg & 63)==I_ADDSP && b==f1->Nparam) break;
			if(i==UNDEF_VALUE) continue;
			if(b) *dest++=',';
			*dest++=' ';
			b++;
			for(s=var->jmeno; neoddel[*s]; s++){
				OemToCharBuff(s, dest, 1);
				dest++;
			}
			*dest++='=';
			_itoa(i, dest, 10);
			dest=strchr(dest, 0);
			if(dest-buf>80) break;
		}
		if(b && !jechyba){
			buf[80-x]=0;
			statusMsg(clwatch, buf);
			isWatch=1;
		}
	}
}

void smaz25()
{
	jechyba=0;
	statusMsg(clstatus, "");
}

//-----------------------------------------------------------------------
//  Run, Debug
//-----------------------------------------------------------------------
void prelozit()
{
	char t[128];

	kompilace();
	if(preklad){
		strcpy(t, lng(212, "Lines :        "));
		_itoa(text.rpoc, strchr(t, 0), 10);
		strcat(t, lng(213, "\nProcedures :   "));
		_itoa(Dprocedur, strchr(t, 0), 10);
		strcat(t, lng(214, "\n\nSource code :  "));
		_ltoa(text.delka, strchr(t, 0), 10);
		strcat(t, lng(215, "\nCompiled code: "));
		_itoa(Dpreklad, strchr(t, 0), 10);
#ifdef DOS
		strcat(t,lng(216,"\nFree memory:   "));
		ltoa(coreleft()/1024,strchr(t,0),10);
		strcat(t,"KB");
#endif
		oknotxt(lng(27, "Compile"), t, 26, 12);
	}
}

bool spust()
{
	Tslovo *p;

	if(jebeh()) return true;
	if(!preklad) kompilace();
	if(preklad){
		//find procedure where is cursor
		for(p=slovnikend; p && p->r > text.cur.r; p=p->pre);
		if(!p) p=slovnik;
		if(!init(p)) return false;
		rprg= dbgrad(uprg);
	}
	return false;
}

void spustit()
{
	if(spust() || uprg && *uprg<128)
		instrukce(true);
	showprg();
}

bool isKrokInstr()
{
	int c= *uprg & 63;
	return fast<=0 && (c==KROK || c==VLEVO_VBOK ||
		c==VPRAVO_VBOK || c==DOMU ||
		c==RYCHLE && fast==0);
}

void krokovat()
{
	if(spust()){
		while(instrukce(false) && !isKrokInstr());
	}
	showprg();
}

void presprocedury()
{
	uchar urad, unext;
	int h=0;
	int c;

	if(spust()){
		urad=uprg;
		unext=tab[rprg+1];
		do{
			c = *uprg & 63;
			if(c==I_CALL) h++;
			if(c==I_RET || c==I_RETP) h--;
		} while(instrukce(false) && (h>0 || (uprg>=urad && uprg<unext)));
	}
	showprg();
}

void jednainstrukce()
{
	if(spust()) instrukce(false);
	showprg();
}

void jedenradek()
{
	uchar urad, unext;

	if(spust()){
		urad=uprg;
		unext=tab[rprg+1];
		while(uprg>=urad && uprg<unext && instrukce(false));
	}
	showprg();
}

void setrychlost()
{
	oknoint(lng(28, "Speed of trace"),
		 lng(217, "Delay in ms:"), rychlost);
}

void setstacksize()
{
	oknoint(lng(29, "Stack size"),
		lng(218, "Stack :"), maxstack0);
}

void trasovat()
{
	int r=text.cur.r;

	tracing=true;
	showRychlost();
	if(spust() || uprg && *uprg<128){
		do{
			if(!instrukce(false)) break;
			if(rychlost>300 && fast<=0){
				showprg();
				text.repaint();
				delay1(rychlost-100);
			}
			else{
				notrunr();
				if(isKrokInstr()){
					tiskCinstr();
					delay1(isShift() ? rychlost/4 : rychlost);
				}
			}
		} while(!jechyba);
	}
	tracing=false;
	text.gotor(r);
	showprg();
}

void gotocursor()
{
	bool b;
	int r=text.cur.r;

	if((spust() || uprg) && tab[r]!=uprg && tab[r]<preklad+Dpreklad){
		b=*tab[r]<128;
		*tab[r]|=128;
		instrukce(true);
		if(b){ *tab[r]&=127; smaz25(); }
	}
	showprg();
}

void nakonec()
{
	int h=0;
	int c;

	if(spust() || uprg){
		do{
			c = *uprg & 63;
			if(c==I_CALL) h++;
			if(c==I_RET || c==I_RETP) h--;
		} while(instrukce(false) && h>=0);
	}
	showprg();
}

//u1 points to previous item
void delbreakp(Tbreakp *u1)
{
	Tbreakp *u=u1->nxt;
	if(u){
		u1->nxt=u->nxt;
		delete u;
	}
}

void smazbreakpointy()
{
	for(Tbreakp *u=&breakp; u->nxt;){
		if(preklad) *tab[u->nxt->r]&=127;
		delbreakp(u);
	}
	text.lst();
}

void breakpoint()
{
	int r=text.cur.r, r1=r;
	Tbreakp *u, *u1;

	for(u1=&breakp, u=u1->nxt;
			 u && (u->r<r  ||  u->r==r && u->text!=&text);
			 u1=u, u=u->nxt);
	if(preklad){
		//don't put breakpoint after end of file
		if(tab[r]==preklad+Dpreklad) return;
		//skip empty lines
		while(tab[r]==tab[r+1]) ++r;
		if(r1<r){
			text.gotor(r);
			if(u && u->r==r) return;
		}
		//set or remove breakpoint in compiled code
		*tab[r]^=128;
	}

	if(u && u->r==r){
		//remove breakpoint
		delbreakp(u1);
	}
	else if((u=new Tbreakp)!=0){
		//create breakpoint
		u->r=r;
		u->text=&text;
		u->nxt=u1->nxt;
		u1->nxt=u;
	}
	text.lst();
}

//-----------------------------------------------------------------------
//  Find, Edit
//-----------------------------------------------------------------------
void najitoncur()
{
	char *u2=text.cur.u, *u1=u2;
	size_t d;

	if(foncur){
		skipslovo(u2);
		backslovo(u1);
		d=(size_t)(u2-u1);
		if(d>=sizeof(ffind)) d=sizeof(ffind)-1;
		if(d){
			memcpy(ffind, u1, d);
			ffind[d]=0;
		}
	}
}

void najit()
{
	najitoncur();
	if(findDlg()) text.find(0);
}

void nahradit()
{
	int i;

	najitoncur();
	i=replaceDlg();
	if(i) text.find(i);
}

void updwkeys()
{
	int i, l;
	char *s;

	for(l=0; keywords[l]; l++)
	 for(i=PROCEDURE; i<Dkeyw; i++)
		 for(s=keywords[l][i]; *s; s++)
			 *s= upcasekeys ? uppism[*s] : downpism[*s];
}

//-----------------------------------------------------------------------
//  File, Help
//-----------------------------------------------------------------------
int onCloseQuery()
{
	if(!text.zavri()) return 0;
	if(!delreg) writeini();
	jechyba=1;
	mode=MODE_QUIT;
	return 1;
}

void konec()
{
	if(onCloseQuery()) quit();
}

void helpeditor() //only for DOS, because Windows version has configurable keys
{
	char *s = formatStr(lng(519,
"Ctrl+Y, Ctrl+Q+Y       - delete line or end of line\n"
"Ctrl+T, Ctrl+Backspace - delete word forward or backward\n"
"Ctrl+Ins, Shift+Ins, Shift+Del - copy, paste, cut\n"
"\n"
"Ctrl+PageUp, Ctrl+PageDown - go to begin or end of page\n"
"Ctrl+Home,   Ctrl+End      - go to begin or end of text\n"
"Ctrl+left,   Ctrl+right    - go to previous or next word\n"
"Ctrl+up,     Ctrl+down     - scroll screen\n"
"Ctrl+Enter  - go to beginning of next line\n"
"\n"
"Shift+Enter - insert keyword %\n"
"Ctrl+N      - insert new line\n"
"Ctrl+P      - insert control character\n"
"Ctrl+K+0-9  - set bookmark\n"
"Ctrl+Q+0-9  - go to bookmark\n"
"Ctrl+K+I    - indent selected block\n"
"Ctrl+K+U    - unindent selected block\n"
"Ctrl+K+A    - change upper/lower case in selected block"
), KONEC);
	oknotxt(lng(30, "Help"), s, 63, 24);
	delete[] s;
}

void helpprik2()
{
	char *s = formatStr(lng(520,
"% | % | % name (parameter1, &parameter2)\n"
"\n"
" %  variable, variable=value, array[], array[size]\n"
"\n"
" % condition\n"
"   stataments1;  %;  stataments2\n"
" %\n"
"\n"
" % condition  |  %  |  % count\n"
"   stataments\n"
" %  |  % condition\n"
"\n"
" % expr\n"
"  % expr1,expr2;  stataments1\n"
"  % expr3;  stataments2\n"
"  %;  stataments\n"
" %\n"
"\n"
"%\n"
), PROCEDURE, PODMINKA, FUNKCE, CISLO, KDYZ, JINAK, KONEC,
DOKUD, OPAKUJ, OPAKUJ, KONEC, AZ, ZVOLIT, PRIPAD, PRIPAD, JINAK, KONEC, KONEC);

	oknotxt(lng(30, "Help"), s, 68, 24);
	delete[] s;
}

void helpprik1()
{
	char *s = formatStr(lng(521,
 "  Basic commands:\n"
 "%; %; %; %; %; %\n"
 "% character\n"
 "% milisec; %; %; %; %\n"
 "\n"
 "  Operators:\n"
 "%, %,  +, -,  *, /, %%,  %,  %\n"
 "\n"
 "  Conditions:\n"
 "%, %, %, %, %, %, %\n"
 "\n"
 "  Functions:\n"
 "%, %, %(n), %(array)"
 ), KROK, VLEVO_VBOK, VPRAVO_VBOK, ZVEDNI, POLOZ, DOMU,
	 MALUJ, CEKEJ, STOP, NAVRAT, RYCHLE, POMALU,
	 JE, NENI, A, NEBO, ZED, ZNACKA, MISTO, SEVER, JIH, VYCHOD, ZAPAD,
	 ZNAK, KLAVESA, NAHODA, DELKA);
	oknotxt(lng(30, "Help"), s, 50, 19);
	delete[] s;
}

//-----------------------------------------------------------------------
//  Field
//-----------------------------------------------------------------------
static unsigned *Pplbuf, plbuf[(PXMAX-2)*(PYMAX-2)*10/sizeof(unsigned)/8+2];
static size_t Dplbuf;
static int plbufbit;

void plPrintHelp()
{
	int i;

	textColor(clFieldHelp);
	for(i=0; i<sizeA(plHelp); i++){
		gotoXY(1, i+1);
		putS(lng(plHelp[i].i, plHelp[i].s));
		clrEol();
	}
	while(i<ymax){
		gotoXY(1, i+1);
		clrEol();
		i++;
	}
}

void pltisk()
{
	int i, j;

	printNinstr();
	for(i=0; i<=plxm; i++)
		for(j=0; j<=plym; j++){
			if(i==kax && j==kay){
				tiskka();
				scrpole[kax][kay]=0x7fff;
				tiskzn();
			}
			else{
				tiskpole(i, j);
			}
		}
}

void plokraj()
{
	int i;

	for(i=0; i<=plxm; i++) pole[i][plym]=pole[i][0]=-1;
	for(i=1; i<plym; i++) pole[plxm][i]=pole[0][i]=-1;
}

void plvymazat()
{
	int i, j;

	for(i=1; i<plxm; i++)
	 for(j=1; j<plym; j++)
		 pole[i][j]=0;
	plInvalidate();
}

void plput(int b, unsigned h)
{
	int bm=sizeof(unsigned)*8-plbufbit;

	*Pplbuf|=h<<plbufbit;
	plbufbit+=b;
	if(b>=bm){
		plbufbit=b-bm;
		Pplbuf++;
		*Pplbuf=h>>bm;
	}
}

int plget(int b)
{
	int bm=sizeof(unsigned)*8-plbufbit;
	unsigned x;

	x=*Pplbuf>>plbufbit;
	plbufbit+=b;
	if(b>=bm){
		plbufbit=b-bm;
		Pplbuf++;
		x|= *Pplbuf<<bm;
	}
	return x & ((1u<<b)-1);
}

//pole -> plbuf
void plkompres2()
{
	int i, j, z;

	Pplbuf=plbuf; plbufbit=0; *plbuf=0;
	for(i=1; i<plxm; i++)
	 for(j=1; j<plym; j++){
		 z=pole[i][j];
		 if(!z) plput(2, 0); //empty square
		 else if(z<0) plput(2, 1); //WALL
		 else if(z<256){ plput(2, 2); plput(8, z); } //CHAR
		 else{ //MARKS
			 plput(2, 3);
			 if(z<264) plput(3, z-257);
			 else { plput(3, 7); plput(1, z>264); }
		 }
	 }
	Dplbuf=(size_t)(Pplbuf-plbuf+1);
}

//plbuf->pole (dimension must match)
void pldekompres2()
{
	int i, j, z;

	Pplbuf=plbuf; plbufbit=0;
	for(i=1; i<plxm; i++)
	 for(j=1; j<plym; j++){
		 switch(plget(2)){
			 default://empty square
				 z=0;
				 break;
			 case 1://WALL
				 z=-1;
				 break;
			 case 2://CHAR
				 z=plget(8);
				 break;
			 case 3://MARKS
				 z=257+plget(3);
				 if(z==264) z+=plget(1);
				 break;
		 }
		 pole[i][j]=z;
	 }
	plokraj();
}

void plkompres(unsigned *&p, size_t &delka)
{
	plkompres2();
	delete[] p;
	while((p=new unsigned[Dplbuf])==0){
		if(freemem()){ delka=0; return; }
	}
	memcpy(p, plbuf, (delka=Dplbuf)*sizeof(unsigned));
}

void pldekompres(void *p, size_t n)
{
	if(p){
		memcpy(plbuf, p, n*sizeof(unsigned));
		pldekompres2();
	}
	else{
		plvymazat();
		plokraj();
	}
}

void plaktiv(int which)
{
	plocha->deaktiv();
	plocha=plochy+which;
	plocha->aktiv();
	plMenu();
}

void plobnovit()
{
	kax=plocha->karelx2, kay=plocha->karely2, smer=plocha->karelsmer2;
	plxm=plocha->xm2, plym=plocha->ym2;
	pldekompres(plocha->pole2, plocha->Dpole2);
	plResize();
	plInvalidate();
}

Tplocha *plpridej()
{
	Tplocha *p;

	if(Nfields==PLMAX){
		oknochyb(lng(778, "You have too many open fields."));
		return 0;
	}
	if(plocha) plocha->deaktiv();
	p=plochy+Nfields;
	//create new name
	strcpy(p->mname, lng(651, "Field_01"));
	p->mname[6]=char(plpoc/10+'0');
	p->mname[7]=char(plpoc%10+'0');
	//inicialize new field except backup
	*p->jmeno=0;
	p->pole1=p->pole2=0;
	return p;
}

void plnova()
{
	Tplocha *p;

	if((p=plpridej())!=0){
		if(++plpoc>99) plpoc=1;
		plocha=p;
		p->zaloha();
		domu();
		plvymazat();
		Nfields++;
		plMenu();
	}
}

void plotevrit()
{
	FILE *f;
	Tplocha *p;

	if((p=plpridej())!=0)
	 if(oknofile(lng(31, "Open field"), EXTPL, p->jmeno, 0))
	 if((f=fopen(p->jmeno, "rb"))==0){
		 chyba(lng(739, "File not found"));
	 }
	 else{
		 if(fgetc(f)!='p'){
			 chyba(lng(779, "File does not contain field"));
		 }
		 else{
			 fgetc(f); fgetc(f);
			 plxm=fgetc(f); plym=fgetc(f);
			 if(plxm>=PXMAX) plxm=PXMAX-1;
			 if(plym>=PYMAX) plxm=PYMAX-1;
			 kax=fgetc(f); kay=fgetc(f); smer=fgetc(f);
			 size_t len2= fgetc(f)+256*fgetc(f);
			 Dplbuf= (2*len2+ sizeof(unsigned)-1)/sizeof(unsigned);
			 if(Dplbuf>sizeof(plbuf) || fread(plbuf, 2, len2, f)!=len2){
				 chyba(lng(754, "Disk read error"));
				 plocha->aktiv();
			 }
			 else{
				 pldekompres2();
				 plResize();
				 Nfields++;
				 plocha=p;
				 p->pojmenuj();
				 p->zaloha();
				 plInvalidate();
			 }
		 }
		 fclose(f);
	 }
}

void plzavrit()
{
	Tplocha *p;
	int i;

	if(Nfields==1){//at least 1 filed must remain
		plvymazat();
		*plocha->jmeno=0;
		strcpy(plocha->mname, lng(651, "Field_01"));
	}
	else{
		plocha->smaz();
		i= Nfields-int(plocha-plochy)-1;
		for(p=plocha; i--; p++) *p=p[1];
		Nfields--;
		plocha=plochy;
		plocha->aktiv();
	}
	plMenu();
}

void plulozit()
{
	FILE *f;

	if(!*plocha->jmeno){
		if(!oknofile(lng(32, "Save field"),
			 EXTPL, plocha->jmeno, 1)) return;
		plocha->pojmenuj();
	}
	if((f=fopen(plocha->jmeno, "wb"))==0){
		chyba(lng(733, "Cannot open file for writing"));
	}
	else{
		plkompres2();
		fputc('p', f);
		fputc(80-plxm-(PXMAX-plxm)/3, f);
		fputc((26-plym)/2, f);
		fputc(plxm, f); fputc(plym, f);
		fputc(kax, f); fputc(kay, f); fputc(smer, f);
		size_t len2= Dplbuf*sizeof(unsigned)/2;
		fputc(len2&255, f); fputc(len2>>8, f);
		if(fwrite(plbuf, 2, len2, f) !=len2)
			chyba(lng(734, "Disk write error"));
		fclose(f);
	}
}

void plulozitjako()
{
	if(oknofile(lng(33, "Save field as"), EXTPL, plocha->jmeno, 2)){
		plocha->pojmenuj();
		plulozit();
	}
}

void plprejmenovat()
{
	if(oknofile(lng(34, "Rename field"),
		 EXTPL, plocha->jmeno, 2)){
		plocha->pojmenuj();
	}
}

void plgenerovat()
{
	int i, j, k;

	for(j=1; j<plym; j++)
	 for(i=1; i<plxm; i++){
		 if(rand()<RAND_MAX/4) k=-1;
		 else if(rand()<RAND_MAX/2) k=0;
		 else k=maxznacek - rand()/(RAND_MAX/(maxznacek-256)+1);
		 pole[i][j]=k;
	 }
	pole[kax][kay]=0;
	plInvalidate();
}

void plrozmery(int x, int y)
{
	int i;

	if(x>=PXMAX) x=PXMAX-1;
	if(x<2) x=2;
	if(y>=PYMAX) y=PYMAX-1;
	if(y<2) y=2;
	//delete borders when enlargement
	for(i=1; i<=plxm; i++) pole[i][plym]=0;
	for(i=1; i<plym; i++) pole[plxm][i]=0;
	//center
	plxm=x; plym=y;
	plResize();
	if(kax>=x) kax=x-1;
	if(kay>=y) kay=y-1;
	plokraj();
	plInvalidate();
}

void plupravit()
{
	if(mode==MODE_FIELD) return;
	xp=1;
	yp=plym-1;
	mode=MODE_FIELD;
	repaintAll();
}

void esc()
{
	if(mode==MODE_TEXT){
		prgreset();
	}
	else{
		mode=MODE_TEXT;
		if(pole[kax][kay]<0){ pole[kax][kay]=0; tiskzn(); }
		repaintAll();
	}
}

void pause()
{
	if(jebeh()){
		if(lastTracing) trasovat(); else spustit();
	}
}

void flLeft(){
	xp--;
}
void flRight(){
	xp++;
}
void flUp(){
	yp--;
}
void flDown(){
	yp++;
}
void flCtrlLeft(){
	do xp--; while(pole[xp][yp]>=0);
}
void flCtrlRight(){
	do xp++; while(pole[xp][yp]>=0);
}
void flCtrlUp(){
	do yp--; while(pole[xp][yp]>=0);
}
void flCtrlDown(){
	do yp++; while(pole[xp][yp]>=0);
}
void flWallLeft(){
	xp=1;
}
void flWallRight(){
	xp=plxm-1;
}
void flWallUp(){
	yp=1;
}
void flWallDown(){
	yp=plym-1;
}
void flLeftTop(){
	xp=yp=1;
}
void flLeftBottom(){
	xp=1; yp=plym-1;
}
void flRightTop(){
	xp=plxm-1; yp=1;
}
void flRightBottom(){
	xp=plxm-1; yp=plym-1;
}
void flDone(){
	esc();
	plocha->zaloha();
}
void flDeleteChar(){
	pole[xp][yp]=0;
}
void flDeleteLastChar(){
	if(xp>1) xp--;
	flDeleteChar();
}
void flWall(){
	pole[xp][yp]= pole[xp][yp]<0 ? 0 : -1;
}
void flKarel(){
	tiskpole(kax, kay);
	kax=xp; kay=yp;
}
void flWidth(){
	int i=plxm-1;
	if(oknoint(lng(35, "Field Dimension"), lng(522, "Width:"), i)){
		plrozmery(i+1, plym);
	}
}
void flHeight(){
	int i=plym-1;
	if(oknoint(lng(35, "Field Dimension"), lng(523, "Height:"), i)){
		plrozmery(plxm, i+1);
	}
}

void Tplocha::aktiv()
{
	plxm=xm, plym=ym;
	kax=karelx, kay=karely, smer=karelsmer;
	pldekompres(pole1, Dpole1);
	plResize();
	plInvalidate();
}

void Tplocha::deaktiv()
{
	plkompres(pole1, Dpole1);
	xm=plxm, ym=plym;
	karelx=kax, karely=kay, karelsmer=smer;
}

void Tplocha::zaloha()
{
	plkompres(pole2, Dpole2);
	xm2=plxm, ym2=plym;
	karelx2=kax, karely2=kay, karelsmer2=smer;
}

void Tplocha::smaz()
{
	delete[] pole1;
	delete[] pole2;
}

//jmeno -> mname
void Tplocha::pojmenuj()
{
	char *b, *e;

	//cut off extension and path
	e=strchr(jmeno, '.');
	if(!e) e=strchr(jmeno, 0);
	for(b=e; b>=jmeno && *b!='\\'; b--);
	b++;
	if(e-b>=MMNAME) e=b+MMNAME-1;
	strncpy(mname, b, (size_t)(e-b));
	mname[int(e-b)]=0;
	plMenu();
	printNinstr();
}

//-------------------------------------------------------------------------
// Find procedure,  Run...
//-------------------------------------------------------------------------
#define MLPBUF 32

static int lpx, lpy, lptopr;
static Tslovo *lpcur, *lptop; //selected word; top of page
static char lpbuf[MLPBUF]; //searched word

void tiskpr1(Tslovo *sl, bool sel)
{
	char *s;

	textColor(sel ? clprik1 : clprik);
	for(s=sl->jmeno; neoddel[*s]; s++) putCh(*s);
}

void tiskpr(bool sel)
{
	gotoXY(1, lpy);
	tiskpr1(lpcur, sel);
}

//repaint window
void lstpr()
{
	int j;
	Tslovo *sl;

	if(lpy>Dprocedur-lptopr+1) lpy=Dprocedur-lptopr+1;
	for(j=1, sl=lptop; j<=ymax && sl; j++, sl=sl->nxt){
		if(j==lpy) lpcur=sl;
		gotoXY(1, j);
		tiskpr1(sl, j==lpy);
		textColor(clprik);
		clrEol();
	}
	while(j<=ymax){
		gotoXY(1, j);
		clrEol();
		j++;
	}
}

void lpsetcur(int y)
{
	if(y>Dprocedur-lptopr+1) y=Dprocedur-lptopr+1; //after end of page
	if(y>=1 && y<=ymax && y!=lpy){
		lpx=0;
		tiskpr(false);
		lpy=y;
		lpcur=lptop;
		while(--y) lpcur=lpcur->nxt;
		tiskpr(true);
	}
}

bool lphledej()
{
	int i, r;
	Tslovo *sl;
	char *u;

	if(!lpx){
		lptop=slovnik; lptopr=1;
		lpy=1;
		lstpr();
	}
	else
	for(sl=slovnik, r=1; sl; sl=sl->nxt, r++)
	 for(u=sl->jmeno, i=0; pism[*u]==pism[lpbuf[i++]]; u++){
		 if(i==lpx){ //found
			 if(r>=lptopr && r<lptopr+ymax){
				 tiskpr(false);
				 lpcur=sl;
				 lpy=r-lptopr+1;
				 tiskpr(true);
			 }
			 else{
				 lptop=sl; lptopr=r; lpy=1;
				 lstpr();
			 }
			 return false; //found
		 }
	 }
	return true; //not found
}

void prLeft(){
	if(lpx) lpx--;
}
void prRight(){
	if(neoddel[lpbuf[lpx]= lpcur->jmeno[lpx]]) lpx++;
}
void prScrollUp(){
	if(lptopr>1){
		lptop=lptop->pre;
		lptopr--;
		if(lpy<ymax) lpy++;
		else lpx=0;
		lstpr();
	}
}
void prUp(){
	if(lpcur->pre){
		lpx=0;
		if(lpy==1){
			lpy--;
			prScrollUp();
		}
		else{
			tiskpr(false);
			lpcur=lpcur->pre;
			lpy--;
			tiskpr(true);
		}
	}
}
void prScrollDown(){
	if(lptopr+ymax<=Dprocedur){
		lptop=lptop->nxt;
		lptopr++;
		if(lpy>1) lpy--;
		else lpx=0;
		lstpr();
	}
}
void prDown(){
	if(lpcur->nxt){
		lpx=0;
		if(lpy==ymax){
			lpy++;
			prScrollDown();
		}
		else{
			tiskpr(false);
			lpcur=lpcur->nxt;
			lpy++;
			tiskpr(true);
		}
	}
}
void prLineStart(){
	lpx=0;
}
void prLineEnd(){
	while(neoddel[lpbuf[lpx]= lpcur->jmeno[lpx]]) lpx++;
}
void prPageUp(){
	lpx=0;
	if(lptopr-ymax>1){
		for(int i=ymax; i && lptop->pre; i--){
			lptop=lptop->pre, lptopr--;
		}
		lstpr();
	}
	else if(lptopr>1){
		lptopr=1; lptop=slovnik;
		lstpr();
	}
	else{
		lpsetcur(1);
	}
}
void prPageDown(){
	if(lptopr+ymax>Dprocedur){
		lpsetcur(ymax);
	}
	else{
		lpx=0;
		lptop=lpcur; lptopr+=lpy-1;
		for(int i=lpy; i<=ymax; i++){
			lptop=lptop->nxt, lptopr++;
		}
		lstpr();
	}
}
void prEditorTop(){
	lpx=0;
	lptop=slovnik; lptopr=1;
	lpy=1;
	lstpr();
}
void prEditorBottom(){
	if(lptopr+ymax>Dprocedur){
		lpsetcur(ymax);
	}
	else{
		lpx=0;
		lptop=slovnikend; lptopr=Dprocedur;
		for(int i=1; i<ymax; i++){
			lptop=lptop->pre, lptopr--;
		}
		lpy=ymax;
		lstpr();
	}
}
void prPageTop(){
	lpsetcur(1);
}
void prPageBottom(){
	lpsetcur(ymax);
}
void prLineBreak(){
	lpx=0;
	text.gotor(lpcur->r);
	if(!prRun){
		mode=MODE_TEXT;
		statusMsg(clstatus, "");
		repaintAll();
	}
	else{
		text.runr=false;
		prgreset();
		do{
			if(jechyba) smaz25();
			spustit();
		} while(uprg && *uprg&128);
	}
}
void prDeleteLastChar(){
	if(lpx){
		lpx--;
		lphledej();
	}
}

//run: 1=Run (F12), 0=Find (F11)
void lstprik(bool run)
{
	if(mode==MODE_FINDPROC) return;
	notrunr();
	lpx=0;
	lpy=lptopr=1;
	lptop=slovnik;
	mode=MODE_FINDPROC;
	prRun=run;
	repaintAll();
}

/* lstpr();
 for(;;){

 if(c==1 && mx-X0<=xmax){//mouse
 }
 switch(c){
 continue;
 case 13://ENTER or doubleclick
 continue;
 case 27://ESC
 break;
 case 8://BACKSPACE
 continue;
 case 0:
 switch(c=getch()){
 default:
 continue;
 case 67://F9
 text.gotor(lpcur->r);
 spustit();
 continue;
 case 45://ALT-X
 break;
 }
 }
 break;
 }
 repaintAll();
 }
 */

void najdiprik()
{
	mkslov(false);
	if(slovnik) lstprik(false);
}

void spustprik()
{
	if(!preklad) kompilace();
	if(preklad) lstprik(true);
}

//-----------------------------------------------------------------------
char *keywordEn[Dkeyw]=
{"PROCEDURE", "FUNCTION", "CONDITION",
"IF", "WHILE", "REPEAT", "ELSE", "UNTIL", "END",
"RETURN", "INTEGER", "SWITCH", "CASE",
"NOT", "IS", "AND", "OR",
"STEP", "LEFT", "RIGHT", "PICK", "PUT", "PAINT",
"DELAY", "STOP", "HOME", "FAST", "SLOW",
"WALL", "MARK", "SPACE", "NORTH", "WEST", "SOUTH", "EAST",
"CHAR", "KEY", "RANDOM", "LENGTH",
"TRUE", "FALSE",
};
#define MLNG 9
char **keywords[MLNG];
Tslovo *keywS[MLNG];

void loadKeywords()
{
	char *s, *t, *word;
	int i, n, l;
	Tslovo *ks;

	if(!*keywordLang){
		s="English,Czech";
		if(!strcmp(lang, "Czech")) s="Czech,English";
		if(!strcmp(lang, "French")) s="English,French,Czech";
		strcpy(keywordLang, s);
	}
	//delete old keywords
	for(l=0; keywords[l]; l++){
		delete[] keywords[l];
		keywords[l]=0;
		delete[] keywS[l];
		keywS[l]=0;
	}
	char *oldLang = _strdup(lang);
	//load keywords from lng files
	for(s=keywordLang, n=0; n<MLNG-1;){
		while(*s==' ') s++;
		if(!*s) break;
		t=strchr(s, ',');
		if(t) *t=0;
		//read lng file
		strcpy(lang, s);
		loadLang();
		//copy keywords to the keywords array and convert them to DOS encoding
		if(lng(250, 0) || !_stricmp(s, "English")){
			keywords[n] = new char*[Dkeyw];
			for(i=0; i<Dkeyw; i++){
				word = lng(i+250, keywordEn[i]);
				CharToOem(word, keywords[n][i]= new char[strlen(word)+1]);
			}
			n++;
		}
		if(!t) break;
		*t=',';
		s= t+1;
	}
	//restore lang
	strcpy(lang, oldLang);
	free(oldLang);
	loadLang();
	//create keywS array
	for(l=0; keywords[l]; l++){
		ks= keywS[l]= new Tslovo[Dkeyw];
		for(i=PROCEDURE; i<KROK; i++) ks[i].result=T_NO;
		for(; i<ZED; i++) ks[i].result=T_VOID;
		for(; i<ZNAK; i++) ks[i].result=T_BOOL;
		for(; i<I_JEDNA; i++) ks[i].result=T_INT;
		for(; i<=I_NULA; i++) ks[i].result=T_BOOL1;
		ks[MALUJ].Nparam= ks[CEKEJ].Nparam= ks[NAHODA].Nparam= ks[DELKA].Nparam= 1;
		for(i=PROCEDURE; i<Dkeyw; i++){
			ks[i].r= -i;
			ks[i].jmeno= keywords[l][i];
		}
	}

	if(n==0){
		strcpy(keywordLang, "English");
		loadKeywords();
	}
}
//-----------------------------------------------------------------------
int makro1[26]=
{A, NEBO, CEKEJ, DOKUD, ZED, FUNKCE, ZNAK, JIH, KDYZ, JINAK, KROK, KLAVESA, MALUJ,
NENI, OPAKUJ, POLOZ, PROCEDURE, VPRAVO_VBOK, SEVER, MISTO, NAHODA, VLEVO_VBOK, ZAPAD,
ZNACKA, VYCHOD, ZVEDNI};

char *upper852="š¶ŽÞ€ÓŠ×‘â™•—›¬µÖàé¤¦¨¸½ÆÑÒ·ãÕæèíÝëü";
char *lower852="‚ƒ„…†‡ˆ‰‹Œ’“”–˜œŸ ¡¢£¥§©«­¾ÇÐÔØäåçêìîûý";
char *norm852= "UEAAUCCLEOILOOLSTCAIOUAZEZSZADDENNSRYTUR";

char *upper850="š¶Ž·€ÒÓÔØ×Þ’â™ãêëYŸµÖàé¥ÇÑIåèí";
char *lower850="‚ƒ„…†‡ˆ‰Š‹Œ‘“”•–—˜›Ÿ ¡¢£¤ÆÐÕäçì";
char *norm850= "UEAAAACEEEIII’OOOUUYŸAIOUNADIOèY";


void startup(char *args)
{
	int c, i;
	char *m, *v, *b;
	char *s;
	long d;
	time_t t;
	FILE *f;

	srand((unsigned)time(&t));

	for(c=0; c<=255; c++){
		neoddel[c]= c>127;
		uppism[c]=downpism[c]=pism[c]=(char)c;
	}
	neoddel['_']= neoddel['?']= neoddel['@']=
		neoddel['#']= neoddel[':']= neoddel['!']= neoddel['`']= 1;
	for(c='0'; c<='9'; c++) neoddel[c]=1;
	for(c='a'; c<='z'; c++){
		pism[c]=uppism[c]= (char)(c-('a'-'A'));
		neoddel[c]=1;
	}
	for(c='A'; c<='Z'; c++){
		downpism[c]= (char)(c+('a'-'A'));
		neoddel[c]=1;
	}
	if(GetOEMCP()==852) m=lower852, v=upper852, b=norm852;
	else m=lower850, v=upper850, b=norm850;
	for(; *m; m++, v++, b++){
		uppism[*m]=*v, downpism[*v]=*m, pism[*m]=pism[*v]=*b;
	}

	//initialize keywords
	loadKeywords();

	//create Field1
	plokraj();
	plnova();

	//initialize macros
	for(i=0; i<26; i++) makra[i]=keywords[0][makro1[i]];
	makra[26]=keywords[0][KONEC];
	if((f=fopen("karel.kmk", "rb"))!=0){
		fseek(f, 0, SEEK_END);
		d=ftell(f);
		rewind(f);
		if(d<3000 && (s=memmak=new char[(size_t)d+2])!=0){
			fread(s, 1, (size_t)d, f);
			s[(int)d]='\r';
			for(i=0; i<=26 && s-memmak<d; i++){
				if(*s!='\r' && *s!='\n') makra[i]=s;
				while(*s!='\r' && *s!='\n') s++;
				*s++=0;
				if(*s=='\n') s++;
			}
		}
		fclose(f);
	}
	updwkeys();

	//open source file
	s=0;
	if(args && *args) s=args;
	else if((f=fopen(SLOVNIK, "r"))!=0){ fclose(f); s=SLOVNIK; }
	if(s){
		strncpy(text.jmeno, s, sizeof(text.jmeno)-1);
		text.otevri();
	}
	else{
		text.novy();
	}
	text.edDone();
}

typedef void(*Tfunc)();

Tfunc funcTab1[]={ //100
	krok, vpravo_vbok, vlevo_vbok, zvedni, poloz,
	domu, esc, plupravit, plobnovit, plvymazat,
	plgenerovat, plnova, plotevrit, plulozit, plulozitjako,
	plprejmenovat, plzavrit, writeini, deleteini, oprogramu,
	konec, smazbreakpointy, moznosti, helpprik1, helpprik2,
	helpeditor, zmenitadr, changekeys, changecolors
};

Tfunc funcTab2[]={ //500
	spustit, trasovat, spustprik, setrychlost, setstacksize,
	prelozit, prgreset, presprocedury, jedenradek, krokovat,
	jednainstrukce, nakonec, gotocursor, breakpoint, showprg,
	najdiprik, najit, nahradit, pause
};

Tfunc funcTabPr[]={ //1000
	prLeft, prRight, prUp, prDown, prLineStart,
	prLineEnd, prScrollUp, prScrollDown, prLineStart, prLineEnd,
	prPageUp, prPageDown, prEditorTop, prEditorBottom, prPageTop,
	prPageBottom, prLineBreak, prDeleteLastChar, prDeleteLastChar,
};

Tfunc funcTabField[]={ //1000
	flLeft, flRight, flUp, flDown, flCtrlLeft,
	flCtrlRight, flCtrlUp, flCtrlDown, flWallLeft, flWallRight,
	flWallUp, flWallDown, flLeftTop, flLeftBottom, flRightTop,
	flRightBottom, flDone, flDeleteLastChar, flDeleteChar, flWall
};

Tfunc funcTabField2[]={ //1500
	flLeftTop, flLeftBottom, flRightTop, flRightBottom, flKarel,
	flWidth, flHeight, flCtrlLeft, flCtrlRight, flCtrlUp,
	flCtrlDown
};

void beforeCmd()
{
	hideCursor();
	if(jechyba) jechyba=0; ///smaz25();
	if(mode==MODE_TEXT) text.edPrepare();
}

void afterCmd()
{
	switch(mode){
		case MODE_TEXT:
			text.edDone();
			break;
		case MODE_FIELD:
			if(xp>=plxm) xp=plxm-1;
			if(yp>=plym) yp=plym-1;
			if(xp<=0) xp=1;
			if(yp<=0) yp=1;
			break;
	}
	tiskCinstr();
	beforeWait();
}

void beforeWait()
{
	switch(mode){
		case MODE_TEXT:
			text.showCursor();
			break;
		case MODE_FIELD:
			gotoSQ(xp, yp);
			showCursor(1);
			break;
		case MODE_FINDPROC:
			if(!jechyba) statusMsg(clstatusFindProc, lng(652,
				" Type several letters or use cursors. Then press ENTER."));
			if(lpx){
				gotoXY(lpx+1, lpy);
				showCursor(0);
			}
			break;
	}
}

void wm_command(int cmd)
{
	if(cmd<100) return;
	if(isBusy){
		if(cmd==506) runCmd(8);
		if(cmd==106 || cmd==518) runCmd(27);
		if(cmd==120) isBusy=0;
		else return;
	}
	beforeCmd();
	if(cmd>=500){
		if(cmd>=9000){
			if(cmd>=10000){ //change field from menu
				if(cmd<10000+PLMAX) plaktiv(cmd-10000);
			}
			else if(cmd>=9900){ //ALT + number
				if(cmd<=9900+maxznacek-256){
					int z=cmd-9900+256;
					if(z==256) z=0;
					if(mode==MODE_FIELD){
						pole[xp][yp]=z;
					}
					else{
						pole[kax][kay]=z;
						tiskzn();
					}
				}
			}
			else{ //language
				if(cmd<9000+Nlang){
					setLang(cmd-9000);
					repaintAll();
				}
			}
		}
		else if(mode==MODE_TEXT){
			if(cmd>=800){
				text.command(cmd);
			}
			else if(cmd<500+sizeA(funcTab2)){
				funcTab2[cmd-500]();
			}
		}
		else if(mode==MODE_FINDPROC){
			if(cmd>=1000 && cmd<1000+sizeA(funcTabPr)){
				funcTabPr[cmd-1000]();
			}
		}
		else if(mode==MODE_FIELD){
			if(cmd>=1500){
				if(cmd<1500+sizeA(funcTabField2)){
					funcTabField2[cmd-1500]();
				}
			}
			else if(cmd>=1000 && cmd<1000+sizeA(funcTabField)){
				funcTabField[cmd-1000]();
			}
		}
	}
	else if(cmd<100+sizeA(funcTab1)){
		funcTab1[cmd-100]();
		tiskCinstr();
	}
	afterCmd();
}

void wm_char(int c)
{
	if(isBusy){
		runCmd(c);
		return;
	}
	beforeCmd();
	switch(mode){
		case MODE_TEXT:
			text.key(c);
			break;
		case MODE_FIELD:
			if(c==' ') c=0;
			pole[xp][yp]= c;
			if(c>='0' && c<='9' || c=='-') xp++;
			break;
		case MODE_FINDPROC:
			if(c>32 && lpx<MLPBUF){
				lpbuf[lpx++]= (char)c;
				if(lphledej()) lpx--;
			}
			break;
	}
	afterCmd();
}

//mx,my is indexed from zero
void wm_mouse(int mx, int my, int button)
{
	if(isBusy) return;
	if(mode!=MODE_FIELD){
		mx=(mx-X0)/charWidth+1;
		my=(my-Y0)/charHeight+1;
	}
	switch(mode){
		case MODE_TEXT:
			text.mouse(mx, my, button);
			break;
		case MODE_FINDPROC:
			if(button==1 && mx<=xmax){
				beforeCmd();
				if(my==lpy) wm_command(1016);
				else lpsetcur(my);
				afterCmd();
			}
			break;
		case MODE_FIELD:
			mx=(mx-plx)/squareWidth;
			my=(my-ply)/squareHeight;
			if(mx>0 && mx<plxm && my>0 && my<plym){
				bool b= xp==mx && yp==my;
				int z=pole[mx][my];
				switch(button){
					case 1: //left button
						beforeCmd();
						xp=mx; yp=my;
						if(b){
							pole[mx][my]= z ? (z>256 && z<maxznacek ? z+1 : 0) : 257;
						}
						afterCmd();
						break;
					case 2: //right button
						beforeCmd();
						xp=mx; yp=my;
						pole[mx][my]= z<0 ? 0 : -1;
						afterCmd();
						break;
				}
			}
			break;
	}
}

void wm_timer(int id)
{
	beforeCmd();
	switch(id){
		case 10:
			text.selScroll();
			break;
	}
	afterCmd();
}

void repaintAll()
{
	showScrollBar(mode==MODE_TEXT);
	textColor(clnull);
	cls();
	switch(mode){
		case MODE_TEXT:
			text.repaint();
			break;
		case MODE_FIELD:
			plPrintHelp();
			break;
		case MODE_FINDPROC:
			lstpr();
			break;
	}
	pltisk();
	paintStatus();
}


/*
	(C) 1999-2005  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include "hdr.h"
#pragma hdrstop

#ifdef DOS

#include "karel.h"
#include "editor.h"
#include "menu.h"
//---------------------------------------------------------------------------
char *CONFIG="karel.ini";
int charWidth=1, charHeight=1, squareWidth=1, squareHeight=1;
int X0=1, Y0=2;
char tabsed[4]="";
int timerId, timerDelay;
char *clipboard;
size_t Dclipboard;//clipboard size

int colorsDos[]={0x00,
0x1e, 0x1f, 0x1f, 0x1b, 0x1a,
0x4f, 0x30, 0x71, 0x31, 0x13,
0x07, 0x0a, 0x4e, 0x0b, 0x05,
0x07, 0x04, 0x30, 0x07, 0x0e,
0x07, 0x20, 0x0e, 0x07, 0x07
};

//---------------------------------------------------------------------------

void clrscr()
{
	struct text_info ti;
	gettextinfo(&ti);
	int i, j, w, h;
	w=ti.winright-ti.winleft+1;
	h=ti.winbottom-ti.wintop+1;
	for(i=1; i<=h; i++){
		gotoxy(1, i);
		if(w==80) cputs("                                                                                ");
		else for(j=w; j>0; j--) putch(' ');
	}
	gotoxy(1, 1);
}

void clrEol()
{
	int x=wherex(), y=wherey();

	if(x<X0+xmax-1){
		window1(x, y, X0+xmax-1, y);
		clrscr();
		window0();
	}
}

Tmenuitem Msoubor0[]={
{"&Novì",0,1080,0},
{"&Otevý¡t...","Ctrl+O",1081,0},
{"&Ulo§it","Ctrl+S",1082,0},
{"Ulo§it j&ako...",0,1083,0},
{"&ZmØnit adres ý",0,126,0},
{"-",0,0,0},
{"Czech -> &English",0,9001,0},
{"-",0,0,0},
{"&Konec","Alt+X",120,0},
{0,0,0,0}},
Mupravy0[]={
{"&ZpØt","Alt+BkSp",1038,lzezpet},
{"Z&novu","Shift+Alt+BkSp",1039,lzevpred},
{"-",0,0,0},
{"&Vyjmout","Ctrl+X",1036,jeblok},
{"&Kop¡rovat","Ctrl+C",1035,jeblok},
{"V&lo§it","Ctrl+V",1037,0},
{"Vym&azat","Ctrl+Del",1073,jeblok},
{"-",0,0,0},
{"&Mo§nosti...","Alt+O",122,0},
{0,0,0,0}},
Mhledat0[]={
{"&Naj¡t...","Ctrl+F",516,0},
{"Na&hradit...","Ctrl+R",517,0},
{"Naj¡t &dalç¡","Ctrl+L",1077,jeffind},
{"Naj¡t &pýedchoz¡","Ctrl+Shift+L",1078,jeffind},
{"-",0,0,0},
{"&J¡t na ý dek...","Ctrl+G",1079,0},
{"Naj¡t p&roceduru...","F11",515,0},
{"Pr &vØ prov dØnì pý¡kaz",0,514,jebeh},
{0,0,0,0}},
Mspustit0[]={
{"Sp&ustit rychle","F9",500,0},
{"Spustit &pomalu","F6",501,0},
{"&Seznam procedur...","F12",502,0},
{"-",0,0,0},
{"&Velikost z sobn¡ku...",0,504,0},
{"&Rychlost...","Ctrl+F6",503,0},
{"&Informace","Alt+F9",505,0},
{"U&konŸit bØh","Escape",506,jebeh},
{0,0,0,0}},
Mladeni0[]={
{"&Jeden ý dek","F7",508,0},
{"&Pýes procedury","F8",507,0},
{"&Krok","Shift+F8",509,0},
{"Jedna &instrukce","Shift+F7",510,0},
{"&DokonŸit proceduru","Shift+F4",511,0},
{"-",0,0,0},
{"J¡t ke ku&rzoru","F4",512,0},
{"&Breakpoint","Ctlr+F8",513,0},
{"&Smazat breakpointy",0,121,jebreakp},
{0,0,0,0}},
Mkarel0[]={
{"&Krok","Alt+nahoru",100,0},
{"Vp&ravo","Alt+vpravo",101,0},
{"&Vlevo","Alt+vlevo",102,0},
{"&Zvedni","Alt+ -",103,0},
{"&Polo§","Alt+ +",104,0},
{"&Dom…","Alt+Home",105,0},
{0,0,0,0}},
Mplocha0[]={
{"Up&ravit...","Ctrl+F5",107,0},
{"O&bnovit","F5",108,0},
{"&Vymazat",0,109,0},
{"&Generovat","Shift+F5",110,0},
{"-",0,0,0},
{"&Nov ",0,111,0},
{"&Otevý¡t...",0,112,0},
{"&Ulo§it",0,113,0},
{"Ulo§it j&ako...",0,114,0},
{"&Pýejmenovat",0,115,0},
{"&Zavý¡t",0,116,0},
{"-",0,0,0},
{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
{0,0,0,0}},
Mnapoveda0[]={
{"&Z kladn¡ pý¡kazy","F1",123,0},
{"Ostatn¡ &pý¡kazy","Shift+F1",124,0},
{"&Kl vesov‚ zkratky","Ctrl+F1",125,0},
{"-",0,0,0},
{"&O programu...",0,119,0},
{0,0,0,0}};

Tmenuitem Msoubor1[]={
{"&New",0,1080,0},
{"&Open...","Ctrl+O",1081,0},
{"&Save","Ctrl+S",1082,0},
{"S&ave as...",0,1083,0},
{"Change &dir...",0,126,0},
{"-",0,0,0},
{"Anglictina->&Cestina",0,9000,0},
{"-",0,0,0},
{"E&xit","Alt+X",120,0},
{0,0,0,0}},
Mupravy1[]={
{"&Undo","Alt+BkSp",1038,lzezpet},
{"&Redo","Shift+Alt+BkSp",1039,lzevpred},
{"-",0,0,0},
{"Cu&t","Ctrl+X",1036,jeblok},
{"&Copy","Ctrl+C",1035,jeblok},
{"&Paste","Ctrl+V",1037,0},
{"&Delete","Ctrl+Del",1073,jeblok},
{"-",0,0,0},
{"&Options...","Alt+O",122,0},
{0,0,0,0}},
Mhledat1[]={
{"&Find...","Ctrl+F",516,0},
{"&Replace...","Ctrl+R",517,0},
{"Find &next","Ctrl+L",1077,jeffind},
{"Find previo&us","Ctrl+Shift+L",1078,jeffind},
{"-",0,0,0},
{"&Go to line number...","Ctrl+G",1079,0},
{"Find &procedure...","F11",515,0},
{"Show &execution point",0,514,jebeh},
{0,0,0,0}},
Mspustit1[]={
{"&Run","F9",500,0},
{"&Trace","F6",501,0},
{"Run &procedure...","F12",502,0},
{"-",0,0,0},
{"Spee&d...","Ctrl+F6",503,0},
{"&Stack size...",0,504,0},
{"&Compile","Alt+F9",505,0},
{"Program r&eset","Escape",506,jebeh},
{0,0,0,0}},
Mladeni1[]={
{"&Step over","F8",507,0},
{"&Trace into","F7",508,0},
{"&Visible step","Shift+F8",509,0},
{"One &instruction","Shift+F7",510,0},
{"&Finish procedure","Shift+F4",511,0},
{"-",0,0,0},
{"Run to c&ursor","F4",512,0},
{"&Breakpoint","Ctlr+F8",513,0},
{"&Clear breakpoints",0,121,jebreakp},
{0,0,0,0}},
Mkarel1[]={
{"&Step","Alt+up",100,0},
{"&Right","Alt+right",101,0},
{"&Left","Alt+left",102,0},
{"&Pick up","Alt+ -",103,0},
{"Put &down","Alt+ +",104,0},
{"&Home","Alt+Home",105,0},
{0,0,0,0}},
Mplocha1[]={
{"&Modify...","Ctrl+F5",107,0},
{"Res&tore","F5",108,0},
{"Cl&ear",0,109,0},
{"&Generate","Shift+F5",110,0},
{"-",0,0,0},
{"&New",0,111,0},
{"&Open...",0,112,0},
{"&Save",0,113,0},
{"S&ave as...",0,114,0},
{"&Rename",0,115,0},
{"&Close",0,116,0},
{"-",0,0,0},
{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
{0,0,0,0}},
Mnapoveda1[]={
{"&Basic commands","F1",123,0},
{"&Other commands","Shift+F1",124,0},
{"&Hot keys","Ctrl+F1",125,0},
{"-",0,0,0},
{"&About...",0,119,0}
,{0,0,0,0}};

Tmenu mainmenu0[]={
{"&Soubor",Msoubor0,0,0},{"épr&avy",Mupravy0,0,0},
{"&Hledat",Mhledat0,0,0},{"Sp&ustit",Mspustit0,0,0},
{"&LadØn¡",Mladeni0,0,0}, {"&Karel",Mkarel0,0,0},
{"&Plocha",Mplocha0,0,0},
{"&N povØda",Mnapoveda0,0,0},{0,0,0,0}};

Tmenu mainmenu1[]={
{"&File",Msoubor1,0,0},{"&Edit",Mupravy1,0,0},
{"&Search",Mhledat1,0,0},{"&Run",Mspustit1,0,0},
{"&Debug",Mladeni1,0,0}, {"&Karel",Mkarel1,0,0},
{"F&ield",Mplocha1,0,0},
{"&Help",Mnapoveda1,0,0},{0,0,0,0}};

Tmainmenu *menu,*menus[2];

Tmenuitem *Mplochy=0; //pointer to Field menu item


char *gsmer0[]={"nahoru","dol…"},*god0[]={"od kurzoru","v cel‚m textu"},
     *gmak0[]={"velkìmi p¡smeny","malìmi p¡smeny","nenahrazovat"};

char *gsmer1[]={"Backward","Forward"},*god1[]={"From cursor","Entire scope"},
     *gmak1[]={"Upper case","Lower case","Disable"};

Tcomponent *Csfind0[]=
{
 new Teditn(1,1,33,"   &Naj¡t:",ffind,sizeof(ffind),false),
 new Teditn(1,3,33,"Na&hradit:",freplace,sizeof(freplace),false),
 new Tcheckbox(1,5,30,"&Jen cel‚ slovo",fword),
 new Tcheckbox(1,6,30,"&Rozliçovat mal  a velk ",fcase),
 new Tgroup(1,8,20,"&SmØr",fsmer,gsmer0,2),
 new Tgroup(25,8,20,"&Od",fod,god0,2),
 new Tbutton(3,12,"  O&K  ",1),
 new Tbutton(16,12,"Storno",0),
 new Tbutton(29,12,"Nahradit &vçe",3)
}
,*Cfind0=Csfind0[0],*Crepl0=Csfind0[1];

Tcomponent *Csfind1[]=
{
 new Teditn(1,1,29,"&Text to Find:",ffind,sizeof(ffind),false),
 new Teditn(1,3,29,"    &New Text:",freplace,sizeof(freplace),false),
 new Tcheckbox(1,5,23,"&Whole words only",fword),
 new Tcheckbox(1,6,23,"&Case-sensitive",fcase),
 new Tgroup(1,8,19,"&Direction",fsmer,gsmer1,2),
 new Tgroup(25,8,19,"O&rigin",fod,god1,2),
 new Tbutton(3,12,"  &OK  ",1),
 new Tbutton(16,12,"Cancel",0),
 new Tbutton(29,12,"Change &All",3)
}
,*Cfind1=Csfind1[0],*Crepl1=Csfind1[1];

Tcomponent *Ccfg0[]=
{
 new Tcheckbox(1,1,36,"&Trval‚ bloky",persistblok),
 new Tcheckbox(1,2,36,"&Pýepisovat bloky",overwrblok),
 new Tcheckbox(1,3,36,"&Odsazov n¡ po ENTER",indent),
 new Tcheckbox(1,4,36,"O&dsazov n¡ pýi Backspace",unindent),
 new Tcheckbox(1,5,36,"&Hledat slovo pod kurzorem",foncur),
 new Tcheckbox(1,6,36,"&Vlo§en¡ ý dku posune kurzor",entern),
 new Tcheckbox(1,7,36,"&ZpØt je po skupin ch",groupUndo),
 new Tcheckbox(1,8,36,"Po &ulo§en¡ lze d t zpØt",undoSave),
 new Tcheckbox(1,9,36,"Kl¡Ÿov  slova velkìmi p¡sme&ny",upcasekeys),
 new Teditn(1,11,5,"T&abul tory:",tabsed,sizeof(tabsed),false),
 new Tbutton(5,13,"  O&K  ",1),
 new Tbutton(20,13,"Storno",0)
};

Tcomponent *Ccfg1[]=
{
 new Tcheckbox(1,1,33,"&Persistent blocks",persistblok),
 new Tcheckbox(1,2,33,"&Overwrite blocks",overwrblok),
 new Tcheckbox(1,3,33,"&Autoindent mode",indent),
 new Tcheckbox(1,4,33,"&Backspace unindent",unindent),
 new Tcheckbox(1,5,33,"&Find text at cursor",foncur),
 new Tcheckbox(1,6,33,"&Insert line moves cursor up",entern),
 new Tcheckbox(1,7,33,"&Group undo",groupUndo),
 new Tcheckbox(1,8,33,"&Undo after save",undoSave),
 new Tcheckbox(1,9,33,"Capital letters in key&words",upcasekeys),
 new Teditn(1,11,5,"&Tab size:",tabsed,sizeof(tabsed),false),
 new Tbutton(5,13,"  O&K  ",1),
 new Tbutton(20,13,"Cancel",0)
};
//---------------------------------------------------------------------------

void setTimer(int id, int ms)
{
	timerId=id;
	timerDelay=ms;
}

void killTimer(int id)
{
	if(timerId==id) timerId=0;
}

void putCh(char ch)
{
	putch(ch);
}

void putS(char *s)
{
	cputs(s);
}

void endOfStatus()
{
	int x=wherex();
	while(x<80){
		putch(' ');
		x++;
	}
}

void gotoXY(int x, int y)
{
	gotoxy(X0+x-1, Y0+y-1);
}

void goto25()
{
	gotoxy(1, 25);
}

void gotoSQ(int x, int y)
{
	gotoxy(plx+x, ply+y);
}

void gotoNI()
{
	gotoxy(48, 24);
}

void showCursor(int ins)
{
	_setcursortype(ins ? _SOLIDCURSOR : _NORMALCURSOR);
}

void hideCursor()
{
	_setcursortype(_NOCURSOR);
}

void sleep(int ms)
{
	delay(ms);
}

int keyPressed()
{
	return bioskey(17);
}

int isShift()
{
	return bioskey(2) & 3;
}

void waitKeyboard()
{
	do{
		int c=getch();
		if(!c) c=-getch();
		runCmd(c);
	} while(bioskey(17));
}

void readKeyboard()
{
	while(bioskey(17)){
		int c=getch();
		if(!c) c=-getch();
		runCmd(c);
	}
}

void quit()
{
	window(1, 1, 80, 25);
	textattr(7);
	clrscr();
	_setcursortype(_NORMALCURSOR);
	exit(0);
}

void showScrollBar(bool show)
{
	if(!show){
		window1(X0+xmax, Y0, X0+xmax, Y0+ymax-1);
		textColor(clnull);
		clrscr();
		window0();
	}
}

int oknofile(char *name, char *ext, char *var, int)
{
	return oknofile(name, ext, var);
}
//---------------------------------------------------------------------------
//-1=wall, 0=nothing, 1-255=character, 257-265=mark
void tiskpole1(int c)
{
	if(c>0){
		if(c>255){
			textColor(clznacka);
			if(c<266) putch(c-256+'0');
			else putch('X');
		}
		else{
			textColor(clznak);
			if(c==10 || c==13 || c==7 || c==8) putch('');
			else putch((char)c);
		}
	}
	else if(!c){
		textColor(clznak);
		putch(' ');
	}
	else{
		textColor(clzed);
		putch('²');
	}
}

void tiskka()
{
	gotoxy(plx+kax, ply+kay);
	textColor(clkarel);
	putch(""[smer]);
}

void setScroll(int top, int bottom, int pos, int page)
{
	int i, j, s;

	s=int((pos-top)/(float)(bottom+2-top-page)*(ymax-2));
	amax(s, ymax-3);
	s+=Y0+1;
	i=X0+xmax;
	textColor(clscroll);
	j=Y0;
	gotoxy(i, j);
	putch('');
	textColor(clscrollbk);
	for(j++; j<Y0+ymax-1; j++){
		if(j!=s){
			gotoxy(i, j);
			putch('±');
		}
	}
	textColor(clscroll);
	gotoxy(i, j);
	putch('');
	gotoxy(i, s);
	putch('þ');
}

void clearPrevField()
{
	textColor(clnull);
	//left
	if(plx>X0+xmax+1){
		window1(X0+xmax+1, 2, plx-1, 24);
		clrscr();
		window0();
	}
	//bottom
	if(ply+plym<24){
		window1(plx, ply+plym+1, 80, 24);
		clrscr();
		window0();
	}
	//right
	if(plx+plxm<80){
		window1(plx+plxm+1, ply, 80, ply+plym);
		clrscr();
		window0();
	}
	//top
	if(ply>2){
		window1(plx, 2, 80, ply-1);
		clrscr();
		window0();
	}
}

void cls()
{
	clearPrevField();
}

void clipboardCopy(char *s, size_t n)
{
	delete[] clipboard;
	while((clipboard = new char[n])==0){
		if(freemem()) return;
	}
	memcpy(clipboard, s, Dclipboard=n);
}

char *clipboardPaste(size_t &len, bool &del)
{
	del=false;
	len=Dclipboard;
	return clipboard;
}

void plResize()
{
	plx= 80-plxm-(PXMAX-plxm)/3;
	ply= (26-plym)/2;
}

void plInvalidate()
{
	clearPrevField();
	pltisk();
}
//---------------------------------------------------------------------------
int findDlg()
{
	switch(lang){
		default:
			Csfind0[1]=Cfind0;
			return showmodal(Csfind0+1, sizeof(Csfind0)/sizeof(*Csfind0)-2,
				 "Naj¡t text", 48, 16, 0);
		case 1:
			Csfind1[1]=Cfind1;
			return showmodal(Csfind1+1, sizeof(Csfind1)/sizeof(*Csfind1)-2,
				 "Find Text", 48, 16, 0);
	}
}

int replaceDlg()
{
	switch(lang){
		default:
			Csfind0[1]=Crepl0;
			return showmodal(Csfind0, sizeof(Csfind0)/sizeof(*Csfind0),
				 "Nahradit text", 48, 16, 0);
		case 1:
			Csfind1[1]=Crepl1;
			return showmodal(Csfind1, sizeof(Csfind1)/sizeof(*Csfind1),
				 "Replace Text", 48, 16, 0);
	}
}

void moznosti()
{
	itoa(tabs, tabsed, 10);
	if(showmodal(lng(Ccfg0, Ccfg1), sizeof(Ccfg0)/sizeof(*Ccfg0),
		 lng(37, "Editor Options"), 40, 17, 0)){
		tabs=atoi(tabsed);
		updwkeys();
		text.lst();
	}
}

void oprogramu()
{
	oknotxt(lng(38, "About"),
	lng("          KAREL\n        Verze 4.0\n\n 1999-2007 Petr LaçtoviŸka\n\
			 \nTento program je freeware.",
	"          KAREL\n        Version 4.0\n\n1999-2007  Petr Lastovicka\n\
	 \n This program is freeware."), 31, 12);
}

void textColor(int cl)
{
	textattr(colorsDos[cl]);
}

void plMenu()
{
	int i;

	for(i=0; i<Nfields; i++){
		Tmenuitem *m= Mplochy+i;
		m->name= plochy[i].mname;
		m->cmd=10000+i;
		m->shortcut=0;
	}
	Mplochy[i].name=0;
}

void zmenitadr()
{
	char buffer[256];

	getcwd(buffer, sizeof(buffer));
	oknoed(lng(39, "Change Directory"),
		 lng(525, "Enter new directory name:"),
		 40, 80, buffer);
	if(chdir(buffer)) oknochyb(lng(795, "Invalid directory name"));
	else if(buffer[1]==':') setdisk(toupper(*buffer)-'A');
}
//---------------------------------------------------------------------------
void writeini()
{
	FILE *f;

	if(!(f=fopen(CONFIG, "w")))
	 oknochyb(lng(796, "Cannot save config file"));
	else{
		fprintf(f, "%d\n", 31);
		for(int i=0; i<sizeof(cfg)/sizeof(*cfg); i++) fprintf(f, "%d\n", cfg[i]);
		fclose(f);
	}
}

void deleteini(){}
void changekeys(){}
void changecolors(){}

//---------------------------------------------------------------------------
void setLang(int l)
{
	aminmax(l, 0, Nlang-1);
	lang=l;
	menu=menus[l];
	Mplochy = (l ? Mplocha1 : Mplocha0) + 12;
	plMenu();
	menu->tisk();
}
//---------------------------------------------------------------------------
int shortkeys[(256+32)*2]={
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1038, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 122, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1079, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 120, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 123,
	1082, 1077, 512, 108, 501, 508, 507, 500, 0, 0,
	0, 1008, 1002, 1010, 103, 1000, 0, 1001, 104, 1009,
	1003, 1011, 1019, 1018, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 125, 506, 0, 0, 107, 503,

	0, 513, 500, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 505, 0, 0, 1004, 1005, 1013, 1015, 1012,
	9901, 9902, 9903, 9904, 9905, 9906, 9907, 9908, 9909, 9900,
	103, 104, 1014, 515, 502, 0, 0, 0, 0, 0,
	0, 1006, 0, 0, 0, 1007, 1035, 1073, 0, 0,
	0, 105, 100, 0, 0, 102, 0, 101, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0, 1043, 0, 1035, 0, 0, 516, 1079, 1017, 1040,
	1070, 1076, 1077, 1016, 1046, 1081, 1074, 1075, 517, 1082,
	1047, 1045, 1037, 1006, 1036, 1048, 1038,
	106, 0, 0, 0, 0,
	//+shift
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1039, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1027, 1023, 1029, 0, 1021, 0, 1022, 0, 1028,
	1024, 1030, 1037, 1036, 124, 0, 1078, 511, 110, 0,
	510, 509, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1025, 1026, 1032, 1034, 1031,
	0, 0, 0, 0, 0, 0, 0, 1020, 0, 0,
	0, 0, 1033, 0, 0, 0, 0, 0, 0, 0,
	0, 1006, 0, 0, 0, 1007, 1035, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0, 1043, 0, 1035, 0, 0, 516, 1079, 1017, 1040,
	1070, 1076, 1078, 1084, 1046, 1081, 1074, 1075, 517, 1082,
	1047, 1045, 1037, 1006, 1036, 1049, 1039,
	106, 0, 0, 0, 0,
};
//---------------------------------------------------------------------------
void main(int argc, char **argv)
{
	int i, j, c, k, mx, my;
	FILE *f;

	textbackground(0);
	clrscr();
	_setcursortype(_NOCURSOR);
	mousereset();
	menus[0]=new Tmainmenu(mainmenu0);
	menus[1]=new Tmainmenu(mainmenu1);
	menus[0]->test();
	menus[1]->test();

	//open config file
	if((f=fopen(CONFIG, "r"))!=0){
		fscanf(f, "%d", &k);
		if(k==31)
			for(i=0; i<sizeof(cfg)/sizeof(*cfg); i++) fscanf(f, "%d", &cfg[i]);
		fclose(f);
	}
	setLang(lang);
	startup(argc>1 ? argv[1] : 0);
	repaintAll();
	text.showCursor();

	for(;;){
		if(timerId){
			do{
				delay(timerDelay);
				wm_timer(timerId);
				c=necekej8(mx, my);
			} while(c<0);
		}
		else{
			c=cekej8(mx, my);
		}
		if(!c){ //key
			c=getch();
			if(!c){
				c=getch();
				if(mode==MODE_FIELD){
					if(c==62)/* F4 */{ wm_command(1504); continue; }
					if(c==65)/* F7 */{ wm_command(1505); continue; }
					if(c==66)/* F8 */{ wm_command(1506); continue; }
					if(c==147)/*Ctlr+Del*/{ wm_command(109); continue; }
				}
				if(isShift()) c+=256+32;
				if(shortkeys[c]){
					wm_command(shortkeys[c]);
				}
				else{
					beforeCmd();
					if(c==68) menu->runF10();
					else menu->alt(c);
					afterCmd();
				}
			}
			else{
				if(c>=32 || ctrlp || ctrlk){
					if(c==127)/*Ctrl+Back*/{ wm_command(1020); continue; }
					wm_char(c);
				}
				else{
					if(isShift()) c+=256+32;
					wm_command(shortkeys[c+256]);
				}
			}
		}
		else{ //mouse
			if(my==1 && c==1){ //menu
				beforeCmd();
				menu->menuclick(mx);
				afterCmd();
			}
			else if(mx==X0+xmax && my>=Y0 && my<Y0+ymax && c==1){ //scrollbar
				beforeCmd();
				if(my==Y0+ymax-1){
					do{
						text.scrollDown();
						text.repaint();
						sleep(50);
					} while(button()&1);
				}
				else if(my==Y0){
					do{
						text.scrollUp();
						text.repaint();
						sleep(50);
					} while(button()&1);
				}
				else{
					j=-1;
					do{
						mouse8xy(mx, my);
						i=int((my-Y0-1)*((float)(text.rpoc-ymax+1))/(ymax-2)+1.5);
						aminmax(i, 1, text.rpoc-ymax+1);
						if(i!=j){
							j=i;
							text.setTop(i);
							text.repaint();
						}
					} while(button()&1);
				}
				afterCmd();
			}
			else{
				wm_mouse(mx, my, c);
			}
		}
	}
}

#endif

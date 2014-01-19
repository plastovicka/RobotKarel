/*
	(C) 1999-2003  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include "hdr.h"
#pragma hdrstop

#ifdef DOS

#include "menu.h"
#include "karel.h"

#define cmenu 0x70
#define cokno 0x7f
#define cclose 0x7a
#define cokraj2 0x7e

Tbutton Ok("  OK  ", 1), Storno("&Storno", 0), Ano(" &Ano  ", 1), Ne("  &Ne  ", 2),
Cancel("&Cancel", 0), Yes(" &Yes  ", 1), No("  &No  ", 2);

Tmenuitem *sender;

static char *buf=new char[4000]; //for menu and dialog windows

//------------------------------------------------------
void * operator new(size_t, void *p)
{return p; }

#define Mwinfo 5
static int Dwinfo;
static struct text_info winfo[Mwinfo];

void window1(int x, int y, int xm, int ym)
{
	if(Dwinfo>=Mwinfo) exit(9);///
	gettextinfo(winfo+Dwinfo);
	Dwinfo++;
	window(x, y, xm, ym);
}

void window0()
{
	struct text_info *u;

	Dwinfo--;
	u=winfo+Dwinfo;
	window(u->winleft, u->wintop, u->winright, u->winbottom);
}

//restore background when window is moved
//new coordinates, old coordinates, screen buffer
void scrput(int x1, int y1, int xm1, int ym1, int x2, int y2, int xm2, int ym2,
 char *w)
{
	int l, r, i;

	if(x1<x2) l=xm1+1, r=xm2;
	else l=x2, r=x1-1;
	if(y1<y2){
		if(l<=r) for(i=y2; i<=ym1; i++) puttext(l, i, r, i, w+i*160+l*2-162);
		for(i=ym1+1; i<=ym2; i++) puttext(x2, i, xm2, i, w+i*160+x2*2-162);
	}
	else{
		for(i=y2; i<y1; i++) puttext(x2, i, xm2, i, w+i*160+x2*2-162);
		if(l<=r) for(; i<=ym2; i++) puttext(l, i, r, i, w+i*160+l*2-162);
	}
}

char pismeno(char *s)
{
	while(*s && *s++!='&');
	return toupper(*s);
}

int strflen(char *s)
{
	int i=0;

	while(*s){
		if(*s!='&') i++;
		s++;
	}
	return i;
}

void putsf(char *s, int cl0, int cl1)
{
	bool f=false;

	textattr(cl0);
	putch(' ');
	while(*s){
		if(*s=='&'){ f=true; textattr(cl1); }
		else{
			putch(*s);
			if(f){ f=false; textattr(cl0); }
		}
		s++;
	}
	putch(' ');
}

void putsfmenu(char *name, bool sel, bool dis)
{
	static int b[2][2]={{0x70, 0x20}, {0x78, 0x28}},
						 c[2][2]={{0x74, 0x24}, {0x78, 0x28}};
	putsf(name, b[dis][sel], c[dis][sel]);
}

Tmainmenu::Tmainmenu(Tmenu *m)
{
	int i=2;

	s=menus=m;;
	for(; m->name; m++){
		m->x=i;
		m->last=0;
		i+=strflen(m->name)+2;
	}
	zamenu=m;
	rozvito=false;
}

bool Tmainmenu::najdimenu(char c)
{
	c=toupper(c);
	for(Tmenu *u=menus; u->name; u++)
		if(pismeno(u->name)==c){
			rozbal(u);
			return true;
		}
	return false;
}

Tmenuitem * Tmainmenu::najdiitem(char c)
{
	c=toupper(c);
	for(Tmenuitem *u=s->items; u->name; u++){
		if(pismeno(u->name)==c) return u;
	}
	return 0;
}

void Tmainmenu::menuclick(int x)
{
	for(Tmenu *u=menus+1; u->name && x>=u->x; u++);
	rozbal(--u);
}

void Tmainmenu::printmenu(bool sel)
{
	gotoxy(s->x, 1);
	putsfmenu(s->name, sel, false);
}

void Tmainmenu::test()
{
	int z[26], zm[26];
	int c, i;

	for(i=0; i<26; i++) zm[i]=0;
	for(Tmenu *u=menus; u->name; u++){
		c=pismeno(u->name);
		if(c && (c<'A' || c>'Z' || zm[c-'A'])){
			cprintf("Hlavn¡ menu obsahuje v¡cekr t p¡smeno %c\r\n", c);
			abort();
		}
		zm[c-'A']=1;

		for(i=0; i<26; i++) z[i]=0;
		for(Tmenuitem *t=u->items; t->name; t++){
			c=pismeno(t->name);
			if(c && (c<'A' || c>'Z' || z[c-'A'])){
				cprintf("Menu %s obsahuje v¡cekr t p¡smeno %c\r\n", u->name, c);
				abort();
			}
			z[c-'A']=1;
		}
	}
}

void Tmainmenu::printitem(int r, bool sel)
{
	int i;
	Tmenuitem *item=&s->items[r];

	if(*item->name=='-'){
		gotoxy(wx+1, wy+1+r);
		textattr(cmenu);
		putch('Ã');
		for(i=wx+3; i<wxm; i++) putch('Ä');
		putch('´');
	}
	else{
		gotoxy(wx+2, wy+1+r);
		putsfmenu(item->name, sel, item->enabled && !item->enabled());
		if(item->shortcut){
			for(i=wxm-2-wherex()-strlen(item->shortcut); i>0; i--) putch(' ');
			cputs(item->shortcut);
			putch(' ');
		}
		else for(i=wxm-1-wherex(); i>0; i--) putch(' ');
	}
}

void Tmainmenu::tisk()
{
	gotoxy(1, 1);
	textattr(cmenu); putch(' ');
	for(Tmenu *u=menus; u->name; u++){
		putsfmenu(u->name, false, false);
	}
	textattr(cmenu); clreol();
}


void Tmainmenu::rozbal(Tmenu *menu)
{
	Tmenuitem *u;
	int i, sirka, sirkam;
	bool rozvito1=rozvito;

	if(s!=menu || !rozvito1){
		zabal();
		rozvito=true;
		s=menu;
		//window size
		sirkam=0;
		for(poc=0, u=menu->items; u->name; poc++, u++){
			sirka=strflen(u->name)+5;
			if(u->shortcut) sirka+=strflen(u->shortcut)+5;
			if(sirka>sirkam) sirkam=sirka;
		}
		if(menu->last>=poc) menu->last=0;
		r=menu->last;
		//place window
		wx=menu->x-1;
		if(wx+sirkam>80) wx=80-sirkam;
		wxm=wx+sirkam; wym=poc+3; wy=2;
		//show
		gettext(wx, wy, wxm, wym, buf);
		textattr(cmenu);
		gotoxy(wx, wy);
		putch(' '); putch('Ú');
		for(i=3; i<sirkam; i++) putch('Ä');
		putch('¿'); putch(' ');
		for(i=wy+1; i<wym; i++){
			gotoxy(wx, i); putch(' '); putch('³');
			gotoxy(wxm-1, i); putch('³'); putch(' ');
		}
		gotoxy(wx, wym);
		putch(' '); putch('À');
		for(i=3; i<sirkam; i++) putch('Ä');
		putch('Ù'); putch(' ');
		for(i=0; i<poc; i++) printitem(i, i==r);
		printmenu(true);
		//run
		if(!rozvito1) run();
	}
}

void Tmainmenu::zabal()
{
	if(rozvito){
		rozvito=false;
		s->last=r;
		puttext(wx, wy, wxm, wym, buf);
	}
	printmenu(false);
}

int Tmainmenu::vyber(int rad)
{
	if(r!=rad){
		printitem(r, false);
		r=rad;
		if(s->items[rad].name[0]=='-') return 1;
	}
	printitem(rad, true);
	return 0;
}

bool Tmainmenu::alt(int c)
{
	if(c>15 && c<26) c="QWERTYUIOP"[c-16];
	else if(c>29 && c<39) c="ASDFGHJKL"[c-30];
	else if(c>43 && c<51) c="ZXCVBNM"[c-44];
	else return false;
	return najdimenu(c);
}

//call function from menu
void Tmainmenu::enter()
{
	zabal();
	sender=&s->items[r];
	if(sender->cmd) wm_command(sender->cmd);
}

void Tmainmenu::runF10()
{
	int x, y;
	int c;

	printmenu(true);
	for(;;){
		c=cekej8(x, y);
		if(c){
			if(c<5){
				if(y==1) menuclick(x);
				else printmenu(false);
				return;
			}
		}
		else switch(c=getch()){
			default:
				if(najdimenu(c)) return;
				break;
			case 13:
				rozbal(s);
				return;
			case 27://ESC
				printmenu(false);
				return;
			case 0:
				switch(c=getch()){
					default:
						if(alt(c)) return;
						break;
					case 75://LEFT
						printmenu(false);
						if(s==menus) s=zamenu;
						s--;
						printmenu(true);
						break;
					case 77://RIGHT
						printmenu(false);
						s++;
						if(!s->name) s=menus;
						printmenu(true);
						break;
					case 80://DOWN
						rozbal(s);
						return;
				}
		}
	}
}

void Tmainmenu::run()
{
	int x, y;
	char c;
	Tmenuitem *u;

	for(;;){
		while((c=cekej8(x, y))!=0){
			if(button()){
				if(y==1) menuclick(x);
				else if(y>wy && y<wym && x>wx && x<wxm) vyber(y-wy-1);
				else printitem(r, false);
			}
			else if(y!=1 && c<5){
				zabal();
				if(y>wy && y<wym && x>wx && x<wxm) enter();
				return;
			}
		}
		switch(c=getch()){
			default:
				if((u=najdiitem(c))!=0) r=(int)(u-s->items);
				else break;
				//!
			case 13://ENTER
				enter();
				return;
			case 27://ESC
				zabal();
				return;
			case 0:
				switch(c=getch()){
					default:
						alt(c);
						break;
					case 75://LEFT
						rozbal(s==menus ? zamenu-1 : s-1);
						break;
					case 77://RIGHT
						rozbal((s+1)->name ? s+1 : menus);
						break;
					case 72://UP
						while(vyber(r ? r-1 : poc-1));
						break;
					case 80://DOWN
						while(vyber(r==poc-1 ? 0 : r+1));
						break;
					case 71://HOME
					case 73://PAGEUP
						vyber(0);
						break;
					case 79://END
					case 81://PAGEDOWN
						vyber(poc-1);
						break;
				}
		}
	}
}

//---------------------------------------------------------------

Tokno::Tokno(Tcomponent **co, int c, char *n, int w, int h, char *t) :
components(co), last(co+c-1), name(n), sirka(w), vyska(h), text(t)
{
}

void Tokno::okraj()
{
	int i, j, l;

	window1(wx, wy, wxm, wym+1);
	textattr(cokno);
	cputs("ÉÍ["); textattr(cclose); putch('þ');
	textattr(cokno); putch(']');
	j=strlen(name)+2;
	l=(sirka-j)/2-5; if(l<1) l=1;
	for(i=l; i>0; i--) putch('Í');
	putch(' '); cputs(name); putch(' ');
	for(i=sirka-6-j-l; i>0; i--) putch('Í');
	putch('»');
	for(i=2; i<vyska; i++){
		gotoxy(1, i); putch('º');
		gotoxy(sirka, i); putch('º');
	}
	putch('È');
	for(i=2; i<sirka; i++) putch('Í');
	putch('¼');
	window0();
}

void Tokno::okraj2()
{
	int i;

	window1(wx, wy, wxm, wym+1);
	textattr(cokraj2);
	putch('Ú');
	for(i=2; i<sirka; i++) putch('Ä');
	putch('¿');
	for(i=2; i<vyska; i++){
		gotoxy(1, i); putch('³');
		gotoxy(sirka, i); putch('³');
	}
	putch('À');
	for(i=2; i<sirka; i++) putch('Ä');
	putch('Ù');
	window0();
}

void Tokno::paint()
{
	Tcomponent **u;
	char *s, *s1, c;
	int i, j, i1;

	window1(wx+1, wy+1, wxm-1, wym-1);
	textattr(0x70);
	clrscr();
	window(wx+2, wy+2, wxm, wym);
	if(text){
		i=1; j=1;
		for(s=text; *s; s++){
			c=*s;
			if(i>wxm-wx-3){
				if(c!=' '){
					s1=s; i1=i;
					while(*s!=' ' && i>1) s--, i--;
					if(i){
						gotoxy(i, j);
						for(; i<i1; i++) putch(' ');
					}
					else s=s1-1;
				}
				c='\n';
			}
			putch(c);
			if(c=='\n'){ putch('\r'); i=1; if(++j>wym-wy-4) break; }
			else if(c=='\r') i=1;
			else i++;
		}
	}
	for(u=components; u<=last; u++) (*u)->paint(0);
	okraj();
	window0();
}

void Tokno::setvar()
{
	Tcomponent **u;
	for(u=components; u<=last; u++) (*u)->setvar();
}

void Tokno::hide()
{
	puttext(1, 1, 80, 25, buf);
	window0();
	_setcursortype(_NOCURSOR);
}


int Tokno::show()
{
	int x, y, x1, y1, xd, yd, curx, cury, b, wx1, wy1, wxm1, wym1;
	int c, cup;
	Tcomponent **u;

	//center window
	if(sirka>80) sirka=80;
	if(vyska>24) vyska=24;
	wx=(82-sirka)/2; wxm=wx+sirka-1; wy=(27-vyska)/2; wym=wy+vyska-1;
	//save background and show window
	gettext(1, 1, 80, 25, buf);
	window1(wx+2, wy+2, wxm-2, wym);

	for(u=components; u<=last; u++) (*u)->getvar();
	paint();
	activ=components;
	(*activ)->paint(2);
	for(;;){
		while((c=cekej8(x, y))!=0){
			if(c>6) continue;
			if(y==wy && x>wx+1 && x<wx+5){ hide(); return 0; }
			if(y==wy || y==wym || x==wx || x==wxm){
				curx=wherex(); cury=wherey();
				okraj2();
				gotoxy(curx, cury);
				mouse8xy(x, y);
				do{//move window
					x1=x, y1=y;
					mouse8xy(x, y);
					xd=x-x1; yd=y-y1;
					if(xd!=0 || yd!=0){
						wx1=wx, wy1=wy, wxm1=wxm, wym1=wym;
						wx+=xd; wxm+=xd; wy+=yd; wym+=yd;
						if(wx<1) wxm-=wx-1, wx=1;
						if(wy<1) wym-=wy-1, wy=1;
						if(wxm>80) wx-=wxm-80, wxm=80;
						if(wym>24) wy-=wym-24, wym=24;
						movetext(wx1, wy1, wxm1, wym1, wx, wy);
						scrput(wx, wy, wxm, wym, wx1, wy1, wxm1, wym1, buf);
						window(wx+2, wy+2, wxm-2, wym);
						gotoxy(curx, cury);
					}
				} while(button()==1);
				okraj();
				gotoxy(curx, cury);
			}
			else if(c==1){//mouse click
				x-=wx+1; y-=wy+1;
				for(u=components; u<=last; u++){
					if(x>=(*u)->x && x<=(*u)->xm && y>=(*u)->y && y<=(*u)->ym){
						(*activ)->paint(0);
						activ=u;
						if((*u)->mod<0) (*u)->mouse(x-(*u)->x, y-(*u)->y);
						else{//Button
							(*activ)->paint(3);
							b=1;
							while((c=cekej8(x, y))!=3)
								if(c>6){
									x-=wx+1; y-=wy+1;
									b= x>=(*u)->x && x<=(*u)->xm && y>=(*u)->y && y<=(*u)->ym;
									(*activ)->paint(2+b);
								}
							if(b){
								hide();
								if((*u)->mod) setvar();
								return (*u)->mod;
							}
						}
						(*u)->paint(1);
						break;
					}
				}
			}
		}
		c=getch();
		if(!c) c=256+getch();
		cup=c; if(c>='a' && c<='z') cup-=('a'-'A');
		if(cup>='A' && cup<='Z' && (*activ)->mod!=-2)
			for(u=components; u<=last; u++)
				if(pismeno((*u)->name)==cup){
					(*activ)->paint(0);
					activ=u;
					c= (*activ)->mod!=-2 ? ' ' : 0;
					(*activ)->paint(2);
					break;
				}
		if((*activ)->mod<0){
			if(c && (*activ)->key(c)){
				c=0;
				(*activ)->paint(1);
			}
		}
		else if(c==' ' || c==13){//Button
			hide();
			if((*activ)->mod) setvar();//OK
			return (*activ)->mod;
		}
		switch(c){
			case 27://ESC
				hide();
				return 0;
			case 13://ENTER
			case 10://CTRL-ENTER
				setvar();
				hide();
				return 1;
			case 9://TAB
			case 333://right
			case 336://down
				(*activ)->paint(0);
				if(activ<last) activ++; else activ=components;
				(*activ)->paint(2);
				break;
			case 271://SHIFT+TAB
			case 331://left
			case 328://up
				(*activ)->paint(0);
				if(activ>components) activ--; else activ=last;
				(*activ)->paint(2);
				break;
		}
	}
}

//---------------------------------------------------------------

Tbutton::Tbutton(int x1, int y1, char *n, int m)
{
	mod=m; name=n; set(x1, y1);
}

void Tbutton::set(int x1, int y1)
{
	x=x1; y=y1; xm=x1+strflen(name)+1; ym=y1;
}

Tcheckbox::Tcheckbox(int x1, int y1, int l, char *n, int &v) :
var(v)
{
	checked=0;
	x=x1; y=y1; xm=x1+l-1; ym=y1; mod=-1; name=n;
}

Tgroup::Tgroup(int x1, int y1, int l, char *n, int &v, char **i, int c) :
var(v), items(i), count(c)
{
	item=0;
	x=x1; y=y1; xm=x1+l-1; ym=y1+c; mod=-1; name=n;
}

Tedit::Tedit(int x1, int y1, int l, char *n, bool c) :
length(l), cr(c)
{
	ins=false;
	smaz();
	zac=c ? 0 : strflen(n)+2;
	x=x1; y=y1; xm=x1+zac+l-1; ym=y+c; mod=-2; name=n;
}



void Tbutton::paint(int activ)
{
	int i;

	gotoxy(x, y);
	textattr(0x70);
	if(activ==3) putch(' ');
	putsf(name, activ ? 0x2f : 0x20, 0x2e);
	textattr(0x70);
	if(activ!=3) putch('Ü');
	gotoxy(x+1, y+1);
	for(i=x; i<=xm; i++) putch(activ==3 ? ' ' : 'ß');
}

void Tcheckbox::paint(int activ)
{
	gotoxy(x, y);
	textattr(activ ? 0x3f : 0x30);
	cputs(" ["); putch(checked ? 'X' : ' '); putch(']');
	putsf(name, activ ? 0x3f : 0x30, 0x3e);
	for(int i=xm-wherex(); i>=0; i--) putch(' ');
}

bool Tcheckbox::key(int k)
{
	if(k==' '){ checked=!checked; return true; }
	else return false;
}

void Tcheckbox::mouse(int, int)
{
	checked=!checked;
}

void Tcheckbox::setvar()
{
	var=checked;
}

void Tcheckbox::getvar()
{
	checked=var;
}

void Tgroup::paint(int activ)
{
	int i, j;

	gotoxy(x, y);
	putsf(name, activ ? 0x7f : 0x70, 0x7e);
	for(i=0; i<count; i++){
		textattr(activ && i==item ? 0x3f : 0x30);
		gotoxy(x, y+i+1);
		cputs(" ("); putch(i==item ? '.' : ' '); cputs(") ");
		cputs(items[i]);
		for(j=xm-wherex(); j>=0; j--) putch(' ');
	}
}

bool Tgroup::key(int k)
{
	if(k==328 || k==331){
		if(item>0) item--; else item=count-1;
	}
	else if(k==336 || k==333 || k==' '){
		if(item<count-1) item++; else item=0;
	}
	else return false;
	return true;
}

void Tgroup::mouse(int, int my)
{
	if(my>0 && my<=count) item=my-1;
}

void Tgroup::setvar()
{
	var=item;
}

void Tgroup::getvar()
{
	item=var;
}

void Tedit::paint(int activ)
{
	char *u;
	int j;

	_setcursortype(_NOCURSOR);
	gotoxy(x, y);
	putsf(name, activ ? 0x7f : 0x70, 0x7e);
	if(activ==2) select=true;
	if(activ==0) select=false;
	if(cur<posun) posun=cur;
	if(cur-posun>length-3) posun=cur-length+3;
	gotoxy(x+zac, y+cr);
	textattr(0x1f);
	putch(posun ? '' : ' ');
	if(select) textattr(0x2f);
	for(u=text+posun, j=x+zac+1; *u && j<xm; u++, j++){
		if(*u!=13 && *u!=10 && *u!=8 && *u!=7) putch(*u); else putch(' ');
	}
	textattr(0x1f);
	if(!*u) while(j++ <= xm) putch(' ');
	else putch('');
	if(activ){
		gotoxy(x+zac+1+cur-posun, y+cr);
		_setcursortype(ins ? _SOLIDCURSOR : _NORMALCURSOR);
	}
}

void Tedit::smaz()
{
	*text=0;
	cur=0;
	posun=0;
	select=false;
}

bool Tedit::key(int k)
{
	int l=strlen(text);
	if(k>=' ' && k<256){
		if(select) smaz();
		if(l<EDSIZE){
			if(!ins || !text[cur]) memmove(text+cur+1, text+cur, l+1-cur);
			text[cur++]=k;
		}
	}
	else if(k==331){//LEFT
		if(cur>0) cur--;
	}
	else if(k==333){//RIGHT
		if(text[cur]) cur++;
	}
	else if(k==327){//HOME
		cur=0;
	}
	else if(k==335){//END
		cur=l;
	}
	else if(k==339){//DEL
		if(text[cur]) memmove(text+cur, text+cur+1, l-cur);
	}
	else if(k==8){//BACKSPACE
		if(cur>0){
			cur--;
			memmove(text+cur, text+cur+1, l-cur);
		}
	}
	else if(k==338){//INS
		ins=!ins;
	}
	else if(k==403){//CTRL+DEL
		smaz();
	}
	else return false;
	select=false;
	return true;
}

void Tedit::mouse(int mx, int my)
{
	int i;

	if(my==cr && mx>=zac){
		if(mx==zac){
			if(posun){
				posun--;
				if(cur>posun+length-3) cur--;
			}
		}
		else if(mx==zac+length-1){
			if(strlen(text+posun)>length-3){
				posun++;
				if(cur<posun) cur++;
			}
		}
		else{
			i=mx-zac-1+posun;
			if(i<=cur) cur=i;
			else for(i-=cur; i>0 && text[cur]; i--, cur++);
			select=false;
		}
	}
}

void Teditn::setvar()
{
	if(maxlength<=EDSIZE) text[maxlength-1]=0;
	strcpy(var, text);
}

void Teditn::getvar()
{
	ins=false;
	smaz();
	strncpy(text, var, EDSIZE);
	select=true;
}

//---------------------------------------------------------------

int showmodal(Tcomponent **co, int c, char *n, int w, int h, char *t)
{
	char okno[sizeof(Tokno)];
	int wn=strlen(n)+10;
	if(wn>w) w=wn;
	return (new(&okno) Tokno(co, c, n, w, h, t))->show();
}

void oknotxt(char *name, char *text, int w, int h)
{
	Tcomponent *button=&Ok;

	Ok.set(w/2-6, h-4);
	showmodal(&button, 1, name, w, h, text);
}

void oknostr(char *name, char *text)
{
	int l=strlen(text), h=7, w;
	if(l<50) w=l+4;
	else if(l<700) w=50, h+=l/43;
	else{
		w=80, h+=l/73;
		if(h>23) h=23;
	}
	oknotxt(name, text, w, h);
}

int oknoano(char *name, char *text)
{
	int i, w;
	Tcomponent *buttons[3];

	switch(lang){
		case 0:
			buttons[0]=&Ano; buttons[1]=&Ne; buttons[2]=&Storno;
			break;
		case 1:
			buttons[0]=&Yes; buttons[1]=&No; buttons[2]=&Cancel;
			break;
	}
	w=strlen(text)+4;
	i=strlen(name)+10;
	if(i>w) w=i;
	if(34>w) w=34;
	i=(w-34)/4;
	((Tbutton*)buttons[0])->set(i+2, 3);
	((Tbutton*)buttons[1])->set(i*2+12, 3);
	((Tbutton*)buttons[2])->set(i*3+22, 3);
	return showmodal(buttons, 3, name, w, 7, text);
}

//title, message, width, buffer size, result
int oknoed(char *name, char *text, int l, int m, void *out)
{
	int w, r, z;
	static char editn[sizeof(Teditn)];
	Tcomponent *components[3];

	switch(lang){
		case 0:
			components[1]=&Ok; components[2]=&Storno;
			break;
		case 1:
			components[1]=&Ok; components[2]=&Cancel;
			break;
	}
	z=strflen(text)+2;
	r=(z+l>50);
	w=l+4+(!r)*z;
	if(w<24) w=24;
	components[0]=(Tcomponent*)&editn;
	new(&editn) Teditn(1, 1, l, text, (char *)out, m, (bool)r);
	((Tbutton*)components[1])->set((w-24)/3+2, 3+r);
	((Tbutton*)components[2])->set((w-24)/3*2+12, 3+r);
	return showmodal(components, 3, name, w, 7+r, 0);
}

int oknoint(char *name, char *text, int &var)
{
	char buf[17];
	int ok;

	if(var) itoa(var, buf, 10);
	else *buf=0;
	ok=oknoed(name, text, 10, sizeof(buf), buf);
	if(ok){
		var=atoi(buf);
		if(buf[0]=='\'') var=buf[1];
	}
	return ok;
}

int oknofile(char *name, char *ext, char *var)
{
	int ok, l;

	if(!(ok=oknoed(name, lng(524, "File name:"), 25, 76, var)))
		return 0;
	l=strlen(var);
	if(!l) return 0;
	//append file extension
	if(l<5 || !strchr(var+l-4, '.')){
		memcpy(var+l, ext, 5);
	}
	return ok;
}

void oknochyb(char *text)
{
	oknostr(lng(36, "Error"), text);
}

#endif

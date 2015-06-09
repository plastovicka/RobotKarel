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

#define maxdelr 999
#define blok 1024

bool ctrlq, ctrlp, ctrlk;
bool ins; //insert mode
bool selecting, scrolling;
int lastc; //last key
int scrollDelay=200;

int fsmer=1, fod=1, fcase=0, fword=0; //options in Find dialog
char ffind[64], freplace[64];

char *makra[27];
const char CR1=CR;

//editor window size
int xmax=46, ymax=23;
//---------------------------------------------------------------------------
void Ttext::setRect(int x1, int y1)
{
	xmax=x1;
	ymax=y1;
	bool b=false;
	if(x>xmax){
		x=xmax;
		b=true;
	}
	if(y>ymax){
		y=ymax;
		b=true;
	}
	if(b){ hideCursor(); setcur(); showCursor(); }
}

/*
 delete delLen bytes, then allocate insLen bytes
 dr is line count difference
 return false if error
 */
bool Ttext::zmena(size_t delLen, size_t insLen, int dr)
{
	Tundo *u=0;
	char *s;
	int g=0;

	if(delLen==0 && insLen==0) return false;
	delpreklad(); //delete compiled code when source is modified

	if(undoCur && groupUndo && !dr){
		if(insLen==0 && undoCur->ins==0){
			if(delLen==1 && unsigned(cur.u-txt.u) == undoCur->offset){ //delete
				//append deleted character to previous string
				s= new char[undoCur->del+1];
				if(s){
					memcpy(s, undoCur->str, undoCur->del);
					s[undoCur->del]= *cur.u;
					undoCur->del++;
					delete[] undoCur->str;
					undoCur->str=s;
					g=1;
				}
			}
			else if((cur.u-txt.u)+delLen == undoCur->offset){ //backspace
				//prepend deleted character to previous string
				s= new char[undoCur->del+delLen];
				if(s){
					undoCur->offset-=delLen;
					undoCur->x-=delLen;
					memcpy(s+delLen, undoCur->str, undoCur->del);
					memcpy(s, cur.u, delLen);
					undoCur->del+=delLen;
					delete[] undoCur->str;
					undoCur->str=s;
					g=1;
				}
			}
		}
		else if(delLen==0 && insLen==1 && undoCur->del==0 &&
			unsigned(cur.u-txt.u) == undoCur->offset+undoCur->ins){ //insert char
			undoCur->ins++;
			g=2;
		}
		else if(delLen==1 && insLen==1 && undoCur->del==undoCur->ins &&
			unsigned(cur.u-txt.u) == undoCur->offset+undoCur->ins){ //overwrite
			//append overwritten character to previous string
			s= new char[undoCur->del+1];
			if(s){
				memcpy(s, undoCur->str, undoCur->del);
				s[undoCur->del]= *cur.u;
				undoCur->del++;
				undoCur->ins++;
				delete[] undoCur->str;
				undoCur->str=s;
				g=1;
			}
		}
	}

	if(!g){
		//create new undo item
		jechyba++; //suppress error message if there is no memory for undo
		do{
			u=new Tundo;
		} while(!u && !freemem());
		if(u){
			u->offset= (size_t)(cur.u-txt.u);
			u->r= cur.r;
			u->x=x;
			u->del=delLen;
			u->ins=insLen;
			u->dr=dr;
			u->str=0;
			u->pre=0;
			//save deleted text for undo
			if(delLen){
				do{
					u->str= new char[delLen];
				} while(!u->str && !freemem());
				if(!u->str){ delete u;  u=0; }
				else{ memcpy(u->str, cur.u, delLen); }
			}
		}
		jechyba--;
	}

	if(!memory((long)insLen-delLen, dr)){
		//not enough memory
		if(!g) delete u;
		else if(g==2 && undoCur) undoCur->ins--;
		return false;
	}

	//delete redo list
	if(undoCur!=undoFirst){
		if(undoCur){
			undoCur->pre->nxt=0;
			undoCur->pre=0;
		}
		else{
			undoLast=0;
		}
		dellist(undoFirst);
		undoFirst=undoCur;
	}

	//add new item to undo list
	if(u){
		u->nxt=undoFirst;
		undoCur=u;
		undoFirst=u;
		if(undoLast) u->nxt->pre=u; else undoLast=u;
	}
	if(!g) modif++;
	return true;
}


bool Ttext::freeUndo()
{
	Tundo *u= undoLast;

	if(!u){
		//undo list is empty
		chyba(lng(800, "Not enough memory"));
		delpreklad();
		return true;
	}
	if(undoCur==u || !undoCur){
		//there is one item in undo list -> delete redo
		dellist(undoFirst);
		undoCur=undoLast=0;
	}
	else{
		//delete last item in undo list
		undoLast= u->pre;
		undoLast->nxt=0;
		delete u;
	}
	return false;
}

bool Ttext::lzezpet()
{
	return undoCur!=0;
}

bool Ttext::lzevpred()
{
	return undoCur!=undoFirst;
}

void Ttext::novy1()
{
	//put CR to both beginning and end of text
	txt.u[-1]=CR;
	if(txt.u[delka-1]!=CR || !delka) txt.u[delka++]=CR, rpoc++;

	txt.r=1;
	cur=top=select1=select2=txt;
	x=y=1;
	posunx=0;
	zaCR=0;
	modif=0;
	lst();
	for(int i=0; i<10; i++) bookmark[i].y=1, bookmark[i].pos=txt;
	undoCur=undoFirst=undoLast=0;
}

void Ttext::novy()
{
	txt.u= new char[delkaa=8];
	if(!txt.u) exit(1);
	txt.u++;
	delka=rpoc=0;
	*jmeno=0;
	novy1();
}

int Ttext::save()
{
	FILE *f;

	if(!*jmeno)
		if(!oknofile(lng(20, "Save"), EXT, jmeno, 1)) return 0;
	if((f=fopen(jmeno, "wt"))==0){
		chyba(lng(733, "Cannot create file"));
		return 0;
	}
	if(fwrite(txt.u, 1, delka, f)!=delka){
		chyba(lng(734, "Disk write error"));
		return 0;
	}
	else if(!undoSave){
		dellist(undoFirst);
		undoCur=undoLast=0;
	}
	fclose(f);
	modif=0;
	return 1;
}

int Ttext::ulozmodif()
{
	int b=1;

	if(modif){
		b=oknoano(lng(21, "KAREL - Close file"),
			lng(750, "Save changes to file ?"));
		if(b==1) b=save();
	}
	return b;
}

void Ttext::zavri1()
{
	delpreklad();
	smazbreakpointy();
	dellist(undoFirst);
	undoCur=undoLast=0;
	delete[](txt.u-1);
}

int Ttext::zavri()
{
	if(!ulozmodif()) return 0;
	zavri1();
	return 1;
}

void Ttext::otevri()
{
	FILE *f;
	int c;
	char *u;
	long l;

	if((f=fopen(jmeno, "rt"))==0){
		chyba(lng(739, "File not found")); novy();
	}
	else{
		fseek(f, 0, SEEK_END);
		l=ftell(f)+3; //CR at beginning; CR,0 at end
		rewind(f);
		if(l>0xffff){
			chyba(lng(753, "File is too long !"));
			novy();
		}
		else if((u= new char[(size_t)l])==0){
			chyba(lng(770, "Not enough memory for file"));
			novy();
		}
		else{
			delkaa=(size_t)l;
			txt.u=++u;
			//read entire file to memory, count lines
			rpoc=0;
			while((c=fgetc(f))!=EOF){
				if(c=='\n') rpoc++;
				*u++= (char)c;
			}
			delka=(size_t)(u-txt.u);
			novy1();
		}
		fclose(f);
	}
}

#define REL(p,u) p=p+u

void opravukaz(Tpos &var, char *u, int r, long du, int dr, int b)
{
	if(var.u-u>=b){
#ifdef DOS
		if(du<0 && var.u<u-du) var.u=u;
		else var.u+=du;
#else
		if(du<0 && (long)var.u<(long)u-du) var.u=u;
		else REL(var.u, du);
#endif
	}
	if(var.r>r || var.r==r && var.u-u>=b){
		if(dr<0 && var.r<r-dr) var.r=r;
		else var.r+=dr;
	}
}

bool Ttext::memory2(char *u, int r, long pocet, int dr)
{
	char *newtxt=0;
	long newd, p, du;
	Tbreakp *b;
	int i;
	char *uzac=u;

	if(pocet){
		//correct breakpoints
		while(*--uzac==' ');
		for(b=breakp.nxt; b; b=b->nxt){
			if(b->text==this  &&  b->r > r-(*uzac==CR))
				if(dr<0 && b->r<r-dr) b->r=r;
				else b->r+=dr;
		}
		//allocate new memory by blocks
		newd=pocet+delka+2;
		if(newd+rpoc+dr>0xfffe){
			chyba(lng(771, "File is 64KB long !"));
			return false;
		}
		if(pocet<0){
			memmove(u, u+(size_t)(-pocet), delka-(size_t)(-pocet)-(size_t)(u-txt.u)+1);
		}
		p=blok - newd % blok;
		if(newd+p>0xfffc) p=0;
		if((size_t)newd > delkaa){ //allocate new block of size newa
			while((newtxt=(char *)realloc(txt.u-1, (size_t)(newd+p)))==0){
				if(!p) return false;
				if(freemem()) p/=2; //shorten blocks if memory is low
			}
		}
		else if(delkaa-newd>blok)  //free memory
		 newtxt=(char *)realloc(txt.u-1, (size_t)(newd+p));
		if(newtxt){
			delkaa=(size_t)(newd+p);
			newtxt++;
			du=(long)newtxt-(long)txt.u;
			if(du){ //realloc moved text
#ifdef DOS
				u+=du; cur.u+=du; top.u+=du;
				pagenxt.u+=du; select1.u+=du; select2.u+=du;
				for(i=0;i<10;i++) bookmark[i].pos.u+=du;
#else
				REL(u, du); REL(cur.u, du); REL(top.u, du);
				REL(pagenxt.u, du); REL(select1.u, du); REL(select2.u, du);
				for(i=0; i<10; i++) REL(bookmark[i].pos.u, du);
#endif
				txt.u=newtxt;
			}
		}
		if(pocet>0){
			memmove(u+(size_t)pocet, u, delka-(size_t)(u-txt.u)+1);
			delka+=(size_t)pocet;
		}
		else{
			delka-=(size_t)(-pocet);
		}
		//correct pointers
		rpoc+=dr;
		//don't change cur,txt
		opravukaz(top, u, r, pocet, dr, 1);
		opravukaz(pagenxt, u, r, pocet, dr, 1);
		opravukaz(select1, u, r, pocet, dr, 1);
		opravukaz(select2, u, r, pocet, dr, 1);
		for(i=0; i<10; i++) opravukaz(bookmark[i].pos, u, r, pocet, dr, 0);
	}
	return true;
}

bool Ttext::memory(long pocet, int dr)
{
	return memory2(cur.u, cur.r, pocet, dr);
}

//---------------------------------------------------------------------------
//x -> cur.u, zaCR
void Ttext::setux()
{
	int j;

	zaCR=0;
	for(j=1; j<x; j++, cur.u++)
		if(*cur.u==CR){
			zaCR=x-j;
			break;
		}
}

//[x,y] -> cur,zaCR
void Ttext::setcur()
{
	int j;
	char *u=top.u;

	aminmax(y, 1, rpoc-top.r+1);
	for(j=1; j<y; j++){
		while(*u++!=CR);
	}
	cur.u=u;
	cur.r=y+top.r-1;
	setux();
}

//cur -> [x,y]
void Ttext::setxy()
{
	char *u;

	y=cur.r-top.r+1;
	for(x=1, u=cur.u; *--u!=CR; x++);
	zaCR=0;
}

void Ttext::homecur()
{
	while(*--cur.u!=CR);
	cur.u++;
}

void notrunr()
{
	rprg=0;
	if(text.runr) text.repaint();
}

//find and overwrite a keyword
int findKeyword(char *u)
{
	for(int l=0; keywords[l]; l++)
	 for(int i=0; i<Dkeyw; i++)
		if(prikcmp(u, keywords[l][i])) return i;
	return -1;
}

int syntaxattr(char *u)
{
	if(!neoddel[*u]) return clsym;
	if(*u>='0' && *u<='9'){
		while(*u>='0' && *u<='9') u++;
		if(!neoddel[*u]) return clnum;
	}
	return findKeyword(u)<0 ? cliden : clkey;
}

char *Ttext::tiskeol1(char *u, int rad)
{
	int i, cl, clr=0;

	//line color
	rad+=top.r-1;
	for(Tbreakp *br=breakp.nxt; br; br=br->nxt){
		if(rad==br->r && br->text==this) clr=clbrkp;
	}
	if(rad==rprg){ clr=clrun; runr=true; }
	char *v=u;
	if(neoddel[*v]) backslovo(v);
	cl=syntaxattr(v);
	while(*v!=CR) v--;
	while(v++<u){
		if(*v=='/' && v[1]=='/') clr=clkom;
	}
	i=1;
	while(*u!=CR && i<=xmax){
		textColor((u>=select1.u && u<select2.u) ? clsel : (clr ? clr : cl));
		putCh(*u!=13 && *u!=10 && *u!=7 && *u!=8 ? *u : ' ');
		i++;
		u++;
		if(*u=='/' && u[1]=='/' && !clr)  clr=clkom;
		if(neoddel[*u] != neoddel[u[-1]])  cl=syntaxattr(u);
	}
	if(i<=xmax){
		//fill spaces to end of line
		textColor((u>=select1.u && u<select2.u) ? clsel : (clr ? clr : cl));
		clrEol();
	}
	else{
		//line is too long
		while(*u!=CR) u++;
	}
	return(u+1);
}

void Ttext::tiskrad()
{
	gotoXY(1, y);
	tiskeol1(cur.u+posunx-x+1+zaCR, y);
}

//redraw window, set variable pagenxt
void Ttext::repaint()
{
	int i, j;
	char *u=top.u;

	dirty=false;
	runr=false;

	for(j=1; j<=ymax && u<txt.u+delka; j++){
		for(i=0; i<posunx && *u!=CR; i++) u++;
		gotoXY(1, j);
		u=tiskeol1(u, j);
	}
	pagenxt.u=u; pagenxt.r=j+top.r-1;

	//erase rest of this page
	textColor(clsym);
	while(j<=ymax){
		gotoXY(1, j);
		clrEol();
		j++;
	}
	//scrollbar
	setScroll(1, rpoc, top.r, ymax);
}

void Ttext::gotor(int r2)
{
	x=1;
	aminmax(r2, 1, rpoc);
	if(r2>=top.r && r2<top.r+ymax){
		y=r2-top.r+1;
	}
	else{
		if(r2<top.r) top=txt;
		while(top.r < r2-ymax/3){
			while(*top.u++!=CR);
			top.r++;
		}
		y=r2-top.r+1;
		lst();
	}
	setcur();
}

void Ttext::setTop(int r)
{
	aminmax(r, 1, rpoc);
	if(r<top.r) top=txt;
	while(top.r < r){
		while(*top.u++!=CR);
		top.r++;
	}
	setcur();
	lst();
}

void Ttext::gotour(char *u, int r, int d)
{
	int i;

	if(r<top.r || r>=top.r+ymax){
		//line is out of visible page
		lst();
		top.u=u; top.r=r;
		y=1;
		for(x=1; *--top.u!=CR; x++);
		for(i=1; i<7 && top.u>=txt.u; i++){
			while(*--top.u!=CR);
			top.r--; y++;
		}
		top.u++;
	}
	if(u!=cur.u){
		cur.u=u; cur.r=r;
		setxy();
	}

	//cursor must be inside window
	if(x-posunx>xmax-d){
		posunx=x-xmax+d;
		lst();
	}
	if(x<=posunx){
		posunx=x-1;
		lst();
	}
}

//delete block at position u,r; its length is du bytes and dr lines
void Ttext::smazat1(char *u, int r, size_t du, int dr)
{
	int zaCR2=0;

	if(du>0){
		gotour(u, r);
		if(cur.u[du]==CR){
			zaCR=0;
			while(*--cur.u==' '){
				du++; zaCR2++;
			}
			cur.u++;
		}
		if(zmena(du, zaCR, -dr)){
			while(zaCR){ *cur.u++=' '; zaCR--; }
		}
		if(dr) lst();
		else{ zaCR=zaCR2; tiskrad(); }
	}
}

void Ttext::smazat(char *za, int dr)
{
	smazat1(cur.u, cur.r, size_t(za-cur.u), dr);
}

//insert block src; length is du bytes; append m spaces
bool Ttext::vlozit(size_t du, int dr, const char *src, int m)
{
	int rm;

	if(*cur.u==CR){ //don't put spaces after end of line
		m=0;
		if(du) for(const char *u=src+du-1; u>=src && *u==' '; u--, du--);
	}
	if(du+m>0){
		if(overwrblok && !persistblok) deleteBlock();
		rm=0;
		if(du && *src==CR){ //delete spaces before CR if new line is inserted
			zaCR=0;
			while(*--cur.u==' ') rm++;
			cur.u++;
		}
		if(zmena(rm, zaCR+du+m, dr)){
			while(zaCR){ *cur.u++=' '; zaCR--; } //insert spaces if position was after end of line
			char *u=cur.u;
			memcpy(u, src, du);
			u+=du;
			while(m--) *(u++)=' ';
		}
		else return false;
		if(dr) lst(); else tiskrad();
	}
	return true;
}

void Ttext::znak(char c)
{
	bool ok;

	if(x<maxdelr){
		if(!ins || *cur.u==CR) ok=vlozit(1, 0, &c);
		else{ ok=zmena(1, 1); *cur.u=c; tiskrad(); }
		//move cursor right
		if(ok) right();
	}
	else oknochyb(lng(772, "Line is too long !"));
}

bool Ttext::jeblok()
{
	return select1.u<select2.u;
}

void Ttext::findpre(char *u, int r, int replace)
{
	fsmer= !fsmer;
	findn(u, r, replace);
	fsmer= !fsmer;
}

void Ttext::findn(char *u, int r, int replace)
{
	char *v, *w, c, buf[sizeof(ffind)];
	int s, i, lf, lr, lr1, lr2;
	int found=0;
	char t[32];

	lf=strlen(ffind); lr=strlen(freplace);
	for(lr1=lr, v=freplace+lr-1; *v==' '; v--, lr1--);//without trailing spaces
	if(!lf) return;
	if(fsmer){//down
		s=1;
		for(v=ffind, w=buf; *v; w++, v++) *w= fcase ? *v : uppism[*v];
	}
	else{//up
		s=-1;
		for(v=ffind, w=buf+lf-1; *v; w--, v++) *w= fcase ? *v : uppism[*v];
	}
	buf[lf]=0;
	c=*buf;
	for(;;){
		//find the first character
		do{
			if(u[-!fsmer]==CR){
				r+=s;
				if(fsmer ? (u>=txt.u+delka-1) : (u<txt.u)){
					//end of file
					gotour(cur.u, cur.r);
					repaint();
					if(!found){
						oknochyb(lng(773, "Search string not found"));
					}
					else if(replace>1){
						sprintf(t, lng(774, "%d replaces made."), found);
						oknotxt(lng(22, "Replace"), t, strlen(t)+4, 7);
					}
					return;
				}
			}
			u+=s;
		} while((fcase ? *u : uppism[*u])!=c || neoddel[u[-s]] && fword);
		//compare strings
		for(v=buf; *v==(fcase ? *u : uppism[*u]); v++, u+=s);
		if(*v || neoddel[*u] && fword) continue; //they are different
		found++;
		if(fsmer) u-=lf; else u++;
		//select found string
		if(replace>1) cur.u=u, cur.r=r;//replace all -> don't redraw
		else{
			select1.u=u; select2.u=u+lf; select1.r=select2.r=r;
			posunx=0;
			gotour(u, r, lf);
			if(replace && x+lf>23 && y>8 && y<16){//hidden under window
				posunx=x-23+lf;
				if(x<=posunx) posunx=x-1;
			}
		}
		if(!replace){ lst(); zrussel=false; return; }
		//replace
		i= replace>1 ? 1 : (repaint(), oknoano(lng(22, "Replace"),
			lng(775, "Replace this occurence ?")));
		if(!i){ //cancel
			lst();
			return;
		}
		if(i==1){//yes
			lr2= cur.u[lf]==CR ? lr1 : lr; //don't put spaces after end of line
			if(lr2){
				if(zmena(lf, lr2)) memcpy(cur.u, freplace, lr2);
			}
			else{
				smazat(cur.u+lf, 0);
			}
			u=cur.u; r=cur.r;
		}
	}
}

void Ttext::find(int replace)
{
	if(fod){ //entire scope
		if(fsmer) findn(txt.u, txt.r, replace);
		else findn(txt.u+delka-1, rpoc, replace);
	}
	else{ //from cursor
		findn(cur.u, cur.r, replace);
	}
}

bool Ttext::makro(int dr, int m)
{
	int l;
	char *s;

	if(isalpha(lastc) && (cur.u[-2]==' ' || cur.u[-2]==CR)){
		s=makra[tolower(lastc)-'a'];
		if((l=strlen(s)) !=0){
			cur.u[-1]=*s;
			if(dr) s[l]=CR;
			if(vlozit(l-1+dr, dr, s+1, m)){
				x--; cur.u--;
				if(dr) s[l]=0; else tiskrad();
				x+=l; cur.u+=l;
				return true;
			}
			s[l]=0;
		}
	}
	return false;
}


void Ttext::sourad()
{
	if(!jechyba && !isWatch){
		statusMsg(clstatus, "%c%6d:%-5d%s", modif ? '*' : ' ',
			cur.r, x,
			ctrlq ? "^Q" : (ctrlk ? "^K" : (ctrlp ? "^P" : "")));
	}
}

//---------------------------------------------------------------------------
void Ttext::edPrepare()
{
	curl=cur;
	isWatch=0;
}

void Ttext::showCursor()
{
	if(mode!=MODE_TEXT) return;
	gotoXY(x-posunx, y);
	::showCursor(ins);
}

void Ttext::edDone()
{
	if(!persistblok && zrussel) cancelBlock();
	if(x-posunx<=0){
		posunx=x-1;
		lst();
	}
	else if(x-posunx>xmax){
		posunx=x-xmax;
		lst();
	}
	sourad();
	if(dirty && !keyPressed()) repaint();
}
//---------------------------------------------------------------------------
void Ttext::cancelBlock(){
	if(select2.u>select1.u){
		if(opacselect) select1=select2;
		else select2=select1;
		lst();
	}
}
void Ttext::sel(){
	if(cur.u!=curl.u){
		if(opacselect){
			if(select1.u!=curl.u){ select1=curl; opacselect=false; }
		}
		else{
			if(select2.u!=curl.u) select1=curl;
		}
		endOfBlock();
	}
	zrussel=false;
}
void Ttext::up(){
	if(cur.r>1){
		if(y==1) scrollUp();
		y--;
		cur.r--;
		homecur();
		cur.u--;
		homecur();
		setux();
	}
}
void Ttext::selUp(){
	up();
	sel();
}
void Ttext::scrollUp(){
	if(top.r>1){
		top.r--;
		top.u--;
		while(*--top.u!=CR);
		top.u++;
		if(y<ymax){
			y++;
		}
		setcur();
		lst();
	}
}
void Ttext::down(){
	if(cur.r<rpoc){
		if(y==ymax) scrollDown();
		y++;
		cur.r++;
		while(*cur.u!=CR) cur.u++;
		cur.u++;
		setux();
	}
}
void Ttext::selDown(){
	down();
	sel();
}
void Ttext::scrollDown(){
	if(top.r+ymax<=rpoc){
		top.r++;
		while(*top.u!=CR) top.u++;
		top.u++;
		if(y>1){
			y--;
		}
		setcur();
		lst();
	}
}
void Ttext::left(){
	if(x>1){
		x--;
		if(zaCR) zaCR--; else cur.u--;
	}
}
void Ttext::selLeft(){
	left();
	sel();
}
void Ttext::wordLeft(){
	if(zaCR){
		x-=zaCR;
		zaCR=0;
	}
	else{
		do{
			cur.u--;
		} while(!neoddel[*cur.u] && *cur.u!=CR);
		if(*cur.u!=CR) backslovo(cur.u);
		else{
			if(cur.r>1){
				if(cur.r==top.r){
					char *u=cur.u;
					scrollUp();
					cur.u=u;
				}
				cur.r--;
			}
			else cur.u=txt.u;
		}
		setxy();
	}
}
void Ttext::selWordLeft(){
	wordLeft();
	sel();
}
void Ttext::right(){
	if(!zaCR || x<maxdelr){
		if(*cur.u==CR) zaCR++;
		else cur.u++;
		x++;
	}
}
void Ttext::selRight(){
	right();
	sel();
}
void Ttext::wordRight(){
	if(*cur.u==CR){
		if(cur.r<rpoc){
			if(y==ymax) scrollDown();
			x=1; y++; cur.r++; cur.u++; zaCR=0;
		}
	}
	else{
		cur.u++;
		x+=1+skipslovo(cur.u);
	}
	while(!neoddel[*cur.u] && *cur.u!=CR) cur.u++, x++;
}
void Ttext::selWordRight(){
	wordRight();
	sel();
}
void Ttext::pageDown(){
	if(top.r+ymax<=rpoc){
		top=pagenxt;
		if(y>rpoc-top.r+1) y=rpoc-top.r+1;
	}
	else y=rpoc-top.r+1;
	setcur();
	lst();
}
void Ttext::selPageDown(){
	pageDown();
	sel();
}
void Ttext::pageBottom(){
	x=1; y=ymax;
	setcur();
}
void Ttext::selPageBottom(){
	pageBottom();
	sel();
}
void Ttext::pageUp(){
	if(top.r-ymax>1){
		top.r-=ymax;
		for(int i=0; i<=ymax; i++){
			while(*--top.u!=CR);
		}
		top.u++;
	}
	else if(top.r>1){
		top=txt;
	}
	else{
		x=1, y=1;
	}
	setcur();
	lst();
}
void Ttext::selPageUp(){
	pageUp();
	sel();
}
void Ttext::pageTop(){
	x=y=1;
	cur=top;
	zaCR=0;
}
void Ttext::selPageTop(){
	pageTop();
	sel();
}
void Ttext::lineStart(){
	x=1;
	zaCR=0;
	homecur();
}
void Ttext::selLineStart(){
	lineStart();
	sel();
}
void Ttext::editorTop(){
	top=cur=txt;
	x=1; y=1;
	lst();
}
void Ttext::selEditorTop(){
	editorTop();
	sel();
}
void Ttext::lineEnd(){
	if(zaCR){
		x-=zaCR;
		zaCR=0;
	}
	else while(*cur.u!=CR){
		cur.u++;
		x++;
	}
}
void Ttext::selLineEnd(){
	lineEnd();
	sel();
}
void Ttext::editorBottom(){
	char *u;
	u= cur.u = txt.u+delka-1;
	for(x=1; *--u!=CR; x++);
	if(top.r+ymax>rpoc){
		y=rpoc-top.r+1;
		cur.r=rpoc;
		zaCR=0;
	}
	else{
		for(int i=1; i<ymax; i++){
			while(*--u!=CR);
		}
		top.u=u+1;
		top.r=rpoc-ymax+1;
		y=ymax;
		setcur();
		lst();
	}
}
void Ttext::selEditorBottom(){
	editorBottom();
	sel();
}
void Ttext::toggleMode(){
	ins=!ins;
}
void Ttext::copy(){
	if(jeblok()){
		clipboardCopy(select1.u, (size_t)(select2.u-select1.u));
	}
	zrussel=false;
}
void Ttext::cut(){
	copy();
	deleteBlock();
}
void Ttext::paste(){
	if(x<maxdelr){
		size_t n, i;
		bool del;
		char *s=clipboardPaste(n, del);
		if(s){
			int r=0;
			for(i=0; i<n; i++){
				if(s[i]==CR) r++;
			}
			vlozit(n, r, s);
			if(del) delete[] s;
		}
	}
}
void Ttext::deleteChar(){
	if(overwrblok && !persistblok && select2.u>select1.u)
		deleteBlock();
	else if(cur.u<txt.u+delka-1){
		smazat(cur.u+1, *cur.u==CR);
	}
}
void Ttext::deleteLastChar(){
	int j, m;
	char *u;

	if(x==1){
		if(cur.u>txt.u) smazat1(cur.u-1, cur.r-1, 1, 1);
	}
	else{
		j=1;
		if(unindent){
			for(u=cur.u-1; *u==' '; u--);
			if(*u==CR){
				do{
					m=1;
					if(u<txt.u) break;
					u--;
					if(*u==CR) m=999;
					else while(*u!=CR){
						if(*u==' ') m++; else m=1;
						u--;
					}
				} while(m>=x);
				j=x-m;
			}
		}
		x-=j;
		if(zaCR>=j) zaCR-=j;
		else{
			j-=zaCR; zaCR=0;
			smazat1(cur.u-j, cur.r, j, 0);
		}
	}
}
void Ttext::deleteLastWord(){
	if(zaCR){ x-=zaCR; zaCR=0; }
	else if(x>1){
		char *u=cur.u;
		if(cur.u[-1]==' ') do{ cur.u--; x--; } while(cur.u[-1]==' ');
		x-=backslovo(cur.u);
		if(u==cur.u) cur.u--, x--;
		smazat(u, 0);
	}
}
void Ttext::undo(){
	Tundo *u=undoCur, *uf;
	int m;
	int g;

	if(lzezpet()){
		if(cur.r!=u->r || x-u->x>1 || u->x-x>1){
			//set cursor to last change
			gotour(txt.u+u->offset, u->r);
			while(x>u->x) left();
			while(x<u->x) right();
		}
		else{
			gotour(txt.u+u->offset, u->r);

			//save redo list, so that it will not be discarded in function zmena
			uf=undoFirst;
			undoFirst= undoCur= u->nxt;
			if(undoCur) undoCur->pre=0; else undoLast=0;

			m=modif;
			g=groupUndo;
			groupUndo=0;
			if(zmena(u->ins, u->del, -u->dr)){
				if(u->del) memcpy(txt.u+u->offset, u->str, u->del);
				modif= 2*m-modif;
			}
			groupUndo=g;

			//restore redo list
			if(undoFirst){
				undoCur=undoFirst->nxt;
				undoFirst->pre= u->pre;
				if(u->pre){ u->pre->nxt=undoFirst; undoFirst=uf; }
			}
			else if(u->pre){
				u->pre->nxt=0;
				dellist(uf);
			}

			//skip spaces which were appended to end of line
			while(x>u->x) left();
			while(x<u->x) right();

			//item u was changed
			delete u;
			lst();
		}
	}
}
void Ttext::redo(){
	Tundo *u, *uf;
	int g;

	if(lzevpred()){
		if(undoCur) u=undoCur->pre; else u=undoLast;
		gotour(txt.u+u->offset, u->r);

		//save redo list, so that it will not be discarded in function zmena
		uf=undoFirst;
		undoFirst= undoCur;
		if(undoCur) undoCur->pre=0; else undoLast=0;

		g=groupUndo;
		groupUndo=0;
		if(zmena(u->ins, u->del, -u->dr)){
			if(u->del) memcpy(txt.u+u->offset, u->str, u->del);
		}
		groupUndo=g;

		//restore redo list
		if(undoFirst){
			undoCur=undoFirst;
			undoFirst->pre= u->pre;
			if(u->pre){ u->pre->nxt=undoFirst; undoFirst=uf; }
		}
		else if(u->pre){
			u->pre->nxt=0;
			dellist(uf);
		}

		while(x>u->x) left();
		while(x<u->x) right();
		delete u;
		lst();
	}
}

void scanLineBack(char *&ptr, int &di, int &endFound, bool &begFound){
	int c, olddi=di;
	char *u=ptr;
start:
	begFound=false;
	endFound=0;
	for(; *u!=CR; u--){
		if(!neoddel[*u]){
			if(u[0]=='/' && u[1]=='/'){
				u--;
				di=olddi;
				goto start;
			}
			if(*u!=' ') endFound=0;
		}
		else{
			endFound=0;
			backslovo(u);
			c=findKeyword(u);
			if(c>=0){
				if(c==KDYZ || c==DOKUD || c==OPAKUJ || c==ZVOLIT ||
					c==PROCEDURE || c==FUNKCE || c==PODMINKA){
					di++;
					if(di>0) begFound=true;
				}
				else if(c==KONEC || c==AZ){
					di--;
					endFound=1;
				}
				else if(c==JINAK || c==PRIPAD){
					endFound=2;
				}
			}
		}
	}
	ptr=u;
}
void Ttext::enter(bool insKonec){
	//m:=indent size, j:= 1 if new line is inserted before current line
	int i, j, l, m, endFound, di;
	char *u, *uzac;
	bool begFound;

	if(indent){
		di=0;
		u=cur.u;
		if(*u==CR) u--;
		scanLineBack(u, di, endFound, begFound);
		uzac=u+1;
		if(endFound){
			di=0;
			while(u>txt.u){
				u--;
				scanLineBack(u, di, endFound, begFound);
				if(begFound){
					for(m=0; *(++u)==' '; m--);
					for(u=uzac; *u==' '; m++, u++);
					if(m>0){
						i=x;
						smazat1(uzac, cur.r, m, 0);
						x=i-m;
						setux();
					}
					break;
				}
			}
		}
	}
	u=cur.u;
	while(*u==CR && u>txt.u) u--;
	while(*--u!=CR);
	u++;
	uzac=u;
	for(m=1; *u==' '; m++, u++);
	j= (u>=cur.u);
	if(j) cur.u=uzac;
	i= j ? 0 : m-1;
	//insert CR and spaces
	if(insKonec){
		if(!makro(1, i)){
			if(zaCR && cur.u[-1]==CR){ deleteLastChar(); m=x; }
			char *s=makra[26];
			if((l=strlen(s)) !=0){
				s[l]=CR;
				if(vlozit(l+1, 1, s, i)) x+=l; cur.u+=l;
				s[l]=0;
			}
		}
	}
	else{
		vlozit(1, 1, &CR1, i);
	}
	//transfer to next line
	if(!j || !entern){
		if(y==ymax) scrollDown();
		y++;
	}
	x=m;
	setcur();
	if(indent){
		di=0;
		u=cur.u-1;
		do{
			assert(*u==CR);
			if(--u<=txt.u) return;
			scanLineBack(u, di, endFound, begFound);
		} while(endFound==2);
		if(endFound==1) di++;
		for(x=1+di*tabs; *(++u)==' '; x++);
		setux();
	}
}
void Ttext::lineBreak(){
	enter(false);
}
void Ttext::blockEnd(){
	enter(true);
}
void Ttext::tab(){
	if(tabs<=0) tabs=1;
	if(tabs>80) tabs=80;
	if(makro(0, 1)) right();
	else if(vlozit(0, 0, 0, tabs))  for(int i=tabs; i>0; i--) right();
}
void Ttext::upperLower(){
	char *u;
	size_t d= (size_t)(select2.u-select1.u);

	if(d>0){
		gotour(select1.u, select1.r);
		zmena(d, d);
		for(u=select1.u; u<select2.u; u++){
			if(*u!=uppism[*u]) *u=uppism[*u]; else *u=downpism[*u];
		}
		lst();
		zrussel=false;
	}
}
void Ttext::endOfBlock(){
	if(opacselect)
		if(cur.u>=select2.u){
			select1=select2;
			select2=cur;
			opacselect=false;
		}
		else{
			select1=cur;
		}
	else
		if(cur.u>=select1.u){
			select2=cur;
		}
		else{
			select2=select1;
			select1=cur;
			opacselect=true;
		}
		lst();
		zrussel=false;
}
void Ttext::selectAll(){
	editorTop();
	selEditorBottom();
}
void Ttext::blockIndent(){
	if(select2.u>select1.u){
		gotour(select2.u, select2.r);
		while(cur.r>select1.r){
			cur.u--; if(*cur.u==CR) cur.r--;
			homecur();
			if(*cur.u!=CR){
				if(zmena(0, 1, 0)) *cur.u=' ';
			}
		}
		gotour(select2.u, select2.r);
		lst();
		zrussel=false;
	}
}
void Ttext::blockUnindent(){
	if(select2.u>select1.u){
		gotour(select2.u, select2.r);
		while(cur.r>select1.r){
			cur.u--; if(*cur.u==CR) cur.r--;
			homecur();
			if(*cur.u==' '){
				zmena(1, 0, 0);
			}
		}
		gotour(select2.u, select2.r);
		lst();
		zrussel=false;
	}
}
void Ttext::insertLine(){
	while(*--cur.u!=CR);
	cur.u++;
	lineBreak();
}
void Ttext::deleteWord(){
	char *u=cur.u;
	if(*cur.u!=CR){
		if(*u==' ') while(*u==' ') u++;
		else skipslovo(u);
		if(u==cur.u) u++;
		smazat(u, 0);
	}
	else if(cur.r<rpoc){
		while(*++u==' ');
		smazat(u, 1);
	}
}
void Ttext::deleteLine(){
	char *u=cur.u;
	x=1;
	zaCR=0;
	homecur();
	while(*u++!=CR);
	if(cur.r==rpoc) u--; //don't delete CR at end of file
	smazat(u, cur.r!=rpoc);
}
void Ttext::deleteEOL(){
	char *u;
	for(u=cur.u; *u!=CR; u++);
	smazat(u, 0);
}
void Ttext::gotoMarker(int which){
	top=cur=bookmark[which].pos;
	for(x=1; *--top.u!=CR; x++);
	int i=bookmark[which].y - 1;
	for(y=1; i>0; i--, y++){
		if(top.u<txt.u) break;
		while(*--top.u!=CR);
		top.r--;
	}
	top.u++;
	lst();
}
void Ttext::gotoMarker0(){
	gotoMarker(0);
}
void Ttext::gotoMarker1(){
	gotoMarker(1);
}
void Ttext::gotoMarker2(){
	gotoMarker(2);
}
void Ttext::gotoMarker3(){
	gotoMarker(3);
}
void Ttext::gotoMarker4(){
	gotoMarker(4);
}
void Ttext::gotoMarker5(){
	gotoMarker(5);
}
void Ttext::gotoMarker6(){
	gotoMarker(6);
}
void Ttext::gotoMarker7(){
	gotoMarker(7);
}
void Ttext::gotoMarker8(){
	gotoMarker(8);
}
void Ttext::gotoMarker9(){
	gotoMarker(9);
}
void Ttext::setMarker(int which){
	bookmark[which].pos=cur;
	bookmark[which].y=y;
}
void Ttext::setMarker0(){
	setMarker(0);
}
void Ttext::setMarker1(){
	setMarker(1);
}
void Ttext::setMarker2(){
	setMarker(2);
}
void Ttext::setMarker3(){
	setMarker(3);
}
void Ttext::setMarker4(){
	setMarker(4);
}
void Ttext::setMarker5(){
	setMarker(5);
}
void Ttext::setMarker6(){
	setMarker(6);
}
void Ttext::setMarker7(){
	setMarker(7);
}
void Ttext::setMarker8(){
	setMarker(8);
}
void Ttext::setMarker9(){
	setMarker(9);
}
void Ttext::nextLine(){
	if(cur.r<rpoc){
		if(y==ymax) scrollDown();
		y++;
		cur.r++;
		while(*cur.u!=CR) cur.u++;
		cur.u++;
		x=1;
	}
}
void Ttext::beginOfBlock(){
	if(select2.u>select1.u) lst();
	select1=select2=cur;
}
void Ttext::selectWord(){
	opacselect=false;
	select1.r=select2.r=cur.r;
	select2.u=cur.u;
	skipslovo(select2.u);
	x-=backslovo(cur.u);
	select1.u=cur.u;
	lst();
	zrussel=false;
}
void Ttext::deleteBlock(){
	if(jeblok()){
		smazat1(select1.u, select1.r, size_t(select2.u-select1.u), select2.r-select1.r);
	}
}
void Ttext::ctrlP(){
	ctrlp=true;
	zrussel=false;
}
void Ttext::ctrlQ(){
	ctrlq=true;
	zrussel=false;
}
void Ttext::ctrlK(){
	ctrlk=true;
	zrussel=false;
}
void Ttext::findNext(){
	findn(cur.u, cur.r, 0);
	zrussel=false;
}
void Ttext::findPrev(){
	findpre(cur.u, cur.r, 0);
	zrussel=false;
}
void Ttext::gotoLine(){
	int i=cur.r;
	if(oknoint(lng(23, "Go To Line Number"),
		 lng(210, "Enter New Line Number"), i)){
		gotor(i);
	}
}
void Ttext::newFile(){
	if(zavri()) novy();
}
void Ttext::openFile(){
	if(ulozmodif())
		if(oknofile(lng(24, "Open"), EXT, jmeno, 0)){
			zavri1();
			otevri();
		}
}
void Ttext::saveFile(){
	save();
}
void Ttext::saveAsFile(){
	if(oknofile(lng(25, "Save as"), EXT, jmeno, 2))  save();
}


typedef void (Ttext::*TtextFunc)();
TtextFunc textFuncTab[]={
	&Ttext::left, &Ttext::right, &Ttext::up, &Ttext::down, &Ttext::wordLeft,
	&Ttext::wordRight, &Ttext::scrollUp, &Ttext::scrollDown, &Ttext::lineStart, &Ttext::lineEnd,
	&Ttext::pageUp, &Ttext::pageDown, &Ttext::editorTop, &Ttext::editorBottom, &Ttext::pageTop,
	&Ttext::pageBottom, &Ttext::lineBreak, &Ttext::deleteLastChar, &Ttext::deleteChar, &Ttext::toggleMode,
	&Ttext::deleteLastWord, &Ttext::selLeft, &Ttext::selRight, &Ttext::selUp, &Ttext::selDown,
	&Ttext::selWordLeft, &Ttext::selWordRight, &Ttext::selLineStart, &Ttext::selLineEnd, &Ttext::selPageUp,
	&Ttext::selPageDown, &Ttext::selEditorTop, &Ttext::selEditorBottom, &Ttext::selPageTop, &Ttext::selPageBottom,
	&Ttext::copy, &Ttext::cut, &Ttext::paste, &Ttext::undo, &Ttext::redo,
	&Ttext::tab, &Ttext::upperLower, &Ttext::endOfBlock, &Ttext::selectAll, &Ttext::blockIndent,
	&Ttext::blockUnindent, &Ttext::insertLine, &Ttext::deleteWord, &Ttext::deleteLine, &Ttext::deleteEOL,
	&Ttext::gotoMarker0, &Ttext::gotoMarker1, &Ttext::gotoMarker2, &Ttext::gotoMarker3, &Ttext::gotoMarker4,
	&Ttext::gotoMarker5, &Ttext::gotoMarker6, &Ttext::gotoMarker7, &Ttext::gotoMarker8, &Ttext::gotoMarker9,
	&Ttext::setMarker0, &Ttext::setMarker1, &Ttext::setMarker2, &Ttext::setMarker3, &Ttext::setMarker4,
	&Ttext::setMarker5, &Ttext::setMarker6, &Ttext::setMarker7, &Ttext::setMarker8, &Ttext::setMarker9,
	&Ttext::nextLine, &Ttext::beginOfBlock, &Ttext::selectWord, &Ttext::deleteBlock, &Ttext::ctrlP,
	&Ttext::ctrlQ, &Ttext::ctrlK, &Ttext::findNext, &Ttext::findPrev, &Ttext::gotoLine,
	&Ttext::newFile, &Ttext::openFile, &Ttext::saveFile, &Ttext::saveAsFile, &Ttext::blockEnd,
	&Ttext::copy, &Ttext::cut, &Ttext::paste
};

void Ttext::key(int c)
{
	zrussel=true;
	if(ctrlq){
		ctrlq=false;
		c=downpism[c];
		if(c=='y' || c==25){
			deleteEOL();
		}
		else if(c=='f' || c==6){
			najit();
		}
		else if(c=='a' || c==1){
			nahradit();
		}
		else if(c>='0' && c<='9'){
			gotoMarker(c-'0');
		}
	}
	else if(ctrlp){
		ctrlp=false;
		if(c!=13 && c!=10) znak((char)c); //special character
	}
	else if(ctrlk){
		ctrlk=false;
		c=downpism[c];
		if(c>0 && c<27) c='a'+c-1;
		if(c>='0' && c<='9'){
			setMarker(c-'0');
		}
		else if(c=='b' || c=='h'){
			beginOfBlock();
		}
		else if(c=='s') save();
		else if(c=='k') endOfBlock();
		else if(c=='t') selectWord();
		else if(c=='i') blockIndent();
		else if(c=='u') blockUnindent();
		else if(c=='a') upperLower();
	}
	else{
		znak((char)c);
		lastc=c;
	}
}

void Ttext::command(int cmd)
{
	if(cmd>=1000+sizeA(textFuncTab)) return;
	zrussel=true;
	(this->*(textFuncTab[cmd-1000]))();
	lastc=0;
}

void Ttext::selScroll()
{
	if(y<2){
		scrollUp();
		y=1;
	}
	else{
		scrollDown();
		y=ymax;
	}
	setcur();
	endOfBlock();
}

void Ttext::mouse(int mx, int my, int button)
{
	bool isOut= my<=0 || my>ymax;
	if((isOut || mx>xmax || mx<=0) && !selecting) return;
	mx+=posunx;
	if(selecting){
		amin(mx, 1);
		if(scrolling && (!isOut || button==3)){
			scrolling=false;
			killTimer(10);
		}
		else if(isOut){
			int d;
			static int d0;
			if(my<=0) d=1-my;
			else d=my-ymax;
			if(d!=d0 || !scrolling){
				d0=d;
				setTimer(10, max(20, scrollDelay/d));
			}
			scrolling=true;
		}
		aminmax(my, 1, ymax);
		if(x!=mx || y!=my){
			beforeCmd();
			x=mx; y=my;
			setcur();
			endOfBlock();
			lst();
			afterCmd();
		}
		if(button==3) selecting=false;
	}
	else if(button==1){//left mouse button
		beforeCmd();
		if(x==mx && y==my){ //select word
			selectWord();
		}
		else{ //move cursor
			zrussel=true;
			x=mx; y=my;
			setcur();
			beginOfBlock();
			selecting=true;
		}
		afterCmd();
	}
}


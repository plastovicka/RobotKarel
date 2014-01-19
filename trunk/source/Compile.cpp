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

#define MCSTACK 50
#define Dglobal 256
#define Dlocal 16

uchar preklad,   //pointer to compiled code
*tab;      //table to convert source line => compiled address
int Dprocedur;   //number of procedures
size_t Dpreklad; //compiled code length

Tascii uppism, downpism, pism, neoddel;

//stack for block commands
struct Tcstk
{
	int kod;      //which command
	uchar adr;    //begin of block
	Tjmp *adrjmp; //jump instruction
} *csp, cstack[MCSTACK];


//these variables are set during lexical analysis
static int term, lex_num;
static Tvar *lex_var;
static Tslovo *lex_slovo;
static unsigned key;
static char *keystr;

static int typ;     //expression type

//hash tables
static Tslovo *global[Dglobal]; //functions and keywords
static Tvar *local[Dlocal];     //local variables

static uchar uexe; //pointer to compiled code
static char *uerr, //syntax error position
*utxt; //pointer to source code
static int rexe,  //curently compiled line
rerr,
					 rtab;

//jump lists for true and false conditions
static Tjmp *Ts, *Fs;  //pointer to relative address in jump instruction
static bool jmpT;     //last condition, 1=IS, 0=NOT

//--------------------------------------------------
void cprintF(const char *s, ...)
{
	char buf[256];

	va_list ap;
	va_start(ap, s);
	vsprintf(buf, s, ap);
	putS(buf);
	va_end(ap);
}

char statusBuf[256];
int statusColor;

void paintStatus()
{
	textColor(statusColor);
	goto25();
	putS(statusBuf);
	endOfStatus();
}

void statusMsg(int color, const char *text, ...)
{
	va_list ap;
	va_start(ap, text);
	statusBuf[0]=' ';
	vsprintf(statusBuf+1, text, ap);
	statusColor=color;
	paintStatus();
	va_end(ap);
	isWatch=0;
}

char *vformatStr(const char *s, va_list ap)
{
	const char *p;
	int i;
	char *buf, *dest;

	//alocate buffer
	i=strlen(s)+1;
	for(p=s; *p; p++) if(*p=='%') i+=20;
	buf=new char[i];

	//copy and format string
	for(dest=buf; *s; s++){
		if(*s=='%'){
			s++;
			if(*s=='w'){
				for(p=va_arg(ap, char *), i=0; neoddel[*p]; p++, i++){
					OemToCharBuff(p, dest, 1);
					dest++;
					if(i==16){
						*dest++='.'; *dest++='.'; *dest++='.';
						break;
					}
				}
			}
			else if(*s=='c'){
				*dest++= (char)va_arg(ap, int);
			}
			else if(*s=='d'){
				sprintf(dest, "%d", va_arg(ap, int));
				dest=strchr(dest, 0);
			}
			else if(*s=='%'){
				*dest++= '%';
			}
			else{
				s--;
				for(p=keywords[0][va_arg(ap, int)]; *p; p++){
					char ch = uppism[*p];
					OemToCharBuff(&ch, dest++, 1);
				}
			}
		}
		else{
			*dest++=*s;
		}
	}
	*dest=0;
	return buf;
}

char *formatStr(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	char *result = vformatStr(s, ap);
	va_end(ap);
	return result;
}

void vchyba(const char *s, va_list ap)
{
	char *buf= vformatStr(s, ap);
	statusMsg(clchyb, buf);
	delete[] buf;
	jechyba=1;
}

//print error message, every % in string s corresponds to a keyword code parameter 
void chyba(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	vchyba(s, ap);
	va_end(ap);
}

void cerror(int id, char *en, ...)
{
	va_list ap;

	va_start(ap, en);
	if(!jechyba){
		vchyba(lng(id, en), ap);
		//set cursor to error position
		if(*uerr==0){
			uerr--;
			rerr=text.rpoc;
		}
		text.gotor(rerr);
		text.cur.u=uerr;
		text.cur.r=rerr;
		text.setxy();
		text.lst();
	}
	va_end(ap);
}

void cerrch(int ch)
{
	if(term!=ch)
		cerror(801, "\"%c\" expected", ch);
}


int typsize(int typ)
{
	if(typ<=T_VOID) return 0;
	if(typ>=T_ARRAY) return typ-T_ARRAY+2;
	return 1;
}

//return true if words match
bool prikcmp(char *s1, char *s2)
{
	char *u1, *u2;

	if(s1==s2) return skipslovo(s1)>0;
	for(u1=s1, u2=s2; pism[*u1]==pism[*u2] && neoddel[*u2]; u1++, u2++);
	if(!(neoddel[*u1] || neoddel[*u2])){
		if(s1==u1) return false;//empty word
		for(u1=s1, u2=s2; neoddel[*u2]; u1++, u2++){
			*u1=*u2;
		}
		return true;
	}
	return false;
}

void dej(int c)
{
	if(preklad) *uexe= (unsigned char)c;
	uexe++;
}

void dejjmp(uchar u)
{
	if(preklad) *((Tjmp *)uexe)= (Tjmp)(u-uexe); //jumps are relative
	uexe+=DJMP;
}

void dejint(int i)
{
	if(preklad) *(int *)uexe = i;
	uexe+=sizeof(int);
}

void dejpdm(int z)
{
	dej(z);
	uexe+=DJMP;
	jmpT=true;
	typ=T_BOOL;
}

void addjmpadr(Tjmp* &list)
{
	if(preklad){
		Tjmp *adr = (Tjmp *)uexe - 1;
		if(list){
			*adr= (Tjmp)((uchar)list-(uchar)adr);
		}
		else{
			*adr=0;
		}
		list=adr;
	}
}

void addjmpadr2(Tjmp* &list, bool neg)
{
	if(preklad){
		addjmpadr(list);
		if(neg==jmpT) uexe[-DJMP-1]^=64;
	}
}

//write adr to address from list l0 to lk; then delete list
void zapisJMP1(Tjmp *l0, Tjmp *lk, uchar adr)
{
	Tjmp l;

	if(preklad){
		while(l0!=lk){
			assert((uchar)l0>=preklad && (uchar)l0<preklad+Dpreklad);
			l=*l0;
			*l0 = (Tjmp)(adr-(uchar)l0); //jumps are relative
			if(!l) break;
			l0= (Tjmp*)((uchar)l0 + l);
		}
	}
}

void zapisJMP(Tjmp *l)
{
	zapisJMP1(l, 0, uexe);
}

//skip white spaces and comments
void skip()
{
	int r;
	char *u;

skip1:
	if(*utxt==' '){ utxt++; goto skip1; }
	if(*utxt=='/' && utxt[1]=='*'){
		r=rexe; u=utxt;
		utxt++;
		do{
			if(*utxt++==CR){
				rexe++;
				if(!*utxt){
					rerr=r; uerr=u;
					cerror(802, "End of comment is missing");
					return;
				}
			}
		} while(*utxt!='*' || utxt[1]!='/');
		utxt+=2;
		goto skip1;
	}
	if(*utxt=='\\' && utxt[1]==CR){
		rexe++;
		utxt+=2;
		if(preklad){
			while(rtab<rexe) tab[++rtab]=uexe;
		}
		goto skip1;
	}
	if(*utxt=='/' && utxt[1]=='/'){
		while(*++utxt!=CR);
	}
}

void pretyp(int t)
{
	if(typ==T_BOOL1 && t==T_BOOL)  dejpdm(I_COND);
	if(typ!=t){
		cerror(803, "Type mismatch");
	}
}

void typ1()
{
	if(typ==T_BOOL || typ==T_BOOL1)
	 cerror(804, "Arithmetical operation is used with condition");
}

void typ2()
{
	if(typ==T_BOOL1) pretyp(T_BOOL);
	if(typ!=T_BOOL)
	 cerror(805, "Conjunction is used with arithmetical expression");
}

void hash()
{
	key=0;
	keystr=utxt;
	while(neoddel[*utxt]){
		key^= pism[*utxt++];
	}
}

//read unsigned integer and put it to lex_num
void cislo()
{
	long x=0;

	while(*utxt>='0' && *utxt<='9'){
		x= x*10 + (*utxt++ - '0');
		if(x>0x7fff){
			uerr=utxt;
			cerror(806, "Number is too big");
			return;
		}
	}
	if(neoddel[*utxt])
		cerror(807, "Space is expected after number");
	lex_num= int(x);
}

//read next terminal to variable term
void lex()
{
	Tslovo *s;
	Tvar *v;

	skip();
	uerr=utxt; rerr=rexe;
	term= *utxt;
	if(term>='0' && term<='9'){
		cislo();
		term=O_NUM;
	}
	else if(neoddel[term]){
		hash();
		for(v = local[key % Dlocal]; v; v=v->nxthash){
			if(prikcmp(keystr, v->jmeno)){
				lex_var=v;
				term=O_VAR;
				return;
			}
		}
		for(s = global[key % Dglobal]; s; s=s->nxthash){
			if(prikcmp(keystr, s->jmeno)){
				lex_slovo=s;
				term= -s->r;
				if(term<0 || term>=KROK) term=O_FCE;
				return;
			}
		}
		term=O_IDEN;
	}
	else if(term==0){
		term=O_EOF;
	}
	else{
		utxt++;
		if(term==CR){
			term=O_CR;
			rexe++;
		}
		else if(term=='\'' && *utxt!=CR){
			lex_num= *utxt++;
			uerr=utxt;
			term= *utxt++;
			cerrch('\'');
			term=O_NUM;
		}
		else if(term=='<'){
			if(*utxt=='=') utxt++, term=O_LE;
			if(*utxt=='>') utxt++, term=O_NE;
		}
		else if(term=='>'){
			if(*utxt=='=') utxt++, term=O_GE;
		}
	}
}

void pdmnebo();
void vyraz();


//parse parameters, put them on stack
//parenthesis are optional
//empty parenthesis are allowed if function has no parameters
void parametry(Tslovo *funkce)
{
	bool zav;
	int n= funkce->Nparam;
	Tvar *var= funkce->var;
	int typpar;

	if(funkce->result!=T_VOID && funkce->r>0)
		dej(I_NULA); //allocate variable for function result

	if(n || term=='('){
		zav= (term=='(');
		if(zav) lex();
		else if(funkce->result!=T_VOID) cerrch('(');

		while(n){
			//find out parameter type
			if(var){
				typpar=var->typ;
			}
			else{
				typpar=T_INT;
			}

			if(typpar==T_REF){
				//parameter is passed by reference
				if(term!=O_VAR){
					cerror(808, "Variable expected, parameter %w is a reference", var->jmeno);
				}
				else{
					if(lex_var->typ==T_REF){
						dej(I_LOAD);
					}
					else{
						if(lex_var->typ!=T_INT) cerror(809, "Parameter type mismatch");
						dej(I_REF);
					}
					dej(lex_var->offset);
					lex();
				}
			}
			else{
				//parameter is passed by value
				if(term==')' || term==O_CR || term==';')
					cerror(810, "Parameters expected");
				vyraz();
				if(-funkce->r==DELKA){
					typpar=T_ARRAY;
					while(typ>T_ARRAY){
						dej(I_POP);
						typ--;
					}
				}
				pretyp(typpar);
			}

			n-=typsize(typpar);
			if(n<=0) break;
			if(term==')'){
				cerror(811, "Too few parameters");
				break;
			}
			cerrch(',');
			lex();
			if(var) var=var->nxt;
		}
		if(term==',') cerror(812, "Too many parameters");
		if(zav){
			cerrch(')');
			lex();
		}
	}
}

void unarni()
{
	Tslovo *id;
	int o, i, n;

	switch(term){
		default:
			cerror(813, "Error in expression");
			break;

		case '+':
			lex();
			unarni();
			typ1();
			break;

		case '-':
			lex();
			if(term!=O_NUM){
				unarni();
				typ1();
				dej(I_NEG);
				break;
			}
			lex_num=-lex_num;
			//!
		case O_NUM:
			if(lex_num==0) dej(I_NULA);
			else if(lex_num==1) dej(I_JEDNA);
			else{
				dej(I_CONST);
				dejint(lex_num);
			}
			lex();
			typ=T_INT;
			break;

		case O_IDEN:
			uerr=keystr;
			cerror(814, "Unknown identifier %w", keystr);
			break;

		case O_VAR:
			typ=lex_var->typ;
			o=lex_var->offset;
			lex();

			if(term=='['){
				for(i=0, n=typ; term=='['; i++, n--){
					if(n<T_ARRAY){
						if(i) cerror(815, "Too many dimensions");
						else cerror(816, "Type of variable is not array");
					}
					lex();
					vyraz();
					pretyp(T_INT);
					cerrch(']');
					lex();
				}
				typ= n<T_ARRAY ? T_INT : n;
				dej(I_LOADA);
				dej(o);
				dej(i);
				dej(n-T_ARRAY+1);
			}
			else if(typ==T_REF){
				dej(I_LOADP);
				dej(o);
				typ=T_INT;
			}
			else{
				for(i=0; i<typsize(typ); i++){
					dej(I_LOAD);
					dej(o+i);
				}
			}
			break;

		case '(':
			lex();
			pdmnebo();
			cerrch(')');
			lex();
			break;

		case O_FCE:
			id=lex_slovo;
			if(id->result==T_VOID){
				cerror(817, "Cannot call procedure in expression");
				return;
			}
			lex();
			parametry(id);

			//call a function
			typ=id->result;
			if(id->r>0){
				dej(I_CALL);
				dejjmp(preklad+id->u);
			}
			else{
				if(typ==T_BOOL) dejpdm(-id->r);
				else dej(-id->r);
			}
			break;
	}
}

void soucin()
{
	int op;

	unarni();
	for(;;){
		if(term=='*') op=I_MULT;
		else if(term=='/') op=I_DIV;
		else if(term=='%') op=I_MOD;
		else break;
		typ1();
		lex();
		unarni();
		typ1();
		dej(op);
		typ=T_INT;
	}
}

void soucet()
{
	int op;

	soucin();
	for(;;){
		if(term=='+') op=I_ADD;
		else if(term=='-') op=I_SUB;
		else break;
		typ1();
		lex();
		soucin();
		typ1();
		dej(op);
		typ=T_INT;
	}
}

void porovnani()
{
	int op;

	soucet();
	if(term=='>') op=I_GT;
	else if(term=='<') op=I_LT;
	else if(term==O_GE) op=I_LT^64;
	else if(term==O_LE) op=I_GT^64;
	else if(term=='=') op=I_EQ;
	else if(term==O_NE) op=I_EQ^64;
	else return;
	typ1();
	lex();
	soucet();
	typ1();
	dejpdm(op);
}

void pdmneni()
{
	Tjmp *l;

	switch(term){
		case NENI:
			lex();
			l=Ts; Ts=Fs; Fs=l;
			pdmneni();
			if(typ==T_INT) dejpdm(I_COND);
			pretyp(T_BOOL);
			l=Ts; Ts=Fs; Fs=l;
			jmpT=!jmpT;
			break;
		case JE:
			lex();
			pdmneni();
			if(typ==T_INT) dejpdm(I_COND);
			pretyp(T_BOOL);
			break;
		default:
			porovnani();
			break;
	}
}

void pdma()
{
	Tjmp *T1;

	T1=Ts;
	pdmneni();
	while(term==A){
		typ2();
		lex();
		addjmpadr2(Fs, false);  //jump is done when condition is false
		zapisJMP1(Ts, T1, uexe);
		Ts=T1;
		pdmneni();
		typ2();
	}
}

void pdmnebo()
{
	Tjmp *F1;

	F1=Fs;
	pdma();
	while(term==NEBO){
		typ2();
		lex();
		addjmpadr2(Ts, true); //jump is done when condition is true
		zapisJMP1(Fs, F1, uexe);
		Fs=F1;
		pdma();
		typ2();
	}
}

void vyraz()
{
	Tjmp *Fs1=Fs, *Ts1=Ts;
	Fs=Ts=0;
	pdmnebo();
	if(typ==T_BOOL){
		addjmpadr2(Fs, false);
		zapisJMP(Ts);
		dej(I_JEDNA);
		dej(I_JUMP);
		dejjmp(uexe+DJMP+1);
		zapisJMP(Fs);
		dej(I_NULA);
		typ=T_BOOL1;
	}
	Ts=Ts1; Fs=Fs1;
}

void podminkaF(Tjmp* &u)
{
	Fs=Ts=0;
	pdmnebo();
	pretyp(T_BOOL);
	zapisJMP(Ts);
	addjmpadr2(Fs, false);
	u=Fs;
}

void podminkaT(Tjmp* &u)
{
	Fs=Ts=0;
	pdmnebo();
	pretyp(T_BOOL);
	zapisJMP(Fs);
	addjmpadr2(Ts, true);
	u=Ts;
}

//-------------------------------------------
int skipslovo(char *&u)
{
	int x;

	for(x=0; neoddel[*(uchar)u]; u++, x++);
	return x;
}

int backslovo(char *&u)
{
	int x;

	u--;
	for(x=0; neoddel[*(uchar)u]; u--, x++);
	u++;
	return x;
}

//write breakpoints to compiled code
void setbreakp()
{
	Tbreakp *u, *u1;
	int r1=0;

	if(preklad){
		for(u1=&breakp, u=u1->nxt; u; u1=u, u=u->nxt)
		 if(u->text==&text){
			 if(u->r<=r1 || tab[u->r]==preklad+Dpreklad){
				 //remove multiple breakpoints and breakpoints which are after the last command
				 u1->nxt=u->nxt;
				 delete u;
				 u=u1;
			 }
			 else{
				 //move breakpoint to next line if this line is empty
				 while(tab[u->r]==tab[u->r+1]) ++u->r;
				 *tab[u->r]|=128;
				 r1=u->r;
			 }
		 }
	}
}

void defvar(bool pruchod, bool par, Tslovo *funkce)
{
	int i, t, n;
	Tvar *var, **a;
	bool dim1, dim0;

	for(;;){
		t=T_INT;

		//reference
		if(term=='&'){
			if(!par)
				cerror(818, "Reference variable can be used only as a parameter of function");
			lex();
			t=T_REF;
		}

		if(term==O_VAR){
			var=lex_var;
			if(!pruchod) cerror(819, "Variable %w already defined", var->jmeno);
		}
		else if(term==O_IDEN || term==O_FCE){
			//add new variable to the list
			while(((var=new Tvar))==0)
				if(freemem1()) return;
			var->jmeno= keystr;
			var->offset= funkce->Nvar;
			*funkce->varend= var;
			funkce->varend= &var->nxt;
			var->nxt=0;
			//put it to hash table
			a= &local[key % Dlocal];
			var->nxthash=*a;
			*a=var;
		}
		else{
			cerror(820, "Identifier expected");
			break;
		}
		lex();

		//array
		dim1=dim0=false;
		if(term=='['){
			if(t==T_REF) cerror(821, "Cannot declare reference to array");
			for(n=0; term=='['; n++){
				lex();
				if(!par){
					if((term!=']' && !dim0) || dim1){
						dim1=true;
						vyraz();
						pretyp(T_INT);
						dej(I_STORE);
						dej(var->offset+n+1);
					}
					else{
						dim0=true;
					}
				}
				cerrch(']');
				lex();
			}
			if(dim1){
				dej(I_ALLOC);
				dej(var->offset);
				dej(n);
			}
			if(dim0 && !par){
				dej(I_NULA);
				dej(I_STORE);
				dej(var->offset);
			}
			t=T_ARRAY+n-1;
		}

		if(!pruchod){
			var->typ=t;
			if(par) funkce->Nparam+= typsize(t);
			else funkce->Nvar+= typsize(t);
		}

		//inicialization
		if(term=='='){
			if(par) cerror(822, "Cannot initialize parametrers");
			if(t>=T_ARRAY) cerror(823, "Cannot initialize array");
			lex();
			vyraz();
			pretyp(t);
			dej(I_STORE);
			dej(var->offset);
		}

		if(term!=',') break;
		lex();
	}

	//set offsets to function parameters
	if(!pruchod && par){
		i= -2 -funkce->Nparam;
		for(var=funkce->var; var; var=var->nxt){
			var->offset= i;
			i+= typsize(var->typ);
		}
		assert(i==-2);
	}
}


void kompilace1()
{
	Tslovo *slovo, *funkce;
	Tvar *var, **a;
	int pruchod;
	Tjmp *skoky;
	int t;
	int i;
	char *u1;
	int kod;

	text.txt.u[text.delka]=0; //terminating zero
	delpreklad();
	if(mkslov(true)) return;

	for(pruchod=0; pruchod<2; pruchod++){
		if(pruchod){
			if(jechyba) return;

			//allocate memory
			Dpreklad=(size_t)uexe;
			if(!Dprocedur) return;
			while(((preklad = new unsigned char[Dpreklad]))==0)
				if(freemem()) return;
			while(((tab = new uchar[rexe+1]))==0)
				if(freemem1()) return;
			rtab=0;
			uexe=preklad;
		}
		else{
			uexe=(uchar)1;
		}
		csp=cstack;
		csp->kod=PROCEDURE;

		utxt=text.txt.u; rexe=1;
		lex();

		//parse all procedures in source file
		for(;;){
			while(term==O_CR) lex();
			if(preklad){
				while(rtab<rexe) tab[++rtab]=uexe;
			}
			if(term==O_EOF) break;
			if(term!=PROCEDURE && term!=FUNKCE && term!=PODMINKA){
				if(term==KONEC) cerror(824, "Misplaced %", KONEC);
				else cerror(825, "%, %, or % expected", PROCEDURE, FUNKCE, PODMINKA);
			}
			lex();
			memset(local, 0, sizeof(local));

			//find procedure identifier
			if(term!=O_FCE){
				cerror(826, "Identifier of a new procedure expected");
				return;
			}
			funkce=lex_slovo;
			if(funkce->r<=0) cerror(827, "Name of procedure expected, but keyword found");
			if(!preklad){
				if(funkce->u!=0xffff) cerror(828, "Procedure %w already defined", funkce->jmeno);
				else funkce->u= (size_t)uexe-1;
			}
			lex();

			u1=utxt;
			for(var=funkce->var; var; var=var->nxt){
				utxt=var->jmeno;
				hash();
				a= &local[key % Dlocal];
				var->nxthash=*a;
				*a=var;
			}
			utxt=u1;

			//parameters
			if(term=='('){
				lex();
				if(term!=')') defvar(true, true, funkce);
				cerrch(')');
				lex();
			}
			if(funkce->Nvar){
				dej(I_ADDSP);
				dej(funkce->Nvar);
			}

			//parse all commands in procedure
			while(funkce){

				if(term!=O_CR) cerrch(';');
				do{
					lex();
				} while(term==O_CR);

				assert(csp>=cstack);
				if(csp>=cstack+MCSTACK-1){
					cerror(829, "Too many nested commands");
					return;
				}
				if(preklad){
					while(rtab<rexe) tab[++rtab]=uexe;
				}

				slovo=lex_slovo;
				var=lex_var;
				kod=term;
				if(kod==O_IDEN){
					uerr=keystr;
					if(term=='=' || term=='['){
						cerror(830, "Variable %w not defined", keystr);
					}
					else{
						cerror(831, "Unknown command %w", keystr);
					}
				}
				lex();

				if(csp->kod==ZVOLIT && kod!=PRIPAD){
					cerror(832, "% must be followed by %", ZVOLIT, PRIPAD);
				}

				switch(kod){
					default:
						cerror(833, "Syntax error");
						break;

					case O_EOF:
						cerror(834, "% is missing", KONEC);
						return;

					case O_VAR: //assignment to variable
						t=var->typ;
						i=0;
						if(term=='['){
							for(; term=='['; i++){
								if(t-i<T_ARRAY){
									if(i) cerror(835, "Too many dimensions");
									else cerror(836, "Type of variable is not array");
								}
								lex();
								vyraz();
								pretyp(T_INT);
								cerrch(']');
								lex();
							}
							if(t-i != T_ARRAY-1) cerror(837, "Too few dimensions");
						}
						cerrch('='); lex();
						vyraz();
						if(t==T_REF){
							pretyp(T_INT);
							dej(I_STOREP);
							dej(var->offset);
						}
						else if(i){
							pretyp(T_INT);
							dej(I_STOREA);
							dej(var->offset);
							dej(i);
						}
						else{
							pretyp(t);
							for(i=typsize(t)-1; i>=0; i--){
								dej(I_STORE);
								dej(var->offset+i);
							}
						}
						break;

					case O_FCE:
						if(slovo==funkce && term=='='){
							//function result code
							lex();
							if(funkce->result==T_VOID)
								cerror(838, "Procedure cannot return a value");
							vyraz();
							if(typ!=funkce->result)
								cerror(839, "Type mismatch");
							dej(I_STORE);
							dej(-3 -funkce->Nparam);
						}
						else{
							//call procedure, function or condition
							parametry(slovo);
							if(slovo->r>0){
								dej(I_CALL);
								dejjmp(preklad+slovo->u);
							}
							else{
								if(slovo->result!=T_VOID)
									cerror(840, "Expected % or % before condition", KDYZ, DOKUD);
								dej(-slovo->r);
							}
							if(slovo->result!=T_VOID) dej(I_POP); //forget result code
						}
						break;

					case KDYZ:
						csp++;
						csp->kod=KDYZ;
						csp->adr=0;
						podminkaF(csp->adrjmp);
						break;

					case ZVOLIT:
						csp++;
						csp->kod=ZVOLIT;
						csp->adr=0;
						csp->adrjmp=0;
						vyraz();
						pretyp(T_INT);
						break;

					case PRIPAD:
						if(csp->kod==JINAK){
							cerror(841, "% must be at the end of %", JINAK, ZVOLIT);
						}
						else if(csp->kod!=PRIPAD && csp->kod!=ZVOLIT){
							cerror(842, "% without %", PRIPAD, ZVOLIT);
						}
						else{
							if(csp->kod==ZVOLIT){
								csp++;
								csp->kod=PRIPAD;
								csp->adr=0;
							}
							else{
								dej(I_JUMP);
								uexe+=DJMP;
								addjmpadr((csp-1)->adrjmp);
								zapisJMP(csp->adrjmp);
							}
							csp->adrjmp=0;
							for(;;){
								vyraz();
								pretyp(T_INT);
								dej(I_CMP);
								uexe+=DJMP;
								addjmpadr(csp->adrjmp);
								if(term!=',') break;
								lex();
								uexe[-DJMP-1]^=64;
							}
							zapisJMP(csp->adrjmp);
							csp->adrjmp=0;
							addjmpadr(csp->adrjmp);
						}
						break;

					case DOKUD:
						csp++;
						csp->kod=DOKUD;
						csp->adr=uexe;
						podminkaF(csp->adrjmp);
						break;

					case OPAKUJ:
						csp++;
						if(term==';' || term==O_CR){
							csp->kod=OPAKUJ0;
							csp->adrjmp=0;
						}
						else{
							csp->kod=OPAKUJ;
							vyraz();
							dej(I_JUMP);
							csp->adrjmp=(Tjmp*)uexe;
							dejjmp(uexe);
						}
						csp->adr=uexe;
						break;

					case AZ:
						if(csp==cstack)
							cerror(843, "Unexpected keyword %", AZ);
						else if(!csp->adr)
							cerror(844, "% does not have %",
								csp->kod==ZVOLIT ? ZVOLIT : KDYZ, KONEC);
						else if(csp->kod==OPAKUJ){
							dej(I_DEC);
							podminkaT(skoky);
							zapisJMP(csp->adrjmp);
							dej(I_TEST);
							dejjmp(csp->adr);
							zapisJMP(skoky);
							dej(I_POP);
							csp--;
						}
						else{
							podminkaF(skoky);
							zapisJMP1(skoky, 0, csp->adr);
							zapisJMP(csp->adrjmp);
							csp--;
						}
						break;

					case KONEC:
						if(csp->kod==PROCEDURE){
							//end of procedure
							assert(funkce);
							if(funkce->Nvar){
								if(!preklad) uexe+=2; //I_ADDSP
							}
							if(funkce->Nparam){
								dej(I_RETP);
								dej(funkce->Nparam);
							}
							else{
								dej(I_RET);
							}
							funkce=0;
						}
						else if(csp->kod==OPAKUJ){
							dej(I_DEC);
							zapisJMP(csp->adrjmp);
							dej(I_TEST);
							dejjmp(csp->adr);
							dej(I_POP);
							csp--;
						}
						else{
							if(csp->adr){
								dej(I_JUMP);
								dejjmp(csp->adr);
							}
							zapisJMP(csp->adrjmp);
							csp--;
							if(csp->kod==ZVOLIT){
								zapisJMP(csp->adrjmp);
								dej(I_POP);
								csp--;
							}
						}
						break;

					case NAVRAT:
						if(funkce->Nparam){
							dej(I_RETP);
							dej(funkce->Nparam);
						}
						else{
							dej(I_RET);
						}
						break;

					case JINAK:
						if(csp->kod==PRIPAD){
							dej(I_JUMP);
							uexe+=DJMP;
							addjmpadr((csp-1)->adrjmp);
							zapisJMP(csp->adrjmp);
							csp->adrjmp=0;
							csp->kod=JINAK;
						}
						else
					 if(csp->kod != KDYZ){
						 cerror(842, "% without %", JINAK, KDYZ);
					 }
					 else{
						 dej(I_JUMP);
						 dejjmp(uexe);
						 zapisJMP(csp->adrjmp);
						 csp->adrjmp= (Tjmp *)uexe -1;
						 csp->kod=JINAK;
					 }
					 break;

					case CISLO:
						defvar(pruchod>0, false, funkce);
						break;

					case PROCEDURE:
					case FUNKCE:
					case PODMINKA:
						cerror(845, "% of previos procedure is missing", KONEC);
						break;

				}//switch
			}//while
		}//for
	}//pass

	if(preklad){
		setbreakp();
		text.lst();
	}
}

void kompilace()
{
	kompilace1();
	if(jechyba) delpreklad();
}

//create procedure list
bool mkslov(bool preloz)
{
	int typ;
	int i, l;
	Tslovo *slovo, **a;

	if(preklad) return false;
	dellist(slovnik);

	//write keywords to hash table
	memset(global, 0, sizeof(global));
	memset(local, 0, sizeof(local));
	for(l=0; keywords[l]; l++){
		for(i=0; i<Dkeyw; i++){
			slovo=&keywS[l][i];
			utxt= slovo->jmeno;
			hash();
			a=&global[key % Dglobal];
			slovo->nxthash=*a;
			*a=slovo;
		}
	}

	text.txt.u[text.delka]=0;
	Dprocedur=0;
	utxt=text.txt.u;
	rexe=1;

	for(;;){
		do{
			lex();
		} while(term==O_CR);
		if(term==PROCEDURE) typ=T_VOID;
		else if(term==FUNKCE) typ=T_INT;
		else if(term==PODMINKA) typ=T_BOOL1;
		else typ=T_NO;
		if(typ!=T_NO){
			lex();
			if(term==O_IDEN){
				while(((slovo=new Tslovo))==0)
					if(freemem1()) return true;
				Dprocedur++;
				if(!slovnik){
					slovnik=slovo;
				}
				else{
					slovnikend->nxt= slovo;
					slovo->pre= slovnikend;
				}
				slovnikend=slovo;

				slovo->jmeno=keystr;
				slovo->r=rexe;
				slovo->result=typ;

				if(preloz){
					a= &global[key % Dglobal];
					slovo->nxthash=*a;
					*a=slovo;
					lex();
					if(term=='('){
						lex();
						if(term!=')') defvar(false, true, slovo);
						cerrch(')');
						memset(local, 0, sizeof(local));
					}
				}
			}
		}
		while(*utxt!=CR){
			if(!*utxt) return false;
			utxt++;
		}
		rexe++;
		utxt++;
	}
}



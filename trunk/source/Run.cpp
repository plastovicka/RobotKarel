/*
	(C) 1999-2007  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include "hdr.h"
#pragma hdrstop

#include "karel.h"
#include "compile.h"
#include "editor.h"

uchar uprg=0; //current instruction address
Tstack *sp, *bp, *stack=0; //stack
int maxstack;
unsigned long Cinstr=0, Ckroky=0;
int klavesa=0;

int pole[PXMAX][PYMAX]; //field including border wall
int plx=49, ply=2, plym=21, plxm= //field coordinates and size
#ifdef DOS
31
#else
41
#endif
;
int kax=1, kay=20, smer=0; //Karel position and direction

void tiskpole(int i, int j)
{
	gotoSQ(i, j);
	tiskpole1(pole[i][j]);
}

void tiskzn()
{
	gotoSQ(0, plym);
	tiskpole1(pole[kax][kay]);
}

void domu()
{
	kax=1; kay=plym-1; smer=0;
	if(pole[kax][kay]<0) pole[kax][kay]=0;
}

void krok()
{
	int kax1, kay1;

	kax1=kax-(smer==2)+(smer==0);
	kay1=kay-(smer==1)+(smer==3);
	if(pole[kax1][kay1]<0){
		chyba(lng(780, "I have crashed to wall"));
	}
	else{
		kax=kax1; kay=kay1;
		Ckroky++;
	}
}

void vlevo_vbok()
{
	smer=(smer+1) % 4;
}

void vpravo_vbok()
{
	smer=(smer+3) % 4;
}

void zvedni()
{
	if(!pole[kax][kay]) {
		chyba(lng(781, "There are no marks to pick up"));
	}
	else{
		if(pole[kax][kay]>257) --pole[kax][kay];
		else pole[kax][kay]=0;
	}
}

void poloz()
{
	if(pole[kax][kay] >= maxznacek){
		chyba(lng(782, "There is no space for mark"));
	}
	else{
		if(pole[kax][kay]>256) ++pole[kax][kay];
		else pole[kax][kay]=257;
	}
}

void runerr()
{
	chyba(lng(783, "Fatal error: invalid instruction"));
	delpreklad();
}

#define JMP uprg+= *(short *)uprg;

#define SKOK(t) \
	if(((c&64)!=0)==(t)) JMP  else uprg+=DJMP;

#define PUSH(i) *(sp++)=i
#define POP *(--sp)

void indexy(bool store)
{
	Tstack *a, *b;
	int ind, j, n, adr;
	int k;

	a= bp + (*(signed char *)uprg++);
	n= *uprg++;
	if(a<stack || a+n>=stack+maxstack){ runerr(); return; }
	if(store) k=POP; else k= *uprg++;

	if(!a[0]){
		chyba(lng(784, "Array is not initialized"));
	}
	adr=0;
	for(j=1; j<=n; j++){
		ind= sp[j-1-n];
		adr= adr*a[j] + ind;
		if(ind<0 || ind>=a[j]){
			chyba(lng(785, "Index is out of range"));
		}
	}
	sp-=n;
	if(store){
		if(!jechyba) stack[a[0]+adr] = k;
	}
	else{
		if(k){
			if(k>15){ runerr(); return; }
			b=sp;
			PUSH(0);
			while(k--){
				PUSH(a[j]);
				adr*= a[j];
				j++;
			}
			if(!jechyba) *b= a[0]+adr;
		}
		else{
			if(jechyba) PUSH(0);
			else PUSH(stack[a[0]+adr]);
		}
	}
}

void showRychlost()
{
	statusMsg(clspeed, lng(653, "Delay= %-5d"), rychlost);
}

void runCmd(int c)
{
	if(c==8){
		chyba(lng(786, "Program has been terminated"));
		prgreset();
		isBusy=0;
		return;
	}
	if(c==27){
		lastTracing=tracing;
		chyba(lng(787, "Program has been interrupted"));
		isBusy=0;
		return;
	}
	if(tracing){
		if(c>='0' && c<='9'){
			static int tabrychl[10]={1, 8, 15, 30, 70, 150, 300, 600, 1000, 2000};
			rychlost=tabrychl[c-'0'];
			showRychlost();
			return;
		}
		if(c=='+'){
			if(rychlost<5000){
				rychlost+= rychlost>40 ? (rychlost>500 ? 50 : 10) : 1;
			}
			showRychlost();
			return;
		}
		if(c=='-'){
			if(rychlost>1){
				rychlost-= rychlost>40 ? (rychlost>500 ? 50 : 10) : 1;
			}
			showRychlost();
			return;
		}
	}
	klavesa=c;
}

void delay1(int ms)
{
	sleep(ms%200);
	for(;;){
		ms-=200;
		readKeyboard();
		if(jechyba || ms<0) break;
		sleep(200);
	}
}


bool instrukce(bool beh)
{
	static int counter=0;
	int i, c;
	Tstack *a;
	long l;

	if(beh){
		klavesa=0;
		notrunr();
		statusMsg(clstatusRun, lng(654,
		 "Program is running    ESC = stop    BACKSPACE = quit"));
	}

	if(uprg<preklad || uprg>=preklad+Dpreklad){ runerr(); return false; }
	c=*uprg;
	do{
		if(sp<stack){ runerr(); return false; }
		if(sp>stack+maxstack-3){
			showprg();
			chyba(lng(788, "Stack overflow !"));
			prgreset();
			return false;
		}
		if((counter--)<=0){
			counter=12345;
			tiskCinstr();
			readKeyboard();
		}
		if(jechyba) break;
		Cinstr++;
		uprg++;
		switch(c & 63){

			case I_RET:
			case I_RETP:
				i= (c==I_RET) ? 0 : *uprg;
				sp=bp;
				bp=stack+POP;
				uprg= preklad + POP;
				sp-=i;
				if(uprg==preklad){
					if(sp>stack){
						statusMsg(clresult, lng(655, "Function result = %d "), sp[-1]);
						jechyba=1;
					}
					prgreset();
					return false;
				}
				break;

			case I_ADDSP:
				i= *uprg++;
				while(i>0){
					PUSH(UNDEF_VALUE);
					i--;
				}
				break;

			case I_CALL:
				PUSH((int)(uprg+DJMP-preklad));
				PUSH((int)(bp-stack));
				bp=sp;
				JMP
			 break;

			case I_JUMP:
				JMP
			 break;

			case I_JEDNA:
				PUSH(1);
				break;

			case I_NULA:
				PUSH(0);
				break;

			case I_CONST:
				PUSH(*(int *)uprg);
				uprg+= Dint;
				break;

			case I_DEC:
				sp[-1]--;
				break;

			case I_COND:
				SKOK(POP!=0);
				break;

			case I_TEST:
				SKOK(sp[-1]<=0);
				break;

			case I_POP:
				sp--;
				break;

			case I_LOAD:
				i=bp[*(signed char *)uprg++];
				if(i==UNDEF_VALUE) chyba(lng(789, "The variable is undefined"));
				PUSH(i);
				break;

			case I_REF:
				PUSH(int(bp-stack) + *(signed char *)uprg++);
				break;

			case I_STORE:
				bp[*(signed char *)uprg++] = POP;
				break;

			case I_LOADP:
				PUSH(stack[bp[*(signed char *)uprg++]]);
				break;

			case I_STOREP:
				stack[bp[*(signed char *)uprg++]] = POP;
				break;

			case I_LOADA:
				indexy(false);
				break;

			case I_STOREA:
				indexy(true);
				break;

			case I_ALLOC:
				//address of array pointer
				a= bp + *(signed char *)uprg++;
				//array size
				l=1;
				for(i= *uprg++; i>0; i--){
					l*=a[i];
					if(l>maxstack) chyba(lng(790, "Array is too large"));
					if(l<0) chyba(lng(791, "Size of array is negative"));
				}
				if(jechyba) a[0]=0;
				else a[0]= int(sp-stack);
				sp+= (int)l;
				break;

			case STOP:
				chyba(lng(792, "Instruction STOP"));
				break;

			case SEVER:
				SKOK(smer==1);
				break;
			case JIH:
				SKOK(smer==3);
				break;
			case VYCHOD:
				SKOK(smer==0);
				break;
			case ZAPAD:
				SKOK(smer==2);
				break;
			case ZED:
				SKOK(pole[kax-(smer==2)+(smer==0)][kay-(smer==1)+(smer==3)]<0);
				break;
			case ZNACKA:
				SKOK(pole[kax][kay]>256);
				break;
			case MISTO:
				SKOK(pole[kax][kay]<maxznacek);
				break;
			case ZNAK:
				i=pole[kax][kay];
				if(i>0 && i<256) PUSH(i); else PUSH(0);
				break;
			case KLAVESA:
				PUSH(klavesa);
				klavesa=0;
				break;
			case NAHODA:
				sp[-1]= (Tstack)(long(rand())*sp[-1]/(RAND_MAX+1));
				break;
			case DELKA:
				sp[-2]=sp[-1];
				sp--;
				break;

			case KROK:
				krok();
				break;
			case VLEVO_VBOK:
				vlevo_vbok();
				break;
			case VPRAVO_VBOK:
				vpravo_vbok();
				break;
			case ZVEDNI:
				zvedni();
				break;
			case POLOZ:
				poloz();
				break;
			case DOMU:
				domu();
				break;
			case RYCHLE:
				fast++;
				break;
			case POMALU:
				fast--;
				break;

			case MALUJ:
				i=POP;
				if(i>255 || i<0) chyba(lng(793, "Cannot paint character %d"), i);
				else pole[kax][kay]= (i!=' ') ? i : 0;
				break;

			case CEKEJ:
				tiskCinstr();
				i=POP;
				if(i<0) waitKeyboard();
				else delay1(i);
				counter=0;
				if(jechyba) return false;
				break;

			case I_ADD:
				sp[-2]=sp[-2]+sp[-1];
				sp--;
				break;
			case I_SUB:
				sp[-2]=sp[-2]-sp[-1];
				sp--;
				break;
			case I_MULT:
				sp[-2]=sp[-2]*sp[-1];
				sp--;
				break;
			case I_DIV:
				if(sp[-1]==0) chyba(lng(794, "Division by zero"));
				else sp[-2]=sp[-2]/sp[-1];
				sp--;
				break;
			case I_MOD:
				if(sp[-1]==0) chyba(lng(794, "Division by zero"));
				else sp[-2]=sp[-2]%sp[-1];
				sp--;
				break;
			case I_NEG:
				sp[-1]= -sp[-1];
				break;

			case I_CMP:
				SKOK(sp[-2]==sp[-1]);
				sp--;
				break;
			case I_EQ:
				SKOK(sp[-2]==sp[-1]);
				sp-=2;
				break;
			case I_LT:
				SKOK(sp[-2]<sp[-1]);
				sp-=2;
				break;
			case I_GT:
				SKOK(sp[-2]>sp[-1]);
				sp-=2;
				break;

			default:
				runerr();
				break;
		}

		if(uprg<preklad || uprg>=preklad+Dpreklad){
			if(uprg) runerr();
			return false;
		}
		if((c=*uprg) & 128)  chyba("Breakpoint");

	} while(beh);

	return !jechyba;
}

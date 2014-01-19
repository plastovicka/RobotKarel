/*
	(C) 1999  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include "hdr.h"
#pragma hdrstop

#ifdef DOS

struct REGPACK reg;

unsigned ismouse;

int mousereset()
{
	reg.r_ax=0;
	intr(0x33, &reg);
	return ismouse=reg.r_ax;
}

void mouseon()
{
	reg.r_ax=1;
	intr(0x33, &reg);
}

void mouseoff()
{
	reg.r_ax=2;
	intr(0x33, &reg);
}

int mousexy(int &x, int &y)
{
	reg.r_ax=3;
	intr(0x33, &reg);
	x=reg.r_cx; y=reg.r_dx;
	return reg.r_bx;
}

void mouse8xy(int &x, int &y)
{
	mousexy(x, y);
	x=x/8+1; y=y/8+1;
}

int button()
{
	reg.r_ax=3;
	intr(0x33, &reg);
	return reg.r_bx;
}

int necekej(int &x, int &y)
{
	static int x1, y1;
	static int btnL, btnR, btnM;
	int b, x0, y0;

	if(bioskey(17)) return 0;
	if(ismouse){
		x0=x1; y0=y1;
		b=mousexy(x, y);
		x1=x; y1=y;
		if((b&1) != btnL){
			if(btnL){
				btnL=0;
				return 3;
			}
			else{
				btnL=1;
				return 1;
			}
		}
		if((b&2) != btnR){
			if(btnR){
				btnR=0;
				return 4;
			}
			else{
				btnR=2;
				return 2;
			}
		}
		if((b&4) != btnM){
			if(btnM){
				btnM=0;
				return 6;
			}
			else{
				btnM=4;
				return 5;
			}
		}
		if(x0!=x1 || y0!=y1){
			return 7;
		}
	}
	return -1;
}

int necekej8(int &x, int &y)
{
	int b;
	b=necekej(x, y);
	x=x/8+1; y=y/8+1;
	return b;
}

int cekej(int &x, int &y)
{
	int result;

	if(ismouse) mouseon();
	do{
		result=necekej(x, y);
	} while(result<0);
	if(ismouse) mouseoff();
	return result;
}

int cekej8(int &x, int &y)
{
	int b;
	b=cekej(x, y);
	x=x/8+1; y=y/8+1;
	return b;
}

#endif

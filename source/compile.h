/*
	(C) 1999-2005  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/

#ifndef runH
#define runH

typedef short Tjmp;
typedef unsigned char *uchar;
typedef int Tstack;

#define UNDEF_VALUE (1<<(sizeof(Tstack)*8-1))
#define DJMP ((int)sizeof(Tjmp))
#define Dint sizeof(int)

extern Tstack *sp, *bp, *stack;
extern int maxstack;
extern uchar uprg;
extern int rychlost, fast;

enum TYP
{
	T_NO, T_VOID, T_INT, T_BOOL, T_BOOL1, T_REF, T_ARRAY
};

enum KOD
{
	O_NE=-9, O_GE, O_LE, O_NUM, O_FCE, O_IDEN, O_VAR, O_CR, O_EOF,

	//keywords
	PROCEDURE, FUNKCE, PODMINKA,
	KDYZ, DOKUD, OPAKUJ, JINAK, AZ, KONEC, NAVRAT, CISLO, ZVOLIT, PRIPAD,
	//operators
	NENI, JE, A, NEBO,
	//basic commands
	KROK, VLEVO_VBOK, VPRAVO_VBOK, ZVEDNI, POLOZ, MALUJ,
	CEKEJ, STOP, DOMU, RYCHLE, POMALU,
	//conditions
	ZED, ZNACKA, MISTO, SEVER, ZAPAD, JIH, VYCHOD,
	//functions
	ZNAK, KLAVESA, NAHODA, DELKA,

	I_JEDNA, I_NULA,
	OPAKUJ0,

	//instructions which don't have appropriate keyword
	I_CALL, I_ADDSP, I_RET, I_RETP,
	I_JUMP, I_COND, I_DEC, I_POP, I_TEST, I_CMP,
	I_LOAD, I_STORE, I_LOADP, I_STOREP, I_LOADA, I_STOREA, I_ALLOC, I_REF,
	I_ADD=1, I_SUB, I_MULT, I_DIV, I_MOD, I_NEG, I_EQ, I_LT, I_GT,
	I_CONST,
};

void krok();
void vlevo_vbok();
void vpravo_vbok();
void zvedni();
void poloz();
void domu();

#endif

/*
	(C) 1999-2003  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#ifndef menuH
#define menuH

#include "mouse.h"
#include "karel.h"

typedef void(*fce)();

struct Tmenuitem {
	char *name;
	char *shortcut;
	int cmd;
	bool(*enabled)();
};

struct Tmenu {
	char *name;
	Tmenuitem *items;
	int x;
	int last;
};

class Tmainmenu{
	Tmenu *menus, *zamenu, *s;
	bool rozvito;
	int wx, wy, wxm, wym;
	int r, poc;

	void printmenu(bool sel);
	void printitem(int r, bool sel);
	int vyber(int rad);
	void rozbal(Tmenu *menu);
	void zabal();
	void enter();
	bool najdimenu(char c);
	Tmenuitem * najdiitem(char c);
public:
	Tmainmenu(Tmenu *m);
	bool alt(int c);
	void menuclick(int x);
	void run();
	void runF10();
	void tisk();
	void test();
};


class Tcomponent{
public:
	int x, y, xm, ym;
	char *name;
	int mod;
	virtual void paint(int activ)=0;
	virtual bool key(int k){ return false; };
	virtual void mouse(int mx, int my){};
	virtual void setvar(){};
	virtual void getvar(){};
};


class Tbutton :public Tcomponent{
public:
	Tbutton(int x1, int y1, char *n, int m);
	Tbutton(char *n, int m){ name=n; mod=m; };
	void set(int x1, int y1);
	virtual void paint(int activ);
};

class Tcheckbox :public Tcomponent{
	int &var;
	int checked;
public:
	Tcheckbox(int x1, int y1, int l, char *n, int &v);
	virtual void paint(int activ);
	virtual bool key(int k);
	virtual void mouse(int mx, int my);
	virtual void setvar();
	virtual void getvar();
};

class Tgroup :public Tcomponent{
	int &var;
	char **items;
	int count, item;
public:
	Tgroup(int x1, int y1, int l, char *n, int &v, char **i, int c);
	virtual void paint(int activ);
	virtual bool key(int k);
	virtual void mouse(int mx, int my);
	virtual void setvar();
	virtual void getvar();
};

#define EDSIZE 79

class Tedit :public Tcomponent{
protected:
	char text[EDSIZE+1];
	int length;
	bool cr;
	int cur, zac, posun;
	bool select;
	bool ins;
public:
	Tedit(int x1, int y1, int l, char *n, bool c);
	virtual void paint(int activ);
	virtual bool key(int k);
	virtual void mouse(int mx, int my);
	void smaz();
};

class Teditn :public Tedit{
	char *var;
	int maxlength;
public:
	Teditn::Teditn(int x1, int y1, int l, char *n, char *v, int m, bool c) :
		Tedit(x1, y1, l, n, c), var(v), maxlength(m){};
	virtual void setvar();
	virtual void getvar();
};

class Tokno{
	Tcomponent **components, **last, **activ;
	int wx, wy, wxm, wym, sirka, vyska;
	char *name;
	char *text;
	void okraj();
	void okraj2();
	void hide();
public:
	Tokno(Tcomponent **co, int c, char *n, int w, int h, char *t);
	int show();
	void paint();
	void setvar();
};

extern Tmainmenu *menu;
extern Tmenuitem *sender;

void oknotxt(char *name, char *text, int w, int h);
void oknostr(char *name, char *text);
void oknochyb(char *text);
int oknoano(char *name, char *text);
int oknoed(char *name, char *text, int l, int m, void *out);
int oknoint(char *name, char *text, int &var);
int oknofile(char *name, char *ext, char *var);
int showmodal(Tcomponent **co, int c, char *n, int w, int h, char *t);
void window1(int x, int y, int xm, int ym);
void window0();

#endif

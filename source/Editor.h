/*
	(C) 1999-2005  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/

#ifndef editorH
#define editorH

#define CR 10

struct Tpos{
	char *u; //pointer to text
	int r;   //line number
};

struct Tbookmark
{
	Tpos pos; //pointer and line
	int y;    //position in window
};

struct Tundo
{
	~Tundo(){ if(del) delete[] str; }
	size_t offset;    //position relative to beginning of file
	int x, r;          //column and line
	int dr;           //total lines difference
	char *str;        //deleted text
	size_t del, ins;   //number of deleted and inserted characters
	Tundo *nxt, *pre;  //next and previous item
};


class Ttext{
public:
	char jmeno[256]; //file name
	int rpoc;    //number of lines
	size_t delka;//text size
	Tpos txt,    //pointer to text
			 cur,    //cursor position
			 select1, select2;  //selected block
	bool runr;             //true=execution line is visible

	void key(int ch);
	void command(int cmd);
	void mouse(int x, int y, int c);
	void edPrepare();
	void edDone();
	void selScroll();
	void showCursor();

	void lst(){ dirty=true; }
	void repaint();
	void otevri();
	void novy();
	void novy1();
	int save();
	int ulozmodif();
	int zavri();
	void zavri1();
	void gotor(int r);
	void setTop(int r);
	bool lzezpet();
	bool lzevpred();
	bool jeblok();
	bool freeUndo();
	void find(int replace); //0=find, 1=replace, 2=replace all
	void findn(char *u, int r, int replace);
	void findpre(char *u, int r, int replace);
	void setxy();
	void setRect(int x, int y);
	void enter(bool insKonec);

	void sel();
	void cancelBlock();
	void gotoMarker(int which);
	void setMarker(int which);
	void up(); void selUp(); void scrollUp(); void down(); void selDown();
	void scrollDown(); void left(); void selLeft(); void wordLeft(); void selWordLeft();
	void right(); void selRight(); void wordRight(); void selWordRight(); void pageDown();
	void selPageDown(); void pageBottom(); void selPageBottom(); void pageUp(); void selPageUp();
	void pageTop(); void selPageTop(); void lineStart(); void selLineStart(); void editorTop();
	void selEditorTop(); void lineEnd(); void selLineEnd(); void editorBottom(); void selEditorBottom();
	void toggleMode(); void copy(); void cut(); void paste(); void deleteChar();
	void deleteLastChar(); void deleteLastWord(); void undo(); void redo(); void lineBreak();
	void tab(); void upperLower(); void endOfBlock(); void selectAll(); void blockIndent();
	void blockUnindent(); void insertLine(); void deleteWord(); void deleteLine(); void deleteEOL();
	void gotoMarker0(); void gotoMarker1(); void gotoMarker2(); void gotoMarker3(); void gotoMarker4();
	void gotoMarker5(); void gotoMarker6(); void gotoMarker7(); void gotoMarker8(); void gotoMarker9();
	void setMarker0(); void setMarker1(); void setMarker2(); void setMarker3(); void setMarker4();
	void setMarker5(); void setMarker6(); void setMarker7(); void setMarker8(); void setMarker9();
	void nextLine(); void beginOfBlock(); void selectWord(); void deleteBlock(); void ctrlP();
	void ctrlQ(); void ctrlK(); void findNext(); void findPrev(); void gotoLine();
	void newFile(); void openFile(); void saveFile(); void saveAsFile(); void blockEnd();
private:
	int posunx;  //horizontal scrolling position
	int x, y;     //cursor position
	int zaCR;    //how much is cursor after CR, otherwise 0
	Tpos pagenxt,//next page
			 top;    //top of page
	bool opacselect;       //1=block is selected backwards
	bool dirty;            //1=window needs to be repainted
	bool zrussel;
	Tpos curl;   //previous cursor position (for block selection)
	int modif;
	size_t delkaa;   //allocated size
	Tbookmark bookmark[10];// ^K,0-9 ; ^Q,0-9
	Tundo *undoFirst, *undoLast, *undoCur;   //undo list

	bool vlozit(size_t du, int dr, const char *src, int m=0); //du=size, dr=lines, m=spaces after
	void smazat(char *za, int dr); //za=deleted block, dr=lines
	void smazat1(char *u, int r, size_t du, int dr);//position (u,r), size (du,dr)
	bool memory(long pocet, int dr=0);//insert or delete |pocet| bytes and |dr| lines
	bool memory2(char *u, int r, long pocet, int dr); //at position (u,r)
	void setcur();   //set cursor to position x,y
	void setux();    //move cur from beginning of line to column x, set zaCR
	void homecur();  //move cur to beginning of line
	char * tiskeol1(char *u, int rad); //print from u to end of line, return next line
	void tiskrad();
	void gotour(char *u, int r, int d=0); //d is visible string length
	bool makro(int dr, int m); //m spaces after, insert CR if dr>=1
	void znak(char c); //insert character
	void sourad();     //redraw cursor coordinates
	bool zmena(size_t del, size_t ins, int dr=0);
};

struct Tbreakp
{
	Tbreakp *nxt;
	Ttext *text; //file
	int r;       //line
};

extern Ttext text;
extern Tbreakp breakp;
extern char ffind[64], freplace[64];
extern char *makra[27];

#endif

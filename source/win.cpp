/*
	(C) 1999-2007  Petr Lastovicka

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License.
	*/
#include "hdr.h"
#pragma hdrstop

#ifndef DOS

#include "karel.h"
#include "editor.h"

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"version.lib")


#define Maccel 256
const int kaW=10;
//---------------------------------------------------------------------------
const int minWidth=385; //minimal window width

COLORREF colorsF[]={0x000000,
0xa00000, 0x000000, 0x000080, 0x800090, 0x00a000,
0xffffff, 0x000000, 0xe0ffe8, 0x000000, 0x000000,
0x000000, 0x000000, 0x00ffff, 0xc00000, 0x70a000,
0x000000, 0x0000a0, 0x000000, 0x000000, 0x000000,
0x000000, 0xffffff, 0x000000, 0x500000, 0x000000,
};
COLORREF colorsB[]={0xfafafa,
0xfafafa, 0xfafafa, 0xfafafa, 0xfafafa, 0xfafafa,
0x5080c0, 0xf0ea80, 0x600000, 0x000000, 0x000000,
0xf5f0f0, 0xe0ffe0, 0x2020e0, 0xfafafa, 0xfafafa,
0xfafafa, 0xe0e0e0, 0xffffe0, 0xffffff, 0xf0ffff,
0xfafafa, 0xa06000, 0xc0ffff, 0xefffff, 0xf9f9fc,
};

struct TmenuEnableCallback {
	int cmd;
	bool(*enabled)();
}
menuEnableCallback[] ={
	{1038, lzezpet},
	{1039, lzevpred},
	{1036, jeblok},
	{1035, jeblok},
	{1073, jeblok},
	{1077, jeffind},
	{1078, jeffind},
	{514, jebeh},
	{506, jebeh},
	{121, jebreakp},
};


int
charWidth, charHeight,
 scrollX, edBottom,
 winX=0, winY=20,
 winW=792, winH=470,
 winWidth, winHeight,
 scrollW=20,
 toolbarH,
 X0=0, Y0=0,
 gap=30,
 squareWidth=8,
 squareHeight=8,
 wheelSpeed=60,
 toolBarH=28,   //toolbar height
 toolBarVisible=1,//toolbar is visible
 noCaret,
 Naccel,
 drawMarks=1,
 notResize;     //prevent changing window size

bool
editing,
 plColorChanged=true,
 ldown, rdown;   //mouse button is pressed

HDC dc, kaDC;
HWND hWin, toolbar, hScroll;
HINSTANCE inst;
HACCEL haccel, haccelField;
ACCEL accel[Maccel];
ACCEL dlgKey;

char *title="Karel";
const char *subkey="Software\\Petr Lastovicka\\karel";
struct Treg { char *s; int *i; } regVal[]={
	{"persistBlock", &persistblok},
	{"overwrBlock", &overwrblok},
	{"indent", &indent},
	{"unindent", &unindent},
	{"findOnCur", &foncur},
	{"entern", &entern},
	{"groupUndo", &groupUndo},
	{"undoSave", &undoSave},
	{"upcasekeys", &upcasekeys},
	{"tabs", &tabs},
	{"toolVis", &toolBarVisible},
	{"wheel", &wheelSpeed},
	{"X", &winX}, {"Y", &winY},
	{"width", &winW}, {"height", &winH},
	{"scroll", &scrollDelay},
	{"drawMarks", &drawMarks},
};
struct Tregb { char *s; void *i; DWORD n; } regValB[]={
	{"keys", accel, sizeof(accel)}, //must be the first item of regValB
	{"colorsF", colorsF, sizeof(colorsF)},
	{"colorsB", colorsB, sizeof(colorsB)},
};
struct Tregs { char *s; char *i; DWORD n; } regValS[]={
	{"language", lang, sizeof(lang)},
	{"keywordLang", keywordLang, sizeof(keywordLang)},
};
//---------------------------------------------------------------------------
void textColor(int cl)
{
	SetTextColor(dc, colorsF[cl]);
	SetBkColor(dc, colorsB[cl]);
}

void tiskpole1(int c)
{
	if(c>0){
		if(c>255){
			textColor(clznacka);
			c-=256;
			if(c<10){
				if(drawMarks){
					POINT pt;
					putCh(' ');
					MoveToEx(dc, 0, 0, &pt);
					int dy= charHeight/9;
					pt.y+=squareHeight-c*dy-1-(charHeight%9)/2;
					HGDIOBJ oldP= SelectObject(dc, CreatePen(PS_SOLID, 0, GetTextColor(dc)));
					while(c>0){
						pt.y+=dy;
						int d=(c&1)<<1;
						MoveToEx(dc, pt.x-squareWidth+d, pt.y, 0);
						LineTo(dc, pt.x-2+d, pt.y);
						c--;
					}
					DeleteObject(SelectObject(dc, oldP));
				}
				else{
					putCh(char(c+'0'));
				}
			}
			else putCh('X');
		}
		else{
			textColor(clznak);
			if(c==10 || c==13 || c==7 || c==8) putCh('');
			else putCh((char)c);
		}
	}
	else if(!c){
		textColor(clznak);
		putCh(' ');
	}
	else{
		textColor(clzed);
		putCh('²');
	}
}

void loadKarelBitmap()
{
	if(charWidth>=kaW && charHeight>kaW){
		HBITMAP kaBmp=LoadBitmap(inst, "KA");
		if(kaBmp){
			HDC dc1=CreateCompatibleDC(dc);
			HGDIOBJ oldB=SelectObject(dc1, kaBmp);
			kaDC=CreateCompatibleDC(dc);
			RECT rc;
			SetRect(&rc, 0, 0, charWidth*4, charHeight);
			SelectObject(kaDC, CreateCompatibleBitmap(dc, rc.right, rc.bottom));
			HBRUSH br=CreateSolidBrush(GetPixel(dc1, 5, 0));
			FillRect(kaDC, &rc, br);
			DeleteObject(br);
			for(int i=0; i<4; i++){
				BitBlt(kaDC, charWidth*i+(charWidth-kaW)/2, (charHeight-kaW)/2,
					kaW, kaW, dc1, kaW*i, 0, SRCCOPY);
			}
			DeleteObject(SelectObject(dc1, oldB));
			DeleteDC(dc1);
		}
	}
}

void tiskka()
{
	gotoSQ(kax, kay);
	if(kaDC){
		POINT pt;
		MoveToEx(dc, 0, 0, &pt);
		if(plColorChanged){
			plColorChanged=false;
			HGDIOBJ oldO=SelectObject(kaDC, CreateSolidBrush(colorsB[clznak]));
			ExtFloodFill(kaDC, 5, 0, GetPixel(kaDC, 5, 0), FLOODFILLSURFACE);
			DeleteObject(SelectObject(kaDC, oldO));
		}
		BitBlt(dc, pt.x, pt.y, charWidth, charHeight, kaDC, smer*charWidth, 0, SRCCOPY);
		MoveToEx(dc, pt.x+squareWidth, pt.y, 0);
	}
	else{
		textColor(clkarel);
		putCh(""[smer]);
	}
}

void putCh(char ch)
{
	/*///
	if(ch<32){
	HGDIOBJ oldF;
	POINT pt,pt2;
	MoveToEx(dc,0,0,&pt);
	MoveToEx(dc,pt.x,pt.y,0);
	TextOut(dc,0,0," ",1);
	MoveToEx(dc,pt.x,pt.y,&pt2);
	oldF=SelectObject(dc,GetStockObject(OEM_FIXED_FONT));
	TextOut(dc,0,0,&ch,1);
	MoveToEx(dc,pt2.x,pt2.y,0);
	SelectObject(dc,oldF);
	return;
	}*/
	TextOut(dc, 0, 0, &ch, 1);
}

void putS(char *s)
{
	int len = strlen(s);
	char *s2 = new char[len+1];
	CharToOem(s, s2);
	TextOut(dc, 0, 0, s2, len);
	delete[] s2;
}

void endOfStatus()
{
	RECT rc;
	POINT pt;

	HBRUSH br= CreateSolidBrush(colorsB[statusColor]);
	MoveToEx(dc, 0, 0, &pt);
	rc.top=pt.y;
	rc.left=pt.x;
	rc.bottom=rc.top+charHeight;
	rc.right=winWidth;
	FillRect(dc, &rc, br);
	DeleteObject(br);
}

void clrEol()
{
	RECT rc;
	POINT pt;

	HBRUSH br= CreateSolidBrush(GetBkColor(dc));
	MoveToEx(dc, 0, 0, &pt);
	rc.top=pt.y;
	rc.left=pt.x;
	rc.bottom=rc.top+charHeight;
	rc.right=scrollX;
	FillRect(dc, &rc, br);
	DeleteObject(br);
}

void setTimer(int id, int ms)
{
	SetTimer(hWin, id, ms, 0);
}

void killTimer(int id)
{
	KillTimer(hWin, id);
}

void showCursor(int ins)
{
	noCaret--;
	POINT p;
	int h;
	h= ins ? charHeight : 2;
	MoveToEx(dc, 0, 0, &p);
	CreateCaret(hWin, 0, charWidth, h);
	SetCaretPos(p.x, p.y+charHeight-h);
	ShowCaret(hWin);
}

void hideCursor()
{
	HideCaret(hWin);
	noCaret++;
}

void gotoXY(int x, int y)
{
	MoveToEx(dc, X0 + (x-1)*charWidth, Y0 + (y-1)*charHeight, 0);
}

void goto25()
{
	MoveToEx(dc, 0, winHeight-charHeight, 0);
}

void gotoSQ(int x, int y)
{
	MoveToEx(dc, plx + x*squareWidth, ply + y*squareHeight, 0);
}

void gotoNI()
{
	int a, b;

	a=33*charWidth;
	b=PXMAX*squareWidth;
	if(b>a) a=(a+b)>>1;
	MoveToEx(dc, winWidth-a, ply + (plym+1)*squareHeight, 0);
}

void sleep(int ms)
{
	static int d=0;

	if(ms<20){
		d+=ms;
		if(d>20){
			Sleep(20);
			d-=20;
		}
	}
	else{
		Sleep(ms);
	}
}

void waitKeyboard()
{
	MSG mesg;

	klavesa=0;
	isBusy++;
	do{
		GetMessage(&mesg, NULL, 0, 0);
		TranslateMessage(&mesg);
		DispatchMessage(&mesg);
		if(!isBusy) return;
	} while(!klavesa);
	isBusy--;
}

void readKeyboard()
{
	MSG mesg;

	isBusy++;
	while(PeekMessage(&mesg, NULL, 0, 0, PM_REMOVE)){
		TranslateMessage(&mesg);
		DispatchMessage(&mesg);
		if(!isBusy) return;
	}
	isBusy--;
}

int keyPressed()
{
	MSG mesg;
	return PeekMessage(&mesg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE);
}

int isShift()
{
	return GetKeyState(VK_SHIFT)<0;
}

HMENU getFieldMenu()
{
	HMENU m=GetMenu(hWin);
	return GetSubMenu(m, GetMenuItemCount(m)-2);
}

void plMenuAddAll()
{
	HMENU m=getFieldMenu();
	for(int i=0; i<Nfields; i++){
		InsertMenu(m, 0xFFFFFFFF, MF_BYPOSITION,
			10000+i, plochy[i].mname);
		if(&plochy[i]==plocha){
			CheckMenuItem(m, 10000+i, MF_BYCOMMAND|MF_CHECKED);
		}
	}
}

void plMenu()
{
	int i;
	HMENU m=getFieldMenu();

	for(i=0; i<=Nfields; i++){
		RemoveMenu(m, 10000+i, MF_BYCOMMAND);
	}
	plMenuAddAll();
}

void quit()
{
	DestroyWindow(hWin);
}

void cls()
{
}

void zmenitadr()
{
}

void clipboardCopy(char *s, size_t n)
{
	HGLOBAL hmem;
	char *p;
	int r;
	size_t i;

	r=0;
	for(i=0; i<n; i++){
		if(s[i]==CR) r++;
	}
	if(OpenClipboard(0)){
		if(EmptyClipboard()){
			if((hmem=GlobalAlloc(GMEM_DDESHARE, n+r+1))!=0){
				if((p=(char*)GlobalLock(hmem))!=0){
					//convert CR to CR/LF
					for(i=0; i<n; i++){
						if((*p=s[i])==CR){
							*p++='\r';
							*p='\n';
						}
						p++;
					}
					*p=0;
					GlobalUnlock(hmem);
					SetClipboardData(CF_OEMTEXT, hmem);
				}
				else{
					GlobalFree(hmem);
				}
			}
		}
		CloseClipboard();
	}
}

char *clipboardPaste(size_t &len, bool &del)
{
	HGLOBAL hmem;
	size_t n;
	char *result=0, *s, *p;

	if(OpenClipboard(0)){
		if((hmem=GetClipboardData(CF_OEMTEXT))!=0){
			if((p=(char*)GlobalLock(hmem))!=0){
				n=strlen(p);
				//convert CR/LF to CR
				result=s=new char[n];
				for(; *p; p++, s++){
					*s=*p;
					if(*p=='\r' && p[1]=='\n'){
						*s=CR;
						p++;
						n--;
					}
				}
				len=n;
				GlobalUnlock(hmem);
			}
		}
		CloseClipboard();
	}
	del=true;
	return result;
}

void showScrollBar(bool show)
{
	ShowScrollBar(hScroll, SB_CTL, show);
}

void eraseBkgnd(HDC _dc)
{
	HBRUSH br=CreateSolidBrush(colorsB[clnull]);
	RECT rc;
	//left
	rc.top=Y0;
	rc.left=scrollX;
	if(mode==MODE_TEXT) rc.left+=scrollW;
	rc.right=plx;
	rc.bottom=winHeight-charHeight;
	FillRect(_dc, &rc, br);
	//bottom
	rc.left=rc.right;
	rc.top=ply+(plym+1)*squareHeight;
	rc.right=winWidth;
	FillRect(_dc, &rc, br);
	//right
	rc.bottom=rc.top;
	rc.left=plx+(plxm+1)*squareWidth;
	rc.top=ply;
	FillRect(_dc, &rc, br);
	//top
	rc.top=Y0;
	rc.bottom=ply;
	rc.left=scrollX+gap;
	FillRect(_dc, &rc, br);
	//below editor
	rc.top=edBottom;
	rc.left=0;
	rc.right=scrollX;
	rc.bottom=winHeight-charHeight;
	if(mode==MODE_TEXT) rc.right+=scrollW;
	FillRect(_dc, &rc, br);
	DeleteObject(br);
}

int borderX()
{
	return 2*GetSystemMetrics(SM_CXFRAME);
}

int borderY()
{
	return 2*GetSystemMetrics(SM_CYFRAME)
		+ GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION);
}

int getMinX()
{
	return max(76*charWidth, PXMAX*squareWidth+gap+100);
}

int getMinY()
{
	return PYMAX*squareHeight+2*charHeight+borderY()+Y0;
}

void plResize()
{
	plx= (winWidth-(plxm+1)*squareWidth+scrollX+gap)>>1;
	ply= (winHeight-2*charHeight-(plym+1)*squareHeight+Y0)>>1;
	if(plx<50 || ply<Y0){
		SetWindowPos(hWin, 0, 0, 0, scrollX+gap+PXMAX*squareWidth+borderX(),
			Y0+PYMAX*squareHeight+2*charHeight+borderY(),
			SWP_NOZORDER|SWP_NOMOVE);
	}
}

void plInvalidate()
{
	RECT rc;
	rc.top=0;
	rc.bottom=winHeight-charHeight;
	rc.left=scrollX+gap;
	rc.right=winWidth;
	InvalidateRect(hWin, &rc, TRUE);
}

void setScroll(int top, int bottom, int pos, int page)
{
	SCROLLINFO si;

	si.cbSize = sizeof(si);
	si.fMask  = SIF_ALL;
	si.nMin   = top;
	si.nMax   = bottom;
	si.nPage  = page;
	si.nPos   = pos;
	SetScrollInfo(hScroll, SB_CTL, &si, TRUE);
}
//---------------------------------------------------------------------------
void printKey(char *s, ACCEL *a)
{
	*s=0;
	if(a->key==0) return;
	if(a->fVirt&FCONTROL) strcat(s, "Ctrl+");
	if(a->fVirt&FSHIFT) strcat(s, "Shift+");
	if(a->fVirt&FALT) strcat(s, "Alt+");
	s+=strlen(s);
	if((a->fVirt&FVIRTKEY) && (a->key<'0' || a->key>'9')){
		UINT scan=MapVirtualKey(a->key, 0);
		if(a->key==VK_DIVIDE){
			strcpy(s, "Num /");
		}
		else{
			if(a->key>0x20 && a->key<0x2f || a->key==VK_NUMLOCK){
				scan+=256; //Ins,Del,Home,End,PgUp,PgDw,Left,Right,Up,Down
			}
			GetKeyNameText(scan<<16, s, 16);
		}
	}
	else if(a->key<32){
		sprintf(s, "Ctrl+%c", a->key+'A'-1);
	}
	else{
		*s++= (char)a->key;
		*s=0;
	}
}

//change names in the menu h, recursively
static void addAccelToMenu(HMENU h)
{
	int i, id;
	char *s;
	HMENU sub;
	ACCEL *a;
	MENUITEMINFO mii;
	char buf[128];

	for(i=GetMenuItemCount(h)-1; i>=0; i--){
		id=GetMenuItemID(h, i);
		if(id<0 || id>=0xffffffff){
			sub=GetSubMenu(h, i);
			if(sub) addAccelToMenu(sub);
		}
		else{
			for(a=accel; a<accel+Naccel; a++){
				if(a->cmd==id){
					mii.cbSize=sizeof(MENUITEMINFO);
					mii.fMask=MIIM_TYPE|MFT_STRING;
					mii.dwTypeData=buf;
					mii.cch= sizeA(buf);
					if(!GetMenuItemInfo(h, i, TRUE, &mii) || (mii.fType&MFT_SEPARATOR)) continue;
					s=strchr(buf, 0);
					*s++='\t';
					printKey(s, a);
					mii.fMask=MIIM_TYPE|MIIM_STATE;
					mii.fType=MFT_STRING;
					mii.fState=MFS_ENABLED;
					mii.dwTypeData=buf;
					mii.cch= (UINT)strlen(buf);
					SetMenuItemInfo(h, i, TRUE, &mii);
					break;
				}
			}
		}
	}
}

static int menuSubId[]={414, 413, 412, 411, 410, 409, 402, 403, 408, 407, 406, 405, 404, 400, 401};

//load menu from resources
void reloadMenu()
{
	HMENU m= GetMenu(hWin);
	HMENU hMenu= loadMenu("MENU_EN", menuSubId);
	/// replace strings in menu
	addAccelToMenu(hMenu);
	SetMenu(hWin, hMenu);
	DestroyMenu(m);
	plMenuAddAll();
	DrawMenuBar(hWin);
}
//---------------------------------------------------------------------------
int vmsg(char *caption, char *formatText, int btn, va_list v)
{
	char buf[1024];
	if(!formatText) return IDCANCEL;
	_vsnprintf(buf, sizeof(buf), formatText, v);
	buf[sizeof(buf)-1]=0;
	return MessageBox(hWin, buf, caption, btn);
}

int msg3(int btn, char *caption, char *formatText, ...)
{
	va_list ap;
	va_start(ap, formatText);
	int result = vmsg(caption, formatText, btn, ap);
	va_end(ap);
	return result;
}

void msg2(char *caption, char *formatText, ...)
{
	va_list ap;
	va_start(ap, formatText);
	vmsg(caption, formatText, MB_OK|MB_ICONERROR, ap);
	va_end(ap);
}

int msg1(int btn, char *formatText, ...)
{
	va_list ap;
	va_start(ap, formatText);
	int result = vmsg(title, formatText, btn, ap);
	va_end(ap);
	return result;
}

void msg(char *formatText, ...)
{
	va_list ap;
	va_start(ap, formatText);
	vmsg(title, formatText, MB_OK|MB_ICONERROR, ap);
	va_end(ap);
}

/*char *strConvert(const char *s)
{
if(!s) return 0;
size_t len=strlen(s);
char *d= new char[len+1];
OemToChar(s,d);
return d;
}
*/

void oknotxt(char *name, char *message, int /*w*/, int /*h*/)
{
	msg3(MB_OK, name, "%s", message);
}

void oknochyb(char *message)
{
	msg("%s", message);
}

int oknoano(char *name, char *message)
{
	int result;
	switch(msg3(MB_YESNOCANCEL, name, "%s", message)){
		case IDYES:
			result=1;
			break;
		case IDNO:
			result=2;
			break;
		default:
			result=0;
	}
	return result;
}

int oknofile(char *name, char *ext, char *var, int action)
{
	static OPENFILENAME ofn={
		sizeof(OPENFILENAME), 0, 0, 0, 0, 0, 1,
		0, 256,
		0, 0, 0, 0, 0, 0, 0, "KRL", 0, 0, 0
	};
	int result;

	ofn.hwndOwner=hWin;
	if(_stricmp(ext, EXTPL)){
		ofn.lpstrFilter=lng(200, "Karel source (*.krl)\0*.krl\0All files\0*.*\0");
	}
	else{
		ofn.lpstrFilter=lng(201, "Field (*.kpl)\0*.kpl\0All files\0*.*\0");
	}
	ofn.lpstrFile=var;
	ofn.lpstrTitle=name;
	ofn.lpstrDefExt=ext;
	if(ext[0]=='.') ofn.lpstrDefExt++;

	for(;;){
		ofn.Flags=OFN_PATHMUSTEXIST|OFN_HIDEREADONLY;
		result= action ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn);
		if(result) break; //ok
		if(CommDlgExtendedError()!=FNERR_INVALIDFILENAME
			|| !var[0]) break; //cancel
		var[0]=0;
	}

	return result;
}
//---------------------------------------------------------------------------
int getRadioButton(HWND hWnd, int item1, int item2)
{
	for(int i=item1; i<=item2; i++){
		if(IsDlgButtonChecked(hWnd, i)){
			return i-item1;
		}
	}
	return 0;
}

int getCheckButton(HWND hWnd, int item)
{
	return IsDlgButtonChecked(hWnd, item)==BST_CHECKED;
}
//---------------------------------------------------------------------------
struct TintDlg {
	char *name, *text;
	int *var;
};

BOOL CALLBACK IntProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP)
{
	switch(msg){
		case WM_INITDIALOG:
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)lP);
			SetWindowText(hWnd, ((TintDlg*)lP)->name);
			SetDlgItemText(hWnd, 102, ((TintDlg*)lP)->text);
			SetDlgItemInt(hWnd, 101, *((TintDlg*)lP)->var, TRUE);
			setDlgTexts(hWnd);
			return TRUE;

		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case IDOK:
					*((TintDlg*)GetWindowLong(hWnd, GWL_USERDATA))->var =
						GetDlgItemInt(hWnd, 101, 0, TRUE);
				case IDCANCEL:
					EndDialog(hWnd, wP);
					return TRUE;
			}
			break;
	}
	return 0;
}
//---------------------------------------------------------------------------
int oknoint(char *name, char *label, int &var)
{
	TintDlg d;
	d.name=name;
	d.text=label;
	d.var=&var;
	return DialogBoxParam(inst, "INT", hWin, (DLGPROC)IntProc, (LONG)&d)==IDOK;
}
//---------------------------------------------------------------------------
DWORD getVer()
{
	HRSRC r;
	HGLOBAL h;
	void *s;
	VS_FIXEDFILEINFO *v;
	UINT i;

	r=FindResource(0, (TCHAR*)VS_VERSION_INFO, RT_VERSION);
	h=LoadResource(0, r);
	s=LockResource(h);
	if(!s || !VerQueryValueA(s, "\\", (void**)&v, &i)) return 0;
	return v->dwFileVersionMS;
}

BOOL CALLBACK AboutProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM)
{
	char buf[48];
	DWORD d;

	switch(msg){
		case WM_INITDIALOG:
			setDlgTexts(hWnd, 38);
			d=getVer();
			sprintf(buf, "%d.%d", HIWORD(d), LOWORD(d));
			SetDlgItemTextA(hWnd, 101, buf);
			return TRUE;

		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case IDOK:
				case IDCANCEL:
					EndDialog(hWnd, wP);
					return TRUE;
				case 123:
					GetDlgItemTextA(hWnd, wP, buf, sizeA(buf)-13);
					if(strcmp(lang, "Czech")) strcat(buf, "/indexEN.html");
					ShellExecuteA(0, 0, buf, 0, 0, SW_SHOWNORMAL);
					break;
			}
			break;
	}
	return 0;
}
//---------------------------------------------------------------------------
BOOL CALLBACK OptionsProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM)
{
	static int* const O[]={&persistblok, &overwrblok, &indent, &unindent,
		&foncur, &entern, &groupUndo, &undoSave, &upcasekeys, &drawMarks};
	int i;

	switch(msg){
		case WM_INITDIALOG:
			setDlgTexts(hWnd, 40);
			for(i=0; i<sizeA(O); i++){
				CheckDlgButton(hWnd, 300+i, *O[i] ? BST_CHECKED : BST_UNCHECKED);
			}
			SetDlgItemInt(hWnd, 101, tabs, FALSE);
			SetDlgItemText(hWnd, 102, keywordLang);
			return TRUE;

		case WM_COMMAND:
			wP=LOWORD(wP);
			switch(wP){
				case IDOK:
					for(i=0; i<sizeA(O); i++){
						*O[i]=getCheckButton(hWnd, 300+i);
					}
					tabs= GetDlgItemInt(hWnd, 101, 0, FALSE);
					GetDlgItemText(hWnd, 102, keywordLang, sizeA(keywordLang));
				case IDCANCEL:
					EndDialog(hWnd, wP);
					return TRUE;
			}
			break;
	}
	return 0;
}
//---------------------------------------------------------------------------
BOOL CALLBACK FindProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP)
{
	int repl;
	static char *windowTitle[2]={"Find text", "Replace text"};

	switch(msg){
		case WM_INITDIALOG:
			OemToChar(ffind, ffind);
			OemToChar(freplace, freplace);
			if(!lP){
				DestroyWindow(GetDlgItem(hWnd, 102));
				DestroyWindow(GetDlgItem(hWnd, 332));
				DestroyWindow(GetDlgItem(hWnd, 323));
			}
			setDlgTexts(hWnd);
			SetWindowText(hWnd, lng(41+lP, windowTitle[lP]));
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)lP);
			SetDlgItemText(hWnd, 101, ffind);
			if(lP) SetDlgItemText(hWnd, 102, freplace);
			CheckDlgButton(hWnd, 324, fword ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, 325, fcase ? BST_CHECKED : BST_UNCHECKED);
			CheckRadioButton(hWnd, 327, 328, 327+fsmer);
			CheckRadioButton(hWnd, 330, 331, 330+fod);
			return TRUE;

		case WM_COMMAND:
			wP=LOWORD(wP);
			repl=GetWindowLong(hWnd, GWL_USERDATA);
			switch(wP){
				case 332:
				case IDOK:
					GetDlgItemText(hWnd, 101, ffind, sizeof(ffind)-1);
					if(repl) GetDlgItemText(hWnd, 102, freplace, sizeof(freplace)-1);
					fword= getCheckButton(hWnd, 324);
					fcase= getCheckButton(hWnd, 325);
					fsmer= getRadioButton(hWnd, 327, 328);
					fod= getRadioButton(hWnd, 330, 331);
				case IDCANCEL:
					CharToOem(ffind, ffind);
					CharToOem(freplace, freplace);
					EndDialog(hWnd, (wP==IDOK) ? 1 : (wP==IDCANCEL ? 0 : 3));
					return TRUE;
			}
			break;
	}
	return 0;
}
//---------------------------------------------------------------------------
void langChanged()
{
	reloadMenu();
	if(!lng(11, 0)){
		//delete Help menu item if the doc file is not available
		DeleteMenu(GetMenu(hWin), 60000, MF_BYCOMMAND);
		DrawMenuBar(hWin);
	}
	printNinstr();
}
//------------------------------------------------------------------
void initMenuPopup(HMENU h)
{
	for(int i=GetMenuItemCount(h)-1; i>=0; i--){
		int id=GetMenuItemID(h, i);
		for(TmenuEnableCallback *e = menuEnableCallback; e < endA(menuEnableCallback); e++){
			if(e->cmd == id){
				EnableMenuItem(h, id, MF_BYCOMMAND | (e->enabled() ? MF_ENABLED : MF_GRAYED));
				break;
			}
		}
	}
}
//------------------------------------------------------------------
void accelChanged()
{
	DestroyAcceleratorTable(haccel);
	if(Naccel>0){
		haccel= CreateAcceleratorTable(accel, Naccel);
	}
	else{
		haccel= LoadAccelerators(inst, "ACC");
		Naccel= CopyAcceleratorTable(haccel, accel, sizeA(accel));
	}
}

void getCmdName(char *buf, int n, int cmd)
{
	char *s, *d;

	*buf=0;
	MENUITEMINFO mii;
	mii.cbSize=sizeof(MENUITEMINFO);
	mii.fMask=MIIM_TYPE;
	mii.dwTypeData=buf;
	mii.cch= n;
	GetMenuItemInfo(GetMenu(hWin), cmd, FALSE, &mii);
	for(s=d=buf; *s; s++){
		if(*s=='\t') *s=0;
		if(*s!='&') *d++=*s;
	}
	*d=0;
}

void changeKey(HWND hDlg, UINT vkey)
{
	char *buf=(char*)_alloca(64);
	BYTE flg;
	ACCEL *a, *e;

	dlgKey.key=(USHORT)vkey;
	//read shift states
	flg=FVIRTKEY|FNOINVERT;
	if(GetKeyState(VK_CONTROL)<0) flg|=FCONTROL;
	if(GetKeyState(VK_SHIFT)<0) flg|=FSHIFT;
	if(GetKeyState(VK_MENU)<0) flg|=FALT;
	dlgKey.fVirt=flg;
	//set hotkey edit control
	printKey(buf, &dlgKey);
	SetWindowText(GetDlgItem(hDlg, 99), buf);
	//modify accelerator table
	e=accel+Naccel;
	for(a=accel; a<e; a++){
		if(a->key==vkey && a->fVirt==flg){
			PostMessage(hDlg, WM_COMMAND, a->cmd, 0);
			return;
		}
	}
	if(dlgKey.cmd){
		for(a=accel;; a++){
			if(a==e){
				*a=dlgKey;
				Naccel++;
				break;
			}
			if(a->cmd==dlgKey.cmd){
				a->fVirt=dlgKey.fVirt;
				a->key=dlgKey.key;
				if(vkey==0){
					memcpy(a, a+1, (e-a-1)*sizeof(ACCEL));
					Naccel--;
				}
				break;
			}
		}
		accelChanged();
		//change the main menu
		reloadMenu();
	}
}

//------------------------------------------------------------------
static WNDPROC editWndProc;

//window procedure for hotkey edit control
LRESULT CALLBACK hotKeyClassProc(HWND hWnd, UINT mesg, WPARAM wP, LPARAM lP)
{
	switch(mesg){
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if(wP!=VK_CONTROL && wP!=VK_SHIFT && wP!=VK_MENU && wP!=VK_RWIN && wP!=VK_LWIN){
				if((wP==VK_BACK || wP==VK_DELETE) && dlgKey.key) wP=0;
				changeKey(GetParent(hWnd), (UINT)wP);
				return 0;
			}
			break;
		case WM_CHAR:
			return 0; //don't write character to the edit control
	}
	return CallWindowProc((WNDPROC)editWndProc, hWnd, mesg, wP, lP);
}

LRESULT CALLBACK KeysDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM)
{
	static const int Mbuf=64;
	char *buf=(char*)_alloca(Mbuf);
	int i, n;

	switch(message){
		case WM_INITDIALOG:
			editWndProc = (WNDPROC)SetWindowLong(GetDlgItem(hWnd, 99),
				GWL_WNDPROC, (LONG)hotKeyClassProc);
			setDlgTexts(hWnd, 43);
			dlgKey.cmd=0;
			return TRUE;

		case WM_COMMAND:
			wParam=LOWORD(wParam);
			switch(wParam){
				default:
					//popup menu item
					if(wParam>400 && wParam<2000 || wParam>=100 && wParam<300){
						//set command text
						dlgKey.cmd=(USHORT)wParam;
						getCmdName(buf, Mbuf, wParam);
						SetDlgItemText(hWnd, 98, buf);
						HWND hHotKey=GetDlgItem(hWnd, 99);
						if(!*buf) dlgKey.cmd=0;
						else SetFocus(hHotKey);
						//change accelerator table
						dlgKey.key=0;
						*buf=0;
						for(ACCEL *a=accel; a<accel+Naccel; a++){
							if(a->cmd==dlgKey.cmd){
								dlgKey.fVirt=a->fVirt;
								dlgKey.key=a->key;
								printKey(buf, &dlgKey);
								break;
							}
						}
						//set hotkey edit control
						SetWindowText(hHotKey, buf);
					}
					break;
				case 111:
				{
					RECT rc;
					GetWindowRect(GetDlgItem(hWnd, 111), &rc);
					//create popup menu from the main menu
					HMENU hMenu= CreatePopupMenu();
					MENUITEMINFO mii;
					n=GetMenuItemCount(GetMenu(hWin));
					for(i=0; i<n; i++){
						mii.cbSize=sizeof(mii);
						mii.fMask=MIIM_SUBMENU|MIIM_TYPE;
						mii.cch=Mbuf;
						mii.dwTypeData=buf;
						GetMenuItemInfo(GetMenu(hWin), i, TRUE, &mii);
						InsertMenuItem(hMenu, 0xffffffff, TRUE, &mii);
					}
					//show popup menu
					TrackPopupMenuEx(hMenu,
						TPM_RIGHTBUTTON, rc.left, rc.bottom, hWnd, NULL);
					//delete popup menu
					for(i=0; i<n; i++){
						RemoveMenu(hMenu, 0, MF_BYPOSITION);
					}
					DestroyMenu(hMenu);
					break;
				}
				case IDOK:
				case IDCANCEL:
					EndDialog(hWnd, wParam);
					return TRUE;
			}
	}
	return FALSE;
}

void changekeys()
{
	DialogBox(inst, "KEYS", hWin, (DLGPROC)KeysDlgProc);
}

//---------------------------------------------------------------------------
static COLORREF custom[16]={
	0x000080, 0x800090, 0x00a000, 0xe0ffe8,
	0x00ffff, 0xc00000, 0x70a000, 0x0000a0,
	0x500000, 0x5080c0, 0xf0ea80, 0x600000,
	0x2020e0, 0xa06000, 0xc0ffff, 0xf9f9fc
};

void colorChanged()
{
	plColorChanged=true;
	repaintAll();
}

BOOL CALLBACK ColorProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP)
{
	static bool chng;
	static CHOOSECOLOR chc;
	static COLORREF cloldF[Ncl], cloldB[Ncl];
	DRAWITEMSTRUCT *di;
	HBRUSH br;
	int cmd;
	RECT rc;

	switch(msg){
		case WM_INITDIALOG:
			setDlgTexts(hWnd, 44);
			memcpy(cloldF, colorsF, sizeof(cloldF));
			memcpy(cloldB, colorsB, sizeof(cloldB));
			chng=false;
			return TRUE;

		case WM_DRAWITEM:
			di = (DRAWITEMSTRUCT*)lP;
			DrawFrameControl(di->hDC, &di->rcItem, DFC_BUTTON,
				DFCS_BUTTONPUSH|(di->itemState&ODS_SELECTED ? DFCS_PUSHED : 0));
			CopyRect(&rc, &di->rcItem);
			InflateRect(&rc, -3, -3);
			cmd= di->CtlID;
			br= CreateSolidBrush(cmd<200 ? colorsF[cmd-100] : colorsB[cmd-200]);
			FillRect(di->hDC, &rc, br);
			DeleteObject(br);
			break;

		case WM_COMMAND:
			cmd=LOWORD(wP);
			switch(cmd){
				default: //color square
					if(cmd>=100 && cmd<100+sizeA(colorsF)){
						chc.rgbResult=colorsF[cmd-100];
					}
					else if(cmd>=200 && cmd<200+sizeA(colorsB)){
						chc.rgbResult=colorsB[cmd-200];
					}
					else break;
					chc.lStructSize= sizeof(CHOOSECOLOR);
					chc.hwndOwner= hWnd;
					chc.hInstance= 0;
					chc.lpCustColors= custom;
					chc.Flags= CC_RGBINIT|CC_FULLOPEN;
					if(ChooseColor(&chc)){
						if(cmd<200) colorsF[cmd-100]=chc.rgbResult;
						else colorsB[cmd-200]=chc.rgbResult;
						InvalidateRect(GetDlgItem(hWnd, cmd), 0, TRUE);
						colorChanged();
						chng=true;
					}
					break;
				case IDCANCEL:
					if(chng){
						memcpy(colorsF, cloldF, sizeof(cloldF));
						memcpy(colorsB, cloldB, sizeof(cloldB));
						colorChanged();
					}
					//!
				case IDOK:
					EndDialog(hWnd, cmd);
			}
			break;
	}
	return FALSE;
}

void changecolors()
{
	DialogBox(inst, "COLOR", hWin, (DLGPROC)ColorProc);
}

//---------------------------------------------------------------------------
//delete settings from registry
void deleteini()
{
	HKEY key;
	DWORD i;

	if(MessageBox(hWin, lng(797, "Do you want to delete your settings ?"),
			title, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) ==IDYES){
		delreg=true;
		if(RegDeleteKey(HKEY_CURRENT_USER, subkey)==ERROR_SUCCESS){
			if(RegOpenKey(HKEY_CURRENT_USER,
				"Software\\Petr Lastovicka", &key)==ERROR_SUCCESS){
				i=1;
				RegQueryInfoKey(key, 0, 0, 0, &i, 0, 0, 0, 0, 0, 0, 0);
				RegCloseKey(key);
				if(!i)
					RegDeleteKey(HKEY_CURRENT_USER, "Software\\Petr Lastovicka");
			}
		}
	}
}

//write settings to registry
void writeini()
{
	HKEY key;
	if(RegCreateKey(HKEY_CURRENT_USER, subkey, &key)!=ERROR_SUCCESS)
		oknochyb(lng(798, "Cannot write to Windows registry"));
	else{
		for(Treg *u=regVal; u<endA(regVal); u++){
			RegSetValueEx(key, u->s, 0, REG_DWORD,
				(BYTE *)u->i, sizeof(int));
		}
		regValB[0].n=Naccel*sizeof(ACCEL);
		for(Tregb *w=regValB; w<endA(regValB); w++){
			RegSetValueExA(key, w->s, 0, REG_BINARY, (BYTE *)w->i, w->n);
		}
		for(Tregs *v=regValS; v<endA(regValS); v++){
			RegSetValueEx(key, v->s, 0, REG_SZ,
				(BYTE *)v->i, (DWORD)strlen(v->i)+1);
		}
		RegCloseKey(key);
	}
}

//read settings from registry
void readini()
{
	HKEY key;
	DWORD d;
	if(RegOpenKey(HKEY_CURRENT_USER, subkey, &key)==ERROR_SUCCESS){
		for(Treg *u=regVal; u<endA(regVal); u++){
			d=sizeof(int);
			RegQueryValueEx(key, u->s, 0, 0, (BYTE *)u->i, &d);
		}
		for(Tregb *w=regValB; w<endA(regValB); w++){
			d=w->n;
			if(RegQueryValueExA(key, w->s, 0, 0, (BYTE *)w->i, &d)==ERROR_SUCCESS
				&& w==regValB) Naccel=d/sizeof(ACCEL);
		}
		for(Tregs *v=regValS; v<endA(regValS); v++){
			d=v->n;
			RegQueryValueEx(key, v->s, 0, 0, (BYTE *)v->i, &d);
		}
		RegCloseKey(key);
	}
}
//---------------------------------------------------------------------------
void moznosti()
{
	DialogBox(inst, "OPTIONS", hWin, (DLGPROC)OptionsProc);
	plInvalidate();
	loadKeywords();
	updwkeys();
	text.lst();
}

void oprogramu()
{
	DialogBox(inst, "ABOUT", hWin, (DLGPROC)AboutProc);
}

int findDlg()
{
	return DialogBoxParam(inst, "FIND", hWin, (DLGPROC)FindProc, 0);
}

int replaceDlg()
{
	return DialogBoxParam(inst, "FIND", hWin, (DLGPROC)FindProc, 1);
}

//---------------------------------------------------------------------------
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT mesg, WPARAM wP, LPARAM lP)
{
	RECT rc;

	switch(mesg){

		case WM_COMMAND:
			wP=LOWORD(wP);
			if(setLang(wP)) break;
			if(wP==60000){
				char *buf=(char*)_alloca(MAX_PATH);
				char *file = lng(11, 0);
				if(file){
					getExeDir(buf, file);
					ShellExecute(0, 0, buf, 0, 0, SW_SHOWNORMAL);
				}
				break;
			}
			wm_command(LOWORD(wP));
			break;

		case WM_TIMER:
			wm_timer(wP);
			break;

		case WM_MOUSEMOVE:
			wm_mouse((short)LOWORD(lP), (short)HIWORD(lP), 7);
			break;

		case WM_LBUTTONDOWN:
			if(!rdown){
				ldown=true;
				SetCapture(hWin);
			}
			wm_mouse((short)LOWORD(lP), (short)HIWORD(lP), 1);
			break;

		case WM_RBUTTONDOWN:
			if(!ldown){
				rdown=true;
				SetCapture(hWin);
			}
			wm_mouse((short)LOWORD(lP), (short)HIWORD(lP), 2);
			break;

		case WM_LBUTTONUP:
			if(ldown){
				ReleaseCapture();
				ldown=false;
			}
			wm_mouse((short)LOWORD(lP), (short)HIWORD(lP), 3);
			break;

		case WM_RBUTTONUP:
			if(rdown){
				ReleaseCapture();
				rdown=false;
			}
			wm_mouse((short)LOWORD(lP), (short)HIWORD(lP), 4);
			break;

		case 0x20A: //WM_MOUSEWHEEL
		{
			static int d;
			d+=(short)HIWORD(wP);
			while(d>=wheelSpeed){
				wm_command(1006);
				d-=wheelSpeed;
			}
			while(d<=-wheelSpeed){
				wm_command(1007);
				d+=wheelSpeed;
			}
		}
			break;

		case WM_CHAR:
		{
			char cDos, cWin;
			cWin=(char)wP;
			CharToOemBuff(&cWin, &cDos, 1);
			wm_char(cDos);
		}
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if(isBusy && wP!=VK_SHIFT && wP!=VK_CONTROL && wP!=VK_MENU){
				int c= -(HIWORD(lP)&255);
				if(GetKeyState(VK_CONTROL)<0){
					if(c==-75) c=-115;
					if(c==-77) c=-116;
					if(c==-72) c=-141;
					if(c==-80) c=-145;
					if(c==-71) c=-119;
					if(c==-79) c=-117;
					if(c==-73) c=-132;
					if(c==-81) c=-118;
					if(c==-82) c=-146;
					if(c==8) c=-127;
					if(c==13) c=10;
					if(c<=-59 && c>-69) c-=35;
				}
				wm_char(c);
			}
			break;

		case WM_ERASEBKGND:
			eraseBkgnd((HDC)wP);
			return 1;

		case WM_PAINT:{
			static PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			repaintAll();
			EndPaint(hWnd, &ps);
		}
			break;

		case WM_SIZE:{
			if(wP==SIZE_MINIMIZED) break;
			winWidth=LOWORD(lP);
			winHeight=HIWORD(lP);
			if(!IsZoomed(hWin)){
				winW=winWidth;
				winH=winHeight;
			}
			text.setRect(
				(winWidth-max(PXMAX*squareWidth, 33*charWidth)-gap)/charWidth,
				(winHeight-Y0)/charHeight-1);
			scrollX= X0+xmax*charWidth;
			edBottom= Y0+ymax*charHeight;
			plResize();
			MoveWindow(hScroll, scrollX, Y0, scrollW, edBottom-Y0, FALSE);
			SendMessage(toolbar, TB_AUTOSIZE, 0, 0);
			GetWindowRect(toolbar, &rc);
			Y0= toolbarH= rc.bottom-rc.top;
			GetClientRect(hWin, &rc);
			rc.top=Y0;
			InvalidateRect(hWin, &rc, TRUE);
		}
			break;

		case WM_GETMINMAXINFO:
			((MINMAXINFO FAR*) lP)->ptMinTrackSize.x = getMinX();
			((MINMAXINFO FAR*) lP)->ptMinTrackSize.y = getMinY();
			break;

		case WM_VSCROLL:
			switch(LOWORD(wP)){
				case SB_PAGEUP:
					wm_command(1010);
					break;
				case SB_PAGEDOWN:
					wm_command(1011);
					break;
				case SB_LINEUP:
					wm_command(1006);
					break;
				case SB_LINEDOWN:
					wm_command(1007);
					break;
				case SB_THUMBTRACK:
					if(mode==MODE_TEXT){
						beforeCmd();
						text.setTop(HIWORD(wP));
						afterCmd();
					}
					break;
			}
			break;

		case WM_NOTIFY:{
			LPNMHDR nmhdr = (LPNMHDR)lP;
			switch(nmhdr->code){
				case TTN_NEEDTEXT:
					TOOLTIPTEXT *ttt = (LPTOOLTIPTEXT)lP;
					int id= ttt->hdr.idFrom+1000;
					char *s= lng(id, 0);
					if(s){
						ttt->hinst= NULL;
						ttt->lpszText= s;
					}
					else{
						ttt->hinst= inst;
						ttt->lpszText= MAKEINTRESOURCE(id);
					}
					break;
			}
		}
			break;

		case WM_SETFOCUS:
		{
			int c=noCaret;
			if(!noCaret) beforeWait();
			noCaret=c;
		}
			break;

		case WM_KILLFOCUS:
			DestroyCaret();
			break;

		case WM_INITMENUPOPUP:
			initMenuPopup((HMENU)wP);
			break;

		case WM_MOVE:
			if(!IsZoomed(hWin) && !IsIconic(hWin)){
				RECT rcw;
				GetWindowRect(hWnd, &rcw);
				winX= rcw.left;
				winY= rcw.top;
			}
			break;

		case WM_QUERYENDSESSION:
			if(!onCloseQuery()) return FALSE;
			return TRUE;

		case WM_CLOSE:
			wm_command(120);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, mesg, wP, lP);
	}
	return 0;
}
//---------------------------------------------------------------------------
int pascal WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR param, int cmdShow)
{
	WNDCLASS wc;
	MSG mesg;
	static const TBBUTTON tbb[]={
		{0, 1081, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{1, 1082, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{2, 516, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{3, 515, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{23, 502, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{0, 0, 0, TBSTYLE_SEP, {0}, 0},
		{5, 1038, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{4, 1039, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{6, 1035, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{8, 1036, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{7, 1037, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{0, 0, 0, TBSTYLE_SEP, {0}, 0},
		{12, 500, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{13, 501, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{24, 518, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{14, 506, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{10, 508, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{11, 507, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{15, 511, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{9, 505, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{16, 513, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{0, 0, 0, TBSTYLE_SEP, {0}, 0},
		{18, 102, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{19, 100, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{20, 101, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{21, 104, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{22, 103, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
		{17, 105, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0},
	};

	inst=hInstance;

	//DPIAware
	typedef BOOL(WINAPI *TGetProcAddress)();
	TGetProcAddress getProcAddress = (TGetProcAddress) GetProcAddress(GetModuleHandle("user32"), "SetProcessDPIAware");
	if(getProcAddress) getProcAddress();

#if _WIN32_IE >= 0x0300
	INITCOMMONCONTROLSEX iccs;
	iccs.dwSize= sizeof(INITCOMMONCONTROLSEX);
	iccs.dwICC= ICC_BAR_CLASSES;
	InitCommonControlsEx(&iccs);
#else
	InitCommonControls();
#endif

	readini();
	initLang();

	wc.style= CS_OWNDC;
	wc.lpfnWndProc= MainWndProc;
	wc.cbClsExtra= 0;
	wc.cbWndExtra= 0;
	wc.hInstance= hInstance;
	wc.hIcon= LoadIcon(hInstance, MAKEINTRESOURCE(1));
	wc.hCursor= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground= 0;
	wc.lpszMenuName= 0;
	wc.lpszClassName= "Karel";
	if(!hPrevInst && !RegisterClass(&wc)) return 1;

	hWin = CreateWindow("Karel", title,
		WS_OVERLAPPEDWINDOW,
		winX, winY, winW+borderX(), winH+borderY(),
		NULL, NULL, hInstance, NULL);
	if(!hWin) return 2;

	int n=sizeA(tbb);
	for(const TBBUTTON *u=tbb; u<endA(tbb); u++){
		if(u->fsStyle==TBSTYLE_SEP) n--;
	}
	toolbar = CreateToolbarEx(hWin,
		WS_CHILD|TBSTYLE_TOOLTIPS/*|0x800*/, 2, n,
		inst, 10, tbb, sizeA(tbb),
		16, 16, 16, 15, sizeof(TBBUTTON));
	dc= GetDC(hWin);
	SetTextAlign(dc, TA_UPDATECP);
	SetBkMode(dc, OPAQUE);

	LOGFONT fnt;
	memset(&fnt, 0, sizeof(LOGFONT));
	fnt.lfHeight=-20;
	fnt.lfWidth=10;
	fnt.lfCharSet=OEM_CHARSET;
	fnt.lfPitchAndFamily=FIXED_PITCH;
	strcpy(fnt.lfFaceName, "Terminal");
	/*///
	static CHOOSEFONT f;
	f.lStructSize=sizeof(CHOOSEFONT);
	f.Flags=CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_FIXEDPITCHONLY|CF_SELECTSCRIPT;
	f.lpLogFont=&fnt;
	ChooseFont(&f);*/
	HFONT fo=CreateFontIndirect(&fnt);
	if(!fo) fo=(HFONT)GetStockObject(OEM_FIXED_FONT);
	SelectObject(dc, fo);
	SIZE sz;
	GetTextExtentPoint32(dc, "A", 1, &sz);
	charWidth=squareWidth=sz.cx;
	charHeight=squareHeight=sz.cy;
	loadKarelBitmap();
	hScroll= CreateWindowEx(0, "SCROLLBAR", 0,
		WS_VISIBLE | WS_CHILD | SBS_VERT,
		0, 0, 0, 0, hWin, 0, hInstance, 0);

	accelChanged();
	haccelField=LoadAccelerators(hInstance, "ACCFLD");
	langChanged();
	startup(param);
	ShowWindow(hWin, cmdShow);
	if(toolBarVisible) ShowWindow(toolbar, SW_SHOW);
	UpdateWindow(hWin);
	UpdateWindow(toolbar);

	for(;;){
		if(GetMessage(&mesg, NULL, 0, 0)!=TRUE) break;
		if(ctrlp || ctrlk || (mode!=MODE_FIELD ||
			 !TranslateAccelerator(hWin, haccelField, &mesg))
			 && !TranslateAccelerator(hWin, haccel, &mesg)){
			TranslateMessage(&mesg);
			DispatchMessage(&mesg);
		}
	}
	DestroyAcceleratorTable(haccel);
	return 0;
}
//---------------------------------------------------------------------------
#endif

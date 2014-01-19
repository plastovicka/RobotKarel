/*
 (C) 1999 Petr Lastovicka

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License.
 */
#ifndef mouseH
#define mouseH

int mousereset();
void mouseon();
void mouseoff();
void mousexy(int &x, int &y);
void mouse8xy(int &x, int &y);
void mouserel(int &x, int &y);
int button();
int click();
int rclick();
int mouseup();
int rmouseup();
int necekej(int &x, int &y);
int necekej8(int &x, int &y);
int cekej(int &x, int &y);
int cekej8(int &x, int &y);

#endif

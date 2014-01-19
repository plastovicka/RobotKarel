#if !defined(__WIN32__) && !defined(_WIN32)
  #define DOS
  typedef int bool;
  #define false 0
  #define true 1
  #include <bios.h>
  #include <dos.h>
  #include <conio.h>
  #include <dir.h>
  #include <alloc.h>
#else
  #include <windows.h>
  #include <commctrl.h>
  #include <malloc.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "assert.h"

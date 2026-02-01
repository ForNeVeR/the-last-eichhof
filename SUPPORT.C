
// module 'SUPPORT.C'
// SPDX-FileCopyrightText: 1993 ALPHA-HELIX <http://www.ife.ee.ethz.ch/~ammann/alphahelix/>
// SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE


// Turn of unwanted warnings.
#pragma warn -stu

#pragma hdrstop
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>

#include "xmode.h"
#include "baller.h"
#include "sound.h"

/*------------------------------------------------------
Function: setspeed

Description: Sets the speed of the timer.
	     0x0000 is about 18 ticks per second.
	     0x8000 "    "   36   "    "    "   .
------------------------------------------------------*/
void setspeed(unsigned speed)
{
   outportb(0x43, 0x36);
   outportb(0x40, speed & 0xff);
   outportb(0x40, speed >> 8);
   inportb(0x61);

}



void waitforkey(void)
{
   while (pressedkeys);
   while (!pressedkeys);
}

int waitdelayedkey(int count)
{
   while (!pressedkeys && --count) waitfortick();
   return (count) ? 1 : 0;
}


void writetext(int x, int y, const char *s, struct sprstrc *font)
{
   int   i;
   int   xs;

   xs = font->xs;
   i = 0;
   while (s[i] != 0) {
      putspritedirect(font, x, y, s[i++] - ' ');
      x += xs;
   }

}


void writenumber(int x, int y, long number, struct sprstrc *font)
{
   int j, xs, i = 0;
   int zahl;

   xs = font->xs;
   for(j = 0; j < 2; j++){
      zahl = number % 10;
      number /= 10;
      putspritedirect(font, x, y, 16 + zahl);
      x -= xs;
   }
   putspritedirect(font, x, y, '.'-' ');
   for(; j < 10; j++){
      x -= xs;
      zahl = number % 10;
      number /= 10;
      if(zahl != 0 || j == 2){
	 putspritedirect(font, x, y, 16 + zahl);
	 if(i!=0) for(; i > 0; i--) putspritedirect(font, x+i*xs, y, 16);
	 i = 0;
      }else{
	 i ++;
      }
   }
}

void killallbuddies(void)
{
   int   i;

   for (i = 0; i < MAXARMS; i++) _arm[i].object = -1;
   for (i = 0; i < MAXFOES; i++) _foe[i].object = -1;
   for (i = 0; i < MAXSHOTS; i++) _shot[i].object = -1;
   for (i = 0; i < MAXEXPLS; i++) _expl[i].object = -1;

}


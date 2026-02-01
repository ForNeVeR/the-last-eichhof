
// Intro [c] Alpha-Helix 1992
// written by Dany Schoch

#pragma warn -stu
#pragma hdrstop

#include <dos.h>
#include <mem.h>

#include "xmode.h"
#include "sound.h"
#include "fileman.h"
#include "baller.h"


#define SCROLLY			311

#define BYTESPERLINE		80
#define SCREENSIZE		28000
#define selectwrite(plane)      outport(0x3c4, (plane << 8) | 0x02)

static int loadEGA(char *file)
{
   char  *data;
   int   dataptr;
   int   count, i, page;
   char  far *scrptr;


   data = loadfile(datapool, file);
   dataptr = 0;

   for (page = 1; page<=8; page <<= 1) {
      selectwrite(page);
      scrptr = MK_FP(0xa000, 0);
      while (FP_OFF(scrptr) < SCREENSIZE) {
	 count = data[dataptr++];
	 if (count < 128) {
	    for (i = 0; i <= count; i++) {
	       *scrptr = data[dataptr];
	       scrptr++;
	    }
	    dataptr++;
	 } else {
	    for (i = 0; i <= count - 128; i++) {
	       *scrptr = data[dataptr++];
	       scrptr++;
	    }
	 }
      }
   }
   selectwrite(0x0f);
   unloadfile(data);

   return 0;
}


static void putpaper(int c, int s, char *paper)
{
   int   i;
   char  far *ptr;
   char  far *data;

   c -= 32;
   data = &paper[(c*5+s)*40];
   ptr = MK_FP(0xa000, (SCROLLY+1)*BYTESPERLINE - 1);

   for (i = 0; i < 40; i++) {
      *ptr = *data;
      data++;
      ptr += BYTESPERLINE;
   }

}

static void paperscroll(void)
{

   movedata(0xa000, SCROLLY*BYTESPERLINE + 1,
	    0xa000, SCROLLY*BYTESPERLINE, 39*BYTESPERLINE);
}


void intro(void)
{
   char   *paper;
   struct sndstrc *snd;

   screenmode(0x10);			// Switch to 640x350 mode.
   setattrib(1);
   setvanillapalette(0);
   snd_cli();
   loadEGA("blick.pak");
   paper = loadfile(datapool, "paper.fnt");
   snd_sti();
   snd = loadfile(datapool, "blick.snd");
   playloop(snd);
   glowto(63, 63, 63);
   glowin(1);

   {
      int  l, c;
      char text[] =
	 " . . . ALPHA HELIX PRODUCTION. (C) COPYRIGHT 1993.   "
	 "PROGRAMMING, SOUND FX AND LEVEL DESIGN BY TRITONE.   "
	 "GRAPHICS AND FONTS BY TWEETY.   "
	 "ADDITIONAL PROGRAMMING AND MUSIC BY ZYNAX OF DARK SYSTEM.   ";

      c = 0; l = 0;
      do {
	 putpaper(text[c], l, paper);
	 if (++l > 4) { l = 0; c++; }
	 if (text[c] == 0) c = 0;
	 retrace();
	 paperscroll();
      } while (!pressedkeys);
   }
   haltsound();
   unloadfile(snd);
   unloadfile(paper);

   glowout();

// All this mode switching is a little bit a 'murks'. I know.
   setxmode();				// Switch back to X mode.
   setstandardpalette();
}



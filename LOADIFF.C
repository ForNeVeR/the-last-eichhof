// SPDX-FileCopyrightText: 1991-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <alloc.h>
#include <mem.h>
#include <dos.h>
#include <stdlib.h>

#define 	LINESIZE	320
#define		LINES		200

#define selectread(plane)       outport(0x3ce, (plane << 8) | 0x04)
#define selectwrite(plane)      outport(0x3c4, (plane << 8) | 0x02)
#define enablewrite(color)      outport(0x3ce, color << 8); outport(0x3ce, 0x0f01)
#define disablewrite()          outport(0x3ce, 1)


void loadpic(char *filename)
{
   char   *dummy;
   int    filvar;
   int    plane, i;
   unsigned scr;

   if ((dummy = malloc(LINESIZE)) == NULL) {
      abort();
   }

   filvar = open(filename, O_RDONLY | O_BINARY, S_IREAD);
   lseek(filvar, 104, SEEK_SET);

   scr = 0;
   for (i = 0; i < LINES; i++) {
   for (plane = 1; plane <= 8; plane <<= 1) {
      read(filvar, dummy, LINESIZE);
   }
   scr+=80;
   }
   close(filvar);

   free(dummy);

}

void screenmode(int mode)
{
   struct REGPACK regs;

   regs.r_ax = mode;
   intr(0x10, &regs);

   regs.r_ax = 0x0100;
   regs.r_cx = 0x2000;
   intr(0x10, &regs);

}

main()
{
   screenmode(16);

   loadpic("screens\\font.lbm");
   getch();

}

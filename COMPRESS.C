
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <alloc.h>
#include <mem.h>
#include <dos.h>
#include <stdlib.h>


#define selectread(plane)       outport(0x3ce, (plane << 8) | 0x04)
#define selectwrite(plane)      outport(0x3c4, (plane << 8) | 0x02)
#define enablewrite(color)      outport(0x3ce, color << 8); outport(0x3ce, 0x0f01)
#define disablewrite()          outport(0x3ce, 1)


#define		PLANESIZE	28000u
#define 	LINESIZE	80
#define		LINES		350



unsigned char  pal[128];

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
   read(filvar, pal, 104);


   scr = 0;
   for (i = 0; i < LINES; i++) {
   for (plane = 1; plane <= 8; plane <<= 1) {
      read(filvar, dummy, LINESIZE);
      selectwrite(plane);
      movedata(FP_SEG(dummy), FP_OFF(dummy), 0xa000, scr, LINESIZE);
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


int savesec(int filvar, char sec[], int secptr)
{
   if (secptr>=512) {
      write(filvar,sec,512);
      memcpy(&sec[0],&sec[512],512);
      secptr -= 512;
   }
   return (secptr);
}


void savepic(unsigned start, char *filename)
{

   int   page, filvar;
   char  far *ptr;
	    char    sec[1024];
	    int     secptr, secsave;
	    int     count;


	    filvar = open(filename, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,S_IWRITE);
	    secptr = 0;
	    write(filvar, &pal[0x30], 48);

	    for (page = 0; page<=3; page++) {
	       selectread(page);
	       ptr = MK_FP(0xa000,start);
	       while (FP_OFF(ptr)<PLANESIZE) {
		  if (*ptr == *(ptr+1)) {
                     count = -1;
		     while (*ptr == *(ptr+1)
			    && count<127 && FP_OFF(ptr)<PLANESIZE) {
			ptr++;
			count++;
		     }
		     sec[secptr++] = count;
                     sec[secptr++] = *ptr;
                     secptr = savesec(filvar,sec,secptr);
                  } else {
                     count = 127;
                     secsave = secptr++;
                     while (*ptr != *(ptr+1)
			    && count<255 && FP_OFF(ptr)<PLANESIZE) {
			sec[secptr++] = *(ptr++);
			count++;
			sec[secsave] = count;
		     }
		     secptr = savesec(filvar,sec,secptr);
		  }
	       }
	    }
	    if (secptr != 0)
	       write(filvar,sec,secptr);

	 close(filvar);
	 }


	 void readpal(char *file)
	 {
	    int  filvar;

	    filvar = open(file, O_RDONLY | O_BINARY, S_IREAD);
   read(filvar, pal, 128);
   close(filvar);

}


void setpal(int i)
{
   struct REGPACK regs;

   regs.r_ax = 0x1012;
   regs.r_bx = 0;
   regs.r_cx = 16;
   regs.r_dx = (unsigned)&pal[i];
   regs.r_es = _DS;
   intr(0x10, &regs);

}

void setattrib(unsigned m)
{
   struct REGPACK regs;

   regs.r_ax = 0x1013;
   regs.r_bx = m;
   intr(0x10, &regs);

}

void setmask(int m)
{
   struct REGPACK regs;

   regs.r_ax = 0x1018;
   regs.r_bx = m;
   intr(0x10, &regs);

}

void setcolor(int c, int r, int g, int b)
{
   outportb(0x3c8, c);
   outportb(0x3c9, r);
   outportb(0x3c9, g);
   outportb(0x3c9, b);
}

main()
{
   int   i;

   screenmode(0x12);
   setattrib(0x100);
   loadpic("screen02.lbm");
//   readpal("story2.pcx");

   savepic(0, "blick.pak");
   getch();

}

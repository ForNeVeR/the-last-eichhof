
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <dos.h>
#include <string.h>


char palette[768];


void loadpic(char *file)
{
   int      filvar;
   char     data, count;
   unsigned pos, i;


   filvar = open(file, O_RDONLY | O_BINARY, S_IREAD);
   lseek(filvar, -768, SEEK_END);
   if (read(filvar, palette, 768) != 768) abort();
   for (i = 0; i < 768; i++) {
      palette[i] = (char) palette[i]>>2;
   }

   lseek(filvar, 128, SEEK_SET);

   pos = 0;
   while ((read(filvar, &count, 1) == 1) && (pos < 0xfa00)) {
      if ((count & 0xc0) == 0xc0) {
	 count = count & 0x3f;
	 read(filvar, &data, 1);
      } else {
	 data = count; count = 1;
      }

      for (i = 0; i < count; i++) {
	 pokeb(0xa000, pos++, data);
      }
   }

   close(filvar);


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

void setcolor(char *ptr)
{
   struct REGPACK regs;

   regs.r_ax = 0x1012;
   regs.r_bx = 0;
   regs.r_cx = 256;
   regs.r_dx = (unsigned) ptr;
   regs.r_es = _DS;
   intr(0x10, &regs);

}

void setattrib(int m)
{
   struct REGPACK regs;

   regs.r_ax = 0x1013;
   regs.r_bx = (m << 8);
   intr(0x10, &regs);

}

struct starstrc {
   int  x, y;
   int  color;
   int  speed;
};


struct starstrc star[500];
int    n;


void main(void)
{
   int    x, y, i;
   char   infile[80];
   char   outfile[80];

   printf("\n\nStarfield Definer [c] by ALPHA-HELIX.\n\n");
   printf("StarMap [.PCX] : ");
   fflush(stdin);
   scanf("%s", infile);
   strcat(infile, ".PCX");
   printf("Output [.STA] : ");
   fflush(stdin);
   scanf("%s", outfile);
   strcat(outfile, ".STA");

   screenmode(0x13);

   setattrib(1);

   loadpic(infile);
   setcolor(palette);

   for (y = 0; y < 200; y++) {
      for (x = 0; x < 320; x++) {
	 if ((i=peekb(0xa000, y*320 + x)) != 0) {
	    star[n].x = x;
	    star[n].y = y;
	    star[n].color = i;
	    switch (i) {
	       case 15: star[n].speed = 8; break;
	       case 14: star[n].speed = 7; break;
	       case 13: star[n].speed = 6; break;
	       case 12: star[n].speed = 5; break;
	       case 11: star[n].speed = 4; break;
	       case 10: star[n].speed = 3; break;
	       case 9: star[n].speed = 2; break;
	       case 7: star[n].speed = 1; break;
	     //  case 7: star[n].speed = 2; break;
	    }
	    n++;
	 }
      }
   }

   i = open(outfile, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IWRITE);
   write(i, &n, 2);
   write(i, star, n*sizeof(struct starstrc));
   close(i);


   screenmode(0x02);


}


#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <alloc.h>


char palette[768];


void loadpic(char *file)
{
   int      filvar;
   char     data, count;
   unsigned pos, i;


   filvar = open(file, O_RDONLY | O_BINARY, S_IREAD);
   lseek(filvar, -768, SEEK_END);
   read(filvar, palette, 768);
   for (i = 0; i < 768; i++) {
      palette[i] /= 4;
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


void main(void)
{
   int  filvar;
   int  x, y, k, p;
   char file[80];
   int  size, ptr;
   int  xs, ys, n, lines;
   int  *mem;


   directvideo = 0;

   printf("\n\n\n\nSprite Converter ver 2.0 [c] by Alpha-Helix 1991.\n\n");
   printf("Enter name of a 320x200 256 color PCX file [.PCX]: ");
   scanf("%s", file); strcat(file, ".PCX");

   screenmode(0x13);

   setattrib(1);
   loadpic(file);
   setcolor(palette);

   gotoxy(1, 24); printf("                         ");
   gotoxy(1, 24); printf("x-size: ");
   scanf("%d", &xs);
   gotoxy(1, 24); printf("                         ");
   gotoxy(1, 24); printf("y-size: ");
   scanf("%d", &ys);
   gotoxy(1, 24); printf("                         ");
   gotoxy(1, 24); printf("n of lines: ");
   scanf("%d", &lines);
   gotoxy(1, 24); printf("                         ");
   gotoxy(1, 24); printf("total n of pics: ");
   scanf("%d", &n);
   gotoxy(1, 24); printf("                         ");
   gotoxy(1, 24); printf("output [.SPR]: ");
   scanf("%s", file); strcat(file, ".SPR");


   size = xs * ys * n;
   mem = (int *) malloc(size + 6);

   ptr = 0;
   mem[ptr++] = xs;
   mem[ptr++] = ys;
   mem[ptr++] = n;

   for (p = 0; p < lines; p++) {
      gotoxy(1, 24); printf("                         ");
      gotoxy(1, 24); printf("n of pics on line %d: ", p);
      scanf("%d", &n);
      for (k = 0; k < n; k++) {

	 for (y = 0; y < ys; y++) {
	    for (x = 0; x < xs / 2; x++) {

	       mem[ptr++] = peek(0xa000, k*xs + y*320 + x*2 + p*ys*320);

	    }
	 }
      }
   }


   filvar = open(file, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IWRITE);
   write(filvar, mem, size+6);
   close(filvar);

   screenmode(2);


}

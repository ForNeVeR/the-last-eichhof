
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <dos.h>


#define retrace() {while(inportb(0x3da)&8); while(!(inportb(0x3da)&8));}


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
      palette[i] = (unsigned char) palette[i]/4;
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

void setpalette(char *ptr)
{
   struct REGPACK regs;

   regs.r_ax = 0x1012;
   regs.r_bx = 0;
   regs.r_cx = 256;
   regs.r_dx = (unsigned) ptr;
   regs.r_es = FP_SEG(ptr);
   intr(0x10, &regs);

}


void setcolor(int c, int r, int g, int b)
{
   outportb(0x3c8, c);
   outportb(0x3c9, r);
   outportb(0x3c9, g);
   outportb(0x3c9, b);
}

void setattrib(int m)
{
   struct REGPACK regs;

   regs.r_ax = 0x1013;
   regs.r_bx = (m << 8);
   intr(0x10, &regs);

}

void setvanillapalette(int c)
{
   int  i;

   for (i = 0; i < 256; i++)
      setcolor(i, c, c, c);

}


void glowto(int r0, int g0, int b0)
{
   int  i, j;
   int  r, g, b;

   for (i = 0; i < 63; i+=3) {
      for (j = 0; j < 256; j++) {
	 r = (r0*i) / 63;
	 g = (g0*i) / 63;
	 b = (b0*i) / 63;
	 setcolor(j, r, g, b);
      }
      retrace();
   }
}


void setbw(void)
{
   int   i;

   for (i = 1; i < 256; i++) {
      setcolor(i, 63, 63, 63);
   }
}

void glowin(int dir)
{
   int   i, j;
   int   r, g, b;

   if (dir) {
      for (i = 0; i < 63; i+=3) {
	 for (j = 1; j < 256; j++) {
	    r = 63 - ((63 - palette[j*3 + 0]) * i) / 63;
	    g = 63 - ((63 - palette[j*3 + 1]) * i) / 63;
	    b = 63 - ((63 - palette[j*3 + 2]) * i) / 63;
	    setcolor(j, r, g, b);
	 }
	 retrace();
      }
   } else {
      for (i = 0; i < 63; i+=3) {
	 for (j = 1; j < 256; j++) {
	    r = (palette[j*3 + 0] * i) / 63;
	    g = (palette[j*3 + 1] * i) / 63;
	    b = (palette[j*3 + 2] * i) / 63;
	    setcolor(j, r, g, b);
	 }
	 retrace();
      }
   }

}


main()
{
   int  i;

   screenmode(0x13);

   setattrib(1);
   setvanillapalette(0);
   loadpic("back.pcx");
   setbw();
   retrace(); retrace();
   glowin(1);
   getch();


}

#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <alloc.h>
#include <dos.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "xmode.h"
#include "mouse.h"
#define MAIN_MODULE
#include "baller.h"


#define MIDX                 160
#define MIDY                 110

#define STATLINE             210


void plot(int x, int y, int c);
int getpixel(int x, int y);

struct sprstrc *font;

int    nsprites;
struct sTableEntry *sprite;
int    sspr = 0;


int    x, y, b, x0, y0;
int    count;

struct explstrc tmpexpl;
int    tmp[1000];
int    tmppos = 0;

int    runpos;


int    nexpls = 0;
#define NEXPLS			80
struct explstrc *bumm[NEXPLS];
int    expllen[NEXPLS];
int    sexpl = -1;

int    nfoes = 0;
int    xstart[400], ystart[400];
int    pathlen[400];
struct foestrc *foe[400];
int    sfoe = -1;


int loadfont(void)
{
   int      filvar;
   long	    size;
   int      nentries;
   int      i;

   if ((filvar = open("font1.spr", O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      return -1;
   }
   size = filelength(filvar);

   if ((font = malloc(size)) == NULL) {
      close(filvar);
      return -1;
   }
   read(filvar, font, size);
   close(filvar);

   return 0;

}



int readslib(char *file)
{

   int      filvar;
   long	    size;
   int      i;
   char     huge *h;

   if ((filvar = open(file, O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      return -1;
   }
   read(filvar, &nsprites, 2);
   size = filelength(filvar) - 2;

   if ((sprite = (void *) farmalloc(size)) == NULL) {
      close(filvar);
      abort();
   }

   h = (char *)sprite;
   while (size > 65000) {
      read(filvar, h, 65000);
      h += 65000; size -= 65000;
   }
   read(filvar, h, size);

   close(filvar);


// Relocate pointers to single data elements.
   for (i = 0; i < nsprites; i++) {
      (long)sprite[i].sprite += (long)sprite;
   }
   return 0;

}


void xorline(int x1, int y1, int x2, int y2, int c)
{

   int   x, y, z, dx, dy, dz, i1, i2;


   dx = abs(x1 - x2); dy = abs(y1 - y2);
   if (x1 < x2) {
      x = x1; y = y1;
      if (y1 > y2) z = -1; else z = 1;
   } else {
      x = x2; y = y2;
      if (y2 > y1) z = -1; else z = 1;
   }

   if (dx > dy) i2 = dx; else i2 = dy;
   dz = i2 >> 1;
   plot(x, y, getpixel(x, y) ^ c);
   for (i1 = 1; i1 <= i2; i1++) {
      if (dz < dx) {
	 dz = dz + dy; x++;
      }
      if (dz >= dx) {
	 dz = dz - dx; y += z;
      }
      plot(x, y, getpixel(x, y) ^ c);
   }

}

int getdec(char *text)
{
   int   i;
   int   ch;
   char  str[40];
   int   pos;

   clearregion(STATLINE-8, 8);
   writetext(0, STATLINE-8, text, font);

   i = 0;
   pos = strlen(text)*8 + 8;
   while ((ch = getch()) != 13) {
      if (ch == 8) {
	 if (i > 0) {
	 pos -= 8; putspritedirect(font, pos, STATLINE-8, 0);
	 i--;
	 }
      } else {
	 str[i++] = ch;
	 putspritedirect(font, pos, STATLINE-8, ch - ' ');
	 pos+=8;
      }

   }
   str[i] = 0;

   return atoi(str);

}


#pragma argsused
void error(char *s, int code, ...)
{
   writetext(0, 0, s, font);
   getch();
   writetext(0,0,"                                   ", font);

}


int findcommand(void *path, int i, int dir)
{
   unsigned *data;

   if (path == NULL) data = (unsigned *)tmp;
   else data = (unsigned *)path;

   if (i < 0) return i+dir;
   if (dir == -1) {
      if (i == 0) return -1;
      i--;
      if ((i > 0) && (data[i-1] == EXPLSOUND)) return i-1;
      if ((i > 2) && (data[i-3] == EXPLNEW)) return i-3;
      if ((i > 2) && (data[i-3] == EXPLRELEASEFOE)) return i-3;
      if ((i > 4) && (data[i-5] == EXPLNEWPATH)) return i-5;
      return i;
   } else {
      if (data[i] == EXPLNEW) return i+4;
      if (data[i] == EXPLSOUND) return i+2;
      if (data[i] == EXPLRELEASEFOE) return i+4;
      if (data[i] == EXPLNEWPATH) return i+6;
      return i+1;
   }

}

int dispatch(int pos, char *s)
{

   if (pos < 0) {
      strcpy(s, "NONE");
      return pos+1;
   }
   switch (tmp[pos]) {
      case EXPLEND:
	 strcpy(s, "END");
	 pos++;
	 break;
      case EXPLNEW:
	 sprintf(s, "NEW %02d", tmp[pos+1]);
	 pos += 4;
	 break;
      case EXPLSOUND:
	 sprintf(s, "SOUND %02d", tmp[pos+1]);
	 pos += 2;
	 break;
      case EXPLWAIT:
	 sprintf(s, "WAIT");
	 pos += 1;
	 break;
      case EXPLREMOVEOBJ:
	 sprintf(s, "REMOVEOBJ");
	 pos++;
	 break;
      case EXPLRELEASEFOE:
	 sprintf(s, "RELEASE %02d", tmp[pos+1]);
	 pos+=4;
	 break;
      case EXPLNEWPATH:
	 sprintf(s, "NEWPATH %02d", tmp[pos+1]);
	 pos += 6;
	 break;
      default:
	 sprintf(s, "ERR %x", tmp[pos]);
	 pos+=1;
	 break;
   }

   return pos;
}

void status(void)
{
   char   text[80];

   clearregion(STATLINE-8, 16);
   sprintf(text, "%04X", count);
   writetext(284, STATLINE, text, font);
   sprintf(text, "%d", sexpl);
   writetext(0, STATLINE, text, font);

   dispatch(runpos, text);
   writetext(20, STATLINE, text, font);

}



int roll(int n, int lb, int ub, int dir)
{
   if (dir == 1) {
      n++; if (n > ub) n = lb;
   } else {
      n--; if (n < lb) n = ub;
   }

   return n;
}

void insertcommand(int pos, int n, ...)
{
   va_list ap;
   int     i;

   if (pos >= tmppos) tmppos = findcommand(NULL, pos, 1);
   memmove(&tmp[pos+n], &tmp[pos], (tmppos-pos)*2);
   tmppos += n;
   va_start(ap, n);
   for (i = 0; i < n; i++) {
      tmp[pos+i] = va_arg(ap, int);
   }
   va_end(ap);

}

void fillwait(void)
{
   int  i;

   for (i = 0; i < 1000; i++)
      tmp[i] = EXPLWAIT;

}

void newexpl(void)
{

   clearscreen();

   fillwait();
   tmpexpl = *bumm[sexpl];

   runpos = -1;

   tmppos = expllen[sexpl]-sizeof(struct explstrc);
   memcpy(tmp, &bumm[sexpl]->data, tmppos);
   tmppos = tmppos/2;
   tmp[tmppos-1] = EXPLWAIT;

}

void storepath(void)
{

   if ((bumm[nexpls] = malloc(sizeof(struct explstrc)+tmppos*2)) != NULL) {
      sexpl = nexpls++;
      expllen[sexpl] = sizeof(struct explstrc) + tmppos*2;
      *bumm[sexpl] = tmpexpl;
      memcpy(&bumm[sexpl]->data, tmp, tmppos*2);
   }
}


void refreshpath(void)
{
   free(bumm[sexpl]);

   tmp[tmppos++] = EXPLEND;
   if ((bumm[sexpl] = malloc(sizeof(struct explstrc)+tmppos*2)) != NULL) {
      *bumm[sexpl] = tmpexpl;
      memcpy(&bumm[sexpl]->data, tmp, tmppos*2);
      expllen[sexpl] = sizeof(struct explstrc) + tmppos*2;
   } else
      error("CRITICAL OUT OF MEMORY. EXPLOSION LOST. DESIGNER UNSTABLE. QUIT!!!",0);
}

void showpath(int x, int y, int path, int c)
{
   int  i;
   int  c1, c2;

   i = 0;
   x += sprite[foe[path]->sprite].sprite->xs/2;
   y += sprite[foe[path]->sprite].sprite->ys/2;
   plot(x, y, c);
   c1 = foe[path]->path[i]; c2 = foe[path]->path[i+1];
   while (((unsigned)c1 != FOEENDPATH) && ((unsigned)c1 != FOECYCLEPATH)) {

      switch (c1) {
	 case FOEMARK:
	    i += 1;
	    break;
	 case FOECHANGESPRITE:
	 case FOESOUND:
	    i += 2;
	    break;
	 case FOERELEASEFOE:
	    showpath(x+foe[path]->path[i+2], y+foe[path]->path[i+3], c2, (c == 0) ? 0 : c+1);
	    i += 4;
	    break;

	 default:
	    plot(x+=c1, y+=c2, c);
	    i += 2;
	    break;

      }
      c1 = foe[path]->path[i]; c2 = foe[path]->path[i+1];
   }

}

int getpath(void)
{
   int   path0;
   int   ch1, ch2;

   sfoe = 0;
   setborder(-sprite[foe[sfoe]->sprite].sprite->xs, -sprite[foe[sfoe]->sprite].sprite->ys, 320, YMAX);
   showpath(x, y, sfoe, 12);
   putsprite(foe[sfoe]->sprite, x, y, 0);
   do {
      x0 = x; y0 = y; path0 = sfoe;
      ch1 = ch2 = 0xff;
      if(kbhit()) ch1 = getch();
      if(ch1 == 0) ch2 = getch();
      if (ch2 == 'A' || ch2 == 'B') sfoe = roll(sfoe, 0, nfoes-1, ch2-'A');
      getpos(&b, &x, &y);
      if ((x0 != x) || (y0 != y) || (sfoe != path0)) {
	 showpath(x0, y0, path0, 0);
	 removesprite(foe[path0]->sprite, x0, y0);
	 if (sfoe != path0) {
	    setborder(-sprite[foe[sfoe]->sprite].sprite->xs, -sprite[foe[sfoe]->sprite].sprite->ys, 320, YMAX);
	 }
	 showpath(x, y, sfoe, 11);
	 putsprite(foe[sfoe]->sprite, x, y, 0);
	 plot(MIDX, MIDY, 15);
      }

   } while (!b);

   return b;

}

void explinsert(void)
{
   writetext(0, STATLINE-8, "1.SND,2.REMOVE,3.RELEASE,4.NEWPATH", font);

   switch (getch()) {

      case '1': // Sound a bit around.
	 insertcommand(findcommand(NULL, runpos, 1), 2, EXPLSOUND, getdec("SND."));
      break;

      case '2': // Remove exploded object.
	 insertcommand(findcommand(NULL, runpos, 1), 1, EXPLREMOVEOBJ);
	 break;
      case '3': // Release a foe.
	 if (nfoes <= 1) break;
	 removesprite(sspr, x, y);
	 b = getpath();
	 if (b == 1) { // Left mouse pressed
	    insertcommand(findcommand(NULL, runpos, 1), 4,
			  EXPLRELEASEFOE, sfoe, x-MIDX, y-MIDY);
	 }
	 break;
      case '4': // Release a foe and move it to a position.
	 if (nfoes <= 1) break;
	 removesprite(sspr, x, y);
	 b = getpath();
	 if (b == 1) { // Left mouse pressed
	    insertcommand(findcommand(NULL, runpos, 1), 6,
			  EXPLNEWPATH, sfoe,
			  -sprite[foe[sfoe]->sprite].sprite->xs/2,
			  -sprite[foe[sfoe]->sprite].sprite->ys/2,
			  x, y);
	 }
	 foe[sfoe]->flags |= FOE_LINE;
	 break;
   }

}

void deletecommand(int pos)
{
   int  i;

   memmove(&tmp[pos], &tmp[i = findcommand(NULL, pos, 1)], (tmppos-pos)*2);
   i -= pos;
   tmppos -= i;
   refreshpath();

}

void delexpl(void)
{
   int  i;

   free(bumm[sexpl]);
   for (i = sexpl + 1; i < nexpls; i++)
      bumm[i-1] = bumm[i];
   nexpls--;
   if (nexpls == 0) {
      tmppos = 1; tmp[0] = EXPLEND;
      storepath();
      tmppos = 0; fillwait();
      runpos = -1;
   } else {
      sexpl--; if (sexpl < 0) sexpl = nexpls - 1;
      newexpl();
   }
}



int getkey(void)
{
   int   ch1, ch2;


   ch1 = ch2 = 0xff;
   if(kbhit()) ch1 = getch();
   if(ch1 == 0) ch2 = getch();

   if (ch1 != 0xff)
   if (ch1 == 0) {

      switch (ch2) {

	 case 'A': // F7 + F8
	 case 'B':
		   refreshpath();
		   sexpl = roll(sexpl, 0, nexpls - 1, ch2 - 'A');
		   count = 0;
		   newexpl();
		   break;

	 case 'C': // F9
	 case 'D':
		   removesprite(sspr, x, y);  // F10
		   sspr = roll(sspr, 0, nsprites - 1, ch2 - 'C');
		   putsprite(sspr, x, y, 0);
		   break;

	 case 'M': count++;  // Cursor right
		   runpos = findcommand(NULL, runpos, 1);
		   break;
	 case 'K': count--;  // cursor left
		   runpos = findcommand(NULL, runpos, -1);
		   break;

	 case 'S': // Delete key
		   deletecommand(runpos);
		   break;
	 case 'R': // Insert key
		   explinsert();
		   break;

      }

      status();

   } else {

      switch (ch1) {

	 case 'd': // Delete explosion
		   delexpl();
                   count = 0;
		   break;
	 case 'n': // New Explosion
		   refreshpath();
		   count = 0;
		   tmppos = 1; tmp[0] = EXPLEND;
		   storepath();
		   tmppos = 0; fillwait();
		   runpos = -1;
		   break;

	 case 'q': return TRUE;

      }

      status();

   }

   return FALSE;

}

int addexpl(void)
{

   insertcommand(findcommand(NULL, runpos, 1), 4,
		 EXPLNEW, sspr, x-MIDX, y-MIDY);

   return 0;

}


int doexpl(void)
{
   int   i;

   if (sprite == NULL) {
      printf("\nSprite Library not loaded.\n");
      return -1;
   }

   initxmode();
   initmouse();
   setborder(0, 0, XMAX, YMAX);

   for (i = 0; i < nsprites; i++)
      if (defsprite(sprite[i].sprite, sprite[i].flags) == -1) {
	 shutxmode();
	 printf("Not enough free offscreen memory.\n");
	 exit(1);
      }

   count = 0;
   if (nexpls > 0) {
      sexpl = 0;
      newexpl();
   } else {
      tmppos = 1; tmp[0] = EXPLEND;
      storepath();
      tmppos = 0; fillwait();
      runpos = -1;
   }

   status();
   getpos(&b, &x, &y);
   putsprite(sspr, x, y, 0);
   plot(MIDX, MIDY, 15);

   do {
      x0 = x; y0 = y;
      getpos(&b, &x, &y);
      if (b == 1) {
	 addexpl();
	 status();
      }
      if ((x0 != x) || (y0 != y)) {
	 removesprite(sspr, x0, y0);
	 putsprite(sspr, x, y, 0);
	 plot(MIDX, MIDY, 15);
      }

   } while (!getkey());

   refreshpath();
   killallsprites();

   clearmouse();
   shutxmode();

   return 0;
}



int loadexpls(char *file)
{
   char     fname[80];
   int      filvar, filvar1;
   int      nentries;
   int      i;

   strcpy(fname, file);
   if ((filvar = open(strcat(fname, ".EXP"), O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      return -1;
   }
   read(filvar, &nexpls, sizeof(nexpls));

// Read start points
   strcpy(fname, file);
   if ((filvar1 = open(strcat(fname, ".ESP"), O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      return -1;
   }
   read(filvar1, expllen, nexpls*2);
   close(filvar1);
// --

   read(filvar, bumm, nexpls*4);

   for (i = 0; i < nexpls; i++) {
      if ((bumm[i] = malloc(expllen[i])) == NULL) {
	 close(filvar);
	 return -1;
      }
      read(filvar, bumm[i], expllen[i]);
   }

   close(filvar);

   return 0;
}


int saveexpls(char *file)
{
   char      fname[80];
   int       filvar, i;
   long      ptr, ptr0;
   long      fptr1, fptr2;
   unsigned  size;


   strcpy(fname, file);
   filvar = open(strcat(fname, ".EXP"), O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE);
   if (filvar == -1) return -1;

   write(filvar, &nexpls, 2);
   fptr1 = tell(filvar);
   ptr = nexpls*4;
   write(filvar, bumm, ptr);

   for (i = 0; i < nexpls; i++) {
      write(filvar, bumm[i], expllen[i]);
      fptr2 = tell(filvar);
      lseek(filvar, fptr1, SEEK_SET);
      ptr0 = (ptr + ((ptr & 0xfff0) << 12)) & 0xffff000fl;
      write(filvar, &ptr0, 4);
      ptr += expllen[i];
      fptr1 = tell(filvar);
      lseek(filvar, fptr2, SEEK_SET);

   }
   close(filvar);

   strcpy(fname, file);
   filvar = open(strcat(fname, ".ESP"), O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE);
   if (filvar == -1) return -1;
   write(filvar, expllen, nexpls*2);
   close(filvar);

   return 0;
}


int loadpathes(char *file)
{
   char     fname[80];
   int      filvar, filvar1;
   int      nentries;
   int      i;

   strcpy(fname, file);
   if ((filvar = open(strcat(fname, ".FOE"), O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      return -1;
   }
   read(filvar, &nfoes, sizeof(nfoes));

// Read start points
   strcpy(fname, file);
   if ((filvar1 = open(strcat(fname, ".FSP"), O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      return -1;
   }
   read(filvar1, xstart, nfoes*2);
   read(filvar1, ystart, nfoes*2);
   read(filvar1, pathlen, nfoes*2);
   close(filvar1);
// --

   read(filvar, foe, nfoes*4);

   for (i = 0; i < nfoes; i++) {
      if ((foe[i] = malloc(pathlen[i])) == NULL) {
	 close(filvar);
	 return -1;
      }
      read(filvar, foe[i], pathlen[i]);
   }
   close(filvar);

   return 0;
}




int savepathes(char *file)
{
   char      fname[80];
   int       filvar, i;
   long      ptr, ptr0;
   long      fptr1, fptr2;
   unsigned  size;


   strcpy(fname, file);
   filvar = open(strcat(fname, ".FOE"), O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE);
   if (filvar == -1) return -1;

   write(filvar, &nfoes, 2);
   fptr1 = tell(filvar);
   ptr = nfoes*4;
   write(filvar, foe, ptr);

   for (i = 0; i < nfoes; i++) {
      write(filvar, foe[i], pathlen[i]);
      fptr2 = tell(filvar);
      lseek(filvar, fptr1, SEEK_SET);
      ptr0 = (ptr & 0xf) + ((ptr & 0xffff0) << 12);
      write(filvar, &ptr0, 4);
      ptr += pathlen[i];
      fptr1 = tell(filvar);
      lseek(filvar, fptr2, SEEK_SET);

   }
   close(filvar);

   return 0;
}

void shutsprites(void)
{
   killallsprites();
   if (sprite != NULL) farfree(sprite);
}


int main(void)
{
   char   libfile[80];
   int    choice;
   int    i;

   screenmode(0x02);

   printf("EXPLOSION DESIGNER [c] Dec 1992 by Alpha-Helix.\n\n");

   if (loadfont() == -1) {
      printf("Font file not found !\n");
      abort();
   }

   do {

   do {
      printf("\n\n");
      printf("1 - Edit Explosions.\n");
      printf("6 - Load Level.\n");
      printf("7 - Save Level.\n");
      printf("8 - Read Sprite Library.\n");
      printf("9 - Quit PE.\n");

      printf("\nYour choice -> "); choice = getche() - '0';
      printf("\n\n");

      switch (choice) {

	 case 1: doexpl();
		 break;

	 case 6: printf("Level : ");
		 fflush(stdin);
		 scanf("%s", libfile);
		 if (loadpathes(libfile) == -1) {
		    perror("File not read: ");
		 }
		 if (loadexpls(libfile) == -1) {
		    perror("File not read: ");
		 } else {
		    sexpl = 0;
		 }
		 break;

	 case 7: printf("Level : ");
		 fflush(stdin);
		 scanf("%s", libfile);
		 if (savepathes(libfile) == -1) {
		    perror("File not written: ");
		 }
		 if (saveexpls(libfile) == -1) {
		    perror("File not written: ");
		 }
		 break;

	 case 8: printf("Sprite Library [.SLI]: ");
		 fflush(stdin);
		 scanf("%s", libfile);
		 strcat(libfile, ".SLI");
		 shutsprites();
		 readslib(libfile);
		 sspr = 0;
		 break;


      }

   } while (choice != 9);

   printf("\n\nQuit without saving (Y/N)? "); choice = getche();
   } while (choice != 'y');

   for (i = 0; i < nexpls; i++)
      free(bumm[i]);
   shutsprites();
   free(font);
   printf("\nThanks for using ED !\n\n\n\n");

   return 0;

}




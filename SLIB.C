// SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

#ifndef __COMPACT__
   #error Memory Model must be compact
#endif

#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <alloc.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <dos.h>


#define   MAXSPRITES		100


#include "xmode.h"
#define MAIN_MODULE
#include "baller.h"

#define MIDX                 100
#define MIDY                 80

#define STATLINE             210


int    nsprs;
int    sspr = -1;
struct sTableEntry spr[MAXSPRITES];

int    handle;
int    n = 0;

struct sprstrc *font;


#pragma argsused
void error(char *s, int code, ...)
{

}


int loadsprite(char *file)
{
   int      filvar;
   unsigned size;

   if (nsprs < MAXSPRITES) {

      sspr = nsprs; nsprs++;
      filvar = open(file, O_RDONLY | O_BINARY, S_IREAD);
      if (filvar == -1) return -1;

      size = filelength(filvar);
      if ((spr[sspr].sprite = malloc(size)) == NULL) return -1;

      read(filvar, spr[sspr].sprite, size);
      close(filvar);

      return 0;
   }

   return -1;
}

void status(void)
{
   char   text[80];

   clearregion(STATLINE-8, 16);

   sprintf(text, "PIC.%d    FLAGS.%XH", n, spr[sspr].flags);
   writetext(0, STATLINE, text, font);

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

void delspr(void)
{
   int  i;

   if (sspr != -1) {

      for (i = sspr + 1; i < nsprs; i++) {
	 spr[i-1] = spr[i];
      }
      nsprs--;

      if (nsprs == 0) sspr = -1;
      else {
	 sspr--; if (sspr < 0) sspr = nsprs - 1;
      }
   }

}

void removedouble(void)
{
   int   size;
   int   i;

   if (spr[sspr].sprite->maxn <= 1) return;

   size = spr[sspr].sprite->xs * spr[sspr].sprite->ys;
   for (i = 2; i < spr[sspr].sprite->maxn; i+=2) {
      memmove(&spr[sspr].sprite->data[size*(i/2)],
	      &spr[sspr].sprite->data[size*i],
	      size);
   }
   spr[sspr].sprite->maxn /= 2;
   spr[sspr].sprite = realloc(spr[sspr].sprite,
		    spr[sspr].sprite->maxn*size + sizeof(struct sprstrc));

}


void newsprite(void)
{
   killsprite(handle);
   handle = defsprite(spr[sspr].sprite, spr[sspr].flags);
   n = 0;
}


int getkey(void)
{
   int  ch1, ch2;
   char  text[80];


   ch1 = getch();
   if(ch1 == 0) ch2 = getch();

   if (ch1 == 0) {
      switch (ch2) {

	 case 'C': // F9
	 case 'D':
		   removesprite(handle, MIDX, MIDY);  // F10
		   sspr = roll(sspr, 0, nsprs - 1, ch2 - 'C');
		   newsprite();
		   break;

	 case 'M': // Cursor Right
	 case 'K': // Cursor Left
		if (sspr != -1) {
		   n = roll(n, 0, spr[sspr].sprite->maxn-1, (ch2 == 'M') ? 1 : 0);
                }
		break;
      }
      status();
   } else {
      switch (ch1) {

	 case 'l': // Load
		   killallsprites();
		   shutxmode();
		   printf("Sprite to load [.SPR]: ");
		   fflush(stdin);
		   scanf("%s", text);
		   strcat(text, ".SPR");
		   if (loadsprite(text) == -1) {
		      perror("loadsprite");
		      if (--nsprs == 0) sspr = -1; else sspr = 0;
		      n = 0;
		   }
		   initxmode();
		   if (sspr != -1) {
		      spr[sspr].flags = 12;
		      handle = defsprite(spr[sspr].sprite, spr[sspr].flags);
		      n = 0;
		   }
		   break;

	 case 'e': // flags.
		   if (sspr != -1) {
		   killallsprites();
		   shutxmode();
		   printf("2,4 = xAlign, 8h=Double\n");
		   printf("Flags [%xh]: ", spr[sspr].flags);
		   fflush(stdin);
		   gets(text);
		   if (strlen(text) != 0) sscanf(text, "%x", &spr[sspr].flags);
		   initxmode();
		   handle = defsprite(spr[sspr].sprite, spr[sspr].flags);
		   n = 0;
		   }
		   break;

	 case 'd': // Delete.
		   removesprite(handle, MIDX, MIDY);
		   delspr();
		   if (sspr != -1) {
		      spr[sspr].flags = 12;
		      handle = defsprite(spr[sspr].sprite, spr[sspr].flags);
		      n = 0;
		   }
		   break;

	 case 'r': // Remove double.
		   removedouble();
                   newsprite();
		   break;

	 case 'q': return 1;

      }

      status();

   }

   return 0;

}


void Und_Los(void)
{

   initxmode();

   if (sspr != -1) {
      handle = defsprite(spr[sspr].sprite, spr[sspr].flags);
      n = 0;
   }

   status();

   do {
      if (sspr != -1) {
	 removesprite(handle, MIDX, MIDY);
	 putsprite(handle, MIDX, MIDY, n);
      }
   } while (!getkey());

   killallsprites();

   shutxmode();

}


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




int saveslib(char *file)
{
   int       filvar, i;
   long      ptr, ptr0;
   long      fptr1, fptr2;
   unsigned  size;

   filvar = open(file, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE);
   if (filvar == -1) return -1;

   write(filvar, &nsprs, 2);
   fptr1 = tell(filvar);
   ptr = nsprs*sizeof(struct sTableEntry);
   write(filvar, spr, ptr);

   for (i = 0; i < nsprs; i++) {
      size = spr[i].sprite->xs*spr[i].sprite->ys*spr[i].sprite->maxn + 6;
      write(filvar, spr[i].sprite, size);
      fptr2 = tell(filvar);
      lseek(filvar, fptr1, SEEK_SET);
      ptr0 = (ptr & 0xf) + ((ptr & 0xffff0) << 12);
      write(filvar, &ptr0, 4); write(filvar, &spr[i].flags, 2);
      ptr += size;
      fptr1 = tell(filvar);
      lseek(filvar, fptr2, SEEK_SET);

   }

   close(filvar);

   return 0;
}


int readslib(char *file)
{

   int      filvar;
   long	    size;
   int      i;
   struct   sTableEntry *ptr;
   char     huge *h;

   if ((filvar = open(file, O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      return -1;
   }
   read(filvar, &nsprs, 2);

   size = filelength(filvar) - 2;
   if ((ptr = farmalloc(size)) == NULL) {
      close(filvar);
      abort();
   }
   h = (char *) ptr;
   while (size > 65000) {
      read(filvar, h, 65000);
      h += 65000; size -= 65000;
   }
   read(filvar, h, size);
   close(filvar);


// Relocate pointers to single data elements.
   for (i = 0; i < nsprs; i++) {
      (long)ptr[i].sprite += (long)ptr;
   }

   for (i = 0; i < nsprs; i++) {
      size = ptr[i].sprite->xs*ptr[i].sprite->ys*ptr[i].sprite->maxn;
      spr[i].sprite = malloc(size+sizeof(struct sprstrc));
      if (spr[i].sprite == NULL) abort();
      spr[i].flags = ptr[i].flags;
      memcpy(spr[i].sprite, ptr[i].sprite, size+sizeof(struct sprstrc));
   }

   farfree(ptr);
   return 0;

}

void shutsprites(void)
{
   if (spr != NULL) free(spr);
}


void main(void)
{
   char  libfile[80];
   int   choice;

   printf("\n\nSPRITE LIBRARY MANAGER Ver 2.007!\n");
   printf("[c] copyrigth 1992/93 by ALPHA-HELIX.\n");

   loadfont();

   do {

   do {
      printf("\n\n");
      printf("1 - Manager.\n");
      printf("7 - Save lib.\n");
      printf("8 - Load sprite library\n");
      printf("9 - Quit SLM.\n");

      printf("\nYour choice -> "); choice = getche() - '0';
      printf("\n\n");

      switch (choice) {
	 case 1: Und_Los();
		 break;

	 case 7: printf("Save: Sprite Library [.SLI]: ");
		 fflush(stdin);
		 scanf("%s", libfile);
		 strcat(libfile, ".SLI");
		 if (saveslib(libfile) == -1) {
		    perror("File not written: ");
		 }
		 break;

	 case 8: printf("Load: Sprite Library [.SLI]: ");
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

   shutsprites();
   free(font);
   printf("\nThanks for using SLM !\n\n\n\n");

}



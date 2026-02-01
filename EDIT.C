
#define MAIN_MODULE

#pragma hdrfile "edit.sym"

#include <stdio.h>
#include <io.h>
#include <sys\stat.h>
#include <stdlib.h>
#include <dos.h>
#include <fcntl.h>
#include <alloc.h>
#include <string.h>
#include <stdarg.h>
#include <conio.h>

#pragma hdrstop

#include "mouse.h"
#include "sound.h"
#include "baller.h"
#include "xmode.h"


#define   STATLINE              212



void plot(int x, int y, int c);


// Globale wurschtli variablen.


int getkey(void);


int    nsprites;
struct sTableEntry *sprite = NULL;
int    sspr;				// Selected sprite;


int    speed = 4;


struct sprstrc *font;


int    pathedit = 0;
int    attackedit = 0;


int    x, y, b, x0, y0;
int    path;
int    handle;
int    handle2;
int    pixelcount;
int    xa, ya;

int    lastkey;

int    tmpxstart, tmpystart;
struct foestrc  tmpfoe; // ____\
int    tmp[3000];	//     / This two lines mustn't be cut.
int    tmppos;
int    runpos;
int    runx, runy;

int    nfoes = 0;
int    xstart[400], ystart[400];
int    pathlen[400];
struct foestrc *foe[400];
int    sfoe = -1;
//---------
unsigned count = 0;
int    nattacks = 0;
#define MAXATT 400
struct attackstrc attack[MAXATT];
int    sattack = -1;
struct pathstrc {
   int   foe;		// Number.
   int   x, y;		// Position of foe.
   int   pos;		// Positon in path.
   int   savepos;
   int   savex, savey;
   int   sprite;	// Current sprite.
};
#define MAXANI 200
struct pathstrc animate[MAXANI];


//---------------------------------------------

void initanimate(void)
{
   int  i;

   for (i = 0; i < MAXANI; i++) animate[i].foe = -1;

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

void line(int x1, int y1, int x2, int y2, int c)
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
   plot(x, y, c);
   for (i1 = 1; i1 <= i2; i1++) {
      if (dz < dx) {
	 dz = dz + dy; x++;
      }
      if (dz >= dx) {
	 dz = dz - dx; y += z;
      }
      plot(x, y, c);
   }

}

void tmpline(int x1, int y1, int x2, int y2, int speed, int ignore)
{
   int  x, y, z0, z1, dx, dy, dz, i1, i2;

   dx = abs(x1-x2); dy = abs(y1-y2);
   if (x1 < x2) z0 = 1; else z0 = -1;
   if (y1 < y2) z1 = 1; else z1 = -1;
   x = x1; y = y1;
   if (dx > dy) i2 = dx; else i2 = dy;
   dz = i2/2;
   if (((pixelcount % speed) == 0) && !ignore) {
      tmp[tmppos++] = x - runx; tmp[tmppos++] = y - runy;
      runx = x; runy = y;
   }
   pixelcount++;
   for (i1 = 1; i1 < i2; i1++) {
      if (dz < dx)  { dz += dy; x += z0; }
      if (dz >= dx) { dz -= dx; y += z1; }
      if ((pixelcount % speed) == 0) {
	 tmp[tmppos++] = x - runx; tmp[tmppos++] = y - runy;
	 runx = x; runy = y;
      }
      pixelcount++;
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
      if ((data[i] == FOEENDPATH) || (data[i] == FOECYCLEPATH)) return i;
      if ((i >= 3) && (data[i-3] == FOERELEASEFOE)) return i-3;
      if (data[i] == FOEMARK) return i;
      return i-1;
   } else {
      if ((data[i] == FOEENDPATH) || (data[i] == FOECYCLEPATH)) return i+1;
      if (data[i] == FOERELEASEFOE) return i+4;
      if (data[i] == FOEMARK) return i+1;
      return i+2;
   }

}


int dispatch(int pos, char *s)
{

   if (pos < 0) {
      strcpy(s, "NONE");
      return pos+1;
   }
   switch (tmp[pos]) {
      case FOEENDPATH:
	 strcpy(s, "END");
	 pos++;
	 break;
      case FOECYCLEPATH:
	 strcpy(s, "CYCLE");
	 pos++;
	 break;
      case FOEMARK:
	 strcpy(s, "MARK");
	 pos++;
	 break;
      case FOECHANGESPRITE:
	 sprintf(s, "C-SPRITE %02d", tmp[pos+1]);
	 pos += 2;
	 break;
      case FOERELEASEFOE:
	 sprintf(s, "RELEASE %02d", tmp[pos+1]);
	 pos += 4;
	 break;
      case FOESOUND:
	 sprintf(s, "SOUND %02d", tmp[pos+1]);
	 pos += 2;
	 break;
      default:
	 sprintf(s, "%03d,%03d", tmp[pos], tmp[pos+1]);
	 pos += 2;
	 break;
   }

   return pos;
}

void dispatchattack(int n, char *s)
{
   switch (attack[n].foe) {
      case A_GOFIELD:
	 sprintf(s, "%X GOSTARFIELD", attack[n].count);
	 break;
      case A_STOPFIELD:
	 sprintf(s, "%X STOPFIELD", attack[n].count);
	 break;
      case A_SOUND:
	 sprintf(s, "%X SOUND %d", attack[n].count, attack[n].x);
	 break;
      case A_MARK:
	 sprintf(s, "%X MARK", attack[n].count);
	 break;
      default:
	 sprintf(s, "%X FOE %d", attack[n].count, attack[n].foe);
	 break;
   }
}

void status(void)
{
   char   text[80];
   char   comm[80];
   int    last;


   if (!attackedit) {
      sprintf(text, "SH.%-3d SC.%-5u EX.%-2d SP.%-2d MEM.%6lu",
	   tmpfoe.shield, tmpfoe.score, tmpfoe.expl, speed, coreleft());
      writetext(0, STATLINE, text, font);

      if (sfoe != -1) {
	 text[0] = 0;    // Clear text.
	 last = findcommand(NULL, runpos, -1);
	 last = dispatch(last, comm);
	 strcat(text, comm); strcat(text, "  ");
	 last = dispatch(last, comm);
	 strcat(text, comm); strcat(text, "  ");
	 dispatch(last, comm);
	 strcat(text, comm); strcat(text, "        ");
	 writetext(0, STATLINE-8, text, font);

	 sprintf(text, "FOE %d", sfoe);
	 writetext(0, 0, text, font);

      }
      writetext(284, STATLINE-8, "    ", font);

   } else {
      clearregion(STATLINE-8, 16);
      sprintf(text, "%04X", count);
      writetext(284, STATLINE, text, font);
      if (sattack != -1) {
	 dispatchattack(sattack, text);
	 writetext(0, STATLINE, text, font);
      }
   }


}



void editvalues(void)
{
   char   text[80];

   printf("%xh=ENDLEVEL, %xh=INVINCIBLE, %xh=TRANSPARENT, %xh=STOPCOUNT\n",
	  FOE_ENDLEVEL, FOE_INVINCIBLE, FOE_TRANSPARENT, FOE_STOPCOUNT);
   printf("%xh=PATH, %xh=LINE\n", FOE_PATH, FOE_LINE);
   printf("Flags [%xh]: ", tmpfoe.flags);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%x", &tmpfoe.flags);

   printf("Shield [%d]: ", tmpfoe.shield);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%d", &tmpfoe.shield);

   printf("Score [%u]: ", tmpfoe.score);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%d", &tmpfoe.score);

   printf("Explosion [%d]: ", tmpfoe.expl);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%d", &tmpfoe.expl);

   printf("Speed [%d]: ", tmpfoe.speed);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%d", &tmpfoe.speed);

   if (sfoe != -1) {
      *foe[sfoe] = tmpfoe;
   }

}

void storepath(void)
{

   if ((foe[nfoes] = malloc(sizeof(struct foestrc)+tmppos*2)) != NULL) {
      sfoe = nfoes++;
      handle2 = defsprite(sprite[tmpfoe.sprite].sprite, sprite[tmpfoe.sprite].flags);
      xstart[sfoe] = tmpxstart; ystart[sfoe] = tmpystart;
      pathlen[sfoe] = sizeof(struct foestrc) + tmppos*2;
      *foe[sfoe] = tmpfoe;
      memcpy(&foe[sfoe]->path, tmp, tmppos*2);
   } else
      error("OUT OF MEMORY IN STOREPATH", 0);

}


void delpath(void)
{
   int      i;
   int      pos;
   unsigned *data;

   if (sfoe != -1) {
      free(foe[sfoe]);
      for (i = sfoe + 1; i < nfoes; i++) {
	 foe[i-1] = foe[i];
	 xstart[i-1] = xstart[i];
	 ystart[i-1] = ystart[i];
	 pathlen[i-1] = pathlen[i];
      }
      nfoes--;
// Adjust references to other foes.
      for (i = 0; i < nfoes; i++) {
	 pos = 0; data = (unsigned *) &foe[i]->path;
	 while (data[pos] != FOEENDPATH && data[pos] != FOECYCLEPATH) {
	    if (data[pos] == FOERELEASEFOE) {
	       if (data[pos+1] > sfoe) data[pos+1]--;
	    }
	    pos = findcommand(data, pos, 1);
	 }
      }
      for (i = 0; i < nattacks; i++) {
	 if (attack[i].foe > sfoe) attack[i].foe--;
      }
      if (nfoes == 0) sfoe = -1;
      else {
	 sfoe--; if (sfoe < 0) sfoe = nfoes - 1;
      }
   }
}


void delattack(void)
{
   int i;
   if (sattack != -1) {
      for (i = sattack + 1; i < nattacks; i++)
	 attack[i-1] = attack[i];
      nattacks--;
      if (nattacks == 0) sattack = -1;
      else {
	 sattack--; if (sattack < 0) sattack = 0;
      }
   }
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
	    plot(x, y, 12);
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



void newfoe(void)
{

   clearscreen();
   status();

   if (sfoe != -1) {
      tmpfoe = *foe[sfoe];
      tmpxstart = xstart[sfoe]; tmpystart = ystart[sfoe];

      killsprite(handle2);
      handle2 = defsprite(sprite[tmpfoe.sprite].sprite, sprite[tmpfoe.sprite].flags);
      runpos = -1; runx = tmpxstart; runy = tmpystart;
      lastkey = 'M';
      putsprite(handle2, runx, runy, 0);

      showpath(runx, runy, sfoe, 11);
      tmppos = pathlen[sfoe]-sizeof(struct foestrc);
      memcpy(tmp, &foe[sfoe]->path, tmppos);
      tmppos = tmppos/2 + 1;
   }

}

void newsprite(void)
{
   killsprite(handle);
   handle = defsprite(sprite[sspr].sprite, sprite[sspr].flags);
   setborder(-sprite[sspr].sprite->xs, -sprite[sspr].sprite->ys, 320, YMAX);
   xa = sprite[sspr].sprite->xs/2; ya = sprite[sspr].sprite->ys/2;
   setpos(x-xa, y-ya);
   getpos(&b, &x, &y);
   putsprite(handle, x, y, 0);

}

int sort_cmp(const void *ptr1, const void *ptr2)
{
   struct attackstrc *a1, *a2;

   a1 = (struct attackstrc *) ptr1;
   a2 = (struct attackstrc *) ptr2;
   return(a1->count - a2->count);

}

void sort(void)
{

   qsort(attack, nattacks, sizeof(struct attackstrc), sort_cmp);

}


void defpath(int f, int x, int y)
{
   int  i;

   for (i = 0; i < MAXANI; i++)
      if (animate[i].foe == -1) {
	 animate[i].foe = f;
	 animate[i].x = x;
	 animate[i].y = y;
	 animate[i].pos = 0;
	 animate[i].sprite = defsprite(sprite[foe[f]->sprite].sprite,
				       sprite[foe[f]->sprite].flags);
	 break;
      }

}

void expath(struct pathstrc *lall, int dir, int show)
{
   int  pos;
   int  *data;

   pos = lall->pos;
   data = foe[lall->foe]->path;


   do {
   if (pos < 0) {
      lall->foe = -1;
      killsprite(lall->sprite);
      return;
   }

   switch (data[pos]) {
      case FOEENDPATH:
	 lall->foe = -1;
	 killsprite(lall->sprite);
	 return;
      case FOECYCLEPATH:
	 lall->pos = pos = lall->savepos;
	 lall->x = lall->savex; lall->y = lall->savey;
	 break;
      case FOEMARK:
	 lall->savepos = pos;
	 lall->savex = lall->x;
	 lall->savey = lall->y;
	 break;
      case FOECHANGESPRITE:
	 killsprite(lall->sprite);
	 lall->sprite = defsprite(sprite[data[pos+1]].sprite, sprite[data[pos+1]].flags);
	 break;
      case FOERELEASEFOE:
	 defpath(data[pos+1], data[pos+2]+lall->x, data[pos+3]+lall->y);
	 break;
      case FOESOUND:
	 break;
      default:
	 if (dir == 1) {
	    lall->x += data[pos]; lall->y += data[pos+1];
	 } else {
	    lall->x -= data[pos]; lall->y -= data[pos+1];
	 }
	 if (show) putsprite(lall->sprite, lall->x, lall->y, 0);
	 lall->pos = pos = findcommand(data, pos, dir);
	 return;
   }
   lall->pos = pos = findcommand(data, pos, dir);
   } while (1);

}


void animateonestep(int dir, int show)
{
   int   i;

   if (count != 0xffff && nattacks > 0) {
   for (i = 0; i < MAXATT; i++) {
      if ((attack[i].count == count) && !(attack[i].foe & 0x8000))
	 defpath(attack[i].foe, attack[i].x, attack[i].y);
   }
   }
   for (i = 0; i < MAXANI; i++) {
      if (animate[i].foe != -1) expath(&animate[i], dir, show);
   }

}

void animateabit(int ch)
{
   int  i;

   switch (ch) {
	 case 'H': // Cursor Up
		   sattack++; if (sattack >= nattacks) sattack = nattacks-1;
		   killallsprites();
		   initanimate();
		   clearscreen();
		   if (sattack != -1) {
		      count = attack[sattack].count-1;
		      showpath(attack[sattack].x, attack[sattack].y, attack[sattack].foe, 14);
		   }
		   break;
	 case 'P': // Cursor Down
		   if (sattack > 0) sattack--;
		   killallsprites();
		   initanimate();
		   clearscreen();
		   if (sattack != -1) {
		      count = attack[sattack].count-1;
		      showpath(attack[sattack].x, attack[sattack].y, attack[sattack].foe, 14);
		   }
		   break;
	 case 'M': count++;  // Cursor right
		   animateonestep(1, TRUE);
		   break;
	 case 'K': --count;  // cursor left
		   killallsprites();
		   initanimate();
		   break;

	 case '6': for (i = 0; i < 15; i++) {
		      count++;
		      animateonestep(1, FALSE);
		   }
		   count++; animateonestep(1, TRUE);
		   break;
	 case '4': count -= 16;  // Fast left
		   killallsprites();
		   initanimate();
		   break;
   }


}


int excommand(int pos, int dir)
{

   if ((pos < 0) && (dir == -1)) return pos;
   if (((unsigned)tmp[pos] == FOEENDPATH) && (dir == 1)) return pos;
   if (((unsigned)tmp[pos] == FOECYCLEPATH) && (dir == 1)) return pos;

   if ((pos = findcommand(NULL, pos, dir)) >= 0) {

      switch (tmp[pos]) {
	 case FOEENDPATH:
	    break;
	 case FOECYCLEPATH:
	    break;
	 case FOEMARK:
	    break;
	 case FOECHANGESPRITE:
	    killsprite(handle2);
	    handle2 = defsprite(sprite[tmp[pos+1]].sprite,sprite[tmp[pos+1]].flags);
	    break;
	 case FOERELEASEFOE:
	 case FOESOUND:
	    break;
	 default:
	    removesprite(handle2, runx, runy);
	    if (dir == 1)
	       putsprite(handle2, runx+=tmp[pos], runy+=tmp[pos+1], 0);
	    else
	       putsprite(handle2, runx-=tmp[pos], runy-=tmp[pos+1], 0);
	    break;
      }
   }
   return pos;
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


void refreshpath(void)
{
   free(foe[sfoe]);

   if ((foe[sfoe] = malloc(sizeof(struct foestrc)+tmppos*2)) != NULL) {
      *foe[sfoe] = tmpfoe;
      memcpy(&foe[sfoe]->path, tmp, tmppos*2);
   } else
      error("CRITICAL OUT OF MEMORY. PATH LOST. EDITOR UNSTABLE. QUIT!!!",0);
}

void deletecommand(int pos)
{
   int  i;

   if (runpos < 0) return;
   memmove(&tmp[pos], &tmp[i = findcommand(NULL, pos, 1)], (tmppos-pos)*2);
   i -= pos;
   tmppos -= i;
   refreshpath();

}

void insertcommand(int pos, int n, ...)
{
   va_list ap;
   int     i;

   memmove(&tmp[pos+n], &tmp[pos], (tmppos-pos)*2);
   tmppos += n;
   pathlen[sfoe] += n*2;
   va_start(ap, n);
   for (i = 0; i < n; i++) {
      tmp[pos+i] = va_arg(ap, int);
   }
   va_end(ap);

   refreshpath();

}


void doonestep(int ch)
{
   int   i;

   switch (ch) {
      case 'M': // Cursor Right
		if (sfoe != -1) {
		   if (lastkey == 'M') {
		      runpos = excommand(runpos, 1);
		   } else {
		      lastkey = 'M';
		      runpos = findcommand(NULL, runpos, -1);
		      runpos = excommand(runpos, 1);
		   }

		}
		break;
      case 'K': // Cursor Left
		if (sfoe != -1) {
		   if (lastkey == 'K') {
		      runpos = excommand(runpos, -1);
		   } else {
		      lastkey = 'K';
		      runpos = findcommand(NULL, runpos, 1);
		      runpos = excommand(runpos, -1);
		   }
		}
		break;

      case '4': // fast left
		if (sfoe != -1) {
		   if (lastkey != 'K') {
		      lastkey = 'K';
		      runpos = findcommand(NULL, runpos, 1);
		   }
		   for (i = 0; i < 10; i++) {
		      runpos = excommand(runpos, -1);
		   }
		}
		break;
      case '6': // fast right
		if (sfoe != -1) {
		   if (lastkey != 'M') {
		      lastkey = 'M';
		      runpos = findcommand(NULL, runpos, -1);
		   }
		   for (i = 0; i < 10; i++) {
		      runpos = excommand(runpos, 1);
		   }
		}
		break;

   }

}


int getpath(void)
{
   int   path0;
   int   tmphandle;
   int   quit;

   if ((tmphandle = defsprite(sprite[foe[path]->sprite].sprite,
	 sprite[foe[path]->sprite].flags)) == -1)
      error("OUT OF MEM. SAVE YOUR WORK. CRASH IS NEAR.",0);
   setborder(-sprite[foe[path]->sprite].sprite->xs, -sprite[foe[path]->sprite].sprite->ys, 320, YMAX);
   showpath(x, y, path, 11);
   putsprite(tmphandle, x, y, 0);
   do {
      x0 = x; y0 = y; path0 = path;
      getpos(&b, &x, &y);
      if (getkey()) return -1;
      if ((x0 != x) || (y0 != y) || (path != path0)) {
	 showpath(x0, y0, path0, 0);
	 removesprite(tmphandle, x0, y0);
	 if (path != path0) {
	    killsprite(tmphandle);
	    if ((tmphandle = defsprite(sprite[foe[path]->sprite].sprite,
		  sprite[foe[path]->sprite].flags)) == -1)
	       error("OUT OF MEM. SAVE YOUR WORK. CRASH IS NEAR.",0);
	 setborder(-sprite[foe[path]->sprite].sprite->xs, -sprite[foe[path]->sprite].sprite->ys, 320, YMAX);
	 }
	 showpath(x, y, path, 11);
	 putsprite(tmphandle, x, y, 0);
      }

   } while (!b);
   killsprite(tmphandle);

   return b;

}

int getsprite(void)
{

   do {
      x0 = x; y0 = y;
      getpos(&b, &x, &y);
      getkey();
      if ((x0 != x) || (y0 != y)) {
	 removesprite(handle, x0, y0);
	 putsprite(handle, x, y, 0);
      }
   } while (!b);

   return b;

}



void copytoindex(void)
{
   int  i;

   for (i = 0; i < nfoes; i++)
      ptrindex[i] = foe[i];
}


void doinsertion(void)
{
   int   i, n;

   if (sfoe == -1) return;
   writetext(0, STATLINE-8, "1.RELE,2.MARK,3.SWITCH,4.SND,5.WAIT", font);

   switch (getch()) {

      case '1': // Release sprite.
	 if (nfoes <= 1) break;
	 if (path == sfoe)
	    if (sfoe == 0) path = 1; else path = 0;
	 pathedit = 0;
	 removesprite(handle, x, y);
	 b = getpath();
	 pathedit = 1;
	 if (b == 1) { // Left mouse pressed
	    insertcommand(findcommand(NULL, runpos, 1), 4,
			  FOERELEASEFOE, path, x-runx, y-runy);
	 }
      break;

      case '2': // Cycle MARK
	 insertcommand(findcommand(NULL, runpos, 1), 1, FOEMARK);
	 break;

      case '3': // Switch CYCLEPATH <-> ENDPATH
	 if ((unsigned)tmp[runpos] == FOECYCLEPATH)
	    tmp[runpos] = FOEENDPATH;
	 else if ((unsigned)tmp[runpos] == FOEENDPATH)
	    tmp[runpos] = FOECYCLEPATH;
	 foe[sfoe]->path[runpos] = tmp[runpos];
      break;

      case '4': // Sound a bit around.
	 insertcommand(findcommand(NULL, runpos, 1), 2,
		       FOESOUND, getdec("SND."));
      break;

      case '5': // Wait at position.
	 n = getdec("WAITS (22 IS 1 SEC).");
	 for (i = 0; i < n; i++)
	    insertcommand(findcommand(NULL, runpos, 1), 2, 0, 0);
      break;

   }

}

void doattackinsert(void)
{
   writetext(0, STATLINE-8, "1.GO,2.STOP,3.SND,4.MARK", font);

   switch (getch()) {

      case '1': // Go starfield, go.
	 sattack = nattacks++;
	 attack[sattack].count = count;
	 attack[sattack].foe = A_GOFIELD;
      break;

      case '2': // Stop Starfield.
	 sattack = nattacks++;
	 attack[sattack].count = count;
	 attack[sattack].foe = A_STOPFIELD;
      break;

      case '3': // Sound a bit around.
	 sattack = nattacks++;
	 attack[sattack].count = count;
	 attack[sattack].foe = A_SOUND;
	 attack[sattack].x = getdec("SND.");
      break;

      case '4': // Mark position.
	 sattack = nattacks++;
	 attack[sattack].count = count;
	 attack[sattack].foe = A_MARK;
      break;

   }

}


void copypath(void)
{

   if (sfoe != -1) killsprite(handle2); else return;
   if ((foe[nfoes] = malloc(pathlen[sfoe])) != NULL) {
      xstart[nfoes] = xstart[sfoe];
      ystart[nfoes] = ystart[sfoe];
      pathlen[nfoes] = pathlen[sfoe];
      memcpy(foe[nfoes], foe[sfoe], pathlen[sfoe]);
      foe[nfoes]->sprite = sspr;
      handle2 = defsprite(sprite[sspr].sprite, sprite[sspr].flags);
      sfoe = nfoes++;
   } else
      error("OUT OF MEMORY IN STOREPATH", 0);

}


int getkey(void)
{
   int   ch1, ch2;
   int   i;


   ch1 = ch2 = 0xff;
   if(kbhit()) ch1 = getch();
   if(ch1 == 0) ch2 = getch();

   if (ch1 != 0xff)
   if (ch1 == 0) {

      switch (ch2) {


	 case 'A':
	 case 'B': if (pathedit) {
		      if (sfoe != -1) {
			 sfoe = roll(sfoe, 0, nfoes - 1, ch2 - 'A');
			 newfoe();
		      }
		   } else if (attackedit) {
		      path = roll(path, 0, nfoes - 1, ch2 - 'A');
		   } else {
		      do {
			 path = roll(path, 0, nfoes - 1, ch2 - 'A');
		      } while (path == sfoe);
		   }
		   break;

	 case 'C': // F9
	 case 'D': if (attackedit) break;
		   removesprite(handle, x, y);  // F10
		   setpos(x+xa, y+ya);
		   getpos(&b, &x, &y);
		   sspr = roll(sspr, 0, nsprites - 1, ch2 - 'C');
		   newsprite();
		   break;

	 case 'S': // Del key
		   if (pathedit) deletecommand(runpos);
		   break;

	 case 'R': // Insert key
		   if (pathedit) {
		      doinsertion();
		      removesprite(handle, x, y);  // F10
		      setpos(x+xa, y+ya);
		      getpos(&b, &x, &y);
		      newsprite();
		   } else {
		      doattackinsert();
		      sort();
		   }
		   break;


	 default:  if (attackedit) animateabit(ch2);
		   if (pathedit) doonestep(ch2);
		   break;
      }

      status();

   } else {

      switch (ch1) {

	 case '-': if (attackedit) break;
		   speed -= 1;
		   if (speed == 0) speed = 1;
		   break;
	 case '=':
	 case '+': if (attackedit) break;
		   speed += 1;
		   if (speed == 16) speed = 15;
		   break;

	 case 'e': if (!pathedit) break;
		   killsprite(handle);
		   shutxmode();
		   editvalues();
		   initxmode();
		   setborder(-sprite[sspr].sprite->xs, -sprite[sspr].sprite->ys, 320, YMAX);
		   handle = defsprite(sprite[sspr].sprite, sprite[sspr].flags);
		   newfoe();
		   putsprite(handle, x, y, 0);
		   break;

	 case 'd': if (pathedit) {
		      delpath();  // Delete.
		      newfoe();
		      putsprite(handle, x, y, 0);
		      writetext(0, 50, "PLEASE REMEMBER, THAT FOE REFERENCES", font);
		      writetext(0, 58, "IN EXPLOSIONS ARE NOT ADJUSTED.", font);
		   } else if (attackedit) {
		      delattack();
		      sort();
		   }
		   break;

	 case 'c': if (attackedit) {
		      clearscreen();
		      for (i = 0; i < MAXANI; i++) {
			 if (animate[i].foe != -1)
			    putsprite(animate[i].sprite,
				      animate[i].x, animate[i].y, 0);
		      }
		   } else if (pathedit) {
		      clearscreen();
		   }
		   break;

	 case 's': // Ein Schuss definieren.
		   if (!pathedit) break;
		   if (sfoe != -1) killsprite(handle2);
		   sfoe = -1; lastkey = 'M';
		   tmpfoe.flags = FOE_LINE;
		   tmpfoe.speed = speed;
		   runx = tmpxstart = XMAX/2;
		   runy = tmpystart = YMAX/2;
		   tmppos = 0;
		   tmpfoe.sprite = sspr;
		   runpos = -1;
		   tmp[tmppos++] = FOEENDPATH;
		   storepath();
		   newfoe();
		   break;

	 case 'o': // Copy path.
		   if (!pathedit) break;
		   copypath();
		   break;

	 case 'q': return TRUE;

	 default:  if (attackedit) animateabit(ch1);
		   if (pathedit) doonestep(ch1);
		   break;

      }

      status();

   }

   return FALSE;

}


int getline(int *x1, int *y1, int *x2, int *y2)
{

   *x1 = x + xa; *y1 = y + ya;

   waitforrelease(&x, &y);
   xorline(*x1, *y1, x0 = x+xa, y0 = y+ya, 15);

   do {
      do {
	getpos(&b, &x, &y);
	getkey();
      } while (!b && (x == x0-xa) && (y == y0-ya));

      xorline(*x1, *y1, x0, y0, 15);
      xorline(*x1, *y1, x+xa, y+ya, 15);
      x0 = x+xa; y0 = y+ya;

   } while (!b);

   *x1 -= xa; *y1 -= ya;
   *x2 = x; *y2 = y;
   return b;

}



int addpath(void)
{
   int   x1, y1, x2, y2;
   int   flag;
   int   cspr;


   if (nfoes < 400) {

      if (sfoe != -1) killsprite(handle2);
      sfoe = -1; lastkey = 'M';
      tmpfoe.flags = FOE_PATH;
      tmpfoe.speed = speed;

      tmppos = 0;
      tmpfoe.sprite = sspr;
      runpos = -1;
      cspr = sspr;
      pixelcount = 0;

      removesprite(handle, x, y);
      x &= (sprite[sspr].flags == 4 ? 0xfffc : 0xfffe);
      runx = tmpxstart = x;
      runy = tmpystart = y;
      putsprite(handle, x, y, 0);

      if ((flag = getline(&x1, &y1, &x2, &y2)) != 2) {
	 if (cspr != sspr) {
	    tmp[tmppos++] = FOECHANGESPRITE;
	    tmp[tmppos++] = sspr;
	    cspr = sspr;
	 }
	 tmpline(x1, y1, x2, y2, speed, 1);
	 flag = getline(&x1, &y1, &x2, &y2);

	 while (flag != 2) {
	    if (cspr != sspr) {
	       tmp[tmppos++] = FOECHANGESPRITE;
	       tmp[tmppos++] = sspr;
	       cspr = sspr;
	    }
	    tmpline(x1, y1, x2, y2, speed, 0);
	    flag = getline(&x1, &y1, &x2, &y2);
	 }

	 tmp[tmppos++] = FOEENDPATH;
      }
      putsprite(handle, x, y, 0);
      runx = tmpxstart; runy = tmpystart;
      return 0;
   } else {
      error("FOE INDEX FULL.",0);
      return -1;
   }
}




int newpath(void)
{


   if (sprite == NULL) {
      printf("\nSprite Library not loaded.\n");
      return -1;
   }


   initxmode();
   initmouse();

   handle = defsprite(sprite[sspr].sprite, sprite[sspr].flags);
   setborder(-sprite[sspr].sprite->xs, -sprite[sspr].sprite->ys, 320, YMAX);
   xa = sprite[sspr].sprite->xs/2; ya = sprite[sspr].sprite->ys/2;

   status();
   getpos(&b, &x, &y);

   if (sfoe != -1)
      handle2 = defsprite(sprite[0].sprite,4); // Just define ANY sprite.
   newfoe();

   do {
      x0 = x; y0 = y;
      getpos(&b, &x, &y);
      if (b == 1) {
	 if (addpath() == 0) {
	    storepath();
	    newfoe();
	 }
	 status();
      }
      if ((x0 != x) || (y0 != y)) {
	 removesprite(handle, x0, y0);
	 putsprite(handle, x, y, 0);
      }


   } while (!getkey());

   killallsprites();

   clearmouse();
   shutxmode();

   return 0;
}



void editattack(void)
{

   if (sprite == NULL) {
      printf("\nSprite library not loaded.\n\n");
      return;
   }
   if (nfoes == 0) {
      printf("\nNot even a single enemy defined. Can't continue.\n\n");
      return;
   }

   initxmode();
   initmouse();

   initanimate();
   lastkey = 'M';
   path = 0;
   getpos(&b, &x, &y);

   status();
   do {
      b = getpath();
      if (b == 1) {
	 sattack = nattacks++;
	 attack[sattack].count = count;
	 attack[sattack].x = x; attack[sattack].y = y;
	 attack[sattack].foe = path;
	 showpath(x, y, path, 12);
	 sort();
	 delay(200);
      }
   } while (b != -1);

   killallsprites();

   clearmouse();
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

   strcpy(fname, file);
   if ((filvar = open(strcat(fname, ".TBL"), O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      return -1;
   }
   read(filvar, &nattacks, sizeof(nattacks));
   read(filvar, attack, nattacks * sizeof(struct attackstrc));
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

   strcpy(fname, file);
   filvar = open(strcat(fname, ".FSP"), O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE);
   if (filvar == -1) return -1;
   write(filvar, xstart, nfoes*2); write(filvar, ystart, nfoes*2);
   write(filvar, pathlen, nfoes*2);
   close(filvar);

   strcpy(fname, file);
   filvar = open(strcat(fname, ".TBL"), O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE);
   if (filvar == -1) return -1;
   write(filvar, &nattacks, 2);
   write(filvar, attack, nattacks*sizeof(struct attackstrc));
   close(filvar);

   return 0;

}


int readslib(char *file)
{
   int      filvar;
   long	    size;
   int      nentries;
   int      i;
   char     huge *h;

   if ((filvar = open(file, O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      return -1;
   }
   read(filvar, &nsprites, sizeof(nsprites));
   size = filelength(filvar) - sizeof(nsprites);

   if ((sprite = farmalloc(size)) == NULL) {
      close(filvar);
      return -1;
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


void shutsprites(void)
{
   if (sprite != NULL) farfree(sprite);
}


int main(void)
{
   char   libfile[80];
   int    choice;
   int    i;

   screenmode(0x02);

   printf("\n\nBaller Game PATH EDITOR "
	  "[c] copyright by ALPHA-HELIX Dec 92.\n\n");

   if (loadfont() == -1) {
      printf("Font file not found !\n");
      abort();
   }

   tmpfoe.score = 33; tmpfoe.shield = 1; tmpfoe.expl = 0;

   do {

   do {
      printf("\n\n");
      printf("1 - Edit Pathes.\n");
      printf("2 - Edit Attack Table.\n");
      printf("6 - Load Pathes.\n");
      printf("7 - Save Pathes.\n");
      printf("8 - Read Sprite Library.\n");
      printf("9 - Quit PE.\n");

      printf("\nYour choice -> "); choice = getche() - '0';
      printf("\n\n");

      switch (choice) {
	 case 1: pathedit = 1;
		 newpath();
		 pathedit = 0;
		 break;

	 case 2: attackedit = 1;
		 editattack();
		 attackedit = 0;
		 break;

	 case 6: printf("Foe File [.FOE]: ");
		 fflush(stdin);
		 scanf("%s", libfile);
		 for (i = 0; i < nfoes; i++)
		    free(foe[i]);
		 if (loadpathes(libfile) == -1) {
		    perror("File not read: ");
		 } else {
		    sfoe = 0;
		 }
		 break;

	 case 7: printf("Foe File [.FOE]: ");
		 fflush(stdin);
		 scanf("%s", libfile);
		 sort();
		 if (savepathes(libfile) == -1) {
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

   for (i = 0; i < nfoes; i++)
      free(foe[i]);
   shutsprites();
   free(font);
   printf("\nThanks for using PE !\n\n\n\n");

   return 0;
}
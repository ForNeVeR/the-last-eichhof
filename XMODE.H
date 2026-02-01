// SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

// Header file for xmode.



#ifndef __XMODE__
#define __XMODE__

// Definiton of graphics window.
#define XMIN			0
#define XMAX			319
#define YMIN			0
#define YMAX			219


#define MAXSPRITES		60
#define MAXOBJS			60

#define OBJ_ONECYCLE		0x01	// Destroy object after a cycle.
#define OBJ_HIGH		0x02	// High priority.
#define OBJ_LOW			0x00	// Low priority.


/* --- SPRITE DEFINITION
   A sprite is defined as the following:
   xs  :  Width of sprite in pixels.
   ys  :  Heigth in pixels.
   maxn:  Number of pictures per sprite. (always > 0)

 Located after this header is the sprite data itself.
 Each pixel is ONE byte. (planar video mode)
 First row of first picture, second row of first picture, ...
 First row of second picture, ... and so on.
*/

#define SPR_ALIGN		0x07	// Align Field.
#define SPR_DOUBLE		0x08    // Double paint picture.

struct sprstrc {

   int   xs;			// x size of sprite.
   int   ys;			// y size.
   int	 maxn;			// Number of pictures.
   // Immediately here follows the SPRITEDATA.
   char  data[];
};


/* --- STAR definition
   Very simple indeed.
   Just a coordinate pair, a color and a base speed. (per star)
   A STARFIELD consists of an array of STARS.
*/

struct starstrc {

   int   x, y;
   int   color;
   int   speed;

};


#define retrace() {while(inportb(0x3da)&8); while(!(inportb(0x3da)&8));}


void setpalette(char *ptr, int ncolors);
void setcolor(int c, int r, int g, int b);
void setvanillapalette(int c);
void setstandardpalette(void);
void glowto(int r, int g, int b);
void glowin(int dir);
void glowout(void);
void setattrib(int m);
int cyclepalette(int c1, int c2, int pos);

void defstarfield(int n, struct starstrc *star);
void gostarfield(void);
void stopstarfield(void);
void killstarfield(void);

int defsprite(void *sprite, unsigned flags);
void killsprite(int sprite);
void killallsprites(void);

int defobject(int sprite, int x, int y, int cycle);
void abandonobject(int obj);
void killobject(int obj);
void killallobjects(void);
void shutxmode(void);
void initxmode(void);
void setpage(int p);
void updatescreen(void);

unsigned long getspritesize(int sprite);
unsigned long getobjectsize(int object);
unsigned long getobjectpos(int object);

void setxmode(void);
void screenmode(int mode);
int VGApresent(void);
void plot(int x, int y, int c);
int getpixel(int x, int y);

int moveobject(int obj, int x, int y);
int moveobjectdelta(int obj, int deltax, int deltay);
void flash(int obj);
int changesprite(int obj, int sprite);
int crashtest(int obj1, int obj2);
int outofwindow(int obj);

void putsprite(int sprite, int x, int y, int n);
void removesprite(int sprite, int x, int y);

void putspritedirect(struct sprstrc *spr, int xp, int yp, int n);

void clearscreen(void);
void clearregion(int y, int n);
void copypage(int src, int dst);
void showpage(int p);
void showline(int line);
void showpcx256(void *pic, int line);

// window definition.
extern int   windowx0, windowy0;
extern int   windowx1, windowy1;

extern int   objflashcolor;
extern int   backgrndcolor;


#endif


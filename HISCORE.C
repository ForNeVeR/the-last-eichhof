// module hiscore.c
// SPDX-FileCopyrightText: 1993 ALPHA-HELIX <http://www.ife.ee.ethz.ch/~ammann/alphahelix/>
// SPDX-FileCopyrightText: 1994-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

#pragma hdrstop

#include <bios.h>
#include <string.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include <alloc.h>

#define	MAXENTRIES	8
#define	NOCP		15	// Number of Chars playername
#define	NOCW		33	// Number of Chars winnertext
#define YOFFS		60	// Y-offset for listbegin
#define SXOFFS		255     // X-offset for centiliter
#define NXOFFS		55	// X-offset for names

#include "xmode.h"
#include "sound.h"
#include "fileman.h"
#include "baller.h"

struct playerdatas{
   long score;
   char name[NOCP+1];
};

struct highscoretext{
   struct playerdatas player[MAXENTRIES];
   char winnertext[NOCW+1];
};

static struct highscoretext hstxt={{{   81000L, "ATOMMUELL"},
				    {   29600L, "POWER"},
				    {   14800L, "JUBEL"},
				    {    8110L, "ZYNAX"},
				    {    8099L, "HEAVY"},
				    {    6367L, "WASSERGLAS"},
				    {    4558L, "TWEETY"},
				    {       1L, "TRITONE"}},
				   "EIN PROSIT AUF ALPHA-HELIX"};

static void cursorblink(int x, int y, char symbol, struct sprstrc *font)
{
   int set;

   set=FALSE;
   if(symbol==0) symbol=' ';
   while(bioskey(1)==0){
      if(set==0){
	 set=8;
	 putspritedirect(font, x, y, 3);
      }
      if(set==4){
	 putspritedirect(font, x, y, symbol-' ');
      }
      set--;
      waitfortick();
   }
   if(set>3) putspritedirect(font, x, y, symbol-' ');
}

static void codescore(void)
{
   int i,j;

   for(j=0;j<MAXENTRIES;j++){
      for(i=0;i<=NOCP;i++){
	 hstxt.player[j].name[i]+=3*i;
      }
      hstxt.player[j].score+=123456789;
   }
   for(j=0;j<=NOCW;j++){
      hstxt.winnertext[j]+=j*2;
   }
}

static void decodescore(void)
{
   int i,j;

   for(j=0;j<MAXENTRIES;j++){
      for(i=0;i<=NOCP;i++){
	 hstxt.player[j].name[i]-=3*i;
      }
      hstxt.player[j].score-=123456789;
   }
   for(j=0;j<=NOCW;j++){
      hstxt.winnertext[j]-=j*2;
   }
}

void loadhighscore(int *filvar)
{
   read(*filvar, &hstxt, sizeof(struct highscoretext));
   decodescore();
}

void savehighscore(int *filvar)
{
   codescore();
   write(*filvar, &hstxt, sizeof(struct highscoretext));
   decodescore();
}

void highscore(char fullmode)
{
   int i,j,x,xs,currentpl,inlist;
   char keys;
   void *ptr;
   int filvar;
   void far *buffer;
   struct sprstrc *font2, *font;

   setvanillapalette(0);
   setpage(0); showpage(0); clearscreen();
   ptr = loadfile(datapool, "score.pcx");
   showpcx256(ptr, 0);
   unloadfile(ptr);

   font  = loadfile(datapool, "font1.spr");
   font2 = loadfile(datapool, "font2.spr");
   buffer = farmalloc(120*1024L);
   filvar = openfile(datapool, "hs.snd");

// Checking for a place in the highscore

   inlist=FALSE;
   if(fullmode){
      currentpl=0;
      while(!inlist && currentpl<MAXENTRIES){
	 if(score<=hstxt.player[currentpl].score) currentpl++;
	 else inlist=TRUE;
      }

// Is in highscore

      if(inlist){
	 for(i=MAXENTRIES-1; i>currentpl; i--) hstxt.player[i]=hstxt.player[i-1];
	 strcpy(hstxt.player[currentpl].name,"");
	 hstxt.player[currentpl].score=score;
	 if(currentpl==0) strcpy(hstxt.winnertext,"");

      }
   }
// Show new highscore

   for(i=0;i<MAXENTRIES;i++){
      writetext(NXOFFS,YOFFS+i*14,hstxt.player[i].name,font);
      writenumber(SXOFFS,YOFFS+i*14,hstxt.player[i].score,font);
   }
   x=(320-strlen(hstxt.winnertext)*font2->xs)/2;
   writetext(x,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext,font2);
   playfile(filvar, buffer, 120);
   glowin(0);

// Enter name

   if(inlist){
      xs=font->xs;
      i=0;
      x=NXOFFS;
      setvect(0x09, int09);
      do{
	 cursorblink(x,YOFFS+currentpl*14,hstxt.player[currentpl].name[i],font);
	 keys=bioskey(0) & 0xFF;
	 if(keys<=122 && keys>=97) keys-=32;
	 if((keys<=90 && keys>=65) || keys==63 || keys == 46 ||
		keys==39 || (keys<=57 && keys >=48) ||
		keys==32 || keys==33 || keys==34){
	    hstxt.player[currentpl].name[i]=keys;
	    hstxt.player[currentpl].name[i+1]=0;
	    putspritedirect(font, x, YOFFS+currentpl*14, keys-' ');
	    if(i<NOCP-1){
	       i++;
	       x+=xs;
	    }
	 }
	 else if(keys==0x08 && i>0){
	    if(i!=(NOCP-1) || hstxt.player[currentpl].name[NOCP-1]==0){
	       i--;
	       x-=xs;
	    }
	    hstxt.player[currentpl].name[i]=0;
	    putspritedirect(font, x, YOFFS+currentpl*14, 0);
	 }
      }while(keys!=0x0d);
      if(hstxt.player[currentpl].name[0]==0){
	 strcpy(hstxt.player[currentpl].name,"TOO DRUNK");
	 writetext(x,YOFFS+currentpl*14,hstxt.player[currentpl].name,font);
      }

// Winnertext

      if(currentpl==0){
	 writetext(64, YOFFS+(MAXENTRIES+1)*14+8, "PLEASE ENTER WINNER TEXT", font);
	 i=0;
	 xs=font2->xs;
	 do{
	    if(i == NOCW){
	       x = (320 + (NOCW-3) * xs) / 2;
	       cursorblink(x,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext[i-1],font2);
	    }else{
	       x = (320 + (i-1) * xs) / 2;
	       cursorblink(x,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext[i],font2);
	    }
	    keys=bioskey(0) & 0xFF;
	    if(keys<=122 && keys>=97) keys-=32;
	    if((keys<=93 && keys>=65) || (keys<=59 && keys>=39) ||
			keys==32 || keys==33 || keys==34 ||
			keys==61 || keys==63){
	       if(i<NOCW){
		  hstxt.winnertext[i]=keys;
		  hstxt.winnertext[i+1]=0;
		  i++;
	       }else{
		  hstxt.winnertext[NOCW-1]=keys;
		  hstxt.winnertext[NOCW]=0;
	       }
	       clearregion(YOFFS+(MAXENTRIES+3)*14-5,10);
	       writetext((320-(i+1)*xs)/2,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext,font2);
	    }
	    else if(keys==0x08 && i>0){
	       i--;
	       hstxt.winnertext[i]=0;
	       clearregion(YOFFS+(MAXENTRIES+3)*14-5,10);
	       writetext((320-(i+1)*xs)/2,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext,font2);
	    }
	 }while(keys!=0x0d);
	 if(hstxt.winnertext[0]==0){
	    strcpy(hstxt.winnertext,"TOO DRUNK TO WRITE DOWN A TEXT!!!");
	    x=(320-strlen(hstxt.winnertext)*xs)/2;
	    writetext(x,YOFFS+(MAXENTRIES+3)*14-6,hstxt.winnertext,font2);
	 }

	 writetext(64, YOFFS+(MAXENTRIES+1)*14+8, "                        ", font);

      }
      setvect(0x09, newint09);
      saveconfig();
   }else if(fullmode){

// Not in highscore

      writetext(NXOFFS,YOFFS+(MAXENTRIES+1)*14-3,"YOUR SCORE",font);
      writenumber(SXOFFS,YOFFS+(MAXENTRIES+1)*14-3,score,font);
   }
   while(pressedkeys && soundbusy());
   while(!pressedkeys && soundbusy());
   glowout();

   haltsound();
   closefile(filvar);
   farfree(buffer);
   unloadfile(font2);
   unloadfile(font);
}


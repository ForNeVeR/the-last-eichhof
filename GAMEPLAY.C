// SPDX-FileCopyrightText: 1992, 1993 ALPHA-HELIX <http://www.ife.ee.ethz.ch/~ammann/alphahelix/>
// SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "xmode.h"
#include "fileman.h"
#include "sound.h"
#include "baller.h"


#define STARTLOGOX		68
#define STARTLOGOY		110
#define STARTTEXTY		40
#define WINTEXTY		204

// Where's the bottle at the beginning ?
#define	XSTART			150
#define	YSTART			190


// Variables local to this module.

static struct sprstrc *scorebar;
static struct sprstrc *barfont;


/*------------------------------------------------------
Function: newgame

Description: Initializes variables, sprites and
	     whatsoever needed for a brand new game.
------------------------------------------------------*/

static void newgame(void)
{
   int   i;			// Just another boring index variable.
   int   *ptr;

   killallbuddies();
   killallsprites();
   killallobjects();
   killstarfield();

   intindexptr = ptrindexptr = 0;	// Reset index table counters.

// Load weapon characteristics.
   ptr = loadfile(datapool, "weapons.wpn");
   weapon.narms = *ptr;
   weapon.arm = (struct armstrc *)(ptr+1);

// Load shot pathes.
   ptr = loadfile(datapool, "weapons.sht");
   weapon.nshots = *ptr;
   weapon.shot = (struct shotstrc * far *)(ptr+1);
   for (i = 0; i < weapon.nshots; i++) {
      (long)weapon.shot[i] += (long)weapon.shot;
   }

// Load weapon sprite library.
   ptr = loadfile(datapool, "weapons.sli");
   weapon.nsprites = *ptr;
   weapon.sprite = (struct sTableEntry *)(ptr+1);
   for (i = 0; i < weapon.nsprites; i++) {
      (long)weapon.sprite[i].sprite += (long)weapon.sprite;
   }

// Define Weaponsprites and store reference number in 'intindex'.
   for (i = 0; i < weapon.nsprites; i++) {
      if ((intindex[intindexptr++] =
	  defsprite(weapon.sprite[i].sprite, weapon.sprite[i].flags)) == -1)
	 error("newgame1", ENOMEM);
   }
// Sprites are defined and copied to offscreen memory.
// So we needn't waste main memory by keeping a unecessary copy.
   ptr = (int *)weapon.sprite;
   unloadfile(ptr-1);

// Store explosion reference in 'intindex'.
   for (i = 0; i < weapon.nshots; i++) {
      ptrindex[ptrindexptr++] = weapon.shot[i];
   }

// Score Bar definition.
   scorebar = loadfile(datapool, "bar.spr");
   barfont = loadfile(datapool, "barfont.spr");
   if ((barfonthandle = defsprite(barfont, 2)) == -1)
      error ("newgame2", ENOMEM);
   unloadfile(barfont);

// Lall variables initializaton.
   score = scoreold = 0;
   money = 0;
   shipspeed = STARTSHIPSPEED;
   lifes = STARLIFES;
   stage = 0;

// Main weapon definition.
   nweapons = 1;				// First we have 1 weapon.
   weaponlst[0].dx = weaponlst[0].dy = 0;       // At origin (0,0).
   weaponlst[0].arm = weapon.arm[0];
/*
// level 1: Weapon definition during level design.
   shipspeed = 6;
   nweapons += 1;
   weaponlst[1].dx = -18; weaponlst[1].dy = 8;
   weaponlst[1].arm = weapon.arm[1];
*/
/*
// level 2: Weapon definition during level design.
   shipspeed = 6;
   nweapons += 2;
   weaponlst[1].dx = -16; weaponlst[1].dy = 6;
   weaponlst[1].arm = weapon.arm[1];
   weaponlst[2].dx = 16; weaponlst[2].dy = 6;
   weaponlst[2].arm = weapon.arm[4];
*/
/*
// level 3: Weapon definition during level design.
   shipspeed = 6;
   nweapons += 4;
   weaponlst[1].dx = -16; weaponlst[1].dy = 6;
   weaponlst[1].arm = weapon.arm[1];
   weaponlst[2].dx = 16; weaponlst[2].dy = 6;
   weaponlst[2].arm = weapon.arm[4];
   weaponlst[3].dx = 30; weaponlst[3].dy = 8;
   weaponlst[3].arm = weapon.arm[8];
   weaponlst[4].dx = 0; weaponlst[4].dy = 32;
   weaponlst[4].arm = weapon.arm[6];
*/
/*
// level 4: Weapon definition during level design.
   shipspeed = 6;
   nweapons += 6;
   weaponlst[1].dx = -20; weaponlst[1].dy = 8;
   weaponlst[1].arm = weapon.arm[8];
   weaponlst[2].dx = 30; weaponlst[2].dy = 8;
   weaponlst[2].arm = weapon.arm[8];
   weaponlst[3].dx = 0; weaponlst[3].dy = -18;
   weaponlst[3].arm = weapon.arm[5];
   weaponlst[4].dx = 0; weaponlst[4].dy = 32;
   weaponlst[4].arm = weapon.arm[6];
   weaponlst[5].dx = -36; weaponlst[5].dy = 6;
   weaponlst[5].arm = weapon.arm[4];
   weaponlst[6].dx = 16; weaponlst[6].dy = 6;
   weaponlst[6].arm = weapon.arm[1];
*/
}

static void shutnewgame(void)
{
   int   *ptr;

   killallsprites();
   unloadfile(scorebar);
   ptr = (int *)weapon.shot;
   unloadfile(ptr-1);
   ptr = (int *)weapon.arm;
   unloadfile(ptr-1);
}


static void showstartlogo(char *text)
{
   struct sprstrc *logo;
   struct sprstrc *font;
   struct sndstrc *snd;

   clearscreen();
   while(pressedkeys);

   logo = loadfile(datapool, "go.spr");
   font = loadfile(datapool, "font3.spr");
   snd  = loadfile(datapool, "go.snd");
   playsample(snd);
   putspritedirect(logo, STARTLOGOX, STARTLOGOY, 0);
   writetext(XMAX/2-strlen(text)*font->xs/2, STARTTEXTY, text, font);

   while(!pressedkeys);

   haltsound();
   unloadfile(snd);
   unloadfile(font);
   unloadfile(logo);

}

static void showwinpic(void)
{
   struct sprstrc *font;
   void	  *ptr;

   while(pressedkeys);
   setvanillapalette(0);
   setpage(0); showpage(0); clearscreen();
   ptr = loadfile(datapool, "sky.pcx");
   showpcx256(ptr, 0);
   unloadfile(ptr);
   font = loadfile(datapool, "font1.spr");
   writetext(40, WINTEXTY, "SORRY. NO HERO TUNE THIS TIME.", font);
   glowin(1);
   unloadfile(font);

   while(!pressedkeys);

   glowout();
   clearscreen();
   setstandardpalette();
}

static void showendlogo(void)
{
   struct sndstrc *snd;
   void   *ptr;

   while(pressedkeys);
   setvanillapalette(0);
   setpage(0); showpage(0); clearscreen();
   ptr = loadfile(datapool, "landscap.pcx");
   showpcx256(ptr,5);
   unloadfile(ptr);
   snd = loadfile(datapool, "tod.snd");
   playsample(snd);
   glowin(0);

   while(!pressedkeys && soundbusy());

   glowout();
   haltsound();
   unloadfile(snd);
   clearscreen();
   setstandardpalette();
}


/*------------------------------------------------------
Function: initlevel

Description: Loads and initializes a new level.
------------------------------------------------------*/

static void initlevel(int stage)
{
   int   i;
   int   *ptr;
   char  levelstr[14], text[14];	// Level filename strings.

   sprintf(levelstr, "level%d", stage);

// Load Level description.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".dsc"));
   level.descript = (struct descrstrc *)ptr;
// Check for wrong level.
   if (level.descript->level != stage) error("initlevel", EINVDAT);
   showstartlogo(level.descript->text);

// Load Sprite Library.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".sli"));
   level.nsprites = *ptr;
   level.sprite = (struct sTableEntry *)(ptr+1);
   for (i = 0; i < level.nsprites; i++) {
      (long)level.sprite[i].sprite += (long)level.sprite;
   }

// Initialize enemy sprites and index tables.
   lsprofs = intindexptr;
   for (i = 0; i < level.nsprites; i++) {
      if ((intindex[intindexptr++] =
	  defsprite(level.sprite[i].sprite, level.sprite[i].flags)) == -1)
	 error("initlevel", ENOMEM);
   }
// Unload unneeded data.
   ptr = (int *)level.sprite;
   unloadfile(ptr-1);

// Load Sound library.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".snd"));
   level.nsounds = *ptr;
   level.sound = (struct sndstrc * far *)(ptr+1);
   for (i = 0; i < level.nsounds; i++) {
      (long)level.sound[i] += (long)level.sound;
   }
   lsndofs = ptrindexptr;
   for (i = 0; i < level.nsounds; i++) {
      ptrindex[ptrindexptr++] = level.sound[i];
   }

// Load Foe pathes.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".foe"));
   level.nfoes = *ptr;
   level.foe = (struct foestrc * far *)(ptr+1);
   for (i = 0; i < level.nfoes; i++) {
      (long)level.foe[i] += (long)level.foe;
   }
   lfoeofs = ptrindexptr;
   for (i = 0; i < level.nfoes; i++) {
      ptrindex[ptrindexptr++] = level.foe[i];
   }

// Load Explosions.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".exp"));
   level.nexpls = *ptr;
   level.expl = (struct explstrc * far *)(ptr+1);
   for (i = 0; i < level.nexpls; i++) {
      (long)level.expl[i] += (long)level.expl;
   }
   lexplofs = ptrindexptr;
   for (i = 0; i < level.nexpls; i++) {
      ptrindex[ptrindexptr++] = level.expl[i];
   }

// Load Attack table.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".tbl"));
   level.nattacks = *ptr;
   level.attack = (struct attackstrc *)(ptr+1);

// Load Starfield.
   strcpy(text, levelstr);
   ptr = loadfile(datapool, strcat(text, ".sta"));
   level.nstars = *ptr;
   level.star = (struct starstrc *)(ptr+1);

// Starfield definition.
   defstarfield(level.nstars, level.star);
   gostarfield();

// Add bonus score and money.
   score += level.descript->score;
   money += level.descript->money;

}


static void shutlevel(void)
{
   int   i;
   int   *ptr;

   killallbuddies();
   killallobjects();
   killstarfield();
   for (i = 0; i < level.nsprites; i++) {
      killsprite(intindex[i + lsprofs]);
   }
   intindexptr -= level.nsprites;
   ptrindexptr -= (level.nfoes + level.nexpls + level.nsounds);

   ptr = (int *)level.sound;
   unloadfile(ptr-1);
   ptr = (int *)level.expl;
   unloadfile(ptr-1);
   ptr = (int *)level.foe;
   unloadfile(ptr-1);
   ptr = (int *)level.attack;
   unloadfile(ptr-1);
   ptr = (int *)level.star;
   unloadfile(ptr-1);
}



static void showplayfield(void)
{
   clearscreen();
   putspritedirect(scorebar, 0, BARY, 0);
   killallobjects();
   killallbuddies();
   setpage(0);
   displifes();
   setpage(1);
   displifes();
}


void playthegame(void)
{
   int    feedback;

   newgame();

   do {
      initlevel(stage);
      setplayposition(level.nattacks, level.attack, level.descript->nbigboss);
      do {
	 if (!(cheatlevel & CHEATLIFES)) lifes--;
	 showplayfield();
	 defallarms(XSTART, YSTART);
	 feedback = play();
      } while (!feedback && (lifes > 0));
      if (!(cheatlevel & CHEATLIFES)) lifes++;
      shutlevel();
      stage++;
      if (!feedback || (stage == LEVELS)) break;
      weaponmanager();
   } while (1);

   shutnewgame();

// Check if player has fought through all the levels.
   if (feedback && (stage == LEVELS)) {
// Here we will have the winner code.
      showwinpic();
   } else {
// Here goes the looser code.
      showendlogo();
   }

   highscore(TRUE);

}

#pragma hdrstop

#include <sys\stat.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <alloc.h>
#include <stdlib.h>

#define MENUX           142     // X-offset for first menupoint
#define MENUY           58      // Y-offset for first menupoint
#define MSTEP           3       // How far to move at once
#define MENUDELTA       9/MSTEP // 1/MSTEP distance between menupoints
#define MENXTXT		160	// X-offset for menutext sprite
#define MENYTXT		46	// Y-offset for menutext sprite
#define MENYTEXT	60	// Y-offset for menutext
#define MENXTEXT	159	// X-offset for menutext
#define MENYERR         99

#define ENDLOGOX		20
#define ENDLOGOY		110

#define CREDITDELAY		100
#define CREDITSPEED		2
#define STORYSPEED		2
#define BACKDELAY		502
#define TITLEDELAY		460

#include "xmode.h"
#include "sound.h"
#include "fileman.h"
#include "baller.h"

const int ownkeycodes[89] = {0,0,53,54,55,56,57,58,59,60,61,52,49,47,3,4,
			     85,91,73,86,88,93,89,77,83,84,95,97,7,8,
			     69,87,72,74,75,76,78,79,80,63,38,43,12,96,
			     94,92,71,90,70,82,81,48,50,51,13,14,15,1,2,
			     24,25,26,27,28,29,30,31,32,33,5,6,9,10,11,36,16,
			     17,18,37,19,20,21,22,23,0,0,96,34,35};

const char keytable[38][7]={"ESC","SPACE","CPSLCK","BKSP","TAB","NUMLCK",
			    "SCRLCK","ENTER","CTRL","HOME","UP","PGUP",
			    "LSHFT","RSHFT","[*]","ALT","LEFT","[5]",
			    "RIGHT","END","DOWN","PGDN","INSERT","DELETE",
			    "F1","F2","F3","F4","F5","F6","F7","F8","F9",
			    "F10","F11","F12","[-]","[+]"};

void loadconfig(void)
{
   int filvar;
   int sound;

   if ((filvar = open("CONFIG.HIG", O_RDONLY | O_BINARY, S_IREAD)) == -1) {
      key_up = KEYUP;
      key_down = KEYDOWN;
      key_left = KEYLEFT;
      key_right = KEYRIGHT;
      key_fire = KEYSPACE;
      key_pause = KEYPAUSE;
      return;
   }
   read(filvar, &key_up, sizeof(int));
   read(filvar, &key_down, sizeof(int));
   read(filvar, &key_left, sizeof(int));
   read(filvar, &key_right, sizeof(int));
   read(filvar, &key_fire, sizeof(int));
   read(filvar, &key_pause, sizeof(int));
   read(filvar, &sound, sizeof(int));
   speaker(sound);
   loadhighscore(&filvar);
   close(filvar);
}

void saveconfig(void)
{
   int filvar;
   int sound;

   filvar = open("CONFIG.HIG", O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE);
   if (filvar == -1) return;
   write(filvar, &key_up, sizeof(int));
   write(filvar, &key_down, sizeof(int));
   write(filvar, &key_left, sizeof(int));
   write(filvar, &key_right, sizeof(int));
   write(filvar, &key_fire, sizeof(int));
   write(filvar, &key_pause, sizeof(int));
   sound = speaker(-1);
   write(filvar, &sound, sizeof(int));
   savehighscore(&filvar);
   close(filvar);
}

static void closingtime(void)
{
   struct sprstrc *logo;
   struct sndstrc *snd;

   while(pressedkeys);
   clearscreen();
   logo = loadfile(datapool, "close.spr");
   snd  = loadfile(datapool, "close.snd");
   playsample(snd);
   putspritedirect(logo, ENDLOGOX, ENDLOGOY, 0);
   glowin(0);

   while(!pressedkeys && soundbusy());

   glowout();
   haltsound();
   unloadfile(snd);
   unloadfile(logo);
   clearscreen();
   setstandardpalette();
}

static void credits(void)
{
   void  *ptr;
   int   d, line, i;

   snd_cli();
   ptr = loadfile(datapool, "credit.pcx");
   glowout();
   setpage(0);
   clearscreen();
   showpcx256(ptr, 0);
   unloadfile(ptr);
   showpage(0);
   glowin(1);
   snd_sti();                           // Make it visible.

   d = CREDITSPEED;			// Direction and speed of movement.
   line = 0;
   i = 0;
   if(!waitdelayedkey(CREDITDELAY)){
      snd_cli();
      do {
	 showline(line);
	 line += d;
	 if (line < 0) {
	    i++;
	    d = -d; line += 2*d;
	    snd_sti();
	    if (waitdelayedkey(CREDITDELAY)) break;
	    snd_cli();
	 }
	 // 204 ist die linie, bei der das scrolling umgekehrt wird.
	 // Es ist ein proebli wert, der sehr stark vom kunstlerischen
	 // koennen des grafikers abhaengt.
	 if (line > 204) {
	    d = -d; line += 2*d;
	    snd_sti();
	    if (waitdelayedkey(CREDITDELAY)) break;
	    snd_cli();
	 }
      } while (i!=2 && !pressedkeys);
   }
}


static void showtitle(void)
{
   void  *ptr;
   int filvar;
   void far *buffer;

   setpage(0);
   showpage(0);
   setvanillapalette(0);
   clearscreen();
   ptr = loadfile(datapool, "back.pcx");
   showpcx256(ptr, 0);
   unloadfile(ptr);
   ptr = loadfile(datapool, "title.pcx");
   setpage(1);
   showpcx256(ptr, 10);
   unloadfile(ptr);
   buffer = farmalloc(120*1024L);
   filvar = openfile(datapool, "title.snd");
   playfile(filvar, buffer, 120);
   snd_cli();
   glowin(0);
   snd_sti();
   waitdelayedkey(BACKDELAY);
   snd_cli();
   glowout();
   showpage(1);
   glowin(0);
   snd_sti();
   if (!waitdelayedkey(TITLEDELAY)) {
      credits();
   }
   snd_cli();
   glowout();
   snd_sti();
   haltsound();
   closefile(filvar);
   farfree(buffer);
}

static int initmenu(struct sndstrc **s, int *pobj, int *pspr, struct sprstrc **mspr)
{
   void  *ptr;

// Load Menu, show and unload it

   clearscreen();
   setpage(0);
   ptr = loadfile(datapool, "menu.pcx");
   showpcx256(ptr, 10);
   copypage(0,1);
   unloadfile(ptr);

// Load Pointer and place it

   ptr = loadfile(datapool, "mencur.spr");
   *pspr = defsprite(ptr, 2);
   unloadfile(ptr);
   *pobj = defobject(*pspr, MENUX, MENUY, OBJ_LOW);

// Load Menutext sprite and put it

   *mspr = loadfile(datapool, "mentxt.spr");
   putspritedirect(*mspr, MENXTXT, MENYTXT, 0);

// Load Sound, play it and display all

   *s = loadfile(datapool, "menu.snd");
   playsample(*s);

   waitfortick();
   updatescreen();
   glowin(0);

   return 1;  // first menupoint is selected
}

static void endmenu(struct sndstrc *snd, int pobj, int pspr, struct sprstrc *mspr)
{
   glowout();
   killobject(pobj);
   killsprite(pspr);
   unloadfile(mspr);
   haltsound();
   unloadfile(snd);
}

static int newkey(int y, const char *s, struct sprstrc *f1)
{
   int keys,k;

   while(pressedkeys);
   writetext(MENXTXT,y,s,f1);
   while(!pressedkeys);
   keys = 0;
   do{
      keys++;
   }while(key[keys] == 0);
   k = ownkeycodes[keys];
   if(k > 37) putspritedirect(f1, MENXTXT+40, y, k-36);
   else writetext(MENXTXT+40,y,&keytable[k][0],f1);
   return keys;
}

static void defkeys(struct sprstrc *font1)
{

// Left

   key_left = newkey(MENYTEXT-7, "LEFT", font1);

// Right

   key_right = newkey(MENYTEXT+3, "RIGHT", font1);

// Up

   key_up = newkey(MENYTEXT+13, "UP", font1);

// Down

   key_down = newkey(MENYTEXT+23, "DOWN", font1);

// Fire

   key_fire = newkey(MENYTEXT+33, "FIRE", font1);

// Pause

   key_pause = newkey(MENYTEXT+43, "PAUSE", font1);

   waitforkey();
   saveconfig();
}

static int showoptions(struct sprstrc *ofont, struct sprstrc *mspr)
{
   putspritedirect(mspr, MENXTXT, MENYTXT, 1);    // Clear mentxt

   writetext(MENXTEXT, MENYTEXT-8, "OPTIONS", ofont);
   if(speaker(-1)) writetext(MENXTEXT, MENYTEXT+3, "1 SOUND ON", ofont);
   else writetext(MENXTEXT, MENYTEXT+3, "1 SOUND OFF", ofont);
   writetext(MENXTEXT, MENYTEXT+MSTEP*MENUDELTA+3, "2 CHOOSE KEYS", ofont);
   writetext(MENXTEXT, MENYTEXT+MSTEP*MENUDELTA*2+3, "3 MAINMENU", ofont);
   return 1;
}

static int options(struct sndstrc *snd, int *pobj, int pspr, struct sprstrc *mspr)
{
   struct sprstrc *ofont;
   int    quit, selected;
   int i;

   haltsound();

   ofont = loadfile(datapool, "ofont.spr");

// Clear old menu and write new one

   selected = showoptions(ofont, mspr);
   moveobject(*pobj, MENUX, MENUY+7);

   quit = FALSE;

// The Selection starts

   do{
      while (pressedkeys) {
	 waitfortick();
	 updatescreen();
      }
      while (!pressedkeys) {
	 waitfortick();
	 updatescreen();
      }
      if(key[KEYONE] || (selected==1 && key[key_fire])){   // Sound on/off
	 if(speaker(-1)){ 	// Turn Sound off
	    writetext(MENXTEXT, MENYTEXT+3, "1 SOUND OFF", ofont);
	    speaker(0);
	    saveconfig();
	 }else{			// Turn Sound on
	    if(speaker(1)){
	       writetext(MENXTEXT, MENYTEXT+3, "1 SOUND ON ", ofont);
	       saveconfig();
	    }else{
	       writetext(MENXTEXT, MENYERR, "SOUNDBLASTER", ofont);
	       writetext(MENXTEXT, MENYERR+ofont->ys+1, " NOT FOUND", ofont);
	    }
	 }
      }
      if(key[KEYTWO] || (selected==2 && key[key_fire])){   // Redefine Keys
	 killobject(*pobj);
	 putspritedirect(mspr, MENXTXT, MENYTXT, 1); 	   // Clear mentxt
	 defkeys(ofont);
	 selected = showoptions(ofont, mspr);
	 *pobj = defobject(pspr, MENUX, MENUY+7, OBJ_LOW);
      }
      if(key[KEYTHREE] || (selected==3 && key[key_fire])){ // Quit Options
	 quit = TRUE;
      }
      if(key[key_up] && selected>1){                       // Up
	 for (i = 0; i < MENUDELTA; i++) {
	    moveobjectdelta(*pobj, 0, -MSTEP);
	    waitfortick();
	    updatescreen();
	 }
	 selected--;
      }
      if(key[key_down] && selected<3){                     // Down
	 if(selected == 1){
	    writetext(MENXTEXT, MENYERR, "            ", ofont);
	    writetext(MENXTEXT, MENYERR+ofont->ys+1, "          ", ofont);
	 }
	 for (i = 0; i < MENUDELTA; i++) {
	    moveobjectdelta(*pobj, 0, MSTEP);
	    waitfortick();
	    updatescreen();
	 }
	 selected++;
      }
   }while(!quit);

// Restore mainmenu

   playsample(snd);
   putspritedirect(mspr, MENXTXT, MENYTXT, 0);
   moveobject(*pobj, MENUX, MENUY);
   waitfortick();
   updatescreen();

   unloadfile(ofont);

   return 1;  // first menupoint is selected
}

static void story(void)
{
   void *ptr;
   void *sound;
   int d, line;

   clearscreen();
   setpage(0);
   ptr = loadfile(datapool, "longtime.pcx");
   showpcx256(ptr,0);
   unloadfile(ptr);

   sound = loadfile(datapool, "longtime.snd");
   playloop(sound);

   showpage(0);
   snd_cli();
   glowin(0);
   snd_sti();

   d = CREDITSPEED;			// Direction and speed of movement.
   line = 0; 				// Starting at line 0.
   while(!pressedkeys);
   snd_cli();
   do {
      showline(line);
      line += d;
   } while (line < 198);

   snd_sti();
   while(!pressedkeys);

   // Stop sound and free memory
   snd_cli();
   glowout();
   snd_sti();
   haltsound();
   unloadfile(sound);
}

 //----------------//
//      Menu	    //
 //----------------//

void menu(void)
{
   int    quit, selected, i;
   int    pointobj, pointspr;
   struct sprstrc *menuspr;
   struct sndstrc *snd;
   int    c;


   showtitle();

   quit = FALSE;
   selected = initmenu(&snd, &pointobj, &pointspr, &menuspr);
   c = 0;
   do{
      while (pressedkeys) {
	 c = cyclepalette(232, 254, c);
	 waitfortick();
	 updatescreen();
      }
      while (!pressedkeys) {
	 c = cyclepalette(232, 254, c);
	 waitfortick();
	 updatescreen();
      }
      if(key[KEYONE] || (selected==1 && key[key_fire])){   // Play
	 endmenu(snd, pointobj, pointspr, menuspr);
	 clearscreen();
	 setstandardpalette();
	 playthegame();
	 selected = initmenu(&snd, &pointobj, &pointspr, &menuspr);
      }
      if(key[KEYTWO] || (selected==2 && key[key_fire])){   // Show Highscore
	 endmenu(snd, pointobj, pointspr, menuspr);
	 highscore(FALSE);
	 selected = initmenu(&snd, &pointobj, &pointspr, &menuspr);
      }
      if(key[KEYTHREE] || (selected==3 && key[key_fire])){ // Show Title
	 endmenu(snd, pointobj, pointspr, menuspr);
	 showtitle();
	 selected = initmenu(&snd, &pointobj, &pointspr, &menuspr);
      }
      if(key[KEYFOUR] || (selected==4 && key[key_fire])){  // Story
	 endmenu(snd, pointobj, pointspr, menuspr);
	 story();
	 selected = initmenu(&snd, &pointobj, &pointspr, &menuspr);
      }
      if(key[KEYFIVE] || (selected==5 && key[key_fire])){  // Options
	 selected = options(snd, &pointobj, pointspr, menuspr);
      }
      if(key[KEYSIX] || (selected==6 && key[key_fire])){   // Quit
	 endmenu(snd, pointobj, pointspr, menuspr);
	 closingtime();
	 quit = TRUE;
      }
      if(key[key_up] && selected>1){                       // Up
	 for (i = 0; i < MENUDELTA; i++) {
	    moveobjectdelta(pointobj, 0, -MSTEP);
	    c = cyclepalette(232, 254, c);
	    waitfortick();
	    updatescreen();
	 }
	 selected--;
      }
      if(key[key_down] && selected<6){                     // Down
	 for (i = 0; i < MENUDELTA; i++) {
	    moveobjectdelta(pointobj, 0, MSTEP);
	    c = cyclepalette(232, 254, c);
	    waitfortick();
	    updatescreen();
	 }
	 selected++;
      }
   }while(!quit);
}




 /*-----------------------------------------------------*/
/*                                                       */
/*            T H E   L A S T   E I C H H O F            */
/*                                                       */
/*           [c] copyrigth 1993 by ALPHA-HELIX           */
/*          This module written by Dany Schoch		 */
/*                                                       */
 /*-----------------------------------------------------*/

#define MAIN_MODULE

#ifndef __COMPACT__
   #error This version only supports the COMPACT memory model.
#endif

#pragma hdrstop

#include <conio.h>
#include <dos.h>
#include <alloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

#include "xmode.h"
#include "fileman.h"
#include "sound.h"
#include "baller.h"


#define MEMORYREQUIRED		500000L	// Memory used to run.
#define CMDLEN			40	// Max length of command line.

static char   cmd[CMDLEN];		// Command line given at startup.
extern unsigned _stklen;                // Programs stacklength.


// Code.

// Error handling routines.

void error(char *text, int code, ...)
{
   void powerdown(void);	// Prototype.
   va_list   ap;		// Variable for variable argument list.

   va_start(ap, code);
   powerdown();			// Leave graphics, close files, ...
   switch (code) {
      case ENOMEM : printf("%s : NO MORE MEMORY !\n\n", text);
		    break;
      case ENOFILE: printf("%s : File '%s' not found !\n\n",
		    text, va_arg(ap, char *));
		    break;
      case E2BIG  : printf("%s : Out of index table space.\n\n", text);
		    break;
      case EINVDAT: printf("%s : Wrong level.\n\n", text);
		    break;
      case -1     :
      case EFAULT : printf("James Bond quitting style !!\n\n");
		    break;
      default     : perror(text);
   }
   va_end(ap);
   exit(1);
}


#pragma argsused
int harderror(int errval, int ax, int bp, int si)
{
    return _HARDERR_FAIL;	// Indicate failed DOS function.
}


/*------------------------------------------------------
Function: powerup & powerdown

powerup   : Initializes the system.
powerdown : Undoes the effect of powerup.
	    Please note that powerdown should not be
	    called within powerup or the sytem will
	    hang. So errors encountered in powerup
	    can't be handled by the function 'error',
	    because this would call powerdown indirectly.
------------------------------------------------------*/
void powerup(void)
{

// Check DOS version.
   if (_osmajor < 3) {
      printf("Try using DOS 3.0 or even a higher version.\n");
      exit(1);
   }

   harderr(harderror);			// Install a DOS error handler.

// Look for a VGA.
   if (!VGApresent() && !strstr(cmd, "/VGA") && !strstr(cmd, "-VGA")) {
      printf("You need a VGA to run this game.\n");
      exit(1);
   }

// Check if there is enough memory available.
   {
      long   mem;

      mem = (_SS - _psp)*16 + _stklen;
#ifdef DEBUG
      printf("Program Code+Data : %ld bytes.\n", mem - _stklen);
      printf("Stack size        : %u bytes.\n", _stklen);
      printf("DOS available     : %ld bytes.\n", coreleft());
#endif
      if (coreleft() < MEMORYREQUIRED) {
	 printf("This Game requires at least %ld bytes of DOS memory.\n", MEMORYREQUIRED + mem);
	 printf("You still need to free up %ld bytes.\n", MEMORYREQUIRED - coreleft());
	 exit(1);
      }
   }

// Initalize the memory system.
   {
      int   xms;

      xms = initfilemanager(40, 512, 8192, error);
      if (xms == -1) abort();		// Suspicious fatal error.
      printf("XMS used      : %d kB.\n", xms);
   }

// Looking for SoundBlaster.
   {
      int   io, irq, dma;
      char  *e1, *e2;			// Environment strings.

      io = irq = dma = -1;		// Set to 'Autodetect'.

// Attempt to read environment variable.
      if ((e1 = getenv("BLASTER")) != NULL) {;
// Duplicate the string so that we don't trash our environment.
	 e2 = strdup(e1);
// Now parse the BLASTER variable.
	 e1 = strtok(e2, " \t");
	 while(e1) {
	    switch(toupper(e1[0])) {
	       case 'A':		// I/O address.
		  io = (int) strtol(e1 + 1, &e1, 16);
		  break;
	       case 'I':		// Hardware IRQ.
		  irq = atoi(e1 + 1);
		  break;
	       case 'D':		// DMA channel.
		  dma = atoi(e1 + 1);
		  break;
	    }
	    e1 = strtok(NULL, " \t");	// Remove blanks.
	 }
	 free(e2);
      }
      printf("SoundBlaster  : %sfound.\n",
	     (initsound(io, irq, dma) ? "" : "not "));
   }

// Clear NUM_LOCK, SCROLL_LOCK and CAPS_LOCK.
   outportb(0x60, 0xed);		// Send 'LED' set command.
   delay(10);				// Wait 10ms.
   outportb(0x60, 0);			// Clear all LEDs.
   pokeb(0, 0x417, peekb(0, 0x417) & 0xff8f);
					// Clear BIOS key states.

// Redefine keyboard and timer interrupts.
   int09 = getvect(0x09);
   int08 = getvect(0x08);
// Der naechste befehl ist fast schon ein guter witz.
// Er kopiert die variable 'int08' in eine variable
// im CS. Dies braucht's, damit man locker in 'newint08'
// mit einem 'jmp' zur alten int-routine springen kann.
// Schoen waere es, wenn ich in C sagen koennte, wo ich
// die variablen haben will. Doch das geht leider nicht.
// Oder ich weiss nicht wie??
   copytoCS();
   setvect(0x08, newint08);
   setvect(0x09, newint09);
   setspeed(GAMESPEED);			// Set timer speed.

#ifdef DEBUG
   printf("After initialization   : %ld bytes.\n", coreleft());
#endif

   printf("\nHit any key to go on.\n");
   waitforkey();
   initxmode();				// Enter graphic mode.

}

void powerdown(void)
{
   shutxmode();
   setspeed(0);
   setvect(0x09, int09);
   setvect(0x08, int08);

   shutsound();
   shutfilemanager();
}



/*------------------------------------------------------
Function: cmdline

Description: Do first steps on the command line.
------------------------------------------------------*/
void cmdline(int argc, char *argv[])
{
   int   i;
   char  *e1;

// Concatenate all command strings together in 'cmd'.
   cmd[0] = '\0';			// Clear string.
   for (i = 1; i < argc; i++) {
      if (strlen(cmd)+strlen(argv[i]) < CMDLEN)
	 strcat(cmd, argv[i]);
   }
   strupr(cmd);

// Help?
   if (strstr(cmd, "/?") || strstr(cmd, "-?")) {	// Help?
      printf("Syntax:   BALLER [options]\n");
      printf("  /vga    Override VGA detection.\n");
      printf("  /ns     Play without sound.\n");
      printf("\n");
      printf("To force SoundBlaster on, use the BLASTER environment variable.\n");
      printf("  e.g. set BLASTER = A220 I7 D1\n");
      printf("\n\n");
      exit(0);                          // Exit nicely.
   }

// Get cheat level.

   if ((e1 = strstr(cmd, "007.")) == NULL) {
      cheatlevel = 0;			// No cheating this time.
   } else {
      cheatlevel = e1[4] - '0';
      printf("Cheat level:\n");
      if(cheatlevel & CHEATLIFES) printf("    - Unlimited lives\n");
      if(cheatlevel & CHEATMONEY) printf("    - Unlimited money\n");
      if(cheatlevel & CHEATCRASH) printf("    - Eichhof can't be destroyed\n");
      printf("\n");
   }

}

int main(int argc, char *argv[])
{

   clrscr();
   printf("\n\n\nTHE LAST EICHHOF [c] copyright 1993 by ALPHA-HELIX.\n");
   printf("Release 1.1\n\n");
   printf("This game is FREEWARE. Please copy like crazy.\n");
   printf("If you like it, just send a postcard to ALPHA-HELIX\n");
   printf("                                        Rehhalde 18\n");
   printf("                                        6332 Hagendorn\n");
   printf("                                        Switzerland\n");
   printf("\nInternet contact address: tritone@ezinfo.vmsmail.ethz.ch\n\n\n");

// Process command line.
   cmdline(argc, argv);

// Do initialization.
   powerup();

// Load options of last time.
   loadconfig();
// Check command line for 'nosound' option.
   if (strstr(cmd, "/NS") || strstr(cmd, "-NS")) speaker(0);

// Open date bases.
   datapool = opendatabase("beer.dat");

#ifdef DEBUG
   screenmode(2);
   printf("Memory available for data : %ld bytes.\n", coreleft());
   waitforkey();
   setxmode();
   setstandardpalette();
#endif

   intro();				// Show Blick intro.

   windowy1 = BARY;			// Game window y-Size.

   menu();

   closedatabase(datapool);

// Going down and return to OS.
   powerdown();
   return 0;
}

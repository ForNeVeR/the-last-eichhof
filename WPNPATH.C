// SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

#include <stdio.h>
#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <alloc.h>
#include <string.h>



#include "baller.h"


#define	MAXX			320
#define	MAXY			220


int    nshots = 0;
int    shotlen[400];
int    *shot[400];

struct shotstrc shothdr[400];


// Eichhof Lager
void build0(void)
{
   int   i, y;
   int   s;
   int   *ptr;


   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0, y = MAXY; y > 0; i+=2, y -= 14) {
      ptr[i] = 0; ptr[i+1] = -14;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 0;
   shothdr[s].shoty = -1;
   shothdr[s].power = 2;
   shothdr[s].sprite = 9;

}

void build12345(void) // Pokal.
{
   int   i, y;
   int   s;
   int   *ptr;


   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0; i < 26; i+=2) {
      ptr[i] = 0; ptr[i+1] = -6;
      shotlen[s] += 4;
   }
   ptr[i++] = SHOTRELEASE; ptr[i++] = 2; shotlen[s] += 4;
   ptr[i++] = SHOTRELEASE; ptr[i++] = 3; shotlen[s] += 4;
   ptr[i++] = SHOTRELEASE; ptr[i++] = 4; shotlen[s] += 4;
   ptr[i++] = SHOTRELEASE; ptr[i++] = 5; shotlen[s] += 4;
   ptr[i++] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 2;
   shothdr[s].shoty = -8;
   shothdr[s].power = 6;
   shothdr[s].sprite = 10;


   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0, y = MAXY; y > 0; y-=6, i+=2) {
      ptr[i] = -6; ptr[i+1] = -6;
      shotlen[s] += 4;
   }
   ptr[i++] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = -6;
   shothdr[s].shoty = -6;
   shothdr[s].power = 3;
   shothdr[s].sprite = 18;

   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0, y = MAXY; y > 0; y-=6, i+=2) {
      ptr[i] = 6; ptr[i+1] = -6;
      shotlen[s] += 4;
   }
   ptr[i++] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 6;
   shothdr[s].shoty = -6;
   shothdr[s].power = 3;
   shothdr[s].sprite = 19;

   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0, y = MAXY; y > 0; y-=6, i+=2) {
      ptr[i] = 6; ptr[i+1] = 6;
      shotlen[s] += 4;
   }
   ptr[i++] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 6;
   shothdr[s].shoty = 6;
   shothdr[s].power = 3;
   shothdr[s].sprite = 20;

   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0, y = MAXY; y > 0; y-=6, i+=2) {
      ptr[i] = -6; ptr[i+1] = 6;
      shotlen[s] += 4;
   }
   ptr[i++] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = -6;
   shothdr[s].shoty = 6;
   shothdr[s].power = 2;
   shothdr[s].sprite = 21;

}

// Side
void build67(void)
{
   int   i, y;
   int   s;
   int   *ptr;


   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   ptr[0] = SHOTRELEASE; ptr[1] = 7; shotlen[s] += 4;
   for (i = 2, y = MAXX; y > 0; i+=2, y -= 10) {
      ptr[i] = -10; ptr[i+1] = 0;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 0;
   shothdr[s].shoty = 0;
   shothdr[s].power = 2;
   shothdr[s].sprite = 17;

   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0, y = MAXX; y > 0; i+=2, y -= 10) {
      ptr[i] = 10; ptr[i+1] = 0;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 16;
   shothdr[s].shoty = 0;
   shothdr[s].power = 2;
   shothdr[s].sprite = 16;


}

//Cannon
void build8(void)
{
   int   i, y;
   int   s;
   int   *ptr;


   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0, y = MAXY; y > 0; i+=2, y -= 10) {
      ptr[i] = 0; ptr[i+1] = -10;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 4;
   shothdr[s].shoty = -12;
   shothdr[s].power = 6;
   shothdr[s].sprite = 15;

}

//Humpe (back shot)
void build9(void)
{
   int   i, y;
   int   s;
   int   *ptr;


   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0, y = MAXY; y > 0; i+=2, y -= 10) {
      ptr[i] = 0; ptr[i+1] = 10;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 0;
   shothdr[s].shoty = 8;
   shothdr[s].power = 3;
   shothdr[s].sprite = 14;

}

// V-shot
void build1011(void)
{
   int   i, y;
   int   s;
   int   *ptr;


   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   ptr[0] = SHOTRELEASE; ptr[1] = 11; shotlen[s] += 4;
   for (i = 2, y = MAXY; y > 0; i+=2, y -= 14) {
      ptr[i] = -6; ptr[i+1] = -14;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 2;
   shothdr[s].shoty = -1;
   shothdr[s].power = 2;
   shothdr[s].sprite = 12;

   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0, y = MAXY; y > 0; i+=2, y -= 14) {
      ptr[i] = 6; ptr[i+1] = -14;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 0;
   shothdr[s].shoty = 0;
   shothdr[s].power = 2;
   shothdr[s].sprite = 12;


}


//Stange
void build12(void)
{
   int   i, y;
   int   s;
   int   *ptr;


   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   for (i = 0, y = MAXY; y > 0; i+=2, y -= 10) {
      ptr[i] = 0; ptr[i+1] = -10;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 0;
   shothdr[s].shoty = -6;
   shothdr[s].power = 3;
   shothdr[s].sprite = 10;

}


//Homing
void build13(void)
{
   int   i, y;
   int   s;
   int   *ptr;


   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;
   ptr[0] = 0; ptr[1] = -12; shotlen[s] += 4;
   for (i = 2; i < 40; i+=2) {
      ptr[i] = SHOTHOMING; ptr[i+1] = SHOTHOMING;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 2;
   shothdr[s].shoty = -1;
   shothdr[s].power = 2;
   shothdr[s].speed = 12;
   shothdr[s].sprite = 11;

}


//Pony Reflector
void build1415(void)
{
   int   i, y;
   int   s;
   int   *ptr;


   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   ptr[0] = SHOTRELEASE; ptr[1] = 15; shotlen[s] += 4;
   ptr[2] = -6; ptr[3] = -14; shotlen[s] += 4;
   for (i = 4; i < 32; i+=2) {
      ptr[i] = SHOTREFLECT; ptr[i+1] = SHOTREFLECT;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 2;
   shothdr[s].shoty = -1;
   shothdr[s].power = 2;
   shothdr[s].sprite = 13;

   s = nshots++;
   ptr = shot[s] = calloc(80, 4);
   shotlen[s] = 0;

   ptr[0] = 6; ptr[1] = -14; shotlen[s] += 4;
   for (i = 2; i < 30; i+=2) {
      ptr[i] = SHOTREFLECT; ptr[i+1] = SHOTREFLECT;
      shotlen[s] += 4;
   }
   ptr[i] = SHOTEND; shotlen[s] += 2;

   shothdr[s].shotx = 0;
   shothdr[s].shoty = 0;
   shothdr[s].power = 2;
   shothdr[s].sprite = 13;

}


int savepathes(char *file)
{
   char      fname[80];
   int       filvar, i;
   long      ptr, ptr0;
   long      fptr1, fptr2;
   unsigned  size;


   strcpy(fname, file);
   filvar = open(strcat(fname, ".SHT"), O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE);
   if (filvar == -1) return -1;

   write(filvar, &nshots, 2);
   fptr1 = tell(filvar);
   ptr = nshots*4;
   write(filvar, shot, ptr);

   for (i = 0; i < nshots; i++) {
      write(filvar, &shothdr[i], sizeof(struct shotstrc));
      write(filvar, shot[i], shotlen[i]);
      fptr2 = tell(filvar);
      lseek(filvar, fptr1, SEEK_SET);
      ptr0 = (ptr & 0xf) + ((ptr & 0xffff0) << 12);
      write(filvar, &ptr0, 4);
      ptr += shotlen[i] + sizeof(struct shotstrc);
      fptr1 = tell(filvar);
      lseek(filvar, fptr2, SEEK_SET);

   }
   close(filvar);

   return 0;

}


void main(void)
{

   clrscr();
   printf("\nALPHA-HELIX Shot Path Builder Ver 0.01\n\n");

   build0();
   build12345();
   build67();
   build8();
   build9();
   build1011();
   build12();
   build13();
   build1415();

   savepathes("weapons");
   printf("Done.\n");

}

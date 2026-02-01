// SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <alloc.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <dos.h>


#define   MAXSPRITES		40
#define   MAXWEAPONS		20

#include "baller.h"

int       nfrnd;
struct armstrc frnd[MAXWEAPONS];


int save(void)
{
   char      file[80];
   int       filvar;
   int       size, i;

   printf("Output file [.WPN]: ");
   fflush(stdin);
   scanf("%s", file);
   strcat(file, ".WPN");

   filvar = open(file, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, S_IWRITE);
   if (filvar == -1) return -1;

   write(filvar, &nfrnd, 2);
   write(filvar, frnd, nfrnd*sizeof(struct armstrc));

   close(filvar);

   return 0;
}

int load(void)
{
   char      file[80];
   int       filvar;
   int       size, i;

   printf("Input file [.WPN]: ");
   fflush(stdin);
   scanf("%s", file);
   strcat(file, ".WPN");

   filvar = open(file, O_RDONLY | O_BINARY, S_IREAD);
   if (filvar == -1) { perror("load:"); return -1; }

   read(filvar, &nfrnd, 2);
   read(filvar, frnd, nfrnd*sizeof(struct armstrc));

   close(filvar);

   return 0;
}


void define(void)
{

   if (nfrnd < MAXWEAPONS) {

      printf("Define new Weapon:\n\n");

      fflush(stdin);
      printf("Enter weapon name >  "); gets(frnd[nfrnd].armname);
      fflush(stdin);
      printf("Gun sprite #      >  "); scanf("%d", &frnd[nfrnd].sprite);
      fflush(stdin);
/*      printf("Release X(-32768) >  "); scanf("%d", &frnd[nfrnd].shotx);
      fflush(stdin);
      printf("Release Y(-32768) >  "); scanf("%d", &frnd[nfrnd].shoty);
      fflush(stdin);
*/
      printf("Shot #            >  "); scanf("%d", &frnd[nfrnd].shot);
      fflush(stdin);
      printf("Enter price       >  "); scanf("%d", &frnd[nfrnd].cost);
      fflush(stdin);
      printf("Enter flags [hex] >  "); scanf("%x", &frnd[nfrnd].flags);
      fflush(stdin);
      printf("Gun reload time   >  "); scanf("%d", &frnd[nfrnd].period);
      fflush(stdin);
//      printf("Max power ups     >  "); scanf("%d", &frnd[nfrnd].maxpower);
//      fflush(stdin);

      nfrnd++;

   }

}

void change(int n)
{
   char   text[80];
   struct armstrc *ptr;

   ptr = &frnd[n];
   printf("Weapon name [%s]: ", ptr->armname);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) strcpy(ptr->armname, text);
   printf("Gun sprite [%d]: ", ptr->sprite);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%d", &ptr->sprite);
/*   printf("Bullet Release X [%d]: ", ptr->shotx);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%d", &ptr->shotx);
   printf("Bullet Release Y [%d]: ", ptr->shoty);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%d", &ptr->shoty);
*/
   printf("Shot [%d]: ", ptr->shot);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%d", &ptr->shot);
   printf("Price [%d]: ", ptr->cost);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%d", &ptr->cost);
   printf("Flags [%xh]: ", ptr->flags);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%x", &ptr->flags);
   printf("Gun reload time in frames [%d]: ", ptr->period);
   fflush(stdin);
   gets(text);
   if (strlen(text) != 0) sscanf(text, "%d", &ptr->period);
//   printf("Max power ups [%d]: ", ptr->maxpower);
//   fflush(stdin);
//   gets(text);
//   if (strlen(text) != 0) sscanf(text, "%d", &ptr->maxpower);

   printf("\n");

}



void list(void)
{
   int   i;


   printf("\n");
   for (i = 0; i < nfrnd; i++) {
      printf("%d. %s\n", i, frnd[i].armname);
   }

}


void delweapon(int n)
{
   int   i;

   nfrnd--;
   for (i = n; i < nfrnd; i++) {
      frnd[i] = frnd[i+1];
   }

}




void main(void)
{

   int   choice;

   printf("\n\nEXTRA WEAPON DESIGNER !\n");
   printf("[c] copyrigth 1992 by ALPHA-HELIX.\n");


   nfrnd = 0;
   do {

   do {
      printf("\n\n");
      printf("1 - Define new weapon.\n");
      printf("2 - Delete weapon.\n");
      printf("3 - Weapon List.\n");
      printf("4 - Change weapon.\n");
      printf("7 - Save weapons.\n");
      printf("8 - Load weapons.\n");
      printf("9 - Quit EWP.\n");

      printf("\nYour choice -> "); choice = getche() - '0';
      printf("\n");

      switch (choice) {
	 case 1: define();
		 break;

	 case 2: list();
		 printf("Enter number of weapon to delete: ");
		 fflush(stdin);
		 scanf("%d", &choice);
		 delweapon(choice);
		 break;

	 case 3: list();
		 break;

	 case 4: list();
		 printf("Enter number of weapon to change: ");
		 fflush(stdin);
		 scanf("%d", &choice);
		 change(choice);
		 break;

	 case 8: load();
		 break;

	 case 7: save();
		 break;

      }

   } while (choice != 9);

   printf("\n\nQuit without saving (Y/N)? "); choice = getche();
   } while (choice != 'y');

   printf("\nThanks for using EWP !\n\n\n\n");

}



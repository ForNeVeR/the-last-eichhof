

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


#include "sound.h"

struct filestrc {
   char  name[14];		// Name of file.
   long size;			// It's size.
   int   flags;			// Buffering flags.
   long  fptr;			// File pointer to access it's data.
};


struct sndstrc lall;
int    nentries;
void   *lib;
long   libsize;
char   *obj;
long   objsize;


int readlib(char *file)
{
   int  filvar;
   long size;
   char huge *ptr;

   filvar = open(file, O_RDONLY | O_BINARY, S_IREAD);
   if (filvar == -1) {
      nentries = 0; libsize = 0;
      return -1;
   }

   read(filvar, &nentries, 2);

   size = libsize = filelength(filvar) - 2;
   ptr = lib = farmalloc(libsize);

   while(size > 65000) {
      read(filvar, ptr, 65000);
      ptr += 65000; size -= 65000;
   }
   read(filvar, ptr, size);

   close(filvar);

   return 0;
}


int readobj(char *file)
{
   int   filvar;
   long  size;
   char  huge *ptr;

   filvar = open(file, O_RDONLY | O_BINARY, S_IREAD);
   if (filvar == -1) {
      perror("readobj : ");
      return -1;
   }

   size = objsize = (filelength(filvar) - 0x21) & 0xfffffffel;
   ptr = obj = farmalloc(objsize);

   lseek(filvar, 0x20, SEEK_SET);
   while(size > 65000) {
      read(filvar, ptr, 65000);
      ptr += 65000; size -= 65000;
   }
   read(filvar, ptr, size);

   close(filvar);

   return 0;
}


int writelib(char *file)
{
   int    filvar;
   int    i;
   long   ptr;
   struct filestrc *waff1;
   long   *waff2;
   long   size;
   char   huge *hptr;


   filvar = open(file, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IWRITE);
   if (filvar == -1) {
      perror("\nwritelib : ");
      exit(1);
      return -1;
   }

   i = nentries + 1;
   write(filvar, &i, 2);

   waff2 = (long *)lib;
   for (i = 0; i < nentries; i++) {
      waff2[i] += sizeof(long);
      write(filvar, waff2+i, sizeof(long));
   }
   ptr = libsize + sizeof(long);
   ptr = (ptr & 0xf) + ((ptr & 0xffff0) << 12);
   write(filvar, &ptr, sizeof(long));
   size = libsize - nentries*sizeof(long);
   hptr = (char *)&waff2[nentries];
   while (size > 65000) {
      write(filvar, hptr, 65000);
      hptr += 65000; size -= 65000;
   }
   write(filvar, hptr, size);

   write(filvar, &lall, sizeof(struct sndstrc));
   hptr = obj; size = objsize;
   while (size > 65000) {
      write(filvar, hptr, 65000);
      hptr += 65000; size -= 65000;
   }
   write(filvar, hptr, size);

   close(filvar);

   return 0;
}




void main(void)
{

   char   libfile[80];
   char   objfile[80];
   char   text[80];
   char   c;


   printf("\n\nSound Librarian [c] 1993 by Alpha-Helix.\n\n");
   printf("Library [.SND]: ");
   fflush(stdin);
   scanf("%s", libfile);
   strcat(libfile, ".SND");
   do {
      printf("Sound [.VOC] : ");
      fflush(stdin);
      scanf("%s", objfile);
      strcat(objfile, ".VOC");
      if (objfile[0] != '.') {
	 readlib(libfile);
	 if (readobj(objfile) != -1 ) {
	    lall.len = objsize;
	    printf("Sampling Rate [9]: ");
	    fflush(stdin);
	    gets(text);
	    if (strlen(text) == 0) lall.samplerate = 9;
	    else lall.samplerate = atoi(text);
	    printf("4 bit packed [n]: ");
	    fflush(stdin);
	    gets(text);
	    lall.flags = 0;
	    if (strlen(text) != 0) {
	       if (text[0] == 'y') lall.flags = SND_PACKED4;
	    }
	    printf("Priority [5]: ");
	    fflush(stdin);
	    gets(text);
	    if (strlen(text) == 0) lall.priority = 5;
	    else lall.priority = atoi(text);
	    writelib(libfile);
	    farfree(obj);
	 }
	 farfree(lib);
      }
   } while (objfile[0] != '.');

   printf("\nDone.\n\n");
}



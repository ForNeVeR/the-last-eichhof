
#ifndef __COMPACT__ || __LARGE__
   #error NEED FAR DATA POINTERS
#endif

#include <stdio.h>
#include <sys\stat.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <alloc.h>
#include <dir.h>
#include <stdlib.h>


#include "fileman.h"

#define HDRTEXTSIZE		30
#define MAXFILES		200


struct filestrc {
   char  name[14];		// Name of file.
   long  size;			// It's size.
   int   flags;			// Buffering flags.
   long  fptr;			// File pointer to access it's data.
};


const char hdrtext[HDRTEXTSIZE] = "ALPHA-HELIX COMBINER VER 3.3\x1a";

char   libfile[80];
int    libfilvar;
long   libsize;
int    nfiles;
int    mode;
struct filestrc file[MAXFILES];

long   objsize;

int openlib(char *txt)
{
   long size;
   char huge *ptr;

   libfilvar = open(txt, O_RDONLY | O_BINARY, S_IREAD);
   if (libfilvar == -1) {
      nfiles = 0; libsize = 0; mode = 0;
   } else {
      lseek(libfilvar, HDRTEXTSIZE, SEEK_SET);
      read(libfilvar, &mode, 2);
      read(libfilvar, &nfiles, 2);
      libsize = filelength(libfilvar)-HDRTEXTSIZE-4-nfiles*sizeof(struct filestrc);
      read(libfilvar, file, nfiles*sizeof(struct filestrc));
   }

   return 0;

}



int addfile(char *txt, int flags)
{
   int   filvar;
   int   obj;
   char  name[MAXFILE+MAXEXT];
   char  ext[MAXEXT];
   long  size;
   char  huge *ptr;
   int   i;


   if (openlib(libfile) == -1) return -1;

   obj = open(txt, O_RDONLY | O_BINARY, S_IREAD);
   if (obj == -1) {
      close(libfilvar);
      perror("openobj : ");
      return -1;
   }
   objsize = filelength(obj);

   filvar = open("COMB.TMP", O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IWRITE);
   if (filvar == -1) {
      close(libfilvar);
      perror("addfile");
      return -1;
   }
   _splitpath(txt, NULL, NULL, name, ext);
   strcat(name, ext);
   strcpy(file[nfiles].name, strupr(name));
   file[nfiles].size = objsize;
   file[nfiles].flags = flags;

// Write Header.
   write(filvar, hdrtext, HDRTEXTSIZE);
   write(filvar, &mode, 2);
   nfiles++;
   write(filvar, &nfiles, 2);

// Write directory.
   for (i = 0; i < nfiles-1; i++) {
      file[i].fptr += sizeof(struct filestrc);
   }
   file[nfiles-1].fptr = libsize+HDRTEXTSIZE+4+nfiles*sizeof(struct filestrc);
   write(filvar, file, nfiles*sizeof(struct filestrc));

// Write old data.
   if ((ptr = farmalloc(65000l)) == NULL) {
      close(libfilvar);
      return -1;
   }
   size = libsize;
   while(size > 65000) {
      read(libfilvar, ptr, 65000);
      write(filvar, ptr, 65000);
      size -= 65000;
   }
   read(libfilvar, ptr, size);
   write(filvar, ptr, size);

// Write new object.
   size = objsize;
   while(size > 65000) {
      read(obj, ptr, 65000);
      write(filvar, ptr, 65000);
      size -= 65000;
   }
   read(obj, ptr, size);
   write(filvar, ptr, size);
   farfree(ptr);

   close(filvar);
   close(obj);
   close(libfilvar);
   remove(libfile);
   rename("COMB.TMP", libfile);

   return 0;
}


int deletefile(char *txt)
{
   int   i, j;
   int   filvar;
   long  size, size1, size2;
   long  fptr1, fptr2;
   void  *ptr;


   if (openlib(libfile) == -1) return -1;
   strupr(txt);
   for (i = 0; i < nfiles; i++) {
      if (strcmp(txt, file[i].name) == 0) {
	 filvar = open("COMB.TMP", O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IWRITE);
	 if (filvar == -1) {
	    close(libfilvar);
	    perror("addfile");
	    return -1;
	 }

// Write Header.
	 write(filvar, hdrtext, HDRTEXTSIZE);
	 write(filvar, &mode, 2);
	 nfiles--;
	 write(filvar, &nfiles, 2);
// Write directory.
	 size = file[i].size;
	 fptr1 = file[0].fptr;
	 fptr2 = file[i+1].fptr;
	 size1 = size2 = 0;
	 for (j = 0; j < nfiles; j++) {
	    if (j < i) {
	       size1 += file[j].size;
	       file[j].fptr -= sizeof(struct filestrc);
	    } else {
	       file[j] = file[j+1];
	       file[j].fptr -= (sizeof(struct filestrc) + size);
	       size2 += file[j].size;
	    }
	 }
	 if (size+size1+size2 != libsize) abort();
	 write(filvar, file, nfiles*sizeof(struct filestrc));

// Write data.
	 if ((ptr = farmalloc(65000l)) == NULL) {
	    close(libfilvar);
	    return -1;
	 }
	 lseek(libfilvar, fptr1, SEEK_SET);
	 while(size1 > 65000) {
	    read(libfilvar, ptr, 65000);
	    write(filvar, ptr, 65000);
	    size1 -= 65000;
	 }
	 read(libfilvar, ptr, size1);
	 write(filvar, ptr, size1);

	 lseek(libfilvar, fptr2, SEEK_SET);
	 while(size2 > 65000) {
	    read(libfilvar, ptr, 65000);
	    write(filvar, ptr, 65000);
	    size2 -= 65000;
	 }
	 read(libfilvar, ptr, size2);
	 write(filvar, ptr, size2);

	 farfree(ptr);

	 close(filvar);
	 remove(libfile);
	 rename("COMB.TMP", libfile);
	 break;

      }
   }
   close(libfilvar);

   return 0;

}

int extractfile(char *txt)
{
   int   i, j;
   int   filvar;
   long  size, size1, size2;
   long  fptr1;
   void  *ptr;


   if (openlib(libfile) == -1) return -1;
   strupr(txt);
   for (i = 0; i < nfiles; i++) {
      if (strcmp(txt, file[i].name) == 0) {
	 filvar = open(txt, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IWRITE);
	 if (filvar == -1) {
	    close(libfilvar);
	    perror("extractfile");
	    return -1;
	 }

	 size = file[i].size;
	 fptr1 = file[i].fptr;

// Write data.
	 if ((ptr = farmalloc(65000l)) == NULL) {
	    close(libfilvar);
	    return -1;
	 }
	 lseek(libfilvar, fptr1, SEEK_SET);
	 while(size > 65000) {
	    read(libfilvar, ptr, 65000);
	    write(filvar, ptr, 65000);
	    size -= 65000;
	 }
	 read(libfilvar, ptr, size);
	 write(filvar, ptr, size);

	 farfree(ptr);

	 close(filvar);
      }
   }
   close(libfilvar);

   return 0;

}


int showdir(int mode)
{
   int   i;
   char  c[20];


   if (openlib(libfile) == -1) return -1;
   printf("\n");
   if (mode == 0) {
      for (i = 0; i < nfiles; i++) {
	 printf("%s  ", file[i].name);
      }
   } else {
      for (i = 0; i < nfiles; i++) {
	 if (file[i].flags & M_NOFREEUP) {
	    strcpy(c, "No FreeUp");
	 } else {
	    strcpy(c, (file[i].flags & M_XMS) ? "XMS" : "NONE");
	 }

	 printf("%s\t\t%s\t%lu\n", file[i].name, c, file[i].size);
      }
   }
   printf("\n\n");
   close(libfilvar);

   return 0;
}


int main(void)
{
   char   text[80], t[20];
   int    flags;


   printf("\nCOMBINER ver 3.3 [c] 1992, 93 by Alpha-Helix.\n\n");
   printf("Command Usage:  +file  : Adds or replaces file.\n");
   printf("                -file  : Deletes file from library.\n");
   printf("                xfile  : Extract file.\n");
   printf("                d      : Directory of library.\n");
   printf("                d+     : Detailed directory.\n");
   printf("                .      : Finish current session.\n");
   printf("\n");
   printf("Library [.DAT]: ");
   fflush(stdin);
   scanf("%s", libfile);
   strcat(libfile, ".DAT");

   do {
      printf("Command: ");
      fflush(stdin);
      scanf("%s", text);
      switch (text[0]) {

	 case '+': // Add file.
	    flags = M_XMS;
	    printf("Caching: Yes, No, Only if free: [Y]: ");
	    fflush(stdin);
	    gets(t);
	    if (strlen(t) != 0)
	       switch(t[0]) {
		  case 'n':
		  case 'N': flags = M_NONE;
			    break;
		  case 'o':
		  case 'O': flags = M_XMS | M_NOFREEUP;
			    break;
	       }
	    deletefile(&text[1]);
	    addfile(&text[1], flags);
	    break;

	 case '-': // Delete file.
	    deletefile(&text[1]);
	    break;

	 case 'x': // Extract file.
	 case 'X':
	    extractfile(&text[1]);
	    break;

	 case 'd':
	    if (text[1] == '+')
	       showdir(1);
	    else
	       showdir(0);
	    break;

      }

   } while (text[0] != '.');


   return 0;
}

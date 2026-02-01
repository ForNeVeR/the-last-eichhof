// SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

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

struct sndstrc lall;


int do_the_nasty_job(char *infile, char *outfile)
{
   int   in, out;
   long  size, objsize;
   char  *ptr;
   int   y;
   char  file[80];
   int   voc;
   int   ref, tmp1, tmp2;
   unsigned i;


   strcpy(file, infile); strcat(file, ".snd");
   in = open(file, O_RDONLY | O_BINARY, S_IREAD);
   out = open(outfile, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IWRITE);
   if (out == -1) {
      perror("writefile : ");
      return -1;
   }

   ptr = malloc(60000);

   read(in, ptr, sizeof(struct sndstrc));
   write(out, ptr, sizeof(struct sndstrc));
   size = filelength(in) - sizeof(struct sndstrc);

   while(size > 60000) {
      read(in, ptr, 60000);
      for (i = 0; i < 60000; i++)
	 ptr[i] = ptr[i] & 0xfe;
      write(out, ptr, 60000);
      size -= 60000;
   }
   read(in, ptr, size);
   for (i = 0; i < size; i++)
      ptr[i] = ptr[i] & 0xfe;
   write(out, ptr, size);

   free(ptr);

   close(out); close(in);

   return 0;
}



void main(void)
{
   char   infile[80], outfile[80];
   char   text[80];
   int    filvar;


   printf("\n\nBit 0 clearer [c] 1993 Alpha-Helix.\n\n");
   printf("Input File [.SND]: ");
   fflush(stdin);
   scanf("%s", infile);
   printf("Sound File [.SND]: ");
   fflush(stdin);
   scanf("%s", outfile);
   strcat(outfile, ".SND");
   do_the_nasty_job(infile, outfile);

}



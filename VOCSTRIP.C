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


   strcpy(file, infile); strcat(file, ".RAW");
   in = open(file, O_RDONLY | O_BINARY, S_IREAD);
   voc = 0;
   if (in == -1) {
      strcpy(file, infile); strcat(file, ".VOC");
      in = open(file, O_RDONLY | O_BINARY, S_IREAD);
      voc = 1;
      if (in == -1) {
	 perror("readobj : ");
	 return -1;
      }
   }

   out = open(outfile, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IWRITE);
   if (out == -1) {
      perror("writefile : ");
      return -1;
   }

   ptr = malloc(60000);

   if (voc) {
      size = objsize = (filelength(in) - 0x21) & 0xfffffffel;
      lseek(in, 0x20, SEEK_SET);
   } else {
      size = objsize = filelength(in);
   }
   lall.len = size;

   write(out, &lall, sizeof(struct sndstrc));

   ref = 0;
   read(in, &ref, 1);
   write(out, &ref, 1);

   y = wherey();
   while(size > 60000) {
      gotoxy(1, y); printf("%d %% completed.", 100*(objsize-size)/objsize);
      read(in, ptr, 60000);
//      if (lall.flags & SND_PACKED4) {
//	 for (i = 0; i < 30000; i+=2) {
//	    tmp1 = ptr[i]-ref;
//	    if (tmp1 > 7) tmp1 = 7; else if (tmp1 < -8) tmp1 = -8;
//	    ref += tmp1;
//	    tmp2 = ptr[i+1]-ref;
//	    if (tmp2 > 7) tmp2 = 7; else if (tmp2 < -8) tmp2 = -8;
//	    ref += tmp2;
//	    ptr[i/2] = (char)(tmp1 & 0xf) | (tmp2 << 4);
//	 }
//	 write(out, ptr, 30000);
//     } else {
	 write(out, ptr, 60000);
//      }
      size -= 60000;
   }
   read(in, ptr, size);
   write(out, ptr, size);
   gotoxy(1, y); printf("100 %% completed.\n");

   free(ptr);

   close(out); close(in);

   return 0;
}



void main(void)
{
   char   infile[80], outfile[80];
   char   text[80];
   int    filvar;


   printf("\n\nVoc Stripper Ver 2.0 [c] 1993 Alpha-Helix.\n\n");
   printf("Input File [.RAW || .VOC]: ");
   fflush(stdin);
   scanf("%s", infile);
   printf("Sound File [.SND]: ");
   fflush(stdin);
   scanf("%s", outfile);
   strcat(outfile, ".SND");
   printf("Sampling Rate [15]: ");
   fflush(stdin);
   gets(text);
   if (strlen(text) == 0) lall.samplerate = 15;
   else lall.samplerate = atoi(text);
   printf("Packed to 4 bit [y]: ");
   fflush(stdin);
   gets(text);
   lall.flags = SND_PACKED4;
   if (strlen(text) != 0) {
      if (text[0] == 'n') lall.flags = 0;
   }
   printf("Priority [5]: ");
   fflush(stdin);
   gets(text);
   if (strlen(text) == 0) lall.priority = 5;
   else lall.priority = atoi(text);
   do_the_nasty_job(infile, outfile);

}



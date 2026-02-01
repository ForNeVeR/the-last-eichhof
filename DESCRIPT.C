
#include <stdio.h>
#include <alloc.h>
#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <string.h>
#include <process.h>




void main(void)
{
   char   file[80];
   int    filvar;
   int    level;
   char   text[40];
   int    bigboss;
   int    score;
   int	  money;
   int    flags;

   printf("\n\nBaller Game Description Creator [c] by ALPHA-HELIX.\n\n");
   printf("Descript File [.DSC]: ");
   fflush(stdin);
   scanf("%s", file);
   strcat(file, ".DSC");

   filvar = open(file, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IWRITE);

   printf("\nLevel #     : ");
   fflush(stdin);
   scanf("%u", &level);
   write(filvar, &level, 2);
   printf("Text[40]    : ");
   fflush(stdin);
   gets(text);
   write(filvar, text, 40);
   printf("# of endlevel Big Bosses : ");
   fflush(stdin);
   scanf("%u", &bigboss);
   write(filvar, &bigboss, 2);
   printf("Bonus score : ");
   fflush(stdin);
   scanf("%u", &score);
   write(filvar, &score, 2);
   printf("Bonus money : ");
   fflush(stdin);
   scanf("%u", &money);
   write(filvar, &money, 2);
   printf("Flags [hex] : ");
   fflush(stdin);
   scanf("%x", &flags);
   write(filvar, &flags, 2);

   close(filvar);


}



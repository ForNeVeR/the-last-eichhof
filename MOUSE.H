/*-------------------------------------------------------*/
/*                    	                                 */
/*                    M O U S E . H                      */
/*                                                       */




#define     M_MAXCURSORS         7
#define     M_CURSORSIZE         (2 * 32)

#define     M_CARROW             1
#define     M_CSMILE             2
#define     M_CALPHA             3
#define     M_CCROSS             4
#define     M_CWAIT              5
#define     M_CUSER1             6
#define     M_CUSER2             7

#define     M_CEXTERN           -1


#define     M_RELEASE            0
#define     M_HOLD               1


extern   int   m_left;
extern   int   m_right;

#include "lowmouse.h"



void setclickmode(int mode);
int getclickmode(void);
void defexterncursor(void (*extcurs)(int x, int y));
int initmouse(void);
void clearmouse(void);
void hidecursor(void);
void showcursor(void);
void selectcursor(int cursor);
void definecursor(int n, int x, int y, char *shape);
void setborder(int x1, int y1, int x2, int y2);
void setpos(int x, int y);
void getpos(int *b, int *x, int *y);
void mouseclick(int *button, int *x, int *y);
void menuclick(int *button, int *x, int *y);
void waitforrelease(int *x, int *y);






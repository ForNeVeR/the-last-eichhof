/*-------------------------------------------------------*/
/*                    	                                 */
/*                    M O U S E . C                      */
/*                                                       */



#include "globdefs.h"

#include <stdlib.h>
#include <mem.h>

#include "mouse.h"
//#include "graph.h"



int   m_left, m_right;


struct MOUSE {

   int    x0, y0;
   int    cursor;
   int    selectedcursor;
   int    mode;
   int    state;
   void   (*extcurs)(int x, int y);

};

struct CURSOR {

   int       x, y;
   unsigned  shape[M_CURSORSIZE / sizeof(int)];

} ;


static struct MOUSE  mouse;
static struct CURSOR cursor[M_MAXCURSORS] =
           {
              0, 0,     /* Arrow */
              1,-1, 0,-1,-1,-1,-1,-1,
              1,-1,-1,-1,-1,-1,-1,-1,
              1,-1,-1,-1, 0,-1,-1,-1,
              1,-1,-1,-1,-1,-1,-1,-1,
              0, 0,     /* Smile */
              0x781f, 0xe007, 0xc003, 0x8421, 0x8811, 0x0c30, 0x0c30, 0x0000,
              0x0000, 0x0000, 0x0c30, 0x87e1, 0x8181, 0xc003, 0xe007, 0xf81f,
              0x87e0, 0x1ff8, 0x3ffc, 0x7bde, 0x77ee, 0xf3cf, 0xf3cf, 0xffff,
              0xffff, 0xffff, 0xf3cf, 0x781e, 0x7e7e, 0x3ffc, 0x1ff8, 0x07e0
           };



void setclickmode(int mode)
{

   mouse.mode = mode;
   mouse.state = 0;

}


int getclickmode(void)
{

   return mouse.mode;

}


void defexterncursor(void (*extcurs)(int x, int y))
{

   mouse.extcurs = extcurs;

}




static void far handler(void)
{
   static int    x, y, b;


   if (mouse.cursor == M_CEXTERN) {

      m_getpos(&b, &x, &y);
      (*mouse.extcurs)(x, y);
      (*mouse.extcurs)(mouse.x0, mouse.y0);
      mouse.x0 = x;  mouse.y0 = y;

   }

}






void hidecursor(void)
{

   if (mouse.cursor) {
      if (mouse.selectedcursor == M_CEXTERN)
         (*mouse.extcurs)(mouse.x0, mouse.y0);
      else
         m_hidecursor();

      mouse.cursor = 0;
   }

}


void showcursor(void)
{
   int   button;


   if (mouse.cursor)
      hidecursor();

   if (mouse.selectedcursor == M_CEXTERN && mouse.extcurs != NULL) {

      m_getpos(&button, &mouse.x0, &mouse.y0);
      (*mouse.extcurs)(mouse.x0, mouse.y0);

   } else {

      m_showcursor();

   }

   mouse.cursor = mouse.selectedcursor;

}


void selectcursor(int n)
{

   int  showed;


   if (n <= M_MAXCURSORS) {
      showed = mouse.cursor;
      hidecursor();
      mouse.selectedcursor = n;
      if (n != M_CEXTERN) {
         n--;
         m_definecursor(cursor[n].x, cursor[n].y,
                        cursor[n].shape);
      }
      if (showed)
         showcursor();
   }

}



void definecursor(int n, int x, int y, byte *shape)
{

   if (--n < M_MAXCURSORS) {

      cursor[n].x = x;  cursor[n].y = y;
      memcpy(cursor[n].shape, shape, M_CURSORSIZE);

      if (mouse.cursor == ++n)
         selectcursor(n);

   }

}




void setborder(int x1, int y1, int x2, int y2)
{

   m_setxborder(x1, x2);
   m_setyborder(y1, y2);

}



void setpos(int x, int y)
{

   m_setpos(x, y);
   if (mouse.cursor == M_CEXTERN) {
      (*mouse.extcurs)(x, y);
      (*mouse.extcurs)(mouse.x0, mouse.y0);
      mouse.x0 = x;  mouse.y0 = y;
   }

}



void getpos(int *b, int *x, int *y)
{

   m_getpos(b, x, y);

   if (mouse.mode == M_HOLD) {
      if (*b != mouse.state) {
	 if (*b == 0) {
	    *b = mouse.state;
	    mouse.state = 0;
	 } else {
	    mouse.state = *b;
	 }
      } else {
         *b = 0;
      }
   } else {
      if (*b == 0) {
	 *b = mouse.state;
	 mouse.state = 0;
      } else {
         mouse.state = *b;
         *b = 0;
      }
   }

}


void mouseclick(int *button, int *x, int *y)
{

   do {
      getpos(button, x, y);
   } while (*button != m_left && *button != m_right);

}


void menuclick(int *button, int *x, int *y)
{

   int   tmp;

   tmp = getclickmode();
   setclickmode(M_RELEASE);

   do {
      getpos(button, x, y);
   } while (*button != m_left && *button != m_right);

   setclickmode(tmp);

}


void waitforrelease(int *x, int *y)
{

   int   button;

   do {
      m_getpos(&button, x, y);
   } while (button != 0);

   mouse.state = 0;

}


int initmouse(void)
{
   int   state, buttons;


   m_deviceinit(&state, &buttons);
   if (state) {
      m_left = 1;  m_right = 2;

      mouse.cursor = 0;
      mouse.mode = M_RELEASE;
      mouse.state = 0;
      mouse.extcurs = NULL;
      m_seteventhdlr(handler, M_CHANGEPOS | M_PRESSLEFT | M_PRESSRIGHT);
      selectcursor(M_CSMILE);

      return TRUE;

   } else {

      return FALSE;

   }

}

void clearmouse(void)
{
   int   state, buttons;

   m_deviceinit(&state, &buttons);
   if (mouse.cursor == M_CEXTERN)
      (*mouse.extcurs)(mouse.x0, mouse.y0);
   mouse.cursor = 0;
   mouse.mode = 0;  mouse.state = 0;

}


/*-------------------------------------------------------*/
/*                    	                                 */
/*                  L O W M O U S E . H                  */
/*                                                       */




#define   M_CHANGEPOS              0x01
#define   M_PRESSLEFT              0x02
#define   M_RELEASELEFT            0x04
#define   M_PRESSRIGHT             0x08
#define   M_RELEASERIGHT           0x10




void m_deviceinit(int *state, int *buttons);
void m_showcursor(void);
void m_hidecursor(void);
void m_getpos(int *b, int *x, int *y);
void m_setpos(int x, int y);
void m_setxborder(int x1, int x2);
void m_setyborder(int y1, int y2);
void m_seteventhdlr(void (far *hdlr)(void), int maskbits);
void m_definecursor(int x, int y, void far *shape);





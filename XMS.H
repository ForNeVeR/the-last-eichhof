// SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

// Header for xms.asm

int initXMS(void);
int shutXMS(void);
int getfreeXMS(void);
int allocXMS(unsigned size);
int freeXMS(int handle);
int movememtoXMS(void far *src, int dst, unsigned long size);
int moveXMStomem(int src, void far *dst, unsigned long size);
int moveXMS(int src, long saddr, int dst, long daddr, unsigned long size);
int lockXMS(int handle);
int unlockXMS(int handle);
int getXMSsize(int handle);


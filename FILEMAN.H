// SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE

#define M_NONE		0x00		// No buffering at all.
#define M_XMS		0x01		// Buffer file in XMS if possible.
#define M_NOFREEUP 	0x02            // Don't throw away any file in
					//   buffer if there's no room
					//   to buffer it.

int initfilemanager(int handles, int minsize, int maxsize,
		    void (*err)(char *text, int code, ...));
void shutfilemanager(void);
void *opendatabase(char *file);
void closedatabase(void *database);
void far *loadfile(void *database, char *file);
void far *loadfiledirect(char *file, int flags);
void unloadfile(void far *ptr);
int openfile(void *database, char *file);
int openfiledirect(char *file);
void closefile(int filvar);

// Available only if DEBUG is defined.
void printbuffer(void);


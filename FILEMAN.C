
/*-------------------------------------------------------*/
/*                                                       */
/*               F I L E M A N A G E R                   */
/*           [c] copyright 1993 by Alpha-Helix           */
/*               written by Dany Schoch                  */
/*                                                       */
/*   Revision List:                                      */
/*	27. May 93: Memory peak meter added.	 	 */
/*      28.       : >64kB file load error corrected.     */
/*	30. June  : 'shutfilemanager' crash corrected.   */
/*	22. Sept  : 'openfile' added.			 */
/*	30. Nov	  : 'errno.h' error compatibility added. */
/*      19. Dez   : 'printbuffer' added.                 */



#include <alloc.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <dir.h>
#include <errno.h>

#include "xms.h"
#include "fileman.h"


#define TRUE		1
#define FALSE		0

typedef unsigned long ulong;
typedef unsigned int uint;

#ifdef DEBUG
ulong   mindosmem = 0xffffffff;
void far *mem_farmalloc(ulong size)
{
   void far *ptr;

   ptr = farmalloc(size);
   if (mindosmem > farcoreleft()) mindosmem = farcoreleft();

   return ptr;
}
#define farmalloc(x)		mem_farmalloc(x)
#endif


#define HDRSIZE			30

struct filestrc {
   char  name[14];		// Name of file.
   ulong size;			// It's size.
   int   flags;			// Buffering flags.
   long  fptr;			// File pointer to access it's data.
};

struct dbasestrc {
   char   basename[MAXPATH];	// Path and name of the open data base.
   int    filvar;		// File handle of data base.
   int    nfiles;		// # of files in data base.
   struct filestrc file[];
};


struct entrystrc {
   char   name[14];		// Name of the buffered file.
   ulong  size;
   ulong  addr;			// 32 bit offset into XMS or far pointer.
   int    hits;			// Cache hit counter.
   int    flags;
};

static int    XMShandle;	// Handle of allocated extended memory.
static ulong  xmspointer;	// Pointer to free XMS.
static ulong  freexms;		// Free xtended mem.
static int    nentries;		// # of table entries.
static struct entrystrc *entry; // Entry table.
static void   (*error)(char *text, int code, ...);


#pragma argsused
static void errf(char *text, int code, ...)
{
   return;
}

/*------------------------------------------------------
Function: initfilemanager

Description: Initializes the Alpha-Helix file manager.
    handles: max # of files that can be buffered.
    minsize:\ extended memory that will be allocated
    maxsize:/ (in kB)
    err    : Pointer to alternative error handler.
Returns: kBytes allocated on success.
	      0 not enough free XMS memory.
	     -1 not enough main memory.
------------------------------------------------------*/
int initfilemanager(int handles,
		    int minsize, int maxsize,
		    void (*err)(char *text, int code, ...))
{

// Install custom error handler.
   if (err == NULL) error = errf; else error = err;

   entry = NULL;
   XMShandle = -1;

// Check if XMS driver is installed.
   if (!initXMS()) return 0;	// Just no XMS available.

// Enough free memory available ?
   freexms = getfreeXMS();
   if (freexms < minsize) return 0;

   if (freexms > maxsize)
      XMShandle = allocXMS(freexms = maxsize);
   else
      XMShandle = allocXMS(freexms);

   if (XMShandle == -1) return 0;
   freexms *= 1024;		// We want it in bytes not Kbs.
   xmspointer = 0;		// First the whole XMS id free.

// Allocate memory for the buffer table.
   nentries = handles;
   entry = (struct entrystrc *)calloc(nentries, sizeof(struct entrystrc));
   if (entry == NULL) {
      freeXMS(XMShandle);
      return -1;                // Fatal error.
   }

   return (freexms / 1024);	// Successful

}


/*------------------------------------------------------
Function: shutfilemanager

Description: Leaves the filemanager and frees the allocated
	     memory.
	     NOTE: memory allocated by 'opendatabase'
		   won't be freed.
------------------------------------------------------*/
void shutfilemanager(void)
{
   if (entry != NULL) free(entry);
   if (XMShandle != -1) freeXMS(XMShandle);
   shutXMS();
#ifdef DEBUG
   printf("\nDOS free memory: %ld bytes.\n", mindosmem);
#endif
}

#ifdef DEBUG
void printbuffer(void)
{
   int   i;

   printf("Name           Size      Addr     Hits\n");
   printf("--------------------------------------\n");
   for (i = 0; i < nentries; i++) {
      if (entry[i].size != 0)
	 printf("%s       %ld        %lx      %d\n",
		entry[i].name, entry[i].size, entry[i].addr, entry[i].hits);
   }

}
#endif

// -- Start of private area.

static struct entrystrc *findfreeentry(void)
{
   int   i;

   for (i = 0; i < nentries; i++) {
      if (entry[i].size == 0) break;
   }
   if (entry[i].size == 0) return &entry[i]; else return NULL;

}


static struct entrystrc *findentry(char *file)
{
   int    i;
   struct entrystrc *result;

   result = NULL;
   if (entry != NULL) {
      for (i = 0; i < nentries; i++) {
	 if (strcmpi(file, entry[i].name) == 0) {
	    result = &entry[i];
	    break;
	 }
      }
   }
   return result;

}


static void freeentry(int i)
{
   ulong  size;
   struct entrystrc *result;

// Move memory blocks.
   result = &entry[i];
   size = result->size;
   moveXMS(XMShandle, result->addr+size,
	   XMShandle, result->addr, xmspointer-result->addr-result->size);
   freexms += size;
   xmspointer -= size;			// Adjust free xms pointer.

// Move index table.
   if (i < nentries-1) {
      memmove(result, result+1, (nentries-i-1)*sizeof(struct entrystrc));
   }
   for ( ; i < nentries-1; i++) entry[i].addr -= size;
   entry[nentries-1].size = 0;		// Mark free slot.

}

static int lightxms(void)
{
   int    count;
   int    i;
   struct entrystrc *result;

   count = 0;
   do {

// Look for a block to remove.
      result = NULL;
      for (i = 0; i < nentries; i++) {
	 if ((entry[i].hits == count) && (entry[i].size != 0)) {
	    result = &entry[i];
	    break;
	 }
      }
      if (result) {
	 freeentry(i);
// Decrement hitcounts.
	 for (i = 0; i < nentries; i++)
	    if (entry[i].hits > 0) entry[i].hits--;

	 return TRUE;
      }
   } while (count++ < 20);

   return FALSE;
}


static void buffer(char *file, void far *data, ulong size, int flags)
{
   struct entrystrc *result;


   strupr(file);
   if ((flags & M_XMS) && (XMShandle != -1)) {
   // Buffer in extended memory.
      if (!(flags & M_NOFREEUP)) {
	 while (freexms < size)
	    if (!lightxms()) break;	// Try to make space.
      }
      result = findfreeentry();
      if ((result != NULL) && (freexms >= size)) {
	 strcpy(result->name, file);
	 result->size = size;
	 result->addr = xmspointer;
	 result->hits = 0;
	 result->flags = flags;
	 moveXMS(0, (ulong)data, XMShandle, result->addr, size);
	 freexms -= size;
	 xmspointer += size;		// Move pointer to remaining free XMS.
      }
   }

}

// --- End of private area.




/*------------------------------------------------------
Function: opendatabase

Description: Opens a special type of file: The Alpha-Helix
	     James Bond Baller game data library file
	     type.
------------------------------------------------------*/
void *opendatabase(char *file)
{
   int    filvar;
   int    nfiles;
   uint   nread;
   struct dbasestrc *ptr;
   char   hdr[HDRSIZE];
   int    mode;

   if (_dos_open(file, O_RDONLY, &filvar)) {
      (*error)("opendatabase", ENOFILE, file);
      return NULL;
   }

   _dos_read(filvar, hdr, HDRSIZE, &nread);
   _dos_read(filvar, &mode, sizeof(int), &nread);
   _dos_read(filvar, &nfiles, sizeof(int), &nread);
   ptr = malloc(sizeof(struct dbasestrc) + nfiles*sizeof(struct filestrc));
   if (ptr == NULL) {
      _dos_close(filvar);
      (*error)("opendatabase", ENOMEM);
      return NULL;
   }

// Fill in the newly created structure.
   strcpy(ptr->basename, file);
   ptr->filvar = filvar;
   ptr->nfiles = nfiles;
   _dos_read(filvar, (void far *)ptr->file,
	     nfiles*sizeof(struct filestrc), &nread);

   return (void *)ptr;

}

/*------------------------------------------------------
Function: closedatabase

Description: Undoes the effect of a previous 'opendatabase'.
------------------------------------------------------*/
void closedatabase(void *database)
{
   struct dbasestrc *ptr;

   ptr = (struct dbasestrc *)database;
   _dos_close(ptr->filvar);
   free(ptr);
}


/*------------------------------------------------------
Function: loadfile

Description: Reserves enough memory to hold the file
	     in memory and loads it from the database.
------------------------------------------------------*/
void far *loadfile(void *database, char *file)
{
   struct entrystrc *result;
   struct dbasestrc *ptr;
   void   far *data;
   char   huge *p;
   ulong  fsize, msize;		// file and memory size of file respectively.
   int    i, j;
   uint   nread;

   ptr = (struct dbasestrc *)database;
   result = findentry(file);
   if (result != NULL) {
      // File is in buffer.
      if (result->hits < 20) result->hits++;
      // Allocated memory and copy block.
      if ((data = farmalloc(result->size)) == NULL) {
	 (*error)("loadfile", ENOMEM);
	 return NULL;
      }
      moveXMS(XMShandle, result->addr, 0, (ulong)data, result->size);
   } else {
      // File must be read from disk.
      for (i = 0, j = -1; i < ptr->nfiles; i++) {
	 if (strcmpi(file, ptr->file[i].name) == 0) {
	    j = i;
	    break;
	 }
      }
      if (j == -1) {
	 (*error)("loadfile", ENOFILE, file);
	 return NULL;
      }
      lseek(ptr->filvar, ptr->file[j].fptr, SEEK_SET);
      fsize = ptr->file[j].size;
      msize = (fsize + 1) & 0xfffffffel;	// make it even.
      if ((data = farmalloc(msize)) == NULL) {
	 (*error)("loadfile", ENOMEM);
	 return NULL;
      }
      p = data;
      while (fsize > 65000) {
	 _dos_read(ptr->filvar, p, 65000, &nread);
	 fsize -= 65000; p += 65000;
      }
      _dos_read(ptr->filvar, p, fsize, &nread);

      buffer(file, data, ptr->file[j].size, ptr->file[j].flags);
   }

   return data;

}


/*------------------------------------------------------
Function: loadfiledirect

Description: Allocates memory for the file and loads
	     it if present from the buffer, otherwise
	     from disk.
------------------------------------------------------*/
void far *loadfiledirect(char *file, int flags)
{
   int    filvar;
   struct entrystrc *result;
   void   far *ptr;
   char   huge *p;
   ulong  fsize, msize;		// real size and size in memory of file.
   uint   nread;

   result = findentry(file);
   if (result != NULL) {
      // File is in buffer.
      if (result->hits < 20) result->hits++;
      // Allocated memory and copy block.
      if ((ptr = farmalloc(result->size)) == NULL) {
	 (*error)("loadfiledirect", ENOMEM);
	 return NULL;
      }
      moveXMS(XMShandle, result->addr, 0, (ulong)ptr, result->size);
   } else {
      // File must be read from disk.
      if (_dos_open(file, O_RDONLY, &filvar)) {
	 (*error)("loadfiledirect", ENOFILE, file);
	 return NULL;
      }
      fsize = filelength(filvar);
      msize = (fsize + 1) & 0xfffffffel;	// round it up.
      if ((ptr = farmalloc(msize)) == NULL) {
	 _dos_close(filvar);
	 (*error)("loadfiledirect", ENOMEM);
	 return NULL;
      }
      p = ptr;
      while (fsize > 65000) {
	 _dos_read(filvar, p, 65000, &nread);
	 fsize -= 65000; p += 65000;
      }
      _dos_read(filvar, p, fsize, &nread);
      _dos_close(filvar);

      buffer(file, ptr, msize, flags);
   }

   return ptr;

}


void unloadfile(void far *ptr)
{
   farfree(ptr);
}


/*------------------------------------------------------
Function: openfile

Description: Same as loadfile, but doesn't read it into
	     memory. Just sets the file pointer to
	     the start of the file.
Returns: File handle for further access.
------------------------------------------------------*/
int openfile(void *database, char *file)
{
   struct dbasestrc *ptr;		// Pointer to data base directory.
   int    i, j;
   int    filvar;


   ptr = (struct dbasestrc *)database;
   for (i = 0, j = -1; i < ptr->nfiles; i++) {
      if (strcmpi(file, ptr->file[i].name) == 0) {
	 j = i;
	 break;
      }
   }
   if (j == -1) {
      (*error)("openfile", ENOFILE, file);
      return -1;
   }

// Re-open database to get a free file pointer.
   if (_dos_open(ptr->basename, O_RDONLY, &filvar) != 0) {
      (*error)("openfile", ENOFILE, ptr->basename);
      return -1;
   }
// Seek to data.
   lseek(filvar, ptr->file[j].fptr, SEEK_SET);

   return filvar;
}


/*------------------------------------------------------
Function: openfiledirect

Description: In fact this is the same as open the file
	     for read only using _dos_open.
------------------------------------------------------*/
int openfiledirect(char *file)
{
   int   filvar;

   if (_dos_open(file, O_RDONLY, &filvar) != 0) {
      (*error)("openfiledirect", ENOFILE, file);
      return -1;
   }

   return filvar;
}


/*------------------------------------------------------
Function: closefile

Description:
------------------------------------------------------*/
void closefile(int filvar)
{
   _dos_close(filvar);
}


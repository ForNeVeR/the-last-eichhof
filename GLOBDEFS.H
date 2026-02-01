
/*-------------------------------------------------------*/
/*                                                       */
/*                G L O B D E F S . H                    */
/*                                                       */
/*-------------------------------------------------------*/
// SPDX-FileCopyrightText: 1989 Alpha - Helix <http://www.ife.ee.ethz.ch/~ammann/alphahelix/>
// SPDX-FileCopyrightText: 1989-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
// SPDX-License-Identifier: LicenseRef-TLE



#define    FALSE         0
#define    TRUE          (!FALSE)


#ifndef BYTEDEF

   typedef unsigned char     byte;
   #define BYTEDEF

#endif

#ifndef ULONGDEF

   typedef unsigned long     ulong;
   #define ULONGDEF

#endif


#ifndef LOCAL

   #define LOCAL(xx)         static xx near pascal

#endif



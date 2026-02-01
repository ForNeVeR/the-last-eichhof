
/*-------------------------------------------------------*/
/*                                                       */
/*                G L O B D E F S . H                    */
/*                                                       */
/*            [c] Copyright by Alpha - Helix             */
/*            written by Dany Schochini 1989             */
/*                                                       */
/*-------------------------------------------------------*/
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


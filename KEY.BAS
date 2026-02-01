REM SPDX-FileCopyrightText: 1992-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
REM SPDX-License-Identifier: LicenseRef-TLE
DEFINT A-Z
CLS
10 :
i = INP(&H60)
IF i <> j THEN
   LOCATE 1, 1: PRINT "                    ";
   LOCATE 1, 1: PRINT HEX$(INP(&H60)) + "h"; " "; INKEY$;
END IF
j = i
GOTO 10


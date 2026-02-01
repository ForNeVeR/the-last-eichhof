; SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
; SPDX-License-Identifier: LicenseRef-TLE


; top of file "sound.ash"


SND_PACKED4         EQU     00001h

IDEAL

STRUC              sndstrc             
priority            DW      ?
samplerate          DW      ?
flags               DW      ?
len                 DD      ?
data                DB      1 DUP ( ? )

ENDS              sndstrc             

GLOBAL C            initsound           :NEAR

GLOBAL C            shutsound           :NEAR

GLOBAL C            playsample          :NEAR

GLOBAL C            playloop            :NEAR

GLOBAL C            playfile            :NEAR

GLOBAL C            playfileloop        :NEAR

GLOBAL C            snd_cli             :NEAR

GLOBAL C            snd_sti             :NEAR

GLOBAL C            haltsound           :NEAR

GLOBAL C            soundbusy           :NEAR

GLOBAL C            speaker             :NEAR

; end of file "sound.ash"


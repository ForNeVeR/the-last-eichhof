


; top of file "baller.ash"


FALSE               EQU     0

TRUE                EQU     1

GAMESPEED           EQU     0e8f6h

LEVELS              EQU     5

BARY                EQU     211

BARSCOREX           EQU     114

BARSCOREY           EQU     212

BARSCORESPC         EQU     6

BARLIFEX            EQU     40

BARCLRDIGIT         EQU     10

BARLIFEDIGIT        EQU     11

INTINDEXSIZE        EQU     100

PTRINDEXSIZE        EQU     200

IDEAL

TYPEDEF              ulong               DWORD 

STARLIFES           EQU     4

STARTSHIPSPEED      EQU     4

MAXSHIPSPEED        EQU     8

W_ISWEAPON          EQU     08000h

W_SPEEDUP           EQU     00001h

W_EXTRALIFE         EQU     00002h

NAMESIZE            EQU     20

STRUC              armstrc             
armname             DB      20 DUP ( ? )
sprite              DW      ?
shot                DW      ?
cost                DW      ?
period              DW      ?
flags               DW      ?

ENDS              armstrc             

SHOTEND             EQU     08000h

SHOTRELEASE         EQU     08001h

SHOTHOMING          EQU     08002h

SHOTREFLECT         EQU     08003h

STRUC              shotstrc            
shotx               DW      ?
shoty               DW      ?
power               DW      ?
speed               DW      ?
sprite              DW      ?
data                DW      1 DUP ( ? )

ENDS              shotstrc            

FOE_ENDLEVEL        EQU     001h

FOE_INVINCIBLE      EQU     002h

FOE_TRANSPARENT     EQU     004h

FOE_STOPCOUNT       EQU     008h

FOE_PATH            EQU     010h

FOE_LINE            EQU     020h

FOEENDPATH          EQU     08000h

FOECHANGESPRITE     EQU     08001h

FOERELEASEFOE       EQU     08002h

FOECYCLEPATH        EQU     08004h

FOEMARK             EQU     08005h

FOESOUND            EQU     08006h

STRUC              foestrc             
flags               DW      ?
shield              DW      ?
score               DW      ?
expl                DW      ?
sprite              DW      ?
speed               DW      ?
path                DW      1 DUP ( ? )

ENDS              foestrc             

EXPLEND             EQU     08000h

EXPLNEW             EQU     08001h

EXPLWAIT            EQU     08002h

EXPLSOUND           EQU     08003h

EXPLREMOVEOBJ       EQU     08004h

EXPLRELEASEFOE      EQU     08005h

EXPLNEWPATH         EQU     08006h

STRUC              explstrc            
dummy               DW      ?
data                DW      1 DUP ( ? )

ENDS              explstrc            

MAXARMS             EQU     7

MAXFOES             EQU     26

MAXSHOTS            EQU     30

MAXEXPLS            EQU     8

WIN_CTIME           EQU     100

LOSE_CTIME          EQU     60

SHIELD_CTIME        EQU     70

STRUC              aniarm              
object              DW      ?
shot                DW      ?
period              DW      ?
flags               DW      ?
periodcnt           DW      ?

ENDS              aniarm              

STRUC              anishot             
object              DW      ?
go                  DW      ?
power               DW      ?
speed               DW      ?
dx                  DW      ?
dy                  DW      ?
data                DD      FAR PTR ?

ENDS              anishot             

STRUC              anifoe              
object              DW      ?
flags               DW      ?
score               DW      ?
expl                DW      ?
shield              DW      ?
path                DD      FAR PTR ?
savex               DW      ?
savey               DW      ?
cpath               DD      FAR PTR ?
speed               DW      ?
dx                  DW      ?
dy                  DW      ?
dz                  DW      ?
z0                  DW      ?
z1                  DW      ?
pixelcnt            DW      ?

ENDS              anifoe              

STRUC              aniexpl             
object              DW      ?
x                   DW      ?
y                   DW      ?
data                DD      FAR PTR ?

ENDS              aniexpl             

STRUC              sTableEntry         
sprite              DD      FAR PTR ?
flags               DW      ?

ENDS              sTableEntry         

STRUC              descrstrc           
level               DW      ?
text                DB      40 DUP ( ? )
nbigboss            DW      ?
score               DW      ?
money               DW      ?
flags               DW      ?

ENDS              descrstrc           

A_COMMAND           EQU     08000h

A_GOFIELD           EQU     08000h

A_STOPFIELD         EQU     08001h

A_SOUND             EQU     08002h

A_MARK              EQU     08003h

STRUC              attackstrc          
count               DW      ?
x                   DW      ?
y                   DW      ?
foe                 DW      ?

ENDS              attackstrc          

STRUC              levelstrc           
descript            DD      FAR PTR ?
nstars              DW      ?
star                DD      FAR PTR ?
nattacks            DW      ?
attack              DD      FAR PTR ?
nsprites            DW      ?
sprite              DD      FAR PTR ?
nfoes               DW      ?
foe                 DD      FAR PTR ?
nexpls              DW      ?
expl                DD      FAR PTR ?
nsounds             DW      ?
sound               DD      FAR PTR ?

ENDS              levelstrc           

STRUC              weaponstrc          
narms               DW      ?
arm                 DD      FAR PTR ?
nshots              DW      ?
shot                DD      FAR PTR ?
nsprites            DW      ?
sprite              DD      FAR PTR ?

ENDS              weaponstrc          

STRUC              weaponlststrc       
object              DW      ?
dx                  DW      ?
dy                  DW      ?
arm                 armstrc <>

ENDS              weaponlststrc       

KEYSPACE            EQU     039h

KEYENTER            EQU     01ch

KEYLEFT             EQU     04bh

KEYRIGHT            EQU     04dh

KEYUP               EQU     048h

KEYDOWN             EQU     050h

KEYESC              EQU     001h

KEYPAUSE            EQU     019h

KEYONE              EQU     002h

KEYTWO              EQU     003h

KEYTHREE            EQU     004h

KEYFOUR             EQU     005h

KEYFIVE             EQU     006h

KEYSIX              EQU     007h

CHEATLIFES          EQU     001h

CHEATMONEY          EQU     002h

CHEATCRASH          EQU     004h

_ext                EQU     <extern>

GLOBAL C            key                 :BYTE  :128

GLOBAL C            key_up              :WORD 

GLOBAL C            key_down            :WORD 

GLOBAL C            key_left            :WORD 

GLOBAL C            key_right           :WORD 

GLOBAL C            key_fire            :WORD 

GLOBAL C            key_pause           :WORD 

GLOBAL C            pressedkeys         :WORD 

GLOBAL C            tick                :WORD 

GLOBAL C            _arm                :aniarm

GLOBAL C            _foe                :anifoe

GLOBAL C            _shot               :anishot

GLOBAL C            _expl               :aniexpl

GLOBAL C            money               :DWORD 

GLOBAL C            score               :DWORD 

GLOBAL C            scoreold            :DWORD 

GLOBAL C            cheatlevel          :WORD 

GLOBAL C            shipspeed           :WORD 

GLOBAL C            lifes               :WORD 

GLOBAL C            stage               :WORD 

GLOBAL C            nweapons            :WORD 

GLOBAL C            weaponlst           :weaponlststrc

GLOBAL C            datapool            :FAR PTR WORD 

GLOBAL C            level               :levelstrc

GLOBAL C            lsprofs             :WORD 

GLOBAL C            lfoeofs             :WORD 

GLOBAL C            lexplofs            :WORD 

GLOBAL C            lsndofs             :WORD 

GLOBAL C            weapon              :weaponstrc

GLOBAL C            intindex            :WORD  :100

GLOBAL C            intindexptr         :WORD 

GLOBAL C            ptrindex            :FAR PTR WORD  :200

GLOBAL C            ptrindexptr         :WORD 

GLOBAL C            barfonthandle       :WORD 

GLOBAL C            int09               :FAR PTR FAR

GLOBAL C            int08               :FAR PTR FAR

GLOBAL C            error               :NEAR

GLOBAL C            intro               :NEAR

GLOBAL C            loadhighscore       :NEAR

GLOBAL C            savehighscore       :NEAR

GLOBAL C            highscore           :NEAR

GLOBAL C            menu                :NEAR

GLOBAL C            loadconfig          :NEAR

GLOBAL C            saveconfig          :NEAR

GLOBAL C            playthegame         :NEAR

GLOBAL C            setspeed            :NEAR

GLOBAL C            killallbuddies      :NEAR

GLOBAL C            waitforkey          :NEAR

GLOBAL C            waitdelayedkey      :NEAR

GLOBAL C            writetext           :NEAR

GLOBAL C            writenumber         :NEAR

GLOBAL C            defallarms          :NEAR

GLOBAL C            weaponmanager       :NEAR

GLOBAL C            copytoCS            :NEAR

GLOBAL C            defarm              :NEAR

GLOBAL C            displifes           :NEAR

GLOBAL C            setplayposition     :NEAR

GLOBAL C            play                :NEAR

GLOBAL C            lall1               :NEAR

GLOBAL C            lall2               :NEAR

GLOBAL C            waitfortick         :NEAR

GLOBAL C            newint09            :FAR

GLOBAL C            newint08            :FAR

; end of file "baller.ash"


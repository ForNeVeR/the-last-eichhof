


; top of file "xmode.ash"


__XMODE__           EQU     1h

XMIN                EQU     0

XMAX                EQU     319

YMIN                EQU     0

YMAX                EQU     219

MAXSPRITES          EQU     60

MAXOBJS             EQU     60

OBJ_ONECYCLE        EQU     001h

OBJ_HIGH            EQU     002h

OBJ_LOW             EQU     0

SPR_ALIGN           EQU     007h

SPR_DOUBLE          EQU     008h

IDEAL

STRUC              sprstrc             
xs                  DW      ?
ys                  DW      ?
maxn                DW      ?
data                DB      1 DUP ( ? )

ENDS              sprstrc             

STRUC              starstrc            
x                   DW      ?
y                   DW      ?
color               DW      ?
speed               DW      ?

ENDS              starstrc            

retrace             EQU     <{while(inportb(0x3da)&8); while(!(inportb(0x3da)&8));}>

GLOBAL C            setpalette          :NEAR

GLOBAL C            setcolor            :NEAR

GLOBAL C            setvanillapalett    :NEAR

GLOBAL C            setstandardpalet    :NEAR

GLOBAL C            glowto              :NEAR

GLOBAL C            glowin              :NEAR

GLOBAL C            glowout             :NEAR

GLOBAL C            setattrib           :NEAR

GLOBAL C            cyclepalette        :NEAR

GLOBAL C            defstarfield        :NEAR

GLOBAL C            gostarfield         :NEAR

GLOBAL C            stopstarfield       :NEAR

GLOBAL C            killstarfield       :NEAR

GLOBAL C            defsprite           :NEAR

GLOBAL C            killsprite          :NEAR

GLOBAL C            killallsprites      :NEAR

GLOBAL C            defobject           :NEAR

GLOBAL C            abandonobject       :NEAR

GLOBAL C            killobject          :NEAR

GLOBAL C            getpixel		:NEAR

GLOBAL C            shutxmode           :NEAR

GLOBAL C            initxmode           :NEAR

GLOBAL C            setpage             :NEAR

GLOBAL C            updatescreen        :NEAR

GLOBAL C            getspritesize       :NEAR

GLOBAL C            getobjectsize       :NEAR

GLOBAL C            getobjectpos        :NEAR

GLOBAL C            setxmode            :NEAR

GLOBAL C            screenmode          :NEAR

GLOBAL C            VGApresent          :NEAR

GLOBAL C            plot                :NEAR

GLOBAL C            killallobjects      :NEAR

GLOBAL C            moveobject          :NEAR

GLOBAL C            moveobjectdelta     :NEAR

GLOBAL C            flash               :NEAR

GLOBAL C            changesprite        :NEAR

GLOBAL C            crashtest           :NEAR

GLOBAL C            outofwindow         :NEAR

GLOBAL C            putsprite           :NEAR

GLOBAL C            removesprite        :NEAR

GLOBAL C            putspritedirect     :NEAR

GLOBAL C            clearscreen         :NEAR

GLOBAL C            clearregion         :NEAR

GLOBAL C            copypage            :NEAR

GLOBAL C            showpage            :NEAR

GLOBAL C            showline            :NEAR

GLOBAL C            showpcx256          :NEAR

GLOBAL C            windowx0            :WORD 

GLOBAL C            windowy0            :WORD 

GLOBAL C            windowx1            :WORD 

GLOBAL C            windowy1            :WORD 

GLOBAL C            objflashcolor       :WORD 

GLOBAL C            backgrndcolor       :WORD 

FALSE               EQU     0

TRUE                EQU     1

SC_INDEX            EQU     03c4h

GC_INDEX            EQU     03ceh

CRTC_INDEX          EQU     03d4h

MISC_OUTPUT         EQU     03c2h

MAP_MASK            EQU     02h

BIT_MASK            EQU     08h

INPUT_STATUS        EQU     03dah

START_ADDR_HIGH     EQU     0ch

START_ADDR_LOW      EQU     0dh

BYTESPERLINE        EQU     80

PAGESIZE            EQU     <(BYTESPERLINE * (YMAX+1))>

OFFSCREEN           EQU     <(2*PAGESIZE)>

PALETTESIZE         EQU     768

STRUC              lowspr              
active              DW      ?
xs                  DW      ?
ys                  DW      ?
nadd                DW      ?
maxn                DW      ?
xsalign             DW      ?
picsize             DW      ?
seqsize             DW      ?
fullsize            DW      ?
data                DD      FAR PTR ?
mask                DD      FAR PTR ?

ENDS              lowspr              

O_FLASH             EQU     00001h

STRUC              objstrc             
active              DW      ?
flags               DW      ?
x                   DW      ?
y                   DW      ?
xa                  DW      ?
ya                  DW      ?
xb                  DW      ?
yb                  DW      ?
xs                  DW      ?
ys                  DW      ?
n                   DW      ?
nadd                DW      ?
maxn                DW      ?
cycle               DW      ?
destroy             DW      ?
sprite              DW      ?

ENDS              objstrc             

STRUC              sfieldstrc          
active              DW      ?
go                  DW      ?
n                   DW      ?
star                DD      FAR PTR ?

ENDS              sfieldstrc          

GLOBAL C            _sprite             :lowspr

GLOBAL C            _obj                :objstrc

GLOBAL C            _sfield             :sfieldstrc

GLOBAL C            base                :WORD 

GLOBAL C            page                :WORD 

GLOBAL C            objflashcolor       :WORD 

GLOBAL C            backgrndcolor       :WORD 

GLOBAL C            palette             :BYTE  :768

; end of file "xmode.ash"


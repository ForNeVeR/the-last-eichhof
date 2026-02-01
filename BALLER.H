

#define FALSE			0
#define TRUE			1

#define GAMESPEED		0xe8f6	// about 20 frames a second.
#define LEVELS			5	// Number of levels in game.


// Several position equates.

#define BARY			211
#define BARSCOREX		114
#define BARSCOREY		212
#define BARSCORESPC		6
#define BARLIFEX		40
#define BARCLRDIGIT		10
#define BARLIFEDIGIT		11

// Index Sizes

#define INTINDEXSIZE		100
#define PTRINDEXSIZE		200



typedef unsigned long ulong;


// Weapon equates and structures.
#define STARLIFES		4	// Lifes at the beginning.
#define STARTSHIPSPEED		4       // Beginning ship speed.
#define MAXSHIPSPEED		8
#define W_ISWEAPON		0x8000
// Waffen Zusaetze.
#define W_SPEEDUP		0x0001
#define W_EXTRALIFE		0x0002

#define NAMESIZE		20
struct armstrc {
   char     armname[NAMESIZE];
   int      sprite;                     // Sprite # of weapon.
   int      shot;                       // Shot number.
   int      cost;			// How many bucks.
   int      period;			// Time till next shot release.
   unsigned flags;			// Weapon specific settings.
};

// Shot command set.
#define SHOTEND			0x8000
#define SHOTRELEASE		0x8001
#define SHOTHOMING		0x8002	// Homing missile.
#define SHOTREFLECT		0x8003	// Border Reflector.

struct shotstrc {

   int   shotx, shoty;			// Release point.
   int   power;				// Power.
   int   speed;				// Speed if homing.
   int   sprite;			// Sprite Number.
   int   data[];

};


// Foe flags
#define FOE_ENDLEVEL		0x01	// Big Boss indicator.
#define FOE_INVINCIBLE		0x02	// Foe can't be harmed.
#define FOE_TRANSPARENT		0x04    // Shots go through foe.
#define FOE_STOPCOUNT		0x08    // Frame counter stops till foe is destroyed.
#define FOE_PATH		0x10    // Foe is a path follower.
#define FOE_LINE		0x20    // Foe is a shot.

// Foe command set.
#define FOEENDPATH		0x8000
#define FOECHANGESPRITE		0x8001
#define FOERELEASEFOE		0x8002
#define FOECYCLEPATH            0x8004
#define FOEMARK			0x8005
#define FOESOUND		0x8006

struct foestrc {

   int    flags;			// Endlevel,invincible,flashing,...
   int    shield;			// Number of hits till destroyed.
   int    score;			// Score for player if destroyed.
   int    expl;				// Explosion number.
   int    sprite;         		// Sprite number.
   int    speed;			// Speed when using line mode.
   int    path[];			// Path data.

};

// Explosion command set.
#define EXPLEND                 0x8000
#define EXPLNEW                 0x8001
#define EXPLWAIT                0x8002
#define EXPLSOUND		0x8003
#define EXPLREMOVEOBJ		0x8004
#define EXPLRELEASEFOE		0x8005
#define EXPLNEWPATH		0x8006

struct explstrc {

   int    dummy;
   int    data[];

};


#define MAXARMS			7
#define MAXFOES			26
#define MAXSHOTS		30
#define MAXEXPLS		8

#define WIN_CTIME		100	// # of frames game will continue
					//   after BIG BOSS has gone.
#define LOSE_CTIME		60	// # of frames game will continue
					//   after Eichli is dead.
#define SHIELD_CTIME		70      // # of frames Eichli will be
					//   invincible at beginning of game.


struct  aniarm {

   int	object;      		// Object #.
   int	shot;                   // Sprite # of shot.
   int	period;			// Fire periode.
   int  flags;
   int	periodcnt;

};


struct	anishot {

   int	object;                 // Object number.
   int	go;			// 0: Shot is on hold. 1: Shot moves.
   int	power;                  // Power.
   int  speed;			// Speed of object if in homing mode.
   int	dx, dy;			// Delta values (direction of homing shot).
   int  far *data;		// Pointer to command path.

};


struct	anifoe {

   int	object;			// Object #.
   int	flags;			// State Flags. (Invincible, endlvl, ...).
   int	score;
   int	expl;                   // Explosion that will be used.
   int	shield;                 // How many hits until destroyed ?
   int  far *path;              // Path pointer stored by MARK.
   int	savex, savey;		// (x, y) for stored by MARK.
   int	far *cpath;
   int  speed;			// Speed of object if in line mode.
   int  dx, dy, dz, z0, z1;     // Line draw varibales.
   int  pixelcnt;		// Count of pixels to draw to reach destination.

};



struct	aniexpl {

   int	object;			// Just to indicate whether active or not.
   int  x, y;			// Coords of impact. Kawumm !!
   int	far *data;

};


//------------------

struct sTableEntry {

   struct sprstrc far *sprite;
   unsigned       flags;

};


// Level description structure.
// Contains useful informations about the level.
struct descrstrc {

   int      level;			// Level number.
   char     text[40];			// Level text.
   int      nbigboss;			// Number of endlevel monsters.
   unsigned score;			// Bonus score for getting so far.
   unsigned money;			// Bonus money.
   unsigned flags;

};


// Attack structure and defines.
#define A_COMMAND		0x8000	// Command bit.
#define A_GOFIELD       	0x8000	// Go starfield.
#define A_STOPFIELD		0x8001	// Stop field.
#define A_SOUND			0x8002	// Play sample.
#define A_MARK			0x8003	// Mark position.
struct attackstrc {

   unsigned count;			// Start action at value 'count'.
   int      x, y;                       // (x, y) of foe.
   int      foe;                        // Foe # to release.

};


struct levelstrc {

   struct descrstrc   *descript;        // Descripton.
   int    nstars;
   struct starstrc    *star;            // Starfield definition.
   int    nattacks;
   struct attackstrc  *attack;          // Attack table.
   int    nsprites;
   struct sTableEntry *sprite;          // Sprite definitions.
   int    nfoes;
   struct foestrc     * far *foe;       // Enemy defs.
   int    nexpls;
   struct explstrc    * far *expl;	// Explosion definitions.
   int    nsounds;
   struct sndstrc     * far *sound;	// SOUNDS.

};


struct weaponstrc {

   int    narms;
   struct armstrc     *arm;
   int    nshots;
   struct shotstrc    * far *shot;
   int    nsprites;
   struct sTableEntry *sprite;

};


// Structure of currently active weapons.
struct weaponlststrc {

   int    object;			// Object # of active weapon.
   int    dx, dy;                       // delta (x, y) to main weapon.
   struct armstrc arm;                  // Weapon definition structure.

};


#define KEYSPACE                0x39
#define KEYENTER		0x1c
#define	KEYLEFT			0x4b
#define KEYRIGHT		0x4d
#define KEYUP			0x48
#define KEYDOWN			0x50
#define KEYESC			0x01
#define KEYPAUSE		0x19
#define KEYONE			0x02
#define KEYTWO			0x03
#define KEYTHREE		0x04
#define KEYFOUR			0x05
#define KEYFIVE			0x06
#define KEYSIX			0x07

// Cheatmode flags:
#define CHEATLIFES              0x01
#define CHEATMONEY              0x02
#define CHEATCRASH              0x04


#ifdef MAIN_MODULE
#define _ext
#else
#define _ext extern
#endif

_ext volatile char key[0x80];		// Keyboard array.
_ext int      key_up, key_down;
_ext int      key_left, key_right;
_ext int      key_fire, key_pause;
_ext volatile int pressedkeys;		// Number of pressed keys.
_ext volatile int tick;			// Int 1ch occured ?

_ext struct aniarm	_arm[MAXARMS];
_ext struct anifoe	_foe[MAXFOES];
_ext struct anishot	_shot[MAXSHOTS];
_ext struct aniexpl	_expl[MAXEXPLS];

_ext long   money;			// Player's money.
_ext long   score;                      // Player's score after Level.
_ext long   scoreold;                   // Player's score before Level.

_ext int    cheatlevel;			// flag of Cheatmode.

_ext int    shipspeed;                  // Speed of ship in pixels per frame.
_ext int    lifes;
_ext int    stage;
_ext int    nweapons;			// # of Weapons active.
_ext struct weaponlststrc weaponlst[MAXARMS];


_ext void   *datapool;			// Handle of Global DataFile.
_ext struct levelstrc  level; 		// Level.
_ext int    lsprofs;
_ext int    lfoeofs;
_ext int    lexplofs;
_ext int    lsndofs;
_ext struct weaponstrc weapon;          // Weapons.
_ext int    intindex[INTINDEXSIZE];	// Level sprite index.
_ext int    intindexptr;
_ext void   far *ptrindex[PTRINDEXSIZE];
_ext int    ptrindexptr;

_ext int    barfonthandle;

_ext void interrupt (*int09)(void);
_ext void interrupt (*int08)(void);

#undef _ext

//--------- Function prototypes ---------//


void error(char *text, int code, ...);

// Functions of module 'intro.c'
void intro(void);

// Functions of module 'hiscore.c'
void loadhighscore(int *filvar);
void savehighscore(int *filvar);
void highscore(char fullmode);

// Functions of module 'menu.c'
void menu(void);
void loadconfig(void);
void saveconfig(void);

// Functions of module 'gameplay.c'
void playthegame(void);

// Functions of module 'support.c'
void setspeed(unsigned speed);
void killallbuddies(void);
void waitforkey(void);
int waitdelayedkey(int count);
void writetext(int x, int y, const char *text, struct sprstrc *font);
void writenumber(int x, int y, long number, struct sprstrc *font);

// Functions of module 'shop.c'
ulong defallarms(int x, int y);
void weaponmanager(void);


void copytoCS(void);
int defarm(int x, int y, struct armstrc far *arm);
void displifes(void);
void setplayposition(int nattacks, void far *attack, int nbigb);
int play(void);
void lall1(void);
void lall2(void);

void waitfortick(void);

void interrupt newint09(void);
void interrupt newint08(void);

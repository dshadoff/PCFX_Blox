/*
 *   Blox - A "falling blocks" type game for the PC-FX
 *
 *   Copyright (C) 2024 David Shadoff
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <eris/types.h>
#include <eris/std.h>
#include <eris/v810.h>
#include <eris/king.h>
#include <eris/7up.h>
#include <eris/low/7up.h>
#include <eris/tetsu.h>
#include <eris/romfont.h>
#include <eris/bkupmem.h>
#include <eris/timer.h>
#include <eris/pad.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Joypad defines (move these to library includes)
//
#define JOY_I            1
#define JOY_II           2
#define JOY_III          4
#define JOY_IV           8
#define JOY_V            16
#define JOY_VI           32
#define JOY_SELECT       64
#define JOY_RUN          128
#define JOY_UP           256
#define JOY_RIGHT        512
#define JOY_DOWN         1024
#define JOY_LEFT         2048
#define JOY_MODE1        4096
#define JOY_MODE2        16384

// HuC6270 defines (move these to library includes)
//
#define HUC6270_REG_MAWR    0x00      // Memory Address Write register
#define HUC6270_REG_MARR    0x01      // Memory Address Read register
#define HUC6270_REG_DATA    0x02      // Data (write or read)
#define HUC6270_REG_CR      0x05      // Control register
#define HUC6270_REG_RCR     0x06      // Raster Counter register
#define HUC6270_REG_BXR     0x07      // BGX Scroll register
#define HUC6270_REG_BYR     0x08      // BGY Scroll register
#define HUC6270_REG_MWR     0x09      // Memory Access Width register
#define HUC6270_REG_HSR     0x0A      // Horizontal Sync register
#define HUC6270_REG_HDR     0x0B      // Horizontal Display register
#define HUC6270_REG_VPR     0x0C      // Vertical Sync register
#define HUC6270_REG_VDR     0x0D      // Vertical Display register
#define HUC6270_REG_VCR     0x0E      // Vertical Display End Position register
#define HUC6270_REG_DCR     0x0F      // Block Transfer Control register
#define HUC6270_REG_SOUR    0x10      // Block Transfer Source Address register
#define HUC6270_REG_DESR    0x11      // Block Transfer Destination Address register
#define HUC6270_REG_LENR    0x12      // Block Transfer Length register
#define HUC6270_REG_DVSSR   0x13      // VRAM-SATB Block Transfer Source Address register

#define HUC6270_STAT_CR     0x0001    // Collision detect
#define HUC6270_STAT_OR     0x0002    // Over detect (too many sprites)
#define HUC6270_STAT_RR     0x0004    // Raster scanline detect
#define HUC6270_STAT_DS     0x0008    // Block xfer from VRAM to SATB end detect
#define HUC6270_STAT_DV     0x0010    // Block xfer from VRAM to VRAM end detect
#define HUC6270_STAT_VD     0x0020    // Vertical Blank Detect
#define HUC6270_STAT_BSY    0x0040    // Busy

#define HUC6270_CR_IRQ_CC   0x0001    // Interrupt Request Enable on collision detect
#define HUC6270_CR_IRQ_OC   0x0002    // Interrupt Request Enable on over detect
#define HUC6270_CR_IRQ_RC   0x0004    // Interrupt Request Enable on raster detect
#define HUC6270_CR_IRQ_VC   0x0008    // Interrupt Request Enable on vertical blank detect

// Note that CR bits 0x0010 and 0x0020 are for external sync ('EX'), which are normally '00'
#define HUC6270_CR_SB       0x0040    // Sprite blank (1 = visible)
#define HUC6270_CR_BB       0x0080    // Background blank (1 = visible)

// Note that CR bits 0x0100 and 0x0200 are for DISP output select ('TE'), which are normally '00'
// Note that CR bit  0x0400 is for Dynamic RAM refresh ('DR'), which is normally '0'

#define HUC6270_CR_IW_01    0x0000    // Bitfield for auto-increment of address pointer of 0x01
#define HUC6270_CR_IW_20    0x0800    // Bitfield for auto-increment of address pointer of 0x20
#define HUC6270_CR_IW_40    0x1000    // Bitfield for auto-increment of address pointer of 0x40
#define HUC6270_CR_IW_80    0x1800    // Bitfield for auto-increment of address pointer of 0x80

// Note that MWR VRAM   access width mode (0x0001 & 0x0002) is usually '00'
// Note that MWR Sprite access width mode (0x0004 & 0x0008) is usually '00'
#define HUC6270_MWR_SCREEN_32x32   0x0000    // Bitfield for virtual screen map of  32 wide, 32 tall
#define HUC6270_MWR_SCREEN_64x32   0x0010    // Bitfield for virtual screen map of  64 wide, 32 tall
#define HUC6270_MWR_SCREEN_128x32  0x0020    // Bitfield for virtual screen map of 128 wide, 32 tall
#define HUC6270_MWR_SCREEN_32x64   0x0040    // Bitfield for virtual screen map of  32 wide, 64 tall
#define HUC6270_MWR_SCREEN_64x64   0x0050    // Bitfield for virtual screen map of  64 wide, 64 tall
#define HUC6270_MWR_SCREEN_128x64  0x0060    // Bitfield for virtual screen map of 128 wide, 64 tall
// Note that MWR CG mode for 4 clocks ode (0x0080) is usually '0'

#define CHR_SIZE        0x10
#define CHRREF(palette, vramaddr)	((palette << 12) | (vramaddr >> 4))

#define SPRITE_Y_INVERT		0x8000
#define SPRITE_Y_HEIGHT_1	0x0
#define SPRITE_Y_HEIGHT_2	0x1000
#define SPRITE_Y_HEIGHT_4	0x3000

#define SPRITE_X_INVERT		0x800
#define SPRITE_X_WIDTH_1	0x0
#define SPRITE_X_WIDTH_2	0x100
#define SPRITE_PRIO_BG		0x0
#define SPRITE_PRIO_SP		0x80
#define SPRITE_PATTERN(vramaddr)	(vramaddr >> 5)


#define SPR_CELL	0x0040
#define SPR_32x32CELL	0x0100




typedef enum {
	VDC0,
	VDC1
} VDCNUM;

// Constants used by program:
//
//
#define CG_VRAMLOC       0x1000
#define CG_FONTLOC       CG_VRAMLOC
#define CG_GRAPHICS      (CG_VRAMLOC+0x1000)

#define SPR_VRAMLOC      0x5000
#define SATB_VRAMLOC     0xFF00

#define BGMAPHEIGHT      32	// BG map is 32 tiles tiles
#define BGMAPWIDTH       64     // BG map is 64 tiles wide (incl. using 'virtual' mode)

#define FIELDWIDTH       10	// Field size - # tiles wide
#define FIELDHEIGHT      20	// (# tiles high)
#define FIELDHIDHT       4	// height of 'hidden' portion at top

#define SCOREPOSX        3	// x-position of score message
#define SCOREPOSY        3	// y-position of score message
#define SCOREPAL         1	// CG palette # for printing scores

#define FIELDX           20	// field x-position in tiles - top left corner
#define FIELDY           1	// (y-position)    * includes hidden portion

#define FLD_SPRXORG      (FIELDX*8+32)	// pixel-based origin x-position (for sprites)
#define FLD_SPRYORG      (FIELDY*8+64)	// (y-position)

#define PAUSEMSGX        22	// pause message (x,y) location
#define PAUSEMSGY        14

#define GAMOVRMSGX       23	// GAME OVER message (x,y) location
#define GAMOVRMSGY       14

#define JOYRPTMASK       (JOY_LEFT|JOY_RIGHT|JOY_DOWN|JOY_I|JOY_II)
#define JOYRPTINIT       15
#define JOYRPTSUBS       3



void print_text(VDCNUM vdc, int x_pos, int y_pos, int palette, char *mesg, int maxlen);
void wait_joypad_run(void);
void disp_blank_playfield(void);
void vsync(int numframes);
void pause(void);
void game_over(void);
void sensejoy(void);
void joypadmv(void);
void setpiece(void);
void setsprvars(void);
void nxtpiece(void);
void snapshot(int type, int phase, int xpos, int ypos);
void init_score(void);
void clear_display_field(void);
void dispbkgnd(void);
void display_score(void);
void disp_playfield(void);
int chkmvok(int type, int phase, int xpos, int ypos, int xdelta, int ydelta);
void init(void);
void testlines(void);

extern u8 font[];

// interrupt-handling variables
volatile int sda_frame_count = 0;
volatile int last_sda_frame_count = 0;

/* HuC6270-A's status register (RAM mapping). Used during VSYNC interrupt */
volatile uint16_t * const MEM_6270A_SR = (uint16_t *) 0x80000400;

int stepval = 0;



//  Difficulty-level data:
//  For now, it's a list of speed and next-level-starts-at scores
//  speed is "vsync-frames per move", and score is in "lines cleared"

typedef struct chlng_levels
{
   int    vsyncs;
   char * score;
} chlng_level;

const chlng_level diff_level[] = {
   { 30, "00004" },
   { 24, "00009" }, 
   { 20, "00014" }, 
   { 16, "00019" }, 
   { 12, "00029" }, 
   { 10, "00039" }, 
   { 8, "00049" }, 
   { 6, "00059" }, 
   { 5, "00069" }, 
   { 4, "00079" }, 
   { 3, "00099" }, 
   { 2, "00119" }, 
   { 1, "99999" }
};

char *scoremsg = "SCORE: ";
char *pausemsg = "PAUSE";
char *gameovermsg1 = "GAME";
char *gameovermsg2 = "OVER";

int  levelval;
char scoreval[6];

int  frampermov;
int  fpmcount;


char displn[(FIELDHEIGHT+FIELDHIDHT)][FIELDWIDTH];

// joypad repeat values
//
int joyrptval;
int joyfrminit;
int joyfrmsubs;
int joyout;

// piece type, rotation, position
char pieceposx;
char pieceposy;
char piecenum;
char phasenum;

int deletelines;



const uint16_t CG_palette[] = {
// pallette #0 - #7
0x0088, 0x4888, 0xB381, 0xFC88, 0x4B5F, 0xB381, 0xB381, 0xB381,
  0xB381, 0xB381, 0xB381, 0xB381, 0xB381, 0xB381, 0xB381, 0xB381,

0x0088, 0xEF48, 0x7F38, 0xC738, 0x6768, 0xAA2A, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0xFC88,

0x0088, 0xAD99, 0x3BBB, 0x8EAC, 0x21A9, 0x59CD, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088,

0x0088, 0xDA56, 0x3F55, 0xB246, 0x3285, 0x7F35, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088,

0x0088, 0xCD8A, 0x356D, 0x7D6D, 0x1D99, 0x4B5F, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088,

0x0088, 0xDA59, 0x753A, 0xBD3A, 0x3288, 0x9F2C, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088,

0x0088, 0xCD86, 0x6493, 0xAC93, 0x3AB5, 0x7A82, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088,

0x0088, 0xB897, 0x25B6, 0x82B5, 0x08A7, 0x42D5, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088
};

const uint16_t SPR_palette[] = {
// sprites pallette #0 - #7:
0x0088, 0x0088, 0x91B3, 0x91B3, 0x91B3, 0x91B3, 0x91B3, 0x91B3,
  0x91B3, 0x91B3, 0x91B3, 0x91B3, 0x91B3, 0x91B3, 0x91B3, 0x91B3,

0x0088, 0xEF48, 0x7F38, 0xC738, 0x6768, 0xAA2A, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0xFC88,

0x0088, 0xAD99, 0x3BBB, 0x8EAC, 0x21A9, 0x59CD, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088,

0x0088, 0xDA56, 0x3F55, 0xB246, 0x3285, 0x7F35, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088,

0x0088, 0xCD8A, 0x356D, 0x7D6D, 0x1D99, 0x4B5F, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088,

0x0088, 0xDA59, 0x753A, 0xBD3A, 0x3288, 0x9F2C, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088,

0x0088, 0xCD86, 0x6493, 0xAC93, 0x3AB5, 0x7A82, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088,

0x0088, 0xB897, 0x25B6, 0x82B5, 0x08A7, 0x42D5, 0x0088, 0x0088,
  0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088, 0x0088
};


#define OFFCHR_VRAMLOC		(CG_GRAPHICS)
#define BKCHR1_VRAMLOC		(OFFCHR_VRAMLOC   +CHR_SIZE)
#define BKCHR2_VRAMLOC		(BKCHR1_VRAMLOC   +CHR_SIZE)
#define CORNERCHR_VRAMLOC	(BKCHR2_VRAMLOC   +CHR_SIZE)
#define ENDCHR_VRAMLOC		(CORNERCHR_VRAMLOC+CHR_SIZE)
#define BOTTOMCHR_VRAMLOC	(ENDCHR_VRAMLOC   +CHR_SIZE)
#define FULLCHR_VRAMLOC		(BOTTOMCHR_VRAMLOC+CHR_SIZE)

#define OFFCHR_PAL		0
#define BKCHR1_PAL		0
#define BKCHR2_PAL		0
#define CORNERCHR_PAL		0
#define ENDCHR_PAL		0
#define BOTTOMCHR_PAL		0
#define FULLCHR_PAL		0


#include "offchr_data.gen_data"
#include "bkchr1_data.gen_data"
#include "bkchr2_data.gen_data"
#include "cornerchr_data.gen_data"
#include "endchr_data.gen_data"
#include "bottomchr_data.gen_data"
#include "fullchr_data.gen_data"



#define SPR_P0PH0VRAM	(SPR_VRAMLOC)
#define SPR_P0PH1VRAM	(SPR_P0PH0VRAM+SPR_32x32CELL)
#define SPR_P0PH2VRAM	(SPR_P0PH1VRAM+SPR_32x32CELL)
#define SPR_P0PH3VRAM	(SPR_P0PH2VRAM+SPR_32x32CELL)

#define SPR_P1PH0VRAM	(SPR_P0PH3VRAM+SPR_32x32CELL)
#define SPR_P1PH1VRAM	(SPR_P1PH0VRAM+SPR_32x32CELL)
#define SPR_P1PH2VRAM	(SPR_P1PH1VRAM+SPR_32x32CELL)
#define SPR_P1PH3VRAM	(SPR_P1PH2VRAM+SPR_32x32CELL)

#define SPR_P2PH0VRAM	(SPR_P1PH3VRAM+SPR_32x32CELL)
#define SPR_P2PH1VRAM	(SPR_P2PH0VRAM+SPR_32x32CELL)
#define SPR_P2PH2VRAM	(SPR_P2PH1VRAM+SPR_32x32CELL)
#define SPR_P2PH3VRAM	(SPR_P2PH2VRAM+SPR_32x32CELL)

#define SPR_P3PH0VRAM	(SPR_P2PH3VRAM+SPR_32x32CELL)
#define SPR_P3PH1VRAM	(SPR_P3PH0VRAM+SPR_32x32CELL)

#define SPR_P4PH0VRAM	(SPR_P3PH1VRAM+SPR_32x32CELL)
#define SPR_P4PH1VRAM	(SPR_P4PH0VRAM+SPR_32x32CELL)

#define SPR_P5PH0VRAM	(SPR_P4PH1VRAM+SPR_32x32CELL)
#define SPR_P5PH1VRAM	(SPR_P5PH0VRAM+SPR_32x32CELL)

#define SPR_P6PH0VRAM	(SPR_P5PH1VRAM+SPR_32x32CELL)

#define SPR_P7PH0VRAM	(SPR_P6PH0VRAM+SPR_32x32CELL)


#include "p0ph0_data.gen_data"
#include "p0ph1_data.gen_data"
#include "p0ph2_data.gen_data"
#include "p0ph3_data.gen_data"

#include "p1ph0_data.gen_data"
#include "p1ph1_data.gen_data"
#include "p1ph2_data.gen_data"
#include "p1ph3_data.gen_data"

#include "p2ph0_data.gen_data"
#include "p2ph1_data.gen_data"
#include "p2ph2_data.gen_data"
#include "p2ph3_data.gen_data"

#include "p3ph0_data.gen_data"
#include "p3ph1_data.gen_data"

#include "p4ph0_data.gen_data"
#include "p4ph1_data.gen_data"

#include "p5ph0_data.gen_data"
#include "p5ph1_data.gen_data"

#include "p6ph0_data.gen_data"

#include "p7ph0_data.gen_data"



typedef struct BgChars
{
   uint16_t pal;
   uint16_t vidaddr;
   uint16_t ref;
   const uint16_t *data;
   uint16_t size;
} Bgchr;

// offchr character pattern = 8x8 all 0's:
//
const Bgchr offchr =
{ OFFCHR_PAL, OFFCHR_VRAMLOC, CHRREF(OFFCHR_PAL, OFFCHR_VRAMLOC), offchr_data, sizeof(offchr_data) };

// bkchr1 character color pattern = 8x8 all 3's:
//
const Bgchr bkchr1 =
{ BKCHR1_PAL, BKCHR1_VRAMLOC, CHRREF(BKCHR1_PAL, BKCHR1_VRAMLOC), bkchr1_data, sizeof(bkchr1_data) };

// bkchr2 character color pattern = 8x8 all 4's:
//
const Bgchr bkchr2 =
{ BKCHR2_PAL, BKCHR2_VRAMLOC, CHRREF(BKCHR2_PAL, BKCHR2_VRAMLOC), bkchr2_data, sizeof(bkchr2_data) };

// cornerchr character color pattern = 8x8 all zeroes, except 1's on top edge and left edge:
//
const Bgchr cornerchr =
{ CORNERCHR_PAL, CORNERCHR_VRAMLOC, CHRREF(CORNERCHR_PAL, CORNERCHR_VRAMLOC), cornerchr_data, sizeof(cornerchr_data) };

// endchr character color pattern = 8x8 all 0's except 1's on left edge:
//
const Bgchr endchr =
{ ENDCHR_PAL, ENDCHR_VRAMLOC, CHRREF(ENDCHR_PAL, ENDCHR_VRAMLOC), endchr_data, sizeof(endchr_data) };

// bottomchr character color pattern = 8x8 all 0's except 1's on top edge:
//
const Bgchr bottomchr =
{ BOTTOMCHR_PAL, BOTTOMCHR_VRAMLOC, CHRREF(BOTTOMCHR_PAL, BOTTOMCHR_VRAMLOC), bottomchr_data, sizeof(bottomchr_data) };

// fullchr character color pattern:
// 11111113
// 21111133
// 22555533
// 22555533
// 22555533
// 22555533
// 22444443
// 24444444
//
const Bgchr fullchr =
{ FULLCHR_PAL, FULLCHR_VRAMLOC, CHRREF(FULLCHR_PAL, FULLCHR_VRAMLOC), fullchr_data, sizeof(fullchr_data) };


// Piece orientation information:
// the square data is used for detecting existing filled-blocks
// (for collision-detection), and for sprite-to-block transfer
// when the piece comes to rest
//
// sprite_x_rotate_adjustment (and y) is only used for piece #2, to
// compensate for its special rotation (around its second square)
//
//
// game pieces' data
// -----------------
//
// piece #:         0     1     2     3     4     5     6
//
// appearance:      XX    XX    X     X      X    X     XX
//                  X      X    XX    X     XX    XX    XX
//                  X      X    X     X     X      X
//                                    X
// # rotation
//   phases:        4     4     4     2     2     2     1
//

struct sqrpos {
   int x;
   int y;
};

typedef struct piecephasedatas {
   int           width;
   int           height;
   struct sqrpos square[4];
   uint16_t      sprpattern_vram_addr;
   int           sprite_x_rotate_adjustment;
   int           sprite_y_rotate_adjustment;
} piecephasedata;


const piecephasedata p0phstbl[4] = {
	{ 2, 3, {{0, 0}, {1, 0}, {0, 1}, {0, 2}}, SPR_P0PH0VRAM, 0, 0 },
	{ 3, 2, {{0, 0}, {0, 1}, {1, 1}, {2, 1}}, SPR_P0PH1VRAM, 0, 0 },
	{ 2, 3, {{1, 0}, {1, 1}, {1, 2}, {0, 2}}, SPR_P0PH2VRAM, 0, 0 },
	{ 3, 2, {{0, 0}, {1, 0}, {2, 0}, {2, 1}}, SPR_P0PH3VRAM, 0, 0 }
};

const piecephasedata p1phstbl[4] = {
	{ 2, 3, {{0, 0}, {1, 0}, {1, 1}, {1, 2}}, SPR_P1PH0VRAM, 0, 0 },
	{ 3, 2, {{0, 0}, {0, 1}, {1, 0}, {2, 0}}, SPR_P1PH1VRAM, 0, 0 },
	{ 2, 3, {{0, 0}, {0, 1}, {0, 2}, {1, 2}}, SPR_P1PH2VRAM, 0, 0 },
	{ 3, 2, {{0, 1}, {1, 1}, {2, 1}, {2, 0}}, SPR_P1PH3VRAM, 0, 0 }
};

const piecephasedata p2phstbl[4] = {
	{ 2, 3, {{0, 0}, {0, 1}, {1, 1}, {0, 2}}, SPR_P2PH0VRAM, 0, 0 },
	{ 3, 2, {{0, 1}, {1, 0}, {1, 1}, {2, 1}}, SPR_P2PH1VRAM, 0, 0 },
	{ 2, 3, {{0, 1}, {1, 0}, {1, 1}, {1, 2}}, SPR_P2PH2VRAM, 0, 0 },
	{ 3, 2, {{0, 0}, {1, 0}, {2, 0}, {1, 1}}, SPR_P2PH3VRAM, 0, 0 }
};

const piecephasedata p3phstbl[4] = {
	{ 1, 4, {{0, 0}, {0, 1}, {0, 2}, {0, 3}}, SPR_P3PH0VRAM,  1, -1 },
	{ 4, 1, {{0, 0}, {1, 0}, {2, 0}, {3, 0}}, SPR_P3PH1VRAM, -1,  1 },
	{ 1, 4, {{0, 0}, {0, 1}, {0, 2}, {0, 3}}, SPR_P3PH0VRAM,  1, -1 },  // Last two are same as first two
	{ 4, 1, {{0, 0}, {1, 0}, {2, 0}, {3, 0}}, SPR_P3PH1VRAM, -1,  1 }
};

const piecephasedata p4phstbl[4] = {
	{ 2, 3, {{1, 0}, {1, 1}, {0, 1}, {0, 2}}, SPR_P4PH0VRAM, 0, 0 },
	{ 3, 2, {{0, 0}, {1, 0}, {1, 1}, {2, 1}}, SPR_P4PH1VRAM, 0, 0 },
	{ 2, 3, {{1, 0}, {1, 1}, {0, 1}, {0, 2}}, SPR_P4PH0VRAM, 0, 0 },  // Last two are same as first two
	{ 3, 2, {{0, 0}, {1, 0}, {1, 1}, {2, 1}}, SPR_P4PH1VRAM, 0, 0 }
};

const piecephasedata p5phstbl[4] = {
	{ 2, 3, {{0, 0}, {0, 1}, {1, 1}, {1, 2}}, SPR_P5PH0VRAM, 0, 0 },
	{ 3, 2, {{0, 1}, {1, 1}, {1, 0}, {2, 0}}, SPR_P5PH1VRAM, 0, 0 },
	{ 2, 3, {{0, 0}, {0, 1}, {1, 1}, {1, 2}}, SPR_P5PH0VRAM, 0, 0 },  // Last two are same as first two
	{ 3, 2, {{0, 1}, {1, 1}, {1, 0}, {2, 0}}, SPR_P5PH1VRAM, 0, 0 }
};

const piecephasedata p6phstbl[4] = {
	{ 2, 2, {{0, 0}, {0, 1}, {1, 0}, {1, 1}}, SPR_P6PH0VRAM, 0, 0 },
	{ 2, 2, {{0, 0}, {0, 1}, {1, 0}, {1, 1}}, SPR_P6PH0VRAM, 0, 0 },  // Last three are same as first one
	{ 2, 2, {{0, 0}, {0, 1}, {1, 0}, {1, 1}}, SPR_P6PH0VRAM, 0, 0 },
	{ 2, 2, {{0, 0}, {0, 1}, {1, 0}, {1, 1}}, SPR_P6PH0VRAM, 0, 0 }
};

// Note: It should not be odd to reference individual square positions as:
// (piecetbl[piecenum] + phasenum)->square[squarenum].x
const piecephasedata * piecetbl[7] = {
   p0phstbl, p1phstbl, p2phstbl, p3phstbl, p4phstbl, p5phstbl, p6phstbl
};




///////////////////////////////// Joypad routines
volatile u32 joypad;
volatile u32 joypad_last;
volatile u32 joytrg;


__attribute__ ((noinline)) void joyread(void)
{
// assume that this is a joypad and not a mouse
// Maybe need to fix this later

   joypad_last = joypad;

   joypad = eris_pad_read(0);

   joytrg = (~joypad_last) & joypad;

//   if ( ((temp >> 28) & 0x0F) == (~PAD_TYPE_FXPAD & 0x0f) ) {  // PAD TYPE
//   }
//   joypad = temp;
}




///////////////////////////////// Interrupt handler
__attribute__ ((interrupt_handler)) void my_vblank_irq (void)
{
   uint16_t vdc_status = *MEM_6270A_SR;

   if (vdc_status & HUC6270_STAT_VD ) {
      sda_frame_count++;
   }
   joyread();
}

void vsync(int numframes)
{
   while (sda_frame_count < (last_sda_frame_count + numframes + 1));

   last_sda_frame_count = sda_frame_count;
}


///////////////////////////////// CODE

//
// for setting breakpoints - add a call to step() and breakpoint on it
// or watchpoint on stepval.
//
__attribute__ ((noinline)) void step(void)
{
   stepval++;;
}
//////////

// note vdc_num = 0 or 1
//
//void poke_vram(VDCNUM vdc_num, uint16_t vid_addr, uint16_t vid_data)
//{
//   eris_low_sup_set_vram_write(vdc_num, vid_addr);
//   eris_low_sup_vram_write(vdc_num, vid_data);
//}
//
//uint16_t peek_vram(VDCNUM vdc_num, uint16_t vid_addr)
//{
//uint16_t vid_data = 0;
//
//   eris_low_sup_set_vram_write(vdc_num, vid_addr);
//   vid_data = eris_low_sup_vram_read(vdc_num);
//
//   return(vid_data);
//}

void load_vram(VDCNUM vdc_num, const uint16_t *data, uint16_t vid_addr, uint16_t size)
{
int i;

   eris_low_sup_set_vram_write(vdc_num, vid_addr);

   for (i = 0; i < size; i++)
   {
      eris_low_sup_vram_write(vdc_num, *data);
      data++;
   }
}


int main(int argc, char *argv[])
{
   init();

//TODO:  Initialize random number generator


   while (1)     // This is a loop for games (each iteration is a game)
   {

      // intialization
      //
      init_score();

      clear_display_field();

      // set initial difficulty level
      //
      levelval = 0;
      frampermov = diff_level[levelval].vsyncs;

      // Wait for a vsync to reduce initial screen flash
      //
      vsync(0);

      // display startup screen
      //
      dispbkgnd();
      display_score();
      disp_playfield();

//TODO:  Get a random piece number
      piecenum  = 0;

      setpiece();

      // set countdown interval - number of frames until piece moves downward
      //
      fpmcount = frampermov;

      while (1)     // This is a loop for vsyncs within a game
      {
         deletelines = 0;

//TODO:  More randomization

         sensejoy();      // figure out joypad auto-repeat
         joypadmv();      // move

         joyout = 0;      // reset

         if ((joytrg & JOY_RUN) == JOY_RUN) {
            pause();
         }

         fpmcount--;      // is it time to move pice down ?

         if (fpmcount == 0) {

            // check if score exceeds threshold to increase difficulty
            if (strcmp(scoreval, diff_level[levelval].score) >= 0) {
               levelval++;
               frampermov = diff_level[levelval].vsyncs;
            }

            fpmcount = frampermov;     // reset down-counter

            // move pirce downward (if possible)
            if (chkmvok(piecenum, phasenum, pieceposx, pieceposy, 0, 1) == 0) {
               pieceposy++;
            }
            else {
               // transfer to background
               snapshot(piecenum, phasenum, pieceposx, pieceposy);

               // check if any part is still in the 'hidden' area at the top
               // if so, "game over"
               if (pieceposy < FIELDHIDHT) {     // are any of the current piece's squares in the hidden area ?
                  game_over();                   // yes, it's game_over
                  break;
               }
               else {
                  testlines();     // delete complete lines & add score
                  nxtpiece();      // set next piece
               }
            }
         }

         setsprvars();

         display_score();
         disp_playfield();

         vsync(0);
      }
   }

   return 0;
}

void pause(void)
{
int palette = 0;

   disp_blank_playfield();

   print_text(VDC0, PAUSEMSGX, PAUSEMSGY, palette, pausemsg, 5);

   wait_joypad_run();

   disp_playfield();
}

void game_over(void)
{
int palette = 0;

   print_text(VDC0, GAMOVRMSGX, GAMOVRMSGY, palette, gameovermsg1, 4);
   print_text(VDC0, GAMOVRMSGX, GAMOVRMSGY+1, palette, gameovermsg2, 4);

   wait_joypad_run();
}

void sensejoy(void)
{
int temppad;

   temppad = joypad & JOYRPTMASK;

   if (temppad == joyrptval) {
      if (joyfrminit >= JOYRPTINIT) {     // initial wait period is done
         if (joyfrmsubs >= JOYRPTSUBS) {  // is it time to repeat ?
            joyout = joyrptval;
	    joyfrmsubs = 0;
         }
         else {
            joyfrmsubs++;
         }
      }
      else {
         // wait for initial period
         joyfrminit++;
	 joyfrmsubs = 0;
      }

      
   } else {
      // different
      joyout     = temppad;   // output keys
      joyrptval  = temppad;   // keep for later (repeat validation)
      joyfrminit = 0;         // init counters
      joyfrmsubs = 0;
   }
}

void joypadmv(void)
{
int tempphase;
int rotatex;
int rotatey;

   if ((joyout & JOY_LEFT) == JOY_LEFT)
      if (chkmvok(piecenum, phasenum, pieceposx, pieceposy, -1, 0) == 0)
         pieceposx--;

   if ((joyout & JOY_RIGHT) == JOY_RIGHT)
      if (chkmvok(piecenum, phasenum, pieceposx, pieceposy, 1, 0) == 0)
         pieceposx++;

//   if ((joytrg & JOY_UP) == JOY_UP)
//      if (chkmvok(piecenum, phasenum, pieceposx, pieceposy, 0, -1) == 0)
//         pieceposy--;

   if ((joyout & JOY_DOWN) == JOY_DOWN) {
      if (chkmvok(piecenum, phasenum, pieceposx, pieceposy, 0, 1) == 0)
         pieceposy++;
   }

   if ((joyout & JOY_I) == JOY_I) {
      tempphase = ((phasenum + 1) & 3);
      rotatex = (piecetbl[(int)piecenum] + tempphase)->sprite_x_rotate_adjustment;
      rotatey = (piecetbl[(int)piecenum] + tempphase)->sprite_y_rotate_adjustment;

      if (chkmvok(piecenum, tempphase, pieceposx, pieceposy, rotatex, rotatey) == 0) {
         phasenum   = tempphase;
         pieceposx += rotatex;
         pieceposy += rotatey;
      }
   }

   if ((joyout & JOY_II) == JOY_II) {
      tempphase = ((phasenum + 3) & 3);
      rotatex = (piecetbl[(int)piecenum] + tempphase)->sprite_x_rotate_adjustment;
      rotatey = (piecetbl[(int)piecenum] + tempphase)->sprite_y_rotate_adjustment;

      if (chkmvok(piecenum, tempphase, pieceposx, pieceposy, rotatex, rotatey) == 0) {
         phasenum   = tempphase;
         pieceposx += rotatex;
         pieceposy += rotatey;
      }
   }

//   if ((joytrg & JOY_III) == JOY_III) {
//      if (piecenum == 6)
//         piecenum = 0;
//      else
//         piecenum++;
//   }
//
//   if ((joytrg & JOY_IV) == JOY_IV) {
//      if (piecenum == 0)
//         piecenum = 6;
//      else
//         piecenum--;
//   }
}

void nxtpiece(void)
{
// actually, this should get a random number from 0 to 6
   piecenum++;

   if (piecenum > 6)
      piecenum = 0;
   setpiece();
}

void setpiece(void)
{
   phasenum = 0;
   pieceposy = FIELDHIDHT - (piecetbl[(int)piecenum] + phasenum)->height;
   pieceposx = (FIELDWIDTH - (piecetbl[(int)piecenum] + phasenum)->width) >> 1;
   setsprvars();
}

int chkmvok(int type, int phase, int xpos, int ypos, int xdelta, int ydelta)
{
int i, xoffset, yoffset;
int flag;

   flag = 0;

   for (i = 0; i < 4; i++) {
      xoffset = (piecetbl[(int)type] + phase)->square[i].x;
      yoffset = (piecetbl[(int)type] + phase)->square[i].y;

      // Check whether movement would put it out of bounds:
      // 
      if ( ((xpos + xdelta + xoffset) < 0) ||
           ((xpos + xdelta + xoffset) >= FIELDWIDTH) || 
           ((ypos + ydelta + yoffset) < 0) ||
           ((ypos + ydelta + yoffset) >= (FIELDHEIGHT + FIELDHIDHT)) )
      {
         flag = 1;
	 break;
      }

      // Check whether movement would have it collide with terrain:
      // 
      if ((displn[ypos + ydelta + yoffset][xpos + xdelta + xoffset]) != 0) {
         flag = 1;
	 break;
      }
   }
   return(flag);
}

void snapshot(int type, int phase, int xpos, int ypos)
{
int i, xdelta, ydelta;

   for (i = 0; i < 4; i++) {
      xdelta = (piecetbl[(int)type] + phase)->square[i].x;
      ydelta = (piecetbl[(int)type] + phase)->square[i].y;
      displn[ypos + ydelta][xpos + xdelta] = (type + 1);
   }
}

void testlines(void)
{
int i, j, k;
int flg;

   for (i = (FIELDHEIGHT+FIELDHIDHT - 1); i > 0; i--) {
      flg = 0;

      for (j = 0; j < FIELDWIDTH; j++) {
         if (displn[i][j] == 0)
            flg = 1;
      }

      if (flg != 1) {
         for (k = i; k > 0; k--) {
            for (j = 0; j < FIELDWIDTH; j++) {
               displn[k][j] = displn[k-1][j];
            }
         }
         deletelines++;
	 i = i + 1;  // the last line will need to be rechecked
      }
   }
   while (deletelines > 0) {
      scoreval[4]++;
      deletelines--;
      for (i = 4; i >= 0; i--) {
         if (scoreval[i] > '9') {
            scoreval[i] = scoreval[i] - 10;
            scoreval[i-1]++;
         }
      }
   }
}	

void setsprvars(void)
{
int patterncode;
int patternctrl;
int blockptnctrl;


   patterncode = SPRITE_PATTERN((piecetbl[(int)piecenum] + phasenum)->sprpattern_vram_addr);
   patternctrl = (SPRITE_Y_HEIGHT_2 | SPRITE_X_WIDTH_2 | SPRITE_PRIO_SP | (piecenum+1) );

   blockptnctrl = (SPRITE_Y_HEIGHT_2 | SPRITE_X_WIDTH_2 | SPRITE_PRIO_BG | 1 );  // palette doesn't actually matter

   eris_sup_set(VDC0);

// set up sprite 1 as the "invisible block":
//
   eris_sup_spr_set(1);
   eris_sup_spr_create((pieceposx * 8) + FLD_SPRXORG, FLD_SPRYORG, SPRITE_PATTERN(SPR_P7PH0VRAM), blockptnctrl);
   
// set up sprite 2 as the "falling block":
//
   eris_sup_spr_set(2);
   eris_sup_spr_create((pieceposx * 8) + FLD_SPRXORG, (pieceposy * 8) + FLD_SPRYORG, patterncode, patternctrl);
}

void dispbkgnd(void)
{
int x, y;

   eris_low_sup_set_vram_write(VDC0, 0);

   for (y = 0; y < BGMAPHEIGHT; y++)
   {
      for (x = 0; x < (BGMAPWIDTH>>1); x++)
      {
         if ((y & 1) ==  0)   // alternating lines
         {
            eris_low_sup_vram_write(VDC0, bkchr1.ref);
            eris_low_sup_vram_write(VDC0, bkchr2.ref);
         } else {
            eris_low_sup_vram_write(VDC0, bkchr2.ref);
            eris_low_sup_vram_write(VDC0, bkchr1.ref);
         }
      }
   }
}

void clear_display_field(void)
{
int i, j;

   for (i = 0; i < (FIELDHEIGHT + FIELDHIDHT); i++) {
      for (j = 0; j < FIELDWIDTH; j++) {
         displn[i][j] = 0;
      }
   }
}

void disp_blank_playfield(void)
{
int i, j;
int addr;

   for (i = FIELDHIDHT; i < (FIELDHIDHT+FIELDHEIGHT); i++)
   {
      addr = ((i + FIELDY) * BGMAPWIDTH) + FIELDX;

      eris_low_sup_set_vram_write(VDC0, addr);

      for (j = 0; j < FIELDWIDTH; j++)
      {
         eris_low_sup_vram_write(VDC0, offchr.ref);
      }
   }

// Move sprite 2 off screen:
//
   eris_sup_set(VDC0);
   eris_sup_spr_set(2);
   eris_sup_spr_xy(0,0);
}

void disp_playfield(void)
{
int i, j;
int addr;

   for (i = FIELDHIDHT; i < (FIELDHIDHT+FIELDHEIGHT); i++)
   {
      addr = ((i + FIELDY) * BGMAPWIDTH) + FIELDX;

      eris_low_sup_set_vram_write(VDC0, addr);

      for (j = 0; j < FIELDWIDTH; j++)
      {
         if (displn[i][j] == 0) {
           eris_low_sup_vram_write(VDC0, offchr.ref);
         }
	 else {
           eris_low_sup_vram_write(VDC0, (fullchr.ref | (displn[i][j] << 12)) );
         }
      }
   }
}

void display_score(void)
{
int x;
char letter;
uint16_t fontref = 0;
int palette = 0;

   eris_low_sup_set_vram_write(VDC0, (SCOREPOSY*BGMAPWIDTH + SCOREPOSX));

   for (x = 0; x < 7; x++)
   {
      letter = *(scoremsg + x);
      if (letter == 0)
         break;

      fontref = ((((CG_FONTLOC) >> 4) + letter)  | (palette << 12));
      eris_low_sup_vram_write(VDC0, fontref);
   }

   for (x = 0; x < 6; x++)
   {
      letter = *(scoreval + x);
      if (letter == 0)
         break;

      fontref = ((((CG_FONTLOC) >> 4) + letter)  | (palette << 12));
      eris_low_sup_vram_write(VDC0, fontref);
   }
}

void print_text(VDCNUM vdc, int x_pos, int y_pos, int palette, char *mesg, int maxlen)
{
int x;
char letter;
uint16_t fontref = 0;

   eris_low_sup_set_vram_write(vdc, (y_pos*BGMAPWIDTH + x_pos));
   for (x = 0; x < maxlen; x++)
   {
      letter = *(mesg + x);
      if (letter == 0)
         break;

      fontref = ((((CG_FONTLOC) >> 4) + letter)  | (palette << 12));
      eris_low_sup_vram_write(vdc, fontref);
   }
}

void init_score(void)
{
   strcpy(scoreval, "00000");
   scoreval[5] = 0x00;
}

void wait_joypad_run(void)
{
   vsync(1);

   while (1)
   {
      vsync(0);

      if ((joytrg & JOY_RUN) == JOY_RUN)
         break;
   }
}


void init(void)
{
int i, j;

//   u32 str[256];
   u16 microprog[16];
   u16 a, img;

   eris_sup_init(0, 1);
//   eris_low_sup_init(0);
   eris_low_sup_init(1);
   eris_king_init();
   eris_tetsu_init();
	
   eris_tetsu_set_priorities(0, 0, 1, 0, 0, 0, 0);
//   eris_tetsu_set_7up_palette(0, 0x100);
   eris_tetsu_set_7up_palette(0, 0);
   eris_tetsu_set_king_palette(0, 0, 0, 0);
   eris_tetsu_set_rainbow_palette(0);

   eris_king_set_bg_prio(KING_BGPRIO_3, KING_BGPRIO_HIDE, KING_BGPRIO_HIDE, KING_BGPRIO_HIDE, 0);
   eris_king_set_bg_mode(KING_BGMODE_4_PAL, 0, 0, 0);
   eris_king_set_kram_pages(0, 0, 0, 0);

   for(i = 0; i < 16; i++) {
      microprog[i] = KING_CODE_NOP;
   }

   microprog[0] = KING_CODE_BG0_CG_0;
   eris_king_disable_microprogram();
   eris_king_write_microprogram(microprog, 0, 16);
   eris_king_enable_microprogram();


   // Set up palette entries
   //
   for (i = 0; i < 128; i++) {
      eris_tetsu_set_palette(i, CG_palette[i]);
      eris_tetsu_set_palette(i+256, SPR_palette[i]);
   }

//   eris_tetsu_set_video_mode(TETSU_LINES_262, 0, TETSU_DOTCLOCK_7MHz, TETSU_COLORS_16,
   eris_tetsu_set_video_mode(TETSU_LINES_262, 0, TETSU_DOTCLOCK_5MHz, TETSU_COLORS_16,
                             TETSU_COLORS_16, 1, 1, 0, 0, 0, 0, 0);
   eris_king_set_bat_cg_addr(KING_BG0, 0, 0);
   eris_king_set_bat_cg_addr(KING_BG0SUB, 0, 0);
   eris_king_set_scroll(KING_BG0, 0, 0);
   eris_king_set_bg_size(KING_BG0, KING_BGSIZE_256, KING_BGSIZE_256, KING_BGSIZE_256, KING_BGSIZE_256);

   eris_low_sup_set_control(0, 0, 1, 1);

   eris_low_sup_set_access_width(0, 0, SUP_LOW_MAP_64X32, 0, 0);
   eris_low_sup_set_scroll(0, 0, 0);
   eris_low_sup_set_video_mode(0, 2, 2, 4, 0x1F, 0x11, 2, 239, 2); // 5MHz numbers
//   eris_low_sup_set_video_mode(0, 3, 3, 6, 0x2B, 0x11, 2, 239, 2); // 7MHz numbers

   eris_king_set_kram_read(0, 1);
   eris_king_set_kram_write(0, 1);

   // Clear BG0's RAM
   for(i = 0; i < 0x1E00; i++) {
      eris_king_kram_write(0);
   }
   eris_king_set_kram_write(0, 1);

   //
   // load font into video memory
   // font background/foreground should be subpalettes #0 and #3 respectively
   //
   eris_low_sup_set_vram_write(0, 0x1200);

   for(i = 0; i < 0x60; i++) {
      // first 2 planes of color
      for (j = 0; j < 8; j++) {
         img = font[(i*8)+j] & 0xff;
         a = (img << 8) | img;
         eris_low_sup_vram_write(0, a);
      }
      // last 2 planes of color
      for (j = 0; j < 8; j++) {
         eris_low_sup_vram_write(0, 0);
      }
   }

   // load CG background gfx into VRAM
   //
   load_vram(VDC0, offchr.data,    offchr.vidaddr,    offchr.size);
   load_vram(VDC0, bkchr1.data,    bkchr1.vidaddr,    bkchr1.size);
   load_vram(VDC0, bkchr2.data,    bkchr2.vidaddr,    bkchr2.size);
   load_vram(VDC0, cornerchr.data, cornerchr.vidaddr, cornerchr.size);
   load_vram(VDC0, endchr.data,    endchr.vidaddr,    endchr.size);
   load_vram(VDC0, bottomchr.data, bottomchr.vidaddr, bottomchr.size);
   load_vram(VDC0, fullchr.data,   fullchr.vidaddr,   fullchr.size);

   load_vram(VDC0, p0ph0_data,     SPR_P0PH0VRAM,     sizeof(p0ph0_data));
   load_vram(VDC0, p0ph1_data,     SPR_P0PH1VRAM,     sizeof(p0ph1_data));
   load_vram(VDC0, p0ph2_data,     SPR_P0PH2VRAM,     sizeof(p0ph2_data));
   load_vram(VDC0, p0ph3_data,     SPR_P0PH3VRAM,     sizeof(p0ph3_data));

   load_vram(VDC0, p1ph0_data,     SPR_P1PH0VRAM,     sizeof(p1ph0_data));
   load_vram(VDC0, p1ph1_data,     SPR_P1PH1VRAM,     sizeof(p1ph1_data));
   load_vram(VDC0, p1ph2_data,     SPR_P1PH2VRAM,     sizeof(p1ph2_data));
   load_vram(VDC0, p1ph3_data,     SPR_P1PH3VRAM,     sizeof(p1ph3_data));

   load_vram(VDC0, p2ph0_data,     SPR_P2PH0VRAM,     sizeof(p2ph0_data));
   load_vram(VDC0, p2ph1_data,     SPR_P2PH1VRAM,     sizeof(p2ph1_data));
   load_vram(VDC0, p2ph2_data,     SPR_P2PH2VRAM,     sizeof(p2ph2_data));
   load_vram(VDC0, p2ph3_data,     SPR_P2PH3VRAM,     sizeof(p2ph3_data));

   load_vram(VDC0, p3ph0_data,     SPR_P3PH0VRAM,     sizeof(p3ph0_data));
   load_vram(VDC0, p3ph1_data,     SPR_P3PH1VRAM,     sizeof(p3ph1_data));

   load_vram(VDC0, p4ph0_data,     SPR_P4PH0VRAM,     sizeof(p4ph0_data));
   load_vram(VDC0, p4ph1_data,     SPR_P4PH1VRAM,     sizeof(p4ph1_data));

   load_vram(VDC0, p5ph0_data,     SPR_P5PH0VRAM,     sizeof(p5ph0_data));
   load_vram(VDC0, p5ph1_data,     SPR_P5PH1VRAM,     sizeof(p5ph1_data));

   load_vram(VDC0, p6ph0_data,     SPR_P6PH0VRAM,     sizeof(p6ph0_data));

   load_vram(VDC0, p7ph0_data,     SPR_P7PH0VRAM,     sizeof(p7ph0_data));


   //
   //
   eris_pad_init(0); // initialize joypad


   // Disable all interrupts before changing handlers.
   irq_set_mask(0x7F);

   // Replace firmware IRQ handlers for the Timer and HuC6270-A.
   //
   // This liberis function uses the V810's hardware IRQ numbering,
   // see FXGA_GA and FXGABOAD documents for more info ...
   irq_set_raw_handler(0xC, my_vblank_irq);

   // Enable Timer and HuC6270-A interrupts.
   //
   // d6=Timer
   // d5=External
   // d4=KeyPad
   // d3=HuC6270-A
   // d2=HuC6272
   // d1=HuC6270-B
   // d0=HuC6273
   irq_set_mask(0x77);

   // Allow all IRQs.
   //
   // This liberis function uses the V810's hardware IRQ numbering,
   // see FXGA_GA and FXGABOAD documents for more info ...
   irq_set_level(8);

   // Enable V810 CPU's interrupt handling.
   irq_enable();

   eris_low_sup_setreg(VDC0, 5, 0xC8);  // Set Hu6270 BG to show, and VSYNC Interrupt

   eris_bkupmem_set_access(1,1);
}


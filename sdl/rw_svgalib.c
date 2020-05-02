/*
** RW_SVGALBI.C
**
** This file contains ALL SDL (Stm32 port) specific stuff having to do with the
** software refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** SWimp_EndFrame
** SWimp_Init
** SWimp_InitGraphics
** SWimp_SetPalette
** SWimp_Shutdown
** SWimp_SwitchFullscreen
*/

#include <stdarg.h>

#include "../ref_soft/r_local.h"
#include "../client/keys.h"
#include "../linux/rw_linux.h"

#include "sdl_video.h"
#include <lcd_main.h>
#include <bsp_sys.h>

/*****************************************************************************/

#define BASEWIDTH (320)
#define BASEHEIGHT (240)

#define pal_t uint32_t
#define pix_t uint8_t

typedef struct {
    int width, height;
    int bytesperpixel;
    int colors;
    int linewidth;
} vga_modeinfo;

static SDL_Surface *screen = NULL;
static pix_t screenbuf[BASEWIDTH * BASEHEIGHT * sizeof(pix_t) + sizeof(SDL_Surface)] = {0};

int		VGA_width, VGA_height, VGA_rowbytes, VGA_bufferrowbytes, VGA_planar;
byte	*VGA_pagebase;
char	*framebuffer_ptr;

void VGA_UpdatePlanarScreen (void *srcbuffer);

int num_modes;
vga_modeinfo *modes;
int current_mode;

// Console variables that we need to access from this module

/*****************************************************************************/

void VID_PreConfig (void)
{
    screen_conf_t conf;
    int hwaccel = 0, p;

    p = bsp_argv_check("-gfxmod");
    if (p >= 0) {
        const char *str = bsp_argv_get(p);
        hwaccel = atoi(str);
    }

    conf.res_x = BASEWIDTH;
    conf.res_y = BASEHEIGHT;
    conf.alloc.malloc = heap_alloc_shared;
    conf.alloc.free = heap_free;
    conf.colormode = GFX_COLOR_MODE_CLUT;
    conf.laynum = 2;
    conf.hwaccel = hwaccel;
    conf.clockpresc = 1;
    vid_config(&conf);
}

void VID_PreInit (void)
{
        screen = (SDL_Surface *)&screenbuf[0];

        // Set video width, height and flags

        d_memset(screen, 0, sizeof(SDL_Surface));
        screen->pixels = (void *)(screen + 1);
        screen->flags = (SDL_SWSURFACE|SDL_HWPALETTE|SDL_FULLSCREEN);
        screen->w = BASEWIDTH;
        screen->h = BASEHEIGHT;
        screen->offset = 0;
        screen->pitch = BASEWIDTH;
}

void VID_SetPalette (byte* palette)
{
    unsigned int i;
    pal_t pal[256];
    byte r, g, b;

    for (i = 0; i < 256; i++)
    {
        r = *palette++;
        g = *palette++;
        b = *palette++;
        pal[i] = GFX_RGBA8888(r, g, b, 0xff);
    }
    vid_set_clut(pal, 256);
    return;
}

void    VID_ShiftPalette (unsigned char *palette)
{
    VID_SetPalette(palette);
}

static inline int vga_getcolors (void)
{
    return 256;
}

static inline int vga_oktowrite (void)
{
    return 1;
}

static inline void vga_setmode (int i)
{
}

static inline void vga_setpage (int i)
{
}

static inline void *vga_getgraphmem (void)
{
    return screen->pixels;
}

static inline void vga_init (void)
{
}

vga_modeinfo supmodes[] =
{
    {320, 240, 1, 256, 320},
    {0, 0, 0, 0, 0},
};

static inline int vga_lastmodenumber (void)
{
    return arrlen(supmodes) - 1;
}

static inline int vga_hasmode (int i)
{
    if (i >= arrlen(supmodes)) {
        return 0;
    }
    if (!supmodes[i].width || !supmodes[i].height) {
        return 0;
    }
    return 1;
}

static inline vga_modeinfo *vga_getmodeinfo (int i)
{
    return &supmodes[i];
}

void VID_InitModes(void)
{

    int i;

	// get complete information on all modes

	num_modes = vga_lastmodenumber()+1;
	modes = heap_malloc(num_modes * sizeof(vga_modeinfo));
	for (i=0 ; i<num_modes ; i++)
	{
		if (vga_hasmode(i))
			d_memcpy(&modes[i], vga_getmodeinfo(i), sizeof (vga_modeinfo));
		else
			modes[i].width = 0; // means not available
	}

	// filter for modes i don't support

	for (i=0 ; i<num_modes ; i++)
	{
		if (modes[i].bytesperpixel != 1 && modes[i].colors != 256) 
			modes[i].width = 0;
	}

	for (i = 0; i < num_modes; i++)
		if (modes[i].width)
			ri.Con_Printf(PRINT_ALL, "mode %d: %d %d\n", modes[i].width, modes[i].height);

    VID_PreInit();

}

/*
** SWimp_Init
**
** This routine is responsible for initializing the implementation
** specific stuff in a software rendering subsystem.
*/
int SWimp_Init( void *hInstance, void *wndProc )
{
	vga_init();

	VID_InitModes();

	return true;
}

int get_mode(int width, int height)
{

	int i;
	int ok, match;

	for (i=0 ; i<num_modes ; i++)
		if (modes[i].width &&
			modes[i].width == width && modes[i].height == height)
				break;
	if (i==num_modes)
		return -1; // not found

	return i;
}

/*
** SWimp_InitGraphics
**
** This initializes the software refresh's implementation specific
** graphics subsystem.  In the case of Windows it creates DIB or
** DDRAW surfaces.
**
** The necessary width and height parameters are grabbed from
** vid.width and vid.height.
*/
static qboolean SWimp_InitGraphics( qboolean fullscreen )
{
	int bsize, zsize, tsize;

	SWimp_Shutdown();

	current_mode = get_mode(vid.width, vid.height);

	if (current_mode < 0) {
		ri.Con_Printf (PRINT_ALL, "Mode %d %d not found\n", vid.width, vid.height);
		return false; // mode not found
	}

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (vid.width, vid.height);

	ri.Con_Printf (PRINT_ALL, "Setting VGAMode: %d\n", current_mode );

//	Cvar_SetValue ("vid_mode", (float)modenum);
	
	VGA_width = modes[current_mode].width;
	VGA_height = modes[current_mode].height;
	VGA_planar = modes[current_mode].bytesperpixel == 0;
	VGA_rowbytes = modes[current_mode].linewidth;

	vid.rowbytes = modes[current_mode].linewidth;

	if (VGA_planar) {
		VGA_bufferrowbytes = modes[current_mode].linewidth * 4;
		vid.rowbytes = modes[current_mode].linewidth*4;
	}

// get goin'

	vga_setmode(current_mode);

	VGA_pagebase = framebuffer_ptr = (char *) vga_getgraphmem();
//		if (vga_setlinearaddressing()>0)
//			framebuffer_ptr = (char *) vga_getgraphmem();
	if (!framebuffer_ptr)
		Sys_Error("This mode isn't hapnin'\n");

	vga_setpage(0);

    /* TODO : */
	vid.buffer = VGA_pagebase;
	if (!vid.buffer)
		Sys_Error("Unabled to alloc vid.buffer!\n");

	return true;
}

/*
** SWimp_EndFrame
**
** This does an implementation specific copy from the backbuffer to the
** front buffer.  In the Win32 case it uses BitBlt or BltFast depending
** on whether we're using DIB sections/GDI or DDRAW.
*/
void SWimp_EndFrame (void)
{
    screen_t lcd_screen = {0};
    lcd_screen.buf = vid.buffer;
    lcd_screen.width = vid.width;
    lcd_screen.height = vid.height;

	if (!vga_oktowrite())
		return; // can't update screen if it's not active

	vid_update(&lcd_screen);
}

/*
** SWimp_SetMode
*/
rserr_t SWimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	rserr_t retval = rserr_ok;

	ri.Con_Printf (PRINT_ALL, "setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( pwidth, pheight, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return rserr_invalid_mode;
	}

	ri.Con_Printf( PRINT_ALL, " %d %d\n", *pwidth, *pheight);

	if ( !SWimp_InitGraphics( false ) ) {
		// failed to set a valid mode in windowed mode
		return rserr_invalid_mode;
	}

	R_GammaCorrectAndSetPalette( ( const unsigned char * ) d_8to24table );

	return retval;
}

/*
** SWimp_SetPalette
**
** System specific palette setting routine.  A NULL palette means
** to use the existing palette.  The palette is expected to be in
** a padded 4-byte xRGB format.
*/
void SWimp_SetPalette( const unsigned char *palette )
{
	static char tmppal[256*3];
	const unsigned char *pal;
	unsigned char *tp;
	int i;

    if ( !palette )
        palette = ( const unsigned char * ) sw_state.currentpalette;
 
	if (vga_getcolors() == 256)
	{
		tp = tmppal;
		pal = palette;

		for (i=0 ; i < 256 ; i++, pal += 4, tp += 3) {
			tp[0] = pal[0];
			tp[1] = pal[1];
			tp[2] = pal[2];
		}

		if (vga_oktowrite())
			VID_ShiftPalette(tmppal);
	}

}

/*
** SWimp_Shutdown
**
** System specific graphics subsystem shutdown routine.  Destroys
** DIBs or DDRAW surfaces as appropriate.
*/
void SWimp_Shutdown( void )
{
    vid.buffer = NULL;
    vga_setmode(-1);
}

/*
** SWimp_AppActivate
*/
void SWimp_AppActivate( qboolean active )
{
}

//===============================================================================

/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}


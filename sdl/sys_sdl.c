/* -*- Mode: C; tab-width: 4 -*- */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#ifndef __WIN32__

#endif

#include "g_local.h"
#include "SDL_keysym.h"
#include "audio_main.h"
#include "input_main.h"
#include "main.h"
#include <misc_utils.h>
#include <dev_io.h>
#include <debug.h>
#include <bsp_cmd.h>
#include <bsp_sys.h>
#include "begin_code.h"

qboolean        isDedicated;

int noconinput = 0;

char *basedir = ".";
char *cachedir = "/tmp";

static int con_dbglvl = 1;

// =======================================================================
// General routines
// =======================================================================

void Sys_DebugNumber(int y, int val)
{
}

void Sys_Printf (char *fmt, ...)
{
    va_list         argptr;

    va_start (argptr, fmt);
    if (con_dbglvl) {
        dvprintf (fmt, argptr);
    }
    va_end (argptr);
}

void Sys_Quit (void)
{
	assert(0);
	for (;;) {}
}

void Sys_Init(void)
{
#if id386
	Sys_SetFPCW();
#endif
    cmd_register_i32(&con_dbglvl, "dbglvl");
}

void SDL_Quit(void)
{
    cmd_execute("reset", 0);
    assert(0);
}

#if !id386

/*
================
Sys_LowFPPrecision
================
*/
void Sys_LowFPPrecision (void)
{
// causes weird problems on Nextstep
}


/*
================
Sys_HighFPPrecision
================
*/
void Sys_HighFPPrecision (void)
{
// causes weird problems on Nextstep
}

#endif	// !id386


void Sys_Error (char *error, ...)
{
    va_list         argptr;

    va_start (argptr, error);
    dvprintf (error, argptr);
    va_end (argptr);

    serial_flush();
    Sys_Quit();
} 

void Sys_Warn (char *warning, ...)
{ 
    va_list         argptr;

    va_start (argptr, warning);
    dvprintf (warning, argptr);
    va_end (argptr);
}

void Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list         argptr;
    
    va_start (argptr, fmt);
    dvprintf (fmt, argptr);
    va_end (argptr);
}

#if 0
DECLSPEC char * SDLCALL SDL_GetError(void)
{
    return "not implemented yet\n";
}

DECLSPEC void SDLCALL SDL_ClearError(void)
{}

DECLSPEC SDLMod SDLCALL SDL_GetModState(void)
{
    return KMOD_NONE;
}

DECLSPEC uint8_t SDLCALL SDL_GetMouseState(int *x, int *y)
{
    return 0;
}
#endif

/*
===============================================================================

FILE IO

===============================================================================
*/
int Sys_FileOpenRead (char *path, int *hndl)
{
    return d_open(path, hndl, "r");
}

int Sys_FileOpenWrite (char *path)
{
    int h;
    d_open(path, &h, "+w");
    return h;
}

void Sys_FileClose (int handle)
{
    d_close(handle);
}

void Sys_FileSeek (int handle, int position)
{
    d_seek(handle, position, DSEEK_SET);
}

int Sys_Feof (int handle)
{
    return d_eof(handle);
}

int Sys_FileRead (int handle, void *dst, int count)
{
    return d_read(handle, dst, count);
}

char *Sys_FileGetS (int handle, char *dst, int count)
{
    return d_gets(handle, dst, count);
}

char Sys_FileGetC (int handle)
{
    return d_getc(handle);
}

int Sys_FileWrite (int handle, void *src, int count)
{
    return d_write(handle, src, count);
}

int Sys_FPrintf (int handle, char *fmt, ...)
{
    va_list ap;
    char p[256];
    int   r;

    va_start (ap, fmt);
    r = vsnprintf(p, sizeof(p), fmt, ap);
    va_end (ap);
    if (Sys_FileWrite(handle, p, r) < 0) {
        dprintf("%s Bad : %s\n", __func__, p);
    }
    return r;
}

int	Sys_FileTime (char *path)
{
    int f, time;

    d_open(path, &f, "r");
    if (f < 0) {
        return -1;
    }
    time = d_time();
    d_close(f);
    return time;
}

void Sys_mkdir (char *path)
{
    d_mkdir(path);
}

double Sys_FloatTime (void)
{
#ifdef __WIN32__

	static int starttime = 0;

	if ( ! starttime )
		starttime = clock();

	return (clock()-starttime)*1.0/1024;

#else
    return d_time();
#endif
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

static volatile int oktogo;

void alarm_handler(int x)
{
	oktogo=1;
}

byte *Sys_ZoneBase (int *size)
{

    Sys_Error("Not supported");
    return NULL;
}

void Sys_LineRefresh(void)
{
}

void Sys_Sleep(uint32_t ms)
{
    volatile static uint32_t time = 0;

    time = d_time() + ms;
    while (time > d_time()) {}
}

void floating_point_exception_handler(int whatever)
{
    Sys_Error("floating point exception\n");
}

void moncontrol(int x)
{
}

extern void bsp_tickle (void);
extern int g_profile_per;

#if 0
int SDL_main (int argc, const char *argv[])
{

    double  time, oldtime, newtime;
    quakeparms_t parms;
    extern int vcrFile;
    extern int recording;
    static int frame;
    int sfxparm;

    moncontrol(0);

    parms.memsize = heap_avail() - (1024 * 32);
    parms.membase = heap_alloc_shared(parms.memsize);
    parms.basedir = basedir;
    // Disable cache, else it looks in the cache for config.cfg.
    parms.cachedir = NULL;

    COM_InitArgv(argc, (char **)argv);
    parms.argc = com_argc;
    parms.argv = com_argv;

    Sys_Init();

    Host_Init(&parms);

    Cvar_RegisterVariable (&sys_nostdout);

    {
        char cfg[128];
        const char *volume = "64";
        sfxparm = COM_CheckParm("-vol");

        if (sfxparm) {
            volume = com_argv[sfxparm + 1];
        }
        snprintf(cfg, sizeof(cfg), "samplerate=11025, volume=%s", volume);
        audio_conf(cfg);
    }

    oldtime = Sys_FloatTime () - 0.1;
    while (1)
    {
// find time spent rendering last frame
        newtime = Sys_FloatTime ();
        time = newtime - oldtime;

        if (cls.state == ca_dedicated)
        {   // play vcrfiles at max speed
            if (time < sys_ticrate.value && (vcrFile == -1 || recording) )
            {
                Sys_Sleep(1);
                continue;       // not time to run a server only tic yet
            }
            time = sys_ticrate.value;
        }

        if (time > sys_ticrate.value*2)
            oldtime = newtime;
        else
            oldtime += time;

        if (++frame > 10)
            moncontrol(1);      // profile only while we do each Quake frame
        Host_Frame (time);
        moncontrol(0);

// graphic debugging aids
        if (sys_linerefresh.value)
            Sys_LineRefresh ();

        if (g_profile_per > 0) {
            cmd_execute("profile", sizeof("profile") - 1);
            g_profile_per--;
        }
        bsp_tickle();
    }

}

#endif

static uint8_t *syscache = NULL;
static uint8_t *syscache_top = NULL,
            *syscache_bot = NULL;

static uint32_t syscahce_size = (1024 * 700); /*~700 Kb*/

void Sys_CacheInit (void)
{
    syscache = heap_malloc(syscahce_size);
    assert(syscache);
    syscache_top = syscache + syscahce_size;
    syscache_bot = syscache;
}

void *Sys_HeapCacheTop (int size) /*alloc forever*/
{
    if (syscahce_size < size) {
        return NULL;
    }
    syscahce_size = syscahce_size - size;
    syscache_top = syscache_top - size;
    return (void *)syscache_top;
}

void Sys_HeapCachePush (int size) /*dealloc*/
{
    syscache_bot = syscache_bot - size;
    syscahce_size = syscahce_size + size;
}

extern void *Sys_HeapCachePop (int size) /*alloc*/
{
    void *ptr;
    if (syscahce_size < size) {
        return NULL;
    }
    ptr = syscache_bot;
    syscache_bot = syscache_bot + size;
    syscahce_size = syscahce_size - size;
    return (void *)ptr;
}

unsigned	sys_frame_time;

int curtime;
int Sys_Milliseconds (void)
{
    curtime = d_time();
    return curtime;
}

void Sys_AppActivate (void)
{
}

char *Sys_GetClipboardData(void)
{
	return NULL;
}

static	char	findbase[MAX_OSPATH];
static	char	findpath[MAX_OSPATH];
static	char	findpattern[MAX_OSPATH];
static	int fdir;

void Sys_FindClose (void)
{
	if (fdir >= 0)
		d_closedir(fdir);
	fdir = -1;
}

char *Sys_FindFirst (char *path, unsigned musthave, unsigned canhave)
{
	fobj_t d;
	char *p;

	if (fdir >= 0)
		Sys_Error ("Sys_BeginFind without close");

//	COM_FilePath (path, findbase);
	strcpy(findbase, path);

	if ((p = strrchr(findbase, '/')) != NULL) {
		*p = 0;
		strcpy(findpattern, p + 1);
	} else
		strcpy(findpattern, "*");

	if (strcmp(findpattern, "*.*") == 0)
		strcpy(findpattern, "*");
	
	if ((fdir = d_opendir(findbase)) == NULL)
		return NULL;
	while (d_readdir(fdir, &d) >= 0) {
		if (!*findpattern || glob_match(findpattern, d.name)) {
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d.name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d.name);
				return findpath;
			}
		}
	}
	return NULL;
}

char *Sys_FindNext (unsigned musthave, unsigned canhave)
{
	fobj_t d;

	if (fdir < 0)
		return NULL;
	while (d_readdir(fdir, &d) >= 0) {
		if (!*findpattern || glob_match(findpattern, d.name)) {
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d.name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d.name);
				return findpath;
			}
		}
	}
	return NULL;
}

void Sys_Mkdir (char *path)
{
    d_mkdir (path);
}

//
// Memory
//

byte *membase;
int maxhunksize;
int curhunksize;

void *Hunk_Begin (int maxsize)
{
	maxhunksize = maxsize + sizeof(int);
	curhunksize = 0;
/* 	membase = mmap(0, maxhunksize, PROT_READ|PROT_WRITE,  */
/* 		MAP_PRIVATE, -1, 0); */
/* 	if ((membase == NULL) || (membase == MAP_FAILED)) */
	membase = heap_malloc(maxhunksize);
	if (membase == NULL)
		Com_Error(ERR_FATAL, "unable to virtual allocate %d bytes", maxsize);

	*((int *)membase) = curhunksize;

	return membase + sizeof(int);
}

void *Hunk_Alloc (int size)
{
	byte *buf;

	// round to cacheline
	size = (size+31)&~31;
	if (curhunksize + size > maxhunksize)
		Com_Error(ERR_FATAL, "Hunk_Alloc overflow");
	buf = membase + sizeof(int) + curhunksize;
	curhunksize += size;
	return buf;
}

int Hunk_End (void)
{
	return curhunksize;
}

void Hunk_Free (void *base)
{
	byte *m;

	if (base) {
		m = ((byte *)base) - sizeof(int);
		heap_free(m);
	}
}


// =======================================================================
// General routines
// =======================================================================

void Sys_ConsoleOutput (char *string)
{
	dprintf("%s", string);
}

#if	id386
/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
	int r;
	unsigned long addr;
	int psize = getpagesize();

	fprintf(stderr, "writable code %lx-%lx\n", startaddr, startaddr+length);

	addr = startaddr & ~(psize-1);

	r = mprotect((char*)addr, length + startaddr - addr, 7);

	if (r < 0)
    		Sys_Error("Protection change failed\n");
}

#endif /*id386*/

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	return GetGameAPI (parms);
}

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
}

qboolean CompareAttributes(char *path, char *name,
	unsigned musthave, unsigned canthave )
{
#if 0
	struct stat st;
	char fn[MAX_OSPATH];

// . and .. never match
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return false;

	sprintf(fn, "%s/%s", path, name);
	if (stat(fn, &st) == -1)
		return false; // shouldn't happen

	if ( ( st.st_mode & S_IFDIR ) && ( canthave & SFF_SUBDIR ) )
		return false;

	if ( ( musthave & SFF_SUBDIR ) && !( st.st_mode & S_IFDIR ) )
		return false;

	return true;
#endif
}

static int glob_match_after_star(char *pattern, char *text);

/* Match the pattern PATTERN against the string TEXT;
   return 1 if it matches, 0 otherwise.

   A match means the entire string TEXT is used up in matching.

   In the pattern string, `*' matches any sequence of characters,
   `?' matches any character, [SET] matches any character in the specified set,
   [!SET] matches any character not in the specified set.

   A set is composed of characters or ranges; a range looks like
   character hyphen character (as in 0-9 or A-Z).
   [0-9a-zA-Z_] is the set of characters allowed in C identifiers.
   Any other character in the pattern must be matched exactly.

   To suppress the special syntactic significance of any of `[]*?!-\',
   and match the character exactly, precede it with a `\'.
*/

int glob_match(char *pattern, char *text)
{
	register char *p = pattern, *t = text;
	register char c;

	while ((c = *p++) != '\0')
		switch (c) {
		case '?':
			if (*t == '\0')
				return 0;
			else
				++t;
			break;

		case '\\':
			if (*p++ != *t++)
				return 0;
			break;

		case '*':
			return glob_match_after_star(p, t);

		case '[':
			{
				register char c1 = *t++;
				int invert;

				if (!c1)
					return (0);

				invert = ((*p == '!') || (*p == '^'));
				if (invert)
					p++;

				c = *p++;
				while (1) {
					register char cstart = c, cend = c;

					if (c == '\\') {
						cstart = *p++;
						cend = cstart;
					}
					if (c == '\0')
						return 0;

					c = *p++;
					if (c == '-' && *p != ']') {
						cend = *p++;
						if (cend == '\\')
							cend = *p++;
						if (cend == '\0')
							return 0;
						c = *p++;
					}
					if (c1 >= cstart && c1 <= cend)
						goto match;
					if (c == ']')
						break;
				}
				if (!invert)
					return 0;
				break;

			  match:
				/* Skip the rest of the [...] construct that already matched.  */
				while (c != ']') {
					if (c == '\0')
						return 0;
					c = *p++;
					if (c == '\0')
						return 0;
					else if (c == '\\')
						++p;
				}
				if (invert)
					return 0;
				break;
			}

		default:
			if (c != *t++)
				return 0;
		}

	return *t == '\0';
}

/* Like glob_match, but match PATTERN against any final segment of TEXT.  */
static int glob_match_after_star(char *pattern, char *text)
{
	register char *p = pattern, *t = text;
	register char c, c1;

	while ((c = *p++) == '?' || c == '*')
		if (c == '?' && *t++ == '\0')
			return 0;

	if (c == '\0')
		return 1;

	if (c == '\\')
		c1 = *p;
	else
		c1 = c;

	while (1) {
		if ((c == '[' || *t == c1) && glob_match(p - 1, t))
			return 1;
		if (*t++ == '\0')
			return 0;
	}
}

/* Return nonzero if PATTERN has any special globbing chars in it.  */
static int glob_pattern_p(char *pattern)
{
	register char *p = pattern;
	register char c;
	int open = 0;

	while ((c = *p++) != '\0')
		switch (c) {
		case '?':
		case '*':
			return 1;

		case '[':		/* Only accept an open brace if there is a close */
			open++;		/* brace to match it.  Bracket expressions must be */
			continue;	/* complete, according to Posix.2 */
		case ']':
			if (open)
				return 1;
			continue;

		case '\\':
			if (*p++ == '\0')
				return 0;
		}

	return 0;
}




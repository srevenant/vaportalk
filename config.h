/* This source code is in the public domain. */

/* config.h: Allow for system-dependent optimization */

/* Comment these out to use library functions */
#define strcspn	    vtstrcspn
#define strcasecmp  vtstricmp
#define strncasecmp vtstrnicmp
#define strstr	    vtstrstr
#define memset	    vtmemset

/* Uncomment these to use library functions */
/* #define bcopy_fwd(x, y, z) memmove(y, x, z) */
/* #define rand random */
/* #define srand srandom */

/* If tolower('a') == 'a' and toupper('A') == 'A' */
/* #define ANSI_CTYPES */


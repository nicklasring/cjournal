#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h> 
#include <X11/Xatom.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <gdk/gdk.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Journal struct */
struct Journal  {
	const char * journal_entry;
};
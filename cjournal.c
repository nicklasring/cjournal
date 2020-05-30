#include "cjournal.h"

/* 
 Grabs surface of Gdk root window with given coords and generates screen capture
 */
void region_to_png( int rw, int rh, int rx, int ry ) {
	gdk_init(0, 0);

	GdkWindow *root_win = gdk_get_default_root_window();
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rw, rh);
	GdkPixbuf *pb = gdk_pixbuf_get_from_window(root_win, rx,ry,rw,rh);

	cairo_t *cr = cairo_create(surface);        
	gdk_cairo_set_source_pixbuf(cr, pb, 0, 0);  
	cairo_paint(cr);

	cairo_surface_write_to_png(surface, "./image.png");

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

int region_capture() {
	int rx = 0, ry = 0, rw = 0, rh = 0;
	int rect_x = 0, rect_y = 0, rect_w = 0, rect_h = 0;
	int btn_pressed = 0, done = 0;

	XEvent ev;
	Display *disp = XOpenDisplay(NULL);

	if(!disp)
		return EXIT_FAILURE;

	Screen *scr = NULL;
	scr = ScreenOfDisplay(disp, DefaultScreen(disp));

	Window root = 0;
	root = RootWindow(disp, XScreenNumberOfScreen(scr));

	Cursor cursor, cursor2;
	cursor = XCreateFontCursor(disp, XC_left_ptr);
	cursor2 = XCreateFontCursor(disp, XC_lr_angle);

	XGCValues gcval;
	gcval.foreground = XWhitePixel(disp, 0);
	gcval.function = GXxor;
	gcval.background = XBlackPixel(disp, 0);
	gcval.plane_mask = gcval.background ^ gcval.foreground;
	gcval.subwindow_mode = IncludeInferiors;

	GC gc;
	gc = XCreateGC(disp, root,
					GCFunction | GCForeground | GCBackground | GCSubwindowMode,
					&gcval);

	/* this XGrab* stuff makes XPending true ? */
	if ((XGrabPointer
		(disp, root, False,
			ButtonMotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync,
			GrabModeAsync, root, cursor, CurrentTime) != GrabSuccess))
		printf("couldn't grab pointer:");

	if ((XGrabKeyboard
		(disp, root, False, GrabModeAsync, GrabModeAsync,
			CurrentTime) != GrabSuccess))
		printf("couldn't grab keyboard:");

	while (!done) {
		while (!done && XPending(disp)) {
			XNextEvent(disp, &ev);
			switch (ev.type) {
				case MotionNotify:
					// this case is purely for drawing rect on screen 
					if (btn_pressed) {
						if (rect_w) {
							// re-draw the last rect to clear it
							XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
						} else {
							// Change the cursor to show we're selecting a region
							XChangeActivePointerGrab(disp,
								ButtonMotionMask | ButtonReleaseMask,
								cursor2, CurrentTime);
						}

						rect_x = rx;
						rect_y = ry;
						rect_w = ev.xmotion.x - rect_x;
						rect_h = ev.xmotion.y - rect_y;

						if (rect_w < 0) {
							rect_x += rect_w;
							rect_w = 0 - rect_w;
						}
						if (rect_h < 0) {
							rect_y += rect_h;
							rect_h = 0 - rect_h;
						}
						/* draw rectangle */
						XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
						XFlush(disp);
					}
					break;
				case ButtonPress:
					btn_pressed = 1;
					rx = ev.xbutton.x;
					ry = ev.xbutton.y;
					break;
				case ButtonRelease:
					done = 1;
					break;
			}
		}
	}
		
	/* clear the drawn rectangle */
	if (rect_w) {
		XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
		XFlush(disp);
	}

	rw = ev.xbutton.x - rx;
	rh = ev.xbutton.y - ry;

	/* cursor moves backwards */
	if (rw < 0) {
		rx += rw;
		rw = 0 - rw;
	}
	if (rh < 0) {
		ry += rh;
		rh = 0 - rh;
	}

	XCloseDisplay(disp);

	// Save region to file
	region_to_png( rw, rh, rx, ry );

	return EXIT_SUCCESS;
}

char * get_current_date() {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	char ymdh[14];
	const char * date_format = "%d-%02d-%02d_%02d";
	
	snprintf( ymdh, sizeof(ymdh), date_format, 
		tm.tm_year + 1900, 
		tm.tm_mon + 1, 
		tm.tm_mday,
		tm.tm_hour
	);

	// Allocate memory on heap for date and return pointer
	char * date_heap_alloc = malloc( strlen(ymdh) + 1 ); // Nullterminator + 1
	if( date_heap_alloc ) {
		memcpy( date_heap_alloc, ymdh, strlen(ymdh) + 1 );
	}
	return date_heap_alloc;
}

char * get_current_user() {
	char user_name[64] = {0};
	if( getlogin_r(user_name, sizeof(user_name)-1) != 0 ) {
		printf("Error fetching current user");
		exit(1);
	}

	// Allocate memory on heap for user_name and return pointer
	char * username_heap_alloc = malloc( strlen(user_name) + 1 ); // Nullterminator + 1
	if( username_heap_alloc ) {
		memcpy( username_heap_alloc, user_name, strlen(user_name) + 1 );
	}

	return username_heap_alloc;
}

int r_create_directory( char * directory ) {
	struct stat st = {0};
	if( stat(directory, &st) == -1 ) {
		mkdir(directory, 0700);
		return 0;
	}

	return 1;
}

/*
 Creates todays active journal folder
 Returns path
 */
char * get_journal_folder() {
	char * current_user = get_current_user();
	char * current_date = get_current_date();

	char journal_folder[128];

	snprintf( journal_folder, sizeof(journal_folder), "/home/%s/.cjournals",
		current_user
	);

	if( r_create_directory( journal_folder ) != 0 ) {
		printf( "Unable to create directory: %s\n", journal_folder );
	}

	snprintf( journal_folder, sizeof(journal_folder), "/home/%s/.cjournals/%s",
		current_user,
		current_date
	);

	// Free the pointers from memory
	free(current_user);
	free(current_date);

	// Allocate memory on heap for journal_folder and return pointer
	char * journal_folder_heap_alloc = malloc( strlen(journal_folder) + 1 ); // Nullterminator + 1
	if( journal_folder_heap_alloc ) {
		memcpy( journal_folder_heap_alloc, journal_folder, strlen(journal_folder) + 1 );
	}

	return journal_folder_heap_alloc;
}

struct Journal parse_input_arguments( int argc, char *argv[] ) {

	struct Journal journal;

	if( argc > 1 ) {
		// Check for journal entry flag
		char journal_flag[3] = "-w\0";
		if( strcmp(journal_flag,argv[1]) == 0 ) {
			journal.journal_entry = argv[2];
			return journal;
		}
	}
	journal.journal_entry = "\0";
	return journal;
}

void write_journal( char * directory, const char* journal_entry ) {
	FILE * f = NULL;
	const char * mode = "w\0";
	char journal_entry_file[128];

	snprintf( journal_entry_file, sizeof(journal_entry_file), "%s%s",
		directory,
		"/1.txt" // Txt files existing +1
	);

	printf("%s", journal_entry_file);

	f = fopen( journal_entry_file, mode );
	fprintf(f, journal_entry);
	fclose(f);
} 

/* TODO:
	arguments:
		./scjournal -w "Text"
		Generates screenshot region capture
		Opens vim to enter journal message (Like git commits)
		Saves screenshot with date in .journal/2020-01-01/1.png
		Saves Journal in .journal/2020-01-01/1.txt

		./scjournal
		Simply allows for screenshot capture
		Saves screenshot with date in .journal/2020-01-01/1.png
*/
int main(int argc, char *argv[]) {

	char * journal_folder = get_journal_folder();
	if( r_create_directory( journal_folder ) == 0 ) {
		printf("Journal directory created\n");
	}

	struct Journal journal = parse_input_arguments( argc, argv );

	if( *journal.journal_entry != 0 ) {
		printf("Adding journal entry: %s\n", journal.journal_entry);
		write_journal( journal_folder, journal.journal_entry );
	}

	free(journal_folder);

	return region_capture( );
}
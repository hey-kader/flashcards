#include 	<stdio.h>
#include 	<stdlib.h>
#include  <string.h>
#include  <time.h>

#include 	<X11/Xlib.h>
#include 	<X11/keysymdef.h>
#include <X11/extensions/Xrender.h>

#define WINDOW_WIDTH    500
#define WINDOW_HEIGHT   500

#define MAX_ENTRIES		 2048

typedef struct STRING {
	char  *value;
	unsigned short length;
} String;

typedef struct PAIR {
	String base;
	String exp;
} Pair;


static short number_of_lines_in_file;

int count_file_lines (const char *filename);
void randomize_pairs (char *pairs[][2], int pairs_length);
void invert_pairs (char *pairs[][2], int pairs_length);

int 
count_file_lines (const char *filename) {
	FILE *file = fopen (filename, "rb");
	short count = 0;
	int c;
	while ((c=fgetc(file)) != EOF) {
		if (c == '\n') {
			++count;
		}
	}
	fclose (file);
	return count;
}

void 
randomize_pairs (char *pairs[][2], int pairs_length) {
	srand(time(NULL));
	printf("shuffling pairs...\n");
	for (int i = pairs_length-1; i > 0 ; --i) {
		int j = rand() % (i + 1);
		char *temp0 = pairs[i][0];
		char *temp1 = pairs[i][1];
		pairs[i][0] = pairs[j][0];
		pairs[i][1] = pairs[j][1];
		pairs[j][0] = temp0;
		pairs[j][1] = temp1;
	}
}

void 
invert_pairs (char *pairs[][2], int pairs_length) {
	printf("inverting pairs...\n");
	for (int i = 0; i < pairs_length; ++i) {
		char *temp = pairs[i][0];
		pairs[i][0] = pairs[i][1];
		pairs[i][1] = temp;
	}
}

int
main (int argc, const char *argv[]) {
	Display *display = XOpenDisplay (
		NULL
	);
	XAutoRepeatOff (
		display
	);
	Screen *screen = DefaultScreenOfDisplay (
		display
	);
	XFontStruct *font_info = XLoadQueryFont (
		display, "fixed"
	);
	XSetFont (
		display, screen->default_gc, font_info->fid
	);
	XSetWindowAttributes attributes = {
		.override_redirect = True,
		.event_mask = (
			ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
		)
	};
	char *pairs[MAX_ENTRIES][2];
	if (argc >= 2) {
		printf("reading index file ...\n");
		number_of_lines_in_file = count_file_lines(argv[1]);
		char *line = NULL;
		ssize_t read;
		ssize_t length_return;
		FILE *file = fopen(argv[1], "rb");
		int nline = 0;
		while ((read = getline(&line, &length_return, file)) > 1 ) {
			char *base = strtok(line, ":");
			char *exp  = strtok(NULL, "");
			exp[strlen(exp)-1] = '\0';
			pairs[nline][0] = strdup(base);
			pairs[nline][1] = strdup(exp);
			++nline;
		}
		printf("number of lines stored: [%d]\n",nline);
		fclose(file);
		randomize_pairs(&pairs, nline);
		if (argc == 3) {
			if (strcmp(argv[2], "-f") == 0) {
				printf("reversal needed\n");
				invert_pairs(&pairs, nline);
			}
			printf("two args\n");
		}
		else {
			printf("no reversal indicated\n");
		}
		
	}
	else {
		printf("needs index file\n");
		exit (argc);
	}
	printf("creating window-\n");
	Window main = XCreateWindow (
		display,
		screen->root,
		(screen->width/2) - (WINDOW_WIDTH/2),
		(screen->height/2) - (WINDOW_HEIGHT/2),
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		1,
		24,
		CopyFromParent,
		screen->root_visual,
		(CWOverrideRedirect | CWEventMask),
		&attributes
	);
	XSelectInput (
		display, main, (
			ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
		)
	);
	XMapWindow (
		display, main
	);
  XSetInputFocus(
    display, main, RevertToNone, CurrentTime
  );
	/* XRender setup */
	XRenderPictFormat *format = XRenderFindStandardFormat (
		display, PictStandardRGB24
	);
	Pixmap back_pixmap = XCreatePixmap (
		display, main, WINDOW_WIDTH, WINDOW_HEIGHT, 24
	);
	XRenderPictureAttributes picture_attributes = {
		.dither = True
	};
	Picture picture = XRenderCreatePicture (
		display, back_pixmap, format, CPDither, &picture_attributes
	);
	XLinearGradient linear_gradient = {
		.p1 = {
			XDoubleToFixed (0.0),
			XDoubleToFixed (0.0)
		},
		.p2 = {
			XDoubleToFixed (0.0),
			XDoubleToFixed (WINDOW_HEIGHT)
		}
	};
	XFixed stops[] = {
		XDoubleToFixed (0.0),
		XDoubleToFixed (1.0)
	};
	XColor xcolor[2];
	XAllocNamedColor (
		display, screen->cmap, "red",
		&xcolor[0], &xcolor[0]
	);
	XAllocNamedColor (
		display, screen->cmap, "white",
		&xcolor[1], &xcolor[1]
	);
	XRenderColor colors[2];
	colors[0].red   = xcolor[0].red;
	colors[0].green = xcolor[0].green;
	colors[0].blue  = xcolor[0].blue;
	colors[0].alpha = 0xFFFF;
	colors[1].red = xcolor[1].red;
	colors[1].green = xcolor[1].green;
	colors[1].blue = xcolor[1].blue;
	colors[1].alpha = 0xFFFF;
	Picture gradient = XRenderCreateLinearGradient (
		display, &linear_gradient, stops, colors, 2
	);
	XRenderComposite (
		display, 
		PictOpSrc, 
		gradient, 
		None,
		picture, 
		0, 0, 
		0, 0, 
		0, 0, 
		WINDOW_WIDTH, 
		WINDOW_HEIGHT
	);
	XSetWindowBackgroundPixmap (
		display, main, back_pixmap
	);
	/* XRender end setup */

	int index = 0;
	int state = 0;
	int RUNNING = True;
  int DRAGGING = False;
  int start_x = 0;
  int start_y = 0;
	XEvent event;
	while (RUNNING) {
		XNextEvent (
			display,
			&event
		);
		XClearWindow (
			display, main
		);
		switch (event.type) {
			default:
				printf("unknown event type\n");
			break;
			case Expose:
				XSetForeground (
					display, screen->default_gc, screen->black_pixel
				);
				XClearArea(
					display, main, 0, 0, 10, 10, True
				);
				char buffer[20];
				sprintf(buffer, "[%d/%d]", index+1, number_of_lines_in_file);
				XDrawString(
					display, main, screen->default_gc, 10, 20, buffer, strlen(buffer)
				);
				int x = 0;
				int y = 0;
				if (state == 0) {
					x = XTextWidth (
						font_info, pairs[index][0], strlen(pairs[index][0])
					);
					x = (WINDOW_WIDTH/2)-(x/2);
					y = (WINDOW_WIDTH/2) - (20);
					XDrawString (
						display, main, screen->default_gc, x, y, pairs[index][0], strlen(pairs[index][0])
					);
				}
				else {
					x = XTextWidth (
						font_info, pairs[index][1], strlen(pairs[index][1])
					);
					x = (WINDOW_WIDTH/2)-(x/2);
					y = (WINDOW_WIDTH/2) - (20);
					XDrawString (
						display, main, screen->default_gc, x, y, pairs[index][1], strlen(pairs[index][1])
					);
				}
				XSync(display, False);
			break;
			case KeyPress:
				if (event.xkey.keycode == 131) {
					if (index > 0) {
						--index;	
					}
					else {
						index = number_of_lines_in_file-1;
					}
				}
				else if (event.xkey.keycode == 132) {
					if (index < number_of_lines_in_file-1) {
						++index;
					}
					else {
						index = 0;
					}
				}
				else if (event.xkey.keycode == 133 || event.xkey.keycode == 134) {
					state = 1;
				}
			break;
			case KeyRelease:
				if (state == 1) {
					state = 0;
				}
			break;
      case ButtonPress:
        XRaiseWindow (
          display, main
        );
        if (DRAGGING == False) {
          DRAGGING = True;
          start_x = event.xbutton.x;
          start_y = event.xbutton.y;
        }
      break;
      case ButtonRelease:
        if (DRAGGING == True) {
          DRAGGING = False;
        }
      break;
      case MotionNotify:
        if (DRAGGING == True) {
          int x = event.xmotion.x_root - start_x;
          int y = event.xmotion.y_root - start_y;
          XMoveWindow (
            display, main, x, y
          );
        }
      break;
		}
	}
	XCloseDisplay (
		display
	);
	return --argc;
}


/**  Puzzle
 **
 ** Don Bennett, HP Labs 
 ** 
 ** this is the interface code for the puzzle program.
 **/

#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "ac.cursor"
#include "ac_mask"

#define max(x,y)	(x>y?x:y)
#define min(x,y)	(x>y?y:x)

#define PUZZLE_BORDER_WIDTH	2

#define TITLE_WINDOW_HEIGHT	25
#define BOUNDARY_HEIGHT		3

#define BOX_WIDTH		10
#define BOX_HEIGHT		10

#define MIN_TILE_HEIGHT		25
#define MIN_TILE_WIDTH		25

#define MAX_STEPS		1000

int 	BoxWidth  =	BOX_WIDTH;
int	BoxHeight =	BOX_HEIGHT;

int	PuzzleSize = 4;
int	PuzzleWidth=4, PuzzleHeight=4;

int	TileHeight, TileWidth;
int	Tx, Ty;
int     TitleWinHeight, BoundaryHeight, TileWinHeight;

int	FgPixel, BgPixel;

Display 	*dpy;
int		screen;
GC		gc, rect_gc;
Colormap	PuzzleColormap;

typedef struct {
    Window	root;
    int		x,y;
    u_int	width, height;
    u_int	border_width;
    u_int	depth;
} WindowGeom;

WindowGeom	PuzzleWinInfo;

Window 		PuzzleRoot, TitleWindow, TileWindow,
    		ScrambleWindow, SolveWindow;

char		*ProgName;

char		*TitleFontName    = "8x13";
char		*TileFontName     = "vtbold";

XFontStruct	*TitleFontInfo,
		*TileFontInfo;

extern int	OutputLogging;
extern int	*position;
extern int	space_x, space_y;

int	UsePicture = 0;
int	UseDisplay = 0;
char	*PictureFileName;

int	PictureWidth;
int	PictureHeight;
Pixmap	PicturePixmap;

int	MoveSteps;
int	VertStepSize[MAX_STEPS];
int	HoriStepSize[MAX_STEPS];

#define LEFT	0
#define RIGHT	1
#define UP	2
#define	DOWN	3

#define indx(x,y)	(((y)*PuzzleWidth) + (x))
#define isdigit(x)	((x)>= '0' && (x) <= '9')

#define ulx(x,y)	((x)*TileWidth)
#define llx(x,y)	((x)*TileWidth)
#define urx(x,y)	(((x)+1)*TileWidth - 1)
#define lrx(x,y)	(((x)+1)*TileWidth - 1)
#define uly(x,y)	((y)*TileHeight)
#define ury(x,y)	((y)*TileHeight)
#define lly(x,y)	(((y)+1)*TileHeight - 1)
#define lry(x,y)	(((y)+1)*TileHeight - 1)

/*
 * PuzzlePending - XPending entry point fo the other module.
 */

PuzzlePending()
{
    return(XPending(dpy));
}

/*
 * SetupDisplay - eastablish the connection to the X server.
 */

SetupDisplay(server)
char *server;
{
    dpy = XOpenDisplay(server);
    if (dpy == NULL) {
	fprintf(stderr, "SetupDisplay: can't open display \"%s\"\n",server);
	exit(1);
    } 
    screen = DefaultScreen(dpy);
}

XQueryWindow(window,frame)
Window window;
WindowGeom *frame;
{
    XGetGeometry(dpy, window,
		 &(frame->root),
		 &(frame->x), &(frame->y),
		 &(frame->width), &(frame->height),
		 &(frame->border_width),
		 &(frame->depth));
}

RectSet(W,x,y,w,h,pixel)
Window W;
int x,y;
u_int w,h;
u_long pixel;
{
    XSetForeground(dpy, rect_gc, pixel);
    XFillRectangle(dpy, W, rect_gc, x, y, w, h);
}

MoveArea(W,src_x,src_y,dst_x,dst_y,w,h)
Window W;
int src_x, src_y, dst_x, dst_y;
u_int w, h;
{
    XCopyArea(dpy,W,W,gc,src_x,src_y,w,h,dst_x,dst_y);
}

/** RepaintTitle - puts the program title in the title bar **/

RepaintTitle()
{
    int Twidth,Theight, Box_x,Box_y;

    Twidth  = XTextWidth(TitleFontInfo,ProgName,strlen(ProgName));
    Theight = TitleFontInfo->ascent + TitleFontInfo->descent;
    Tx	    = (PuzzleWinInfo.width-Twidth)/2;
    Ty	    = (TitleWinHeight-Theight)/2 + TitleFontInfo->ascent;
    
    XSetFont(dpy, gc, TitleFontInfo->fid);
    XDrawString(dpy, TitleWindow, gc,
	  Tx, Ty, ProgName,strlen(ProgName));
}

/*
 * RepaintBar - Repaint the bar between the title window and
 *              the tile window;
 */
RepaintBar()
{
    XFillRectangle(dpy, PuzzleRoot, gc,
		   0, TitleWinHeight,
		   PuzzleWinInfo.width, BoundaryHeight);
}

/**
 ** RepaintTiles - draw the numbers in the tiles to match the
 **                locations array;
 **/
RepaintTiles()
{
   if (UsePicture)
      RepaintPictureTiles();
   else
      RepaintNumberTiles();
}

RepaintNumberTiles()
{
    int i,j,counter;
    int width,height;
    int x_offset,y_offset;
    char str[30];
    
    /** cut the TileWindow into a grid of nxn pieces by inscribing
     ** each rectangle with a black border;
     ** I don't want to use subwindows for each tile so that I can
     ** slide groups of tiles together as a single unit, rather than
     ** being forced to move one tile at a time.
     **/

#define line(x1,y1,x2,y2) XDrawLine(dpy,TileWindow,gc,(x1),(y1),(x2),(y2))

#define rect(x,y)	(line(ulx(x,y),uly(x,y),urx(x,y),ury(x,y)),	\
			 line(urx(x,y),ury(x,y),lrx(x,y),lry(x,y)),	\
			 line(lrx(x,y),lry(x,y),llx(x,y),lly(x,y)),	\
			 line(llx(x,y),lly(x,y),ulx(x,y),uly(x,y)))


    for (i=0; i<PuzzleHeight;i++)	/** iterate y values **/
	for(j=0; j<PuzzleWidth; j++) {  	/** iterate x values **/
	    RectSet(TileWindow,ulx(j,i),uly(j,i),TileWidth,TileHeight,BgPixel);
	    rect(j,i);
	}
    height = TileFontInfo->ascent + TileFontInfo->descent;
    y_offset = (TileHeight - height)/2;

    XSetFont(dpy, gc, TileFontInfo->fid);

    counter = 0;
    for (i=0; i<PuzzleHeight; i++)
	for (j=0; j<PuzzleWidth; j++) {
	    if (position[counter] == 0) {
		RectSet(TileWindow,ulx(j,i),uly(j,i),
			TileWidth,TileHeight,FgPixel);
	    }
	    else {
		sprintf(str,"%d",position[counter]);
		width = XTextWidth(TileFontInfo,str,strlen(str));
		x_offset = (TileWidth - width)/2;
		XDrawString(dpy, TileWindow, gc,
			    ulx(j,i)+x_offset,uly(j,i)+y_offset,
			    str,strlen(str));
	    }
	    counter++;
	}    
}

RepaintPictureTiles()
{
    int i, j, counter;
    int tmp, orig_x,orig_y;

    counter = 0;
    for (i=0; i<PuzzleHeight; i++)
	for (j=0; j<PuzzleWidth; j++) {
	    if (position[counter] == 0)
		RectSet(TileWindow,ulx(j,i),uly(j,i),
			TileWidth,TileHeight,FgPixel);
	    else {
		tmp = position[counter] - 1;
		orig_x = tmp % PuzzleWidth;
		orig_y = tmp / PuzzleWidth;

		XCopyArea(dpy,PicturePixmap,TileWindow,gc,
			  ulx(orig_x,orig_y), uly(orig_x,orig_y),
			  TileWidth, TileHeight,
			  ulx(j,i), uly(j,i));
	    }
	    counter++;
	}    
    
}

/**
 ** Setup - Perform initial window creation, etc.
 **/

Setup(server,geom,argc,argv)
char *server,*geom;
int argc;
char *argv[];
{
    Cursor ArrowCrossCursor;
    int minwidth, minheight;
    Pixmap VEGsetup();
    Visual visual;
    XGCValues xgcv;
    XSetWindowAttributes xswa;
    XSizeHints sizehints;

    /*******************************************/
    /** let the puzzle code initialize itself **/
    /*******************************************/
    initialize();
    OutputLogging = 1;

    SetupDisplay(server);

    FgPixel = BlackPixel(dpy,screen);
    BgPixel = WhitePixel(dpy,screen);

    /*****************************************************/
    /** if we want to use a picture file, initialize it **/
    /*****************************************************/
    if (UsePicture) {
#ifdef UNDEFINED
	/**
	 ** This was fun to do back with X10 when you could create
	 ** a pixmap from the current display contents;  Maybe eventually
	 ** I'll do the same with X11.
	 **/
	if (UseDisplay) {
	    WindowGeom RootWinInfo;
	    int x,y;

	    x = PUZZLE_BORDER_WIDTH;
	    y = TITLE_WINDOW_HEIGHT + BOUNDARY_HEIGHT + PUZZLE_BORDER_WIDTH;
	    XQueryWindow(RootWindow(dpy, screen),&RootWinInfo);
	    PictureWidth  = RootWinInfo.width  - x;
	    PictureHeight = RootWinInfo.height - y;
	    PicturePixmap = XPixmapSave(RootWindow(dpy,screen),
					x,y,PictureWidth,PictureHeight);
	}
	else
#endif UNDEFINED
	    PuzzleColormap = XCreateColormap(dpy,RootWindow(dpy,screen),
					 DefaultVisual(dpy,screen),AllocNone);
	    PicturePixmap = VEGsetup(PictureFileName,&PictureWidth,&PictureHeight);
    }

    if (UsePicture) {
	minwidth = PuzzleWidth;
	minheight = PuzzleHeight + TITLE_WINDOW_HEIGHT + BOUNDARY_HEIGHT;
    }
    else {
	minwidth = MIN_TILE_WIDTH * PuzzleWidth;
	minheight = MIN_TILE_HEIGHT * PuzzleHeight + TITLE_WINDOW_HEIGHT +
	    BOUNDARY_HEIGHT;
    }

    /*************************************/
    /** configure the window size hints **/
    /*************************************/

    {
	int x, y, width, height;
	int flags;

	sizehints.flags = PMinSize | PPosition | PSize;
	sizehints.min_width = minwidth;
	sizehints.min_height = minheight;
	sizehints.width = minwidth;
	sizehints.height = minheight;
	sizehints.x = 100;
	sizehints.y = 300;

	if(strlen(geom)) {
	    flags = XParseGeometry(geom, &x, &y, &width, &height);
	    if(WidthValue & flags) {
		sizehints.flags |= USSize;
		if (width > sizehints.min_width)
		    sizehints.width = width;
	    }
	    if(HeightValue & flags) {
		sizehints.flags |= USSize;
		if (height > sizehints.min_height)
		    sizehints.height = height;
	    }
	    if(XValue & flags) {
		if(XNegative & flags)
		    x = DisplayWidth(dpy, DefaultScreen(dpy)) + x 
			- sizehints.width;
		sizehints.flags |= USPosition;
		sizehints.x = x;
	    }
	    if(YValue & flags) {
		if(YNegative & flags)
		    y = DisplayHeight(dpy, DefaultScreen(dpy)) + y
			-sizehints.height;
		sizehints.flags |= USPosition;
		sizehints.y = y;
	    }
	}
    }

    /*******************************************************************/
    /** create the puzzle main window and set its standard properties **/
    /*******************************************************************/

    xswa.event_mask = ExposureMask;
    visual.visualid = CopyFromParent;

#ifdef UNDEFINED
    PuzzleRoot = XCreateWindow(dpy, RootWindow(dpy,screen),
			       sizehints.x, sizehints.y,
			       sizehints.width, sizehints.height,
			       PUZZLE_BORDER_WIDTH,
			       DefaultDepth(dpy,screen),
			       InputOutput,
			       &visual,
			       CWEventMask, &xswa);
#endif UNDEFINED
    PuzzleRoot = XCreateSimpleWindow(dpy, RootWindow(dpy,screen),
			       sizehints.x, sizehints.y,
			       sizehints.width, sizehints.height,
			       PUZZLE_BORDER_WIDTH, FgPixel,BgPixel);

    XSetStandardProperties(dpy, PuzzleRoot,"puzzle","Puzzle",
			   None, argv, argc, &sizehints);

    xgcv.foreground = FgPixel;
    xgcv.background = BgPixel;
    xgcv.line_width = 1;
    gc = XCreateGC(dpy, PuzzleRoot,
		   GCForeground|GCBackground|GCLineWidth,
		   &xgcv);

#ifdef UNDEFINED
    /*********************************/
    /** load the arrow-cross cursor **/
    /*********************************/
    
    /** This code really works, but I haven't converted the cursor to the
     ** the new format, so it looks pretty mangled;
     **/
    {
	Pixmap ACPixmap, ACMask;
	Cursor ACCursor;
	XColor FGcolor, BGcolor;

	FGcolor.red = 0;	FGcolor.green = 0;	FGcolor.blue = 0;
	BGcolor.red = 0xffff;	BGcolor.green = 0xffff;	BGcolor.blue = 0xffff;

	ACPixmap = XCreateBitmapFromData(dpy,RootWindow(dpy,screen),
				 arrow_cross_bits,
				 arrow_cross_width, arrow_cross_height);
	ACMask = XCreateBitmapFromData(dpy,RootWindow(dpy,screen),
				 arrow_cross_mask_bits,
				 arrow_cross_width, arrow_cross_height);
	ACCursor = XCreatePixmapCursor(dpy,ACPixmap,ACMask,
				       FGcolor,BGcolor,
				       8,8);
	if (ACCursor == NULL)
	    error("Unable to store ArrowCrossCursor.");
    
	XDefineCursor(dpy,PuzzleRoot,ACCursor);
    }
#endif UNDEFINED

    /*****************************************/
    /** allocate the fonts we will be using **/
    /*****************************************/

    TitleFontInfo    = XLoadQueryFont(dpy,TitleFontName);
    TileFontInfo     = XLoadQueryFont(dpy,TileFontName);

    if (TitleFontInfo    == NULL) error("Opening title font.\n");
    if (TileFontInfo     == NULL) error("Opening tile font.\n");

    XQueryWindow(PuzzleRoot,&PuzzleWinInfo);
    Reset();
}

static short old_height = -1;
static short old_width = -1;

SizeChanged()
{
    XQueryWindow(PuzzleRoot,&PuzzleWinInfo);
    
    if (PuzzleWinInfo.width == old_width &&
	PuzzleWinInfo.height == old_height)
	return(0);
    else
	return(1);
}

Reset()
{
    int width,height,Box_x,Box_y;
    int i,j;
    int TileBgPixel;
    
    /** TileWindow is that portion of PuzzleRoot that contains
     ** the sliding pieces;
     **/

    if (UsePicture)
	TileBgPixel = BlackPixel(dpy,screen);
    else
	TileBgPixel = WhitePixel(dpy,screen);
    
    XDestroySubwindows(dpy,PuzzleRoot);
    
    old_width  = PuzzleWinInfo.width;
    old_height = PuzzleWinInfo.height;

    TitleWinHeight = TITLE_WINDOW_HEIGHT;
    BoundaryHeight = BOUNDARY_HEIGHT;

    /** fix the dimensions of PuzzleRoot so the height and width
     ** of the TileWindow will work out to be multiples of PuzzleSize;
     **/

    /** If we're dealing with a picture, the tile region can be no larger
     ** than the picture!
     **/

    if (UsePicture) {
	int tmp;

	tmp = PuzzleWinInfo.height - TitleWinHeight - BoundaryHeight;
	if (tmp > PictureHeight)
	    PuzzleWinInfo.height = PictureHeight+TitleWinHeight+BoundaryHeight;
	if (PuzzleWinInfo.width > PictureWidth)
	    PuzzleWinInfo.width = PictureWidth;
    }

    TileHeight=(PuzzleWinInfo.height-TitleWinHeight-BoundaryHeight)/PuzzleHeight;
    PuzzleWinInfo.height = TileHeight*PuzzleHeight+TitleWinHeight+BoundaryHeight;

    TileWidth = PuzzleWinInfo.width/PuzzleWidth;
    PuzzleWinInfo.width = TileWidth * PuzzleWidth;

    /** fixup the size of PuzzleRoot **/

    XResizeWindow(dpy,PuzzleRoot,PuzzleWinInfo.width,PuzzleWinInfo.height);
    TileWinHeight = PuzzleWinInfo.height - TitleWinHeight;

    TitleWindow = XCreateSimpleWindow(dpy, PuzzleRoot,
			0,0,
			PuzzleWinInfo.width, TitleWinHeight,
			0,0,BgPixel);

    TileWindow  = XCreateSimpleWindow(dpy, PuzzleRoot,
			0,TitleWinHeight+BoundaryHeight,
			PuzzleWinInfo.width, TileWinHeight,
			0,0,TileBgPixel);
   
    rect_gc = XCreateGC(dpy,TileWindow,0,0);
    XCopyGC(dpy, gc, -1, rect_gc);

    XMapWindow(dpy,PuzzleRoot);
    XMapWindow(dpy,TitleWindow);
    XMapWindow(dpy,TileWindow);

    RepaintBar();
    RepaintTitle();

    /** locate the two check boxes **/

    Box_x = Tx/2 - BoxWidth/2;
    Box_y = TitleWinHeight/2 - BoxHeight/2;
    
    ScrambleWindow = XCreateSimpleWindow(dpy, TitleWindow,
				 Box_x, Box_y,
				 BoxWidth, BoxHeight,
				 1,FgPixel,BgPixel);

    Box_x = PuzzleWinInfo.width - Box_x - BoxWidth;
    
    SolveWindow = XCreateSimpleWindow(dpy, TitleWindow,
				 Box_x,Box_y,
				 BoxWidth,BoxHeight,
				 1,FgPixel,BgPixel);

    XMapWindow(dpy,ScrambleWindow);
    XMapWindow(dpy,SolveWindow);

    XSelectInput(dpy, TitleWindow,   ButtonPressMask|ExposureMask);
    XSelectInput(dpy, TileWindow,    ButtonPressMask|ExposureMask|
		 			VisibilityChangeMask);
    XSelectInput(dpy, ScrambleWindow,ButtonPressMask|ExposureMask);
    XSelectInput(dpy, SolveWindow,   ButtonPressMask|ExposureMask);

    RepaintTiles();
    CalculateStepsize();
    XSync(dpy,1);
}

CalculateStepsize()
{
    int i, rem;
    int error,sum;

    for (i=0; i<MoveSteps; i++)
	VertStepSize[i] = TileHeight/MoveSteps;
         
    rem = TileHeight % MoveSteps;
    error = - MoveSteps/2;

    if (rem > 0)
	for (i=0; i<MoveSteps; i++) {
	    if (error >= 0) {
		VertStepSize[i]++;
		error -= MoveSteps;
	    }
	    error += rem;
	}   
    
    for (i=0; i<MoveSteps; i++)
	HoriStepSize[i] = TileWidth/MoveSteps;
         
    rem = TileWidth % MoveSteps;
    error = - MoveSteps/2;

    if (rem > 0)
	for (i=0; i<MoveSteps; i++) {
	    if (error >= 0) {
		HoriStepSize[i]++;
		error -= MoveSteps;
	    }
	    error += rem;
	}   

    /** This code is a little screwed up and I don't want to fix it
     ** right now, so just do a little hack to make sure the total
     ** distance comes out right;
     **/

    sum = 0;
    for (i=0; i<MoveSteps; i++)
	sum += HoriStepSize[i];
    HoriStepSize[0] += TileWidth - sum;

    sum = 0;
    for (i=0; i<MoveSteps; i++)
	sum += VertStepSize[i];
    VertStepSize[0] += TileHeight - sum;
}

SlidePieces(event)
XButtonEvent *event;
{
    int x,y;

    x = (*event).x / TileWidth;
    y = (*event).y / TileHeight;
    if (x == space_x || y == space_y)
	move_space_to(indx(x,y));   
}

ProcessButton(event)
XButtonEvent *event;
{
    Window w;

/* printf("state: 0x%-4x\n",event->state); */
    w = event->window;
    if (w == TileWindow) {
	if (SolvingStatus())
	    AbortSolving();
	else
	    SlidePieces(event);
    }
    else if (w == ScrambleWindow) {
	AbortSolving();
	Scramble();
	RepaintTiles();
    }
    else if (w == SolveWindow)
	Solve();
    else if (w == TitleWindow) /*  && (*event).state == MiddleButton) */
	exit(0);
}

ProcessEvents()
{
    XEvent event;

    while(XPending(dpy)) {
	XNextEvent(dpy,&event);  
	ProcessEvent(&event);
    }
}

ProcessInput()
{
    XEvent event;

    while(1) {
	XNextEvent(dpy, &event);  
	ProcessEvent(&event);
    }
}

ProcessEvent(event)
XEvent *event;
{
/* printf("type: %d\n",event->type); */

    switch(event->type) {
      case ButtonPress: ProcessButton(event);
			break;
      case Expose:	if (SizeChanged())
	  		    Reset();
			else {
			    if (event->xany.window == TitleWindow)
				RepaintTitle();
			    else if (event->xany.window == TileWindow)
				RepaintTiles();
			    else if (event->xany.window == PuzzleRoot)
				RepaintBar();
			}
                        break;
#ifdef UNDEFINED
      case Visibility:
			RedrawWindows();
			break;
#endif
   default:		break;
   }
}

main(argc,argv)
int argc;
char *argv[];
{
   int i, count;
   char *ServerName, *Geometry;

   ServerName = "";
   Geometry   = "";  
   MoveSteps  = 1;

   ProgName = argv[0];

   /********************************/
   /** parse command line options **/
   /********************************/

   for (i=1; i<argc; i++) {
      if (argv[i][0] == '=') 
         Geometry = argv[i];
      else if (strchr(argv[i],':') != NULL)
         ServerName = argv[i];
      else if (strchr(argv[i],'x') != NULL) {
         sscanf(argv[i],"%dx%d",&PuzzleWidth,&PuzzleHeight);
         if (PuzzleWidth<4 || PuzzleHeight<4) {
	     printf("Puzzle size must be at least 4x4.\n");
	     exit(1);
	 }
	 PuzzleSize = min((PuzzleWidth/2)*2,(PuzzleHeight/2)*2);
      }
      else if (strncmp(argv[i],"-s",2) == 0) {
         count = sscanf(&(argv[i][2]),"%d",&MoveSteps);
         if (count != 1) usage(ProgName);
         if (MoveSteps > MAX_STEPS) {
           fprintf(stderr,"max steps=%d\n",MAX_STEPS);
           exit(1);
         }
      }
      else if (strncmp(argv[i],"-p",2) == 0) {
	 if (argv[i][2] == 0) {
	    UseDisplay++;
            UsePicture++;
	 }
	 else {
            UsePicture++;
            PictureFileName = &(argv[i][2]);
	 }
      }
      else if (isdigit(argv[i][0])) {
         sscanf(argv[i],"%d",&PuzzleSize);
         if (PuzzleSize<4) {
	     printf("Puzzle size must be at least 4x4.\n");
	     exit(1);
	 }
         PuzzleWidth = PuzzleSize;
	 PuzzleHeight = PuzzleSize;
	 PuzzleSize = (PuzzleSize/2) * 2;
      }
      else 
         usage(ProgName);
   }

   Setup(ServerName,Geometry,argc,argv);
   ProcessInput();
}

usage(name)
char *name;
{
   fprintf(stderr,"usage: %s [geometry] [display] [size]\n", name);
   fprintf(stderr,"       [-s<move-steps>] [-p<picture-file>]\n");
   exit(1);
}

error(str)
char *str;
{
   fprintf(stderr,"Error %s\n",str);
   exit(1);
}

LogMoveSpace(first_x,first_y,last_x,last_y,dir)
int first_x,first_y,last_x,last_y,dir;
{
   int min_x,min_y,max_x,max_y;
   int x,y,w,h,dx,dy,x2,y2;
   int i, clear_x, clear_y;


   max_x = max(first_x,last_x);
   min_x = min(first_x,last_x);
   max_y = max(first_y,last_y);
   min_y = min(first_y,last_y);

   x = ulx(min_x,0);
   y = uly(0,min_y);   
   w = (max_x - min_x + 1)*TileWidth;
   h = (max_y - min_y + 1)*TileHeight;

   dx = x;
   dy = y;

   x2 = x;
   y2 = y;

   switch(dir) {
   case UP:	clear_x = llx(max_x,0);
                clear_y = lly(0,max_y) + 1;

		for (i=0; i<MoveSteps; i++) {
                   dy = VertStepSize[i];
                   y2 = y - dy;
                   clear_y -= dy;
                   
                   MoveArea(TileWindow,x,y,x2,y2,w,h);
                   RectSet(TileWindow,clear_x,clear_y,
		           TileWidth,dy,FgPixel);
                   y -= dy;
                }
		break;
   case DOWN:	clear_x = llx(max_x,0);
		clear_y = uly(0,min_y);

		for (i=0; i<MoveSteps; i++) {
		   dy = VertStepSize[i];
		   y2 = y + dy;

		   MoveArea(TileWindow,x,y,x2,y2,w,h);
                   RectSet(TileWindow,clear_x,clear_y,
		           TileWidth,dy,FgPixel);
		   y += dy;
		   clear_y += dy;
		}
		break;
   case LEFT:	clear_x = urx(max_x,0) + 1;
		clear_y = ury(0,max_y);

		for (i=0; i<MoveSteps; i++) {
		   dx = HoriStepSize[i];
		   x2 = x - dx;
		   clear_x -= dx;

		   MoveArea(TileWindow,x,y,x2,y2,w,h);
                   RectSet(TileWindow,clear_x,clear_y,
		           dx,TileHeight,FgPixel);
		   x -= dx;
		}
		break;
   case RIGHT:	clear_x = ulx(min_x,0);
		clear_y = uly(0,max_y);
		
                for (i=0; i<MoveSteps; i++) {
		   dx = HoriStepSize[i];
		   x2 = x + dx;

		   MoveArea(TileWindow,x,y,x2,y2,w,h);
                   RectSet(TileWindow,clear_x,clear_y,
		           dx,TileHeight,FgPixel);
		   x += dx;
		   clear_x += dx;
		}
		break;
   }

   XFlush(dpy);
}

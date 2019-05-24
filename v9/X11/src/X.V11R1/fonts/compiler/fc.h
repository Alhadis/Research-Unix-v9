#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)>(b)?(a):(b))

typedef struct _GlyphMap {
    char	*bits;
    int		h;
    int		w;
    int		widthBytes;
} GlyphMap;

/*
 * a structure to hold all the pointers to make it easy to pass them all
 * around. Much like the FONT structure in the server.
 */

typedef struct _TempFont {
    FontInfoPtr pFI;
    CharInfoPtr pCI;
    unsigned char *pGlyphs;
    FontPropPtr pFP;
} TempFont; /* not called font since collides with type in X.h */

#ifdef vax

#	define DEFAULTGLPAD 	1;		/* default padding for glyphs */
#	define DEFAULTBITORDER 	LSBFirst;	/* default bitmap bit order */

#else
# ifdef sun

#	define DEFAULTGLPAD 	4;		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst;	/* default bitmap bit order */

# else
#  ifdef apollo

#	define DEFAULTGLPAD 	2;		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst;	/* default bitmap bit order */

#  else
#   ifdef ibm032

#	define DEFAULTGLPAD 	1;		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst;	/* default bitmap bit order */

#   else
#	define DEFAULTGLPAD 	1;		/* default padding for glyphs */
#	define DEFAULTBITORDER 	MSBFirst;	/* default bitmap bit order */
#   	define UNSPECIFIED

#   endif
#  endif
# endif
#endif

#define GLWIDTHBYTESPADDED(bits,nbytes) \
	((nbytes) == 1 ? (((bits)+7)>>3)	/* pad to 1 byte */ \
	:(nbytes) == 2 ? ((((bits)+15)>>3)&~1)	/* pad to 2 bytes */ \
	:(nbytes) == 4 ? ((((bits)+31)>>3)&~3)	/* pad to 4 bytes */ \
	:(nbytes) == 8 ? ((((bits)+63)>>3)&~7)	/* pad to 8 bytes */ \
	: 0)


#include "copyright.h"

/* $Header: Xkeymap.h,v 11.4 87/09/11 08:00:42 toddb Exp $ */
/* Copyright 1985, Massachusetts Institute of Technology */

#define X_KEYMAP_MAGIC 0373  /* magic number which must be first byte
    	    	    	    	of a keymap file (was 0372 in V10) */

/* KeyMap: contains single-byte character bindings, or indications
   that a keycode is actually bound in the extension or runtime table */

typedef short KeyMapEntry [8];
typedef unsigned char KeyMapElt [16];

#define UNBOUND (unsigned char)'\377'
#define EXTENSION_BOUND (unsigned char)'\376'
#define RUNTIME_TABLE_BOUND (unsigned char)'\375'
#define SingleCharBound(c) ((unsigned char)c < (unsigned char)'\375')

/* Extension: contains multiple-byte character bindings from
   the keymap file.  Not modified at runtime.  */

typedef struct {
    unsigned char keycode;
    unsigned char metabits;
    unsigned char length;
    } ExtensionHeader;

#define ExtensionHeaderSize 3
    /* since sizeof (ExtensionHeader) is 4 on some machines, e.g. Sun */

/* macro used to iterate through the extension */
#define NextExtension(this) \
  this = (ExtensionHeader *) ((char *)this + ExtensionHeaderSize + this->length)

/* Compose: contains bindings from sequence of single-byte chars to single- or 
   multiple-byte character. */

typedef struct {
    unsigned char source_length;
    unsigned char result_length;    
    } ComposeHeader;

#define ComposeHeaderSize 2

/* macro used to iterate through the compose sequences */
#define NextCompose(this) \
  this = (ComposeHeader *) ((char *)this + ComposeHeaderSize + \
	 this->source_length + this->result_length)

#define DontCareMetaBits 0377
  /* appears in extension whenever the binding applies to all possible
     combinations of shift/lock/meta/control keys */

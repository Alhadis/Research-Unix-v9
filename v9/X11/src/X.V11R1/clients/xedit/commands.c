#ifndef lint
static char rcs_id[] = "$Header: commands.c,v 1.9 87/09/11 08:22:06 toddb Exp $";
#endif

/*
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT RIGHTS,
 * APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN ADDITION TO THAT
 * SET FORTH ABOVE.
 *
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting documentation,
 * and that the name of Digital Equipment Corporation not be used in advertising
 * or publicity pertaining to distribution of the software without specific, 
 * written prior permission.
 */

#include "xedit.h"
#include <sys/types.h>
#include <sys/stat.h>

static int loadChangeNumber;
static int quitChangeNumber;

DoQuit()
{
    if((lastChangeNumber == PSchanges(source)) ||
       (quitChangeNumber == PSchanges(source))){
        exit();
    } else {    
        XeditPrintf("\nUnsaved changes. Save them, or press Quit again.");
        Feep();
        quitChangeNumber = PSchanges(source);
        return;
    }
}

static int searchEndPos = 999;
static int searchBegPos = 999;
static int FileMode = 0640;
ReplaceOne()
{
  int searchlen = strlen(searchstring);
  int startpos  = QXtTextGetInsertionPoint(CurDpy, textwindow);
  int  count, result;
  XtTextPosition pos, destpos;
  char *buf;
  XtTextBlock t, *text;
    text = &t;
     if((startpos != searchEndPos) && (startpos != searchBegPos)){
	return(0);
    }
    buf = malloc(searchlen);
    count = searchlen;
    destpos = 0;
    pos = searchBegPos;
    while(count){
        pos = (*source->read)(source, pos, text, count);
        strncpy(&buf[destpos], text->ptr, text->length);
	count -= text->length;
        destpos += text->length;
    }
    if(strncmp(buf, searchstring, searchlen)){
	result = 0;
    } else {
	text->length = (strlen(replacestring));
	text->firstPos = 0;
	text->ptr = replacestring;
	if(QXtTextReplace(CurDpy, textwindow, searchBegPos,searchEndPos,text) != EDITDONE) 
	    result = 0;
	else 
	    result = 1;
    }       
    free(buf);
    return result;
}
DoReplaceOne()
{
    if(!ReplaceOne()){
	XeditPrintf("\nReplaceOne: nothing replaced");
	Feep();
    }
    else 
	QXtTextUnsetSelection(CurDpy, textwindow);
    if(SearchRight())
        QXtTextSetNewSelection(CurDpy, textwindow, searchBegPos, searchEndPos);
}

DoReplaceAll()
{
  int count;
    count = 0;
    QXtTextSetInsertionPoint(CurDpy, textwindow, 0); 
    while(SearchRight()){
	if(!ReplaceOne())
	    break;
	count++;
    }
    if(!count){
	XeditPrintf("\nReplaceAll: nothing replaced");
	Feep();
    } else {
	XeditPrintf("\n%d Replacement%c made", count, (count>1)?'s':' ');
    }
}

DoSearchRight()
{
    if(SearchRight()){
	QXtTextSetNewSelection(CurDpy, textwindow, searchBegPos, searchEndPos);
    } else {
        Feep();  
	XeditPrintf("\nSearch: couldn't find ` %s ' ", searchstring);
    }
}

SearchRight()
{
  XtTextPosition pos, startpos = QXtTextGetInsertionPoint(CurDpy, textwindow);
  int searchlen;
  int n, i, destpos, size = (*source->getLastPos)(source) - startpos;
  char *s1, *s2, *buf = malloc(size);
  XtTextBlock t, *text;
    text = &t;
    destpos = 0;
    searchlen = strlen(searchstring);
    if(!searchlen){
	text->ptr = QXFetchBuffer(CurDpy, &(text->length), 0);
	if(!text->length){
	    Feep();
	    XeditPrintf("\nSearch: nothing selected.");
	    return 0;
	}
	searchlen = text->length;
	QXtTextReplace(CurDpy, searchstringwindow, 0,0, text); 
	free(text->ptr);
    }
    for(pos = startpos; pos < startpos + size; ){
        pos = (*source->read)(source, pos, text, size);
	strncpy(&buf[destpos], text->ptr, text->length);
	destpos += text->length;
    }
    for( i = 0; i < size; i++){
	n = searchlen;
	s1 = &buf[i];
	s2 = searchstring;
	while (--n >= 0 && *s1++ == *s2++);
	if(n < 0)
	    break;
    }
    free(buf);
    if( n < 0){
        i += startpos;
        QXtTextSetInsertionPoint(CurDpy, textwindow, i + searchlen); 
        searchBegPos = i;
        searchEndPos = i + searchlen;
	return(1);
    } else {
        return(0);
    }
}

DoSearchLeft()
{
  XtTextPosition end = (*source->getLastPos)(source);
  XtTextPosition pos, startpos = QXtTextGetInsertionPoint(CurDpy, textwindow);
  int searchlen = strlen(searchstring);
  int n, i, destpos, count =  startpos + searchlen;
  char *s1, *s2, *buf = calloc(1, count);
  XtTextBlock t, *text;
    text = &t;
    destpos = 0;
    /* if there's a string in the window, use it, otherwize, use selected */
    if(!searchlen){
	text->ptr = QXFetchBuffer(CurDpy, &(text->length), 0);
	if(!text->length){
	    XeditPrintf("\nSearch: nothing selected.");
	    Feep();
	    return;
	}
	searchlen = text->length;
	QXtTextReplace(CurDpy, searchstringwindow, 0,0, text); 
	free(text->ptr);
    }
    /* buffer portion of file from insertion point, on */
    for(pos = 0; pos < min(count, end);){
        pos = (*source->read)(source, pos, text, (count - pos));
	strncpy(&buf[destpos], text->ptr, text->length);
	destpos += text->length;
    }
    /* search for the target string */
    for( i = startpos-1; i >= 0; i--){
	n = searchlen;
	s1 = &buf[i];
	s2 = searchstring;
	while (--n >= 0 && *s1++ == *s2++);
	if(n < 0)
	    break;
    }
    /* process result */
    if(n < 0){
        QXtTextSetInsertionPoint(CurDpy, textwindow, i); 
        searchBegPos = i;
        searchEndPos = i + searchlen;
	QXtTextSetNewSelection(CurDpy, textwindow, searchBegPos, searchEndPos);
    }
    else {
	XeditPrintf("\nSearch: couldn't find ` %s ' ", searchstring);
	Feep();
    }
    free(buf);
}
DoUndo()
{
  XtTextPosition from;
    from = (*source->replace)(source, -1, 0, 0); 
    FixScreen(from);
}
DoUndoMore()
{
  XtTextPosition from;
    from = (*source->replace)(source, 0, -1, 0); 
    FixScreen(from);
}
setLoadedFile(name)
  char *name;
{
    if(loadedfile) free(loadedfile);
    loadedfile = malloc(strlen(name));
    strcpy(loadedfile, name);
}


setSavedFile(name)
  char *name;
{
    if(savedfile) free(savedfile);
    savedfile = malloc(strlen(name));
    strcpy(savedfile, name);
}

char *makeTempName()
{
  extern char* mktemp();
  char *tempname = malloc(1024);
    sprintf(tempname, "%s.XXXXXX", filename);
    return(mktemp(tempname));
}
	
char *makeBackupName()
{
  char *backupName = malloc(1024);
    sprintf(backupName, "%s.BAK", filename);
    return (backupName);
}
/*
error()
{
        extern int errno, sys_nerr;
        extern char *sys_errlist[];

        if (errno > 0 && errno < sys_nerr)
                tvcprintf(stderr, "(%s)\n", sys_errlist[errno]);

}
*/
  
DoSave()
{
  char *backupFilename, *tempName;
  XtTextPosition pos, end;
  XtTextBlock t, *text;
  FILE *outStream;
  int outfid;
    text = &t;
    backupFilename = tempName = NULL;
    if( (!filename) || (!strlen(filename)) ){
	XeditPrintf("\nSave:  no filename specified -- nothing saved");
	Feep();
	return;
    }
    if(((savedfile && !strcmp(savedfile, filename)) 
                     || (!strcmp(loadedfile,filename)))
    		     && (lastChangeNumber == PSchanges(source))){
	XeditPrintf("\nSave:  no changes to save -- nothing saved");
	Feep();
	return;
    }
    if((!backedup) && (strlen(loadedfile)) && (!strcmp(filename, loadedfile))){
        backupFilename = makeBackupName(filename);
        unlink(backupFilename);
        if(link(filename, backupFilename)){
	    XeditPrintf("\ncan't create backup file");
	    Feep();
	    return;
	}
	TDestroyEDiskSource(dsource);  
	dsource = (XtTextSource*)TCreateEDiskSource(backupFilename, XttextRead);  
	PSsetROsource(source, dsource);
        backedup = 1;
    }
    if(editInPlace){
	if(!(outStream = fopen(filename, "w"))){
	    XeditPrintf("\nfile is not writable: %s", filename);
	    return;
	}
    } else {
	tempName = makeTempName();
	if((outfid = creat(tempName, FileMode)) < 0){
	    XeditPrintf("\n??? Can't create temporary file");
	    return;
	}
	outStream = fdopen(outfid, "w");
    }
/* WRITE ALL THE BITS OUT TO THE OUTPUT STREAM */
    end = (*source->scan)(source, 0, XtstFile, XtsdRight, 1, FALSE);
    for(pos = 0; pos < end; ){
	pos = (*source->read)(source, pos, text, 1024);
	if(text->length == 0)
	    break;
	if(fwrite(text->ptr, 1, text->length, outStream) < text->length){
	    XeditPrintf("\nerror writing to output file");
	    break;
	}
    }
    fclose(outStream);
    if(!editInPlace)
	if(rename(tempName, filename) < 0){
	    XeditPrintf("\nerror writing file.  Edits will be left in: %s",
		tempName);
	    Feep();
	}
    if(!enableBackups && backupFilename)
	if(unlink(backupFilename) < 0)
	    XeditPrintf("\nerror deleting backupfile:  %s",  backupFilename);        lastChangeNumber = PSchanges(source);
    setSavedFile(filename);
    XeditPrintf("\nSaved file:  %s", savedfile);
    loadChangeNumber = 0;
    quitChangeNumber = 0;
    if(backupFilename)
	free(backupFilename);
    if(tempName)
	free(tempName);
    PSbreakInput(source);
}

DoLoad()
{
  int numargs;
  Arg args[2];
  struct stat stats;
    if((lastChangeNumber != PSchanges(source)) &&
       (loadChangeNumber != PSchanges(source))){
        XeditPrintf("\nUnsaved changes. Save them, or press Load again.");
        Feep();
        loadChangeNumber = PSchanges(source);
        return;
    }
    numargs = 0;
    MakeArg(XtNwindow, (caddr_t)editbutton);
    MakeArg(XtNindex,  (caddr_t)2);
    if ((strlen(filename)&&access(filename, R_OK) == 0)) {
	stat(filename, &stats);
	FileMode = stats.st_mode;
	TDestroyEDiskSource(dsource);  
	TDestroyApAsSource(asource);
	DestroyPSource(source);
	dsource = (XtTextSource*)TCreateEDiskSource(filename, XttextRead);  
	asource = TCreateApAsSource();
	source = CreatePSource(dsource, asource);
	QXtTextNewSource(CurDpy, textwindow, dsource, 0);
        if(Editable){
            QXtButtonBoxAddButton(CurDpy, Row1, args, numargs);
	    Editable = 0;
	}
        backedup = 0;
	{
	    static Arg setargs[] = {
	        { XtNlabel,	 (caddr_t)0 }
	    };
	    setargs[0].value = (caddr_t)filename;
	    QXtLabelSetValues(CurDpy, labelwindow, setargs, XtNumber(setargs));
	}
	setLoadedFile(filename);
	lastChangeNumber = 0;
    }
    else {
	XeditPrintf("\nLoad: couldn't access file ` %s '", filename);
	Feep();
    }
}

DoEdit()
{
  XtTextPosition newInsertPos = QXtTextGetInsertionPoint(CurDpy, textwindow);
  int numargs;
  Arg args[1];
    numargs = 0;
    MakeArg(XtNwindow, (caddr_t)editbutton);
    if (access(filename, W_OK) == 0) {
        QXtTextNewSource(CurDpy, textwindow, source, 0);  
        QXUnmapWindow(CurDpy, editbutton);
        QXtButtonBoxDeleteButton(CurDpy, Row1, args, numargs); 
	QXtTextSetInsertionPoint(CurDpy, textwindow, newInsertPos);
        Editable = 1;
    } else {
	XeditPrintf("\nEdit: File is not writable");
	Feep();
    }
}

static Jump(line)
  int line;
{
  XtTextPosition pos;
    if(line <= 1)
	pos = 0;
    else
        pos =  (*source->scan)(source, 0, XtstEOL, XtsdRight, line-1, 1);
    QXtTextSetInsertionPoint(CurDpy, textwindow, pos); 
}

DoJump()
{
    char *XcutBuf, *buf;
    int   XcutSize, line;
	
    XcutBuf = QXFetchBuffer( CurDpy, &(XcutSize), 0);
    buf = malloc(XcutSize+1);
    strncpy(buf, XcutBuf, XcutSize);
    free(XcutBuf);
    buf[XcutSize] = 0;
    if(sscanf(buf, "%d", &line) > 0){
	Jump(line);
    } else {
         XeditPrintf("\nPlease 'Select' a line number and try again");
    }
    free(buf);	
}

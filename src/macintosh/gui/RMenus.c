/* 
	************************************************************************************************ 
	 
	RMenus.c
	by wing kwong (Tiki), WAN 3/2/99
	
	Code cleanup and fix for q() based quiting.
	Ihaka 30/6/99
	
	************************************************************************************************ 
	Description

    This file is based on the WASTE and WASTE demo, I had do some modification to make it function
    as the way R want. The routine in here is used to handle event (high or low level event.)
    There have a lot of function which is related to the Menu of Console and edit window in here.
    We will not suing them, however, it is pretty good to leave it in here. Cause it is fully
    function, and it function correctly, when you feel that you need to add a size, font or color
    Menu..., what you need to do is to change the R.rsrc file ('MBAR' resource), You can simply
    overwrite the resource with ID 130 to ID 128. Then, a fullly function Menu will appear again.
    	
	************************************************************************************************ 
	Description of WASTE and WASTE Demo :
	
	??? Is it necessary in here  ??? I think we need

	WASTE Demo Project:
	Macintosh Controls with Long Values

	Copyright � 1993-1998 Marco Piovanelli
	All Rights Reserved

	C port by John C. Daub

	************************************************************************************************

*/

/* ************************************************************************************************ */
/*                                    INCLUDE HEADER FILE                                           */
/* ************************************************************************************************ */
#ifndef __ALIASES__
#include <Aliases.h>
#endif

#ifndef __DEVICES__
#include <Devices.h>
#endif

#ifndef __ERRORS__
#include <Errors.h>
#endif

#ifndef __LOWMEM__
#include <LowMem.h>
#endif

#ifndef __STANDARDFILE__
#include <StandardFile.h>
#endif

#ifndef __FILETYPESANDCREATORS__
#include <FileTypesAndCreators.h>
#endif

#ifndef __TOOLUTILS__
#include <ToolUtils.h>
#endif
#include <AppleEvents.h>
#include <Processes.h>

#ifndef __WEDEMOAPP__
#include "RIntf.h"
#endif

#include "WETabs.h"
#include "Defn.h"
#include <Scrap.h>
#include "Graphics.h"
#include "PicComments.h"

/* ************************************************************************************************ */
/*                                           DEFINE CONSTANT                                        */
/* ************************************************************************************************ */ 
#define eNoSuchFile                      9
#define eSelectNextTime                  10
#define kTypeMenuColorTable              'mctb'
#define kFinderSig                       'FNDR'
#define kAEFinderEvents                  'FNDR'
#define kSystemType                      'MACS'
#define kAEOpenSelection                 'sope'
#define keySelection                     'fsel'

/* ************************************************************************************************ */
/*                                        Global variables                                          */
/* ************************************************************************************************ */ 
Boolean                                  HaveContent = false;
Boolean                                  PrintPicture;
Handle	                                 myHandle;
SInt32                                   curPaste, finalPaste;
static Handle                            sColors;    /* handle to the 'mctb' resource for the Color menu*/


/* ************************************************************************************************ */
/*                                    Extern Global variables                                       */
/* ************************************************************************************************ */ 
extern char                              *gbuf;
extern SInt32                            gbuflen;
extern Handle                            gTextHdl;
extern FSSpec                            gHelpFile;
extern WindowPtr                         Console_Window;
extern SInt32                            gChangeAble, gpmtLh;
extern Boolean                           gfinishedInput;
extern Graphic_Ref                       gGReference[MAX_NUM_G_WIN + 1];
/*extern                                   gInhibitPrintRecordsInfo;
extern                                   gInhibitPrintStructuresInfo;
*/
extern Boolean gPrintStructureInited;
/*extern Boolean gPrintRecordInited;
*/
extern PicHandle	                gPictureHdl;
extern  char                        InitFile[256];
extern SInt16                      Edit_Window;
extern WindowPtr                   Edit_Windows[MAX_NUM_E_WIN + 1];
extern Boolean            defaultPort;
/* ************************************************************************************************ */
/*                                       Protocols                                                  */
/* ************************************************************************************************ */ 
void    assignPString                    (unsigned char* , char* , SInt16);
OSErr   FindAProcess                     (OSType, OSType, ProcessSerialNumber*);
OSErr   OpenSelection                    (FSSpecPtr theDoc);
void    ConsolePaste                     (void);
void    ConsoleCopyAndPaste              (void);
OSErr DoOpenText(void);
OSErr DoOpen(void);

/* ************************************************************************************************ */
/*                                   Extern Global variables                                        */
/* ************************************************************************************************ */ 
extern  void   doWindowsMenu             (SInt16);
extern  void   changeGWinPtr             (WindowPtr, Str255);
extern  void   *dlopen                   (const char*, int);
extern  void   savePreference            (void);
extern void DoGenKeyDown (const EventRecord *event, Boolean Console);
extern Boolean EqualNumString(Str255 Str1, Str255 Str2, SInt16 Num);
extern void adjustEditPtr(SInt16 EditIndex);
extern pascal	OSErr	FSpGetFullPath (const FSSpec*, short*, Handle*);
extern void LoadWindow();
extern void DoLineTo();
extern void Do_EditObject();
/*extern void PrintPicture();*/
extern void printLoop();
extern void DoUpdate (WindowPtr window);
extern void DoActivate (Boolean isActivating, WindowPtr window);
extern void Do_About();
/* ************************************************************************************************ */
/*                                            enum                                                  */
/* ************************************************************************************************ */ 

enum {
	kButtonSave		= 1,
	kButtonCancel,
	kButtonDontSave
};


/* ************************************************************************************************
SetDefaultDirectory:
************************************************************************************************ */

void SetDefaultDirectory (const FSSpec * spec)
{
	LMSetCurDirStore (spec->parID);
	LMSetSFSaveDisk (- spec->vRefNum);
}


/* ************************************************************************************************
MySFDialogFilter
************************************************************************************************ */
static pascal Boolean MySFDialogFilter(DialogPtr dialog, EventRecord *event, SInt16 *item, void *yourData)
{
#pragma unused (item, yourData)

	/*    intercept window events directed to windows behind the dialog */
	if ((event->what == updateEvt) || (event->what == activateEvt)) {
		if ((WindowPtr) event->message != dialog) {
			DoWindowEvent(event);
		}
	}
	return false;
}


/* ************************************************************************************************
ModalFilterYDUPP
************************************************************************************************ */
static ModalFilterYDUPP GetMySFDialogFilter(void)
{
#ifdef __cplusplus
	static ModalFilterYDUPP sFilterUPP = NewModalFilterYDProc(MySFDialogFilter);
#else
	static ModalFilterYDUPP sFilterUPP = nil;
	if (sFilterUPP == nil) {
		sFilterUPP = NewModalFilterYDProc(MySFDialogFilter);
	}
#endif
	return sFilterUPP;
}


/* ************************************************************************************************
FindMenuItemText
************************************************************************************************ */
SInt16 FindMenuItemText(MenuHandle menu, ConstStr255Param stringToFind)
{
	SInt16      item;
	Str255      itemString;

	for (item = CountMenuItems(menu); item >= 1; item--) {
		GetMenuItemText(menu, item, itemString);
		if (EqualString(itemString, stringToFind, false, false))
			break;
	}
	return item;
}


/* ************************************************************************************************
EqualColor : use to maintain the color menus
************************************************************************************************ */
Boolean  EqualColor(const RGBColor *rgb1, const RGBColor *rgb2)
{
	return ((rgb1->red == rgb2->red) && (rgb1->green == rgb2->green) && (rgb1->blue == rgb2->blue));
}


/* ************************************************************************************************
PrepareMenus: Used to maintain which menus item ought to be enable, and which is disable.
This function will be called when you use a mouse and click on the menus bar.
************************************************************************************************ */
void PrepareMenus(void)
{
	WindowPtr		window;
	WEReference		we;
	MenuHandle		menu;
	MenuCRsrcPtr	pColors;
	SInt16			item;
	Str255			itemText;
	SInt32			selStart, selEnd;
	SInt32			threshold;
	WEActionKind	actionKind;
	WEStyleMode		mode;
	TextStyle		ts;
	Boolean			temp;
	SInt32			scrapOffset;
	SInt16			i;
	Str255			Cur_Title, Menu_Title;
	MenuHandle		windowsMenu;
	Boolean			EqString;
   
   
/*   
   Boolean *			present = nil ;
	Boolean				isContinuous = false ;
*/	        
	/* get a pointer to the frontmost window, if any
 */
	window = FrontWindow ();

	/* get associated WE instance
 */
	we = (window != nil) ? GetWindowWE (window) : nil;

	/* *** FILE MENU ***
 */
	menu = GetMenuHandle (kMenuFile);

	/* first disable all items
 */
	for (item = CountMenuItems (menu); item >= 1; item --) {
		DisableItem (menu, item);
	}
	if (isGraphicWindow(window)){
		SetMenuItemText(menu,kItemOpen, "\pActivate");
		SetItemCmd(menu, kItemOpen, 'G');
	}
   	else{
      SetMenuItemText(menu,kItemOpen, "\pOpen"); 
      SetItemCmd(menu, kItemOpen, 'O');         
   }
   
   /* New, Open, and Quit are always enabled
 */
   if (isEditWindow(window)){
       EnableItem (menu, kItemLoad);
       EnableItem (menu , kItemEditObject);
   }
   
   if (window == Console_Window)
       {
       EnableItem (menu, kItemLoad);
       EnableItem (menu , kItemEditObject);
       }
   EnableItem (menu, kItemNew);
   EnableItem (menu, kItemOpen);
   EnableItem (menu, kItemPageSetup);
   EnableItem (menu, kItemPrint);
   EnableItem (menu, kItemQuit);
   
   /* Enable "Close" and "Save As" if there is an active window
 */
	if (window != nil) {
		EnableItem (menu, kItemClose);
		if (!isHelpWindow(window)) {
			EnableItem (menu, kItemSaveAs);
			EnableItem (menu, kItemSave);
		}
#ifdef   XXX
		/* We have not make any agreement about this yet.
		 What is dirty, and what is not.        
		 enable Save is the active window is dirty
		*/
		if (WEGetModCount(we) > 0) {
			EnableItem (menu, kItemSave);
		}
#endif      
	}

	/* *** EDIT MENU ***
 */
	menu = GetMenuHandle (kMenuEdit);

	/* first, disable all items
 */
	for (item = CountMenuItems(menu); item >= 1; item--) {
		DisableItem(menu, item);
	}
	if (isGraphicWindow(window)) {
		EnableItem (menu, kItemCopy);
	}
	EnableItem(menu, kItemPreference);
	
	/* by default, the Undo menu item should read "Can't Undo"
 */
	GetIndString(itemText, kUndoStringsID, 1);
	SetMenuItemText(menu, kItemUndo, itemText);

	if (window != nil) {
	
		/* enable Paste if there's anything pasteable on the Clipboard
 */
		if (GetScrap(NULL,'TEXT',&scrapOffset) > 0) {   
			if (!isGraphicWindow(window)){
				EnableItem (menu, kItemPaste);
			}
		}

		/* enable Undo if anything can be undone
 */
		actionKind = WEGetUndoInfo (&temp, we);

		if (actionKind != weAKNone) {
			EnableItem (menu, kItemUndo);

			/* change the Undo menu item to "Undo/Redo" + name of action to undo/redo
 */
			GetIndString (itemText, kUndoStringsID, 2 * actionKind + temp);
			SetMenuItemText (menu, kItemUndo, itemText);
		}

		/* enable Select All if there is anything to select
 */
		if (WEGetTextLength (we) > 0)
		{
			EnableItem (menu, kItemSelectAll);
		}

		/* get the current selection range
 */
		WEGetSelection (&selStart, &selEnd, we);
		if (selStart != selEnd)
		{
	
			/* enable Cut, Copy, and Clear if the selection range is not empty
			 */
			 if (FrontWindow() == Console_Window)
			    {	EnableItem(menu, kItemCopyPaste);
			EnableItem (menu, kItemCut);
			EnableItem (menu, kItemCopy);
			EnableItem (menu, kItemClear);
			}
		}

		if (isEditWindow(window)) {
			EnableItem (menu, kItemLineTo);
		}
		if (isHelpWindow(window)){
			DisableItem (menu, kItemCut);
			DisableItem (menu, kItemCopyPaste);
			DisableItem (menu, kItemClear);
			DisableItem (menu, kItemPaste);   
		}
		/* determine which style attributes are continuous over
		 the current selection range we'll need this information
		 in order to check the Font/Size/Style/Color menus properly
*/
		mode = weDoAll;  /* query about all attributes
*/
		WEContinuousStyle (&mode, &ts, we);
		
    	
/*      WEMatchAttributes(1,1,mode,1,1,&ts,present,&isContinuous,we);
*/
	}
	else {
		mode = 0;        /* no window, so check no items*/
	}

	/* *** Window Menu *** */
	windowsMenu = GetMenu(mWindows);
	GetWTitle(window, (unsigned char *) &Cur_Title);

	for(i=1; i<=CountMenuItems(windowsMenu); i++) {
		GetMenuItemText(windowsMenu, i , (unsigned char*)&Menu_Title);
		/* RnWrite((char *)&Menu_Title[1], Menu_Title[0]);
		 RWrite("\r");
		*/
		EqString = EqualNumString(Menu_Title, Cur_Title, Menu_Title[0]);
		CheckMenuItem(windowsMenu,i,false);
		if (EqString){
			CheckMenuItem(windowsMenu,i,true);
		}
	}  

#ifdef UNUSE
	/* *** FONT MENU ***
*/
	menu = GetMenuHandle (kMenuFont);

	/* first, remove all check marks
*/
	for (item = CountMenuItems (menu); item >= 1; item --)
	{
		CheckItem (menu, item, false);
	}

	/* if there is a continuous font all over the selection range,
	 check the corresponding menu item
*/
	if (mode &weDoFont)
	{
		GetFontName (ts . tsFont, itemText);
		CheckItem (menu, FindMenuItemText (menu, itemText), true);
	}

	/* *** SIZE MENU ***
*/
	menu = GetMenuHandle(kMenuSize);

	/* first, remove all check marks
*/
	for (item = CountMenuItems (menu); item >= 1; item --)
	{
		CheckItem (menu, item, false);
	}

	/* if there is a continuous font size all over the selection range
	 check the corresponding menu item
*/
	if (mode &weDoSize)
	{
		NumToString (ts . tsSize, itemText);
		CheckItem (menu, FindMenuItemText (menu, itemText), true);
	}

	/* *** STYLE MENU ***
*/
	menu = GetMenuHandle (kMenuStyle);

	/* first, remove all check marks
	*/
	for (item = CountMenuItems (menu); item >= 1; item --) {
		CheckItem (menu, item, false);
	}

	/* check the style menu items corresponding to style attributes
	*/
	if (mode &weDoFace)
	{
		if (ts.tsFace == normal)
		{
			CheckItem(menu, kItemPlainText, true);
		}

		for (item = kItemBold; item <= kItemExtended; item ++)
		{
			if (ts.tsFace & (1 << (item - kItemBold)))
			{
				CheckItem(menu, item, true);
			}
		}
	}

	/* *** COLOR MENU ***
*/
	menu = GetMenuHandle (kMenuColor);

	/* first, remove all check marks
*/
	for (item = CountMenuItems (menu); item >= 1; item --)
	{
		CheckItem (menu, item, false);
	}

	/* if there is a continuous color all over the selection range,
	 check the corresponding menu item (if any)
*/
	if (mode &weDoColor)
	{
		pColors = * (MenuCRsrcHandle) sColors;
		for (item = pColors -> numEntries - 1; item >= 0; item --)
		{
			if (EqualColor (&ts.tsColor, &pColors -> mcEntryRecs [ item ] . mctRGB2))
			{
				CheckItem (menu, pColors -> mcEntryRecs [ item ] . mctItem, true);
			}

		} /* end for loop */
	}

	/* *** FEATURES MENU ***
*/
	menu = GetMenuHandle(kMenuFeatures);

	/* first remove all check marks
	 (except the first two items, which have submenus!)
*/
   for (item = CountMenuItems (menu); item >= 3; item --)
   {
		CheckItem (menu, item, false);
   }

   if (window != nil)
   {
		/* mark each item according to the corresponding feature
*/
		if (WEIsTabHooks (we)) {
			CheckItem (menu, kItemTabHooks, true);
			DisableItem (menu, kItemAlignment);
			DisableItem (menu, kItemDirection);
		}
		else {
			EnableItem (menu, kItemAlignment);
			EnableItem (menu, kItemDirection);
		}

		if (WEFeatureFlag (weFAutoScroll, weBitTest, we)) {
			CheckItem (menu, kItemAutoScroll, true);
		}

		if (WEFeatureFlag (weFOutlineHilite, weBitTest, we)) {
			CheckItem (menu, kItemOutlineHilite, true);
		}

		if (WEFeatureFlag(weFReadOnly, weBitTest, we)) {
			CheckItem (menu, kItemReadOnly, true);
		}

		if (WEFeatureFlag (weFIntCutAndPaste, weBitTest, we)) {
			CheckItem (menu, kItemIntCutAndPaste, true);
		}

		if (WEFeatureFlag(weFDragAndDrop, weBitTest, we)) {
			CheckItem (menu, kItemDragAndDrop, true);
		}

		if ((WEGetInfo(weTranslucencyThreshold, &threshold, we) == noErr) && (threshold > 0)) {
			CheckItem (menu, kItemTranslucentDrags, true);
		}

		if (WEFeatureFlag(weFDrawOffscreen, weBitTest, we)) {
			CheckItem (menu, kItemOffscreenDrawing, true);
		}
	}

	/* *** ALIGNMENT MENU ***
*/
	menu = GetMenuHandle (kMenuAlignment);

	/* first, remove all check marks
	*/
	
	for (item = CountMenuItems(menu); item >= 1; item --) {
		CheckItem(menu, item, false);
	}

	if (window != nil) {
	
		/* find the Aligment menu item corresponding to the current alignment
		*/
		switch (WEGetAlignment (we)) {
		
		case weFlushLeft:
			item = kItemAlignLeft;
			break;

		case weFlushRight:
			item = kItemAlignRight;
			break;

		case weFlushDefault:
			item = kItemAlignDefault;
			break;

		case weCenter:
			item = kItemCenter;
			break;

		case weJustify:
			item = kItemJustify;
			break;
		}

		/* check the menu item
		*/
		CheckItem(menu, item, true);
	}

	/* *** DIRECTION MENU ***
*/
	menu = GetMenuHandle(kMenuDirection);

	/* first, remove all check marks
	*/
	for (item = CountMenuItems(menu); item >= 1; item--) {
		CheckItem(menu, item, false);
	}

	if (window != nil) {
	
      /* find the Direction menu item corresponding to the current direction
      */
		switch (WEGetDirection(we)) {
		
		case weDirDefault:
			item = kItemDirectionDefault;
			break;

		case weDirLeftToRight:
			item = kItemDirectionLR;
			break;

		case weDirRightToLeft:
			item = kItemDirectionRL;
			break;
		}

		/* check the menu item
		*/
		CheckItem(menu, item, true);
	}
   
#endif
}


/* ************************************************************************************************
DoDeskAcc
************************************************************************************************ */
void DoDeskAcc(SInt16 menuItem)
{
	Str255 deskAccessoryName;
	GetMenuItemText(GetMenuHandle(kMenuApple), menuItem, deskAccessoryName);
	OpenDeskAcc(deskAccessoryName);
}


/* ************************************************************************************************
DoNew
************************************************************************************************ */
OSErr DoNew(void)
{
	/* create a new window from scratch
	*/
	return CreateWindow(nil);
}


/* ************************************************************************************************
DoOpen : Now implemented. It is usefule to load object .rda-like from the "load" menu.
         Based on previous work of Ross Ihaka. Jago Nov 2000 (Stefano M. Iacus)
************************************************************************************************ */
OSErr DoOpen(void)
{
	StandardFileReply	reply;
	SFTypeList			typeList;
	FInfo				fileInfo;
	OSErr				err = noErr;
	Point				where = { -1, -1 };  /* auto center dialog */
	SInt16				pathLen;
	Handle				pathName;
	FILE				*fp;
    SEXP img, lst;
    int i;
    
	/* set up a list of file types we can open for StandardGetFile
	typeList[0] = kTypeText;
	typeList[1] = ftSimpleTextDocument;
  
	//typeList[0] = 'RSES';	// I will either create files with type R Session or R Object
	//typeList[1] = 'ROBJ';
	//typeList[2] = kTypeText;
*/
	typeList[0] = 'BINA';
	/* put up the standard open dialog box.
	 (we use CustomGetFile instead of StandardGetFile because we want to provide
	 our own dialog filter procedure that takes care of updating our windows)
	*/
	CustomGetFile(nil, 1, typeList, &reply, 0, where, nil,
				  GetMySFDialogFilter(), nil, nil, nil);
    err = FSpGetFInfo(&reply.sfFile, &fileInfo);
    if (err != noErr) return err;
    FSpGetFullPath(&reply.sfFile, &pathLen, &pathName);
    HLock((Handle)pathName);
    strncpy(InitFile, *pathName, pathLen);
    InitFile[pathLen] = '\0';
    HUnlock((Handle) pathName);

/* 
     Routine now handle XDR object. Jago Nov2000 (Stefano M. Iacus)  
 */          
    if(!(fp = fopen(InitFile, "rb"))) { /* binary file */
	    RWrite("File cannot be opened !");
	    /* warning here perhaps */
	    return;
	}
#ifdef OLD
	FRAME(R_GlobalEnv) = R_LoadFromFile(fp, 1);
#else
	PROTECT(img = R_LoadFromFile(fp, 1));
	switch (TYPEOF(img)) {
	case LISTSXP:
	    while (img != R_NilValue) {
		defineVar(TAG(img), CAR(img), R_GlobalEnv);
		img = CDR(img);
	    }
	    break;
	case VECSXP:
	    for (i = 0; i < LENGTH(img); i++) {
		lst = VECTOR_ELT(img,i);
		while (lst != R_NilValue) {
		    defineVar(TAG(lst), CAR(lst), R_GlobalEnv);
		    lst = CDR(lst);
		}
	    }
	    break;
	}
        UNPROTECT(1);
#endif
          
	fclose(fp);

	return err;
}

/* ************************************************************************************************
DoOpen
************************************************************************************************ */
OSErr DoOpenText(void)
{
	StandardFileReply	reply;
	SFTypeList			typeList;
	FInfo				fileInfo;
	OSErr				err = noErr;
	Point				where = { -1, -1 };  /* auto center dialog */
	SInt16				pathLen;
	Handle				pathName;
	FILE				*fp;

	/* set up a list of file types we can open for StandardGetFile
   */
	typeList[0] = kTypeText;
	typeList[1] = ftSimpleTextDocument;

	/* put up the standard open dialog box.
	 (we use CustomGetFile instead of StandardGetFile because we want to provide
	 our own dialog filter procedure that takes care of updating our windows)
	*/
	
	CustomGetFile(nil, 2, typeList, &reply, 0, where, nil,
				  GetMySFDialogFilter(), nil, nil, nil);

	err = FSpGetFInfo(&reply.sfFile, &fileInfo);
	if (err != noErr) return err;
	DoNew();    
	ReadTextFile(&reply.sfFile, GetWindowWE(Edit_Windows[Edit_Window-1]));
	return err;
}


/* ************************************************************************************************
SaveWindow
************************************************************************************************ */
OSErr SaveWindow(const FSSpec *pFileSpec, WindowPtr window)
{
	DocumentHandle	hDocument;
	AliasHandle		alias = nil;
	OSErr			err;

	hDocument = GetWindowDocument(window);
	ForgetHandle(&(*hDocument)->fileAlias);

	if (isGraphicWindow(window)) {
		/* We don't save these ... */
	}
	else {
		/* save the text */
		if ((err = WriteTextFile(pFileSpec, (*hDocument)->we)) == noErr) {
			SetWTitle(window, pFileSpec->name);

			/* replace the old window alias (if any) with a new one created from pFileSpec
			*/
			NewAlias(nil, pFileSpec, &alias);

			/* if err, alias will be nil, and it's not fatal, just will make subsequent saves annoying
			*/
			(* hDocument)->fileAlias = (Handle)alias;
		}
	}
	return err;
}

/* ************************************************************************************************
DoSaveAs
************************************************************************************************ */
OSErr DoSaveAs(const FSSpec *suggestedTarget, WindowPtr window)
{
	StringHandle		hPrompt;
	Str255				defaultName;
	StandardFileReply	reply;
	Point				where = { -1, -1 }; /* autocenter's dialog */
	OSErr				err;

	/* get the prompt string for CustomPutFile from a string resource and lock it
	*/
	hPrompt = GetString(kPromptStringID);
	HLockHi((Handle) hPrompt);

	if (suggestedTarget != nil) {
		/* if a suggested target file is provided,
		 use its name as the default name
		*/
		PStringCopy(suggestedTarget->name, defaultName);
		SetDefaultDirectory(suggestedTarget);
	}
	else {
		/* otherwise use the window title
		 as default name for CustomPutFile
		*/
		GetWTitle(window, defaultName);
	}

	/* put up the standard Save dialog box
	*/
	CustomPutFile(*hPrompt, defaultName, &reply, 0, where, nil,
				  GetMySFDialogFilter(), nil, nil, nil);

	/* unlock the string resource
	*/
	HUnlock((Handle)hPrompt);

	/* if the user ok'ed the dialog, save the window to the specified file
	*/
	if (reply.sfGood)
		err = SaveWindow(&reply.sfFile, window);
	else
		err = userCanceledErr;
	return err;
}

/* ************************************************************************************************
DoSave
************************************************************************************************ */
OSErr DoSave(WindowPtr window)
{
	FSSpec		spec;
	FSSpecPtr	suggestedTarget = nil;
	Boolean		promptForNewFile = true;
	Boolean		aliasTargetWasChanged;
	OSErr		err;

	/* resolve the alias associated with this window, if any
*/
	if ((* GetWindowDocument(window))->fileAlias != nil) {
		if ((ResolveAlias(nil, (AliasHandle)(*GetWindowDocument(window))->fileAlias, &spec,
      												&aliasTargetWasChanged) == noErr)) {
			if (aliasTargetWasChanged)
				suggestedTarget = &spec;
			else
				promptForNewFile = false;
		}
	}

	/* if no file has been previously associated with this window,
	 or if the alias resolution has failed, or if the alias target
	 was changed prompt the user for a new destination
*/
	if (promptForNewFile)
		err = DoSaveAs(suggestedTarget, window);
	else
		err = SaveWindow(&spec, window);
	return err;
}


/* ************************************************************************************************
SaveChangesDialogFilter
************************************************************************************************ */
static pascal Boolean SaveChangesDialogFilter(DialogPtr dialog, EventRecord *event, SInt16 *item)
{
	/* map command + D to the "Don't Save" button
	*/
	if ((event->what == keyDown) && (event->modifiers & cmdKey)
					&& ((event->message & charCodeMask) == 'd')) {
		/* flash the button briefly
		*/
		FlashButton(dialog, kButtonDontSave);
		/* fake an event in the button
		*/
		*item = kButtonDontSave;
		return true;
	}
	/* route everything else to our default handler
	*/
	return CallModalFilterProc(GetMyStandardDialogFilter(), dialog, event, item);
}



/* ************************************************************************************************
DoClose:
In here, you need to understand that different window have different close method.
For Graphic window, when you close it, you need to tell R to remove the corresponding device.
For Console window, when you close, it implies that you close the application. Thus, you need to 
ensure that the application closed.
For Console and Edit window, when you close, you need to ask whether they want to save the content
or not, but for graphic window, it is not necessary.
************************************************************************************************ */
OSErr DoClose(ClosingOption closing, SavingOption saving, WindowPtr window)
{
	Str255		s1, s2;
	SInt16		alertResult;
	OSErr		err;
	Boolean		haveCancel;


#ifdef __cplusplus
	static ModalFilterUPP sFilterProc = NewModalFilterProc(SaveChangesDialogFilter);
#else
	static ModalFilterUPP sFilterProc = nil;
	if (sFilterProc == nil) {
		sFilterProc = NewModalFilterProc(SaveChangesDialogFilter);
	}
#endif
	if (isHelpWindow(window)) {
		DestroyWindow(window);
		return noErr;
	}
	err = noErr;
	/* is this window dirty?
	*/
	if (WEGetModCount(GetWindowWE(window)) > 0) {
		/* do we have to ask the user whether to save changes?
*/
		if (saving == savingAsk) {
			/* prepare the parametric strings to be used in the Save Changes alert box
*/
			GetWTitle(window, s1);
			GetIndString(s2, kClosingQuittingStringsID, 1 + closing);
			ParamText(s1, s2, nil, nil);

			/* put up the Save Changes? alert box
*/
			SetCursor(&qd.arrow);
			alertResult = Alert(kAlertSaveChanges, sFilterProc);

			/* exit if the user canceled the alert box
*/
			if (alertResult == kButtonCancel)
				return userCanceledErr;

			if (alertResult == kButtonSave)
				saving = savingYes;
			else
				saving = savingNo;
		}

		if (saving == savingYes) {
			if (isGraphicWindow(window)) {
				err = doSaveGraCommand();
				if (err != noErr) {
					/* You can handle error in here!
					 However, there have some warning return,
					 don't treat it as error.
					*/
				} 
			}
			else {
            	if (window == Console_Window) {  
					err = doRSave(&haveCancel);
					if (haveCancel){ 
						RWrite("\r"); 
						jump_to_toplevel();
					}
					else { 
						if (err == noErr){
							R_SaveGlobalEnv();							
							}
						else
							error("File Corrupt or Memory error. Unrecoverable!");  
					}
				}
				else {
					DoSave(window);
				}
			}
		}
	}


	/*if it is a graphic window, maintain the menus and title first.
	*/
	if (isGraphicWindow(window)){
		/* GetWTitle(window, (unsigned char *) &Cur_Title);*/
		Mac_Dev_Kill(window);
		/*changeGWinPtr(window, Cur_Title);*/
	}
	else {
		if (isEditWindow(window)) {
			adjustEditPtr(isEditWindow(window));
		}  
		/* destroy the window */
		DestroyWindow(window);
		if (window == Console_Window) 
			ExitToShell();
	} 
	return err;
}

void MacFinalCleanup()
{
	WindowPtr window;
	OSErr err;
	SavingOption saving = 1;
	
	/* Close all windows
	 query the user about contents
	 if "saving" is non-zero.
*/

#ifdef DEBAG
	printf("\n        MacFinalCleanup");
#endif
	
	do {
		if ((window = FrontWindow()) != nil) {
			if ((err = DoClose(closingApplication, saving, window)) != noErr) {
				return;
			}   
		}
	}
	while (window != nil);
	/* set a flag so we drop out of the event loop */
	printf("\n        ExitToShell");
	ExitToShell();
}

/* ************************************************************************************************
MySFDialogFilter
************************************************************************************************ */
OSErr DoQuit(SavingOption saving)
{
	WindowPtr	window;
	OSErr		err;

	/* Close all windows
	 query the user about contents
	 if "saving" is non-zero.
	*/
	
	do {
		if ((window = FrontWindow()) != nil) {
			if ((err = DoClose(closingApplication, saving, window)) != noErr) {
				return err;
			}   
		}
	}
	while (window != nil);

	/* set a flag so we drop out of the event loop
	*/
	ExitToShell();
	return noErr;
}


/* ************************************************************************************************
DoAppleChoice : Which will be called when you click on the apple icon on the top left corner.
When you choice 'R about', it will open the predefine html files and launch Netscape.
************************************************************************************************ */
void DoAppleChoice(SInt16 menuItem)
{   
	StandardFileReply        fileReply;
	SFTypeList               fileTypes; 
	OSErr                    launchErr;
	if (menuItem == kItemAbout){
		Do_About();
#ifdef  Preference   
		/* Test whether the predefine file exist or not. If not, you need to prompt out the stardand
		 Get file dialog to query the user, where they put the file to.
		*/
		launchErr = FSpSetFLock(&gHelpFile);
		/* File not found with a reference number -43, 
		 However, I would like to check more general.
		 If you have any err, then, you need to select it
		 by yourself
		*/
		if (launchErr){
			GWdoErrorAlert(eNoSuchFile);
			fileTypes[0] = 'TEXT';
			StandardGetFile(nil,1,fileTypes,&fileReply);
			if (fileReply.sfGood){
				gHelpFile = fileReply.sfFile;
				/* savePreference(); */         /* not sure whether it is necessary or not. */
				OpenSelection(&gHelpFile);
			}
			else 
				GWdoErrorAlert(eSelectNextTime);   
		}
		else{
			launchErr = FSpRstFLock(&gHelpFile);
			launchErr = OpenSelection(&gHelpFile);
		}
#endif       
	}
	else
		DoDeskAcc(menuItem);
	/* Do another normal choice, like "control panel' */
}


/* ************************************************************************************************
DoFileChoice: Which is used to deal with the menus choice on under "File"
We still have no agreement in this moment. Those I don't know when to chance the codes about 
doNew, DoOpen, DoClose and DoSaveAs.
************************************************************************************************ */
void DoFileChoice(SInt16 menuItem)
{
	WindowPtr	window = FrontWindow();
	OSErr		osError, err;
	EventRecord	myEvent;
   SInt16		WinIndex;
   Boolean		haveCancel;
   switch(menuItem) {
   
	case kItemNew:
		if (isGraphicWindow(window)){
			RWrite("macintosh()\r");
			myEvent.message = 140301;
			DoGenKeyDown (&myEvent, true); 
		}
		else{
			DoNew();
		}
		break;

	case kItemOpen:
		if (isGraphicWindow(window)){
			WinIndex = isGraphicWindow(window);
			selectDevice(deviceNumber((DevDesc *)gGReference[WinIndex].devdesc));
		}
		else {
			DoOpenText();
		}
		break;
		
	case kItemEditObject:
		Do_EditObject();
		break;
		 
	case kItemLoad :
		if (isEditWindow(window)){
			LoadEditEnvironment();             
			LoadWindow();
		}
		if (window == Console_Window) {  
				err = DoOpen();
				if (err) { 
					RWrite("\r"); 
					jump_to_toplevel();
				}  
		}		           
		break;
		
	case kItemClose:
		DoClose(closingWindow, savingAsk, window);
		break;

	case kItemSave:
		if (isGraphicWindow(window)) {
			osError = doSaveGraCommand();
			if (osError != noErr){
				/* You can handle error in here!
				 However, there have some warning return,
				 don't treat it as error.
				 */
			} 
		}
		else {
			if (window == Console_Window) {  
				err = doRSave(&haveCancel);
				if (haveCancel) { 
					RWrite("\r"); 
					jump_to_toplevel();
				}             
			}
			else {
				DoSave(window);
			}
		}
		break;

	case kItemSaveAs:
		if (isGraphicWindow(window)) {
			osError = doSaveAsGraCommand();
			if (osError != noErr) {
				/* You can handle error in here! */
			}
		}
		else {
			if (window == Console_Window) {
				err = doRSaveAs(&haveCancel);
				if (haveCancel){
					RWrite("\r");
					jump_to_toplevel();
				}
			}
			else {
				DoSaveAs(nil, window);
			}
		}
		break;
        
	case kItemPageSetup:
		do_PageSetup();
		break;
		
	case kItemPrint:
		do_Print();
		break;
		
	case kItemQuit:
		err = DoQuit(savingAsk);
		break;
	}
	HiliteMenu(0);
}


/* ************************************************************************************************
DoEditChoice: Which is desgin to handle the Menus choice about "Edit". We have different 
implemenation on different kind of window.
In the Preference, you can assign the tab size, the number of History command that you can keep and
.....
************************************************************************************************ */
void DoEditChoice(SInt16 menuItem)
{
	WindowPtr		window;
	WEReference		we;
	SInt32			selEnd, selStart, i; 
	long			scrapOffset, rc;
	EventRecord		myEvent;
	char			TempChar;
	
   /* do nothing is no window is active
*/
	if ((window = FrontWindow()) == nil) {
		return;
	}
	
	we = GetWindowWE(window);

	switch (menuItem) {
	
	case kItemUndo:
		WEUndo(we);
		break;

	case kItemCut :
		if (window == Console_Window) {
			WEGetSelection(&selStart, &selEnd, we);
			if (inRange(selStart, selEnd, gChangeAble -1, gChangeAble - 1))
				SysBeep(10);
			else{
				if (selStart <= gChangeAble)
				gChangeAble = gChangeAble + selStart - selEnd;
				WECut(we);
			}
		}
		else
			WECut(we);          
		break;
       
	case kItemCopy:
		if (isGraphicWindow(window)) {
			GraphicCopy(window);
		}
		else
			WECopy(we);
		break;

	case kItemPaste:
		if (FrontWindow() == Console_Window) {
			WESetSelection(WEGetTextLength(we), WEGetTextLength(we), we);           
			myHandle = NewHandle(0);		/* allocate 0-length data area */
			rc = GetScrap(myHandle, 'TEXT', &scrapOffset);
			if (rc < 0) {
				/* . . . process the error . . . */ }
			else {
				SetHandleSize(myHandle, rc+1);	/* prepare for printf() */
				HLock(myHandle);
				(*myHandle)[rc] = 0;			/* make it ASCIIZ */
				finalPaste = rc;
				for (i = 0; i <=rc; i++) {
					TempChar = (*myHandle)[i];
					if ((TempChar == '\r') || (i == finalPaste)) {
						RnWrite(*myHandle, i);
						if (i != finalPaste) {
							myEvent.message = 140301;
							DoKeyDown (&myEvent);
						}
						curPaste = i;
						break;
					}
				}
				finalPaste = rc;
				/* RWrite(*myHandle);
				 printf("Scrap contained text: %s'\n", *myHandle);
				*/
				HUnlock(myHandle);
				if (rc != i) HaveContent = true;
				else DisposeHandle(myHandle);
			}
			/* WEPaste(we); */
		}
		else {
			if (isGraphicWindow(window) == 0)
				WEPaste(we);
		}  
		/*WEPaste (we);*/
		break;

	case kItemClear:
		if (window == Console_Window) {
			WEGetSelection(&selStart, &selEnd, we);
			if (inRange(selStart, selEnd, gChangeAble -1, gChangeAble - 1))
				SysBeep(10);
			else{
				if (selStart <= gChangeAble)
					gChangeAble = gChangeAble + selStart - selEnd;
				WEDelete(we);
			}
		}
		else
			WEDelete (we);
		break;

	case kItemCopyPaste:
		ConsoleCopyAndPaste();
		break;
		
	case kItemSelectAll:
		WESetSelection(0, LONG_MAX, we);
		break;
         
	case kItemLineTo:
		DoLineTo();
		break; 

	case kItemPreference:
		DoPreference(kDialogAboutBox);      
		break;   
	}
}


/* ************************************************************************************************
DoFontChoice :
************************************************************************************************ */
/*
void DoFontChoice(SInt16 menuItem, EventModifiers modifiers)
{
	WindowPtr	window;
	Str255		fontName;
	WEStyleMode	mode;
	TextStyle	ts;

	// do nothing if there is no front window
	if ((window = FrontWindow()) == nil) {
		return;
	}

	GetMenuItemText(GetMenuHandle(kMenuFont), menuItem, fontName);
	GetFNum(fontName, &ts.tsFont);

	// use script-preserving mode by default (see WASTE docs)
	// force font change across the whole selection if the option key was held down
	mode = (modifiers &optionKey) ? weDoFont : (weDoFont + weDoPreserveScript
												+ weDoExtractSubscript);
	// set the font of the selection

	WESetStyle(mode, &ts, GetWindowWE(window));

}
*/

/* ************************************************************************************************
DoSizeChoice
************************************************************************************************ */
/*
void DoSizeChoice(SInt16 menuItem)
{
	WindowPtr	window;
	Str255		sizeString;
	SInt32		size;
	WEStyleMode	mode;
	TextStyle	ts;

	// do nothing if there is no front window
	if ((window = FrontWindow()) == nil) {
		return;
	}

	if (menuItem <= kItemLastSize) {
		GetMenuItemText(GetMenuHandle(kMenuSize), menuItem, sizeString);
		StringToNum(sizeString, &size);
		ts.tsSize = size;
		mode = weDoSize;
	}
	else if (menuItem == kItemSmaller) {
		ts.tsSize = - 1;
		mode = weDoAddSize;
	}
	else if (menuItem == kItemLarger) {
		ts.tsSize = + 1;
		mode = weDoAddSize;
	}
	else {
		return;
	}
	// set the size of the selection
	WESetStyle(mode, &ts, GetWindowWE(window)); 

}

*/

void changeSize(WindowPtr window, SInt16 newSize)
{
	 WESetOneAttribute ( kCurrentSelection, kCurrentSelection, weTagFontSize,
		& newSize, sizeof ( Fixed ), GetWindowWE ( window ) ) ;

/* Old code: mod Jago 08/28/00

	TextStyle	ts;
	WEStyleMode	mode;
	ts.tsSize = newSize;
	
	mode = weDoSize;

	
	WESetStyle(mode, &ts, GetWindowWE(window));
*/
	

}

/* ************************************************************************************************
DoStyleChoice
************************************************************************************************ */
/*
void DoStyleChoice(SInt16 menuItem)
{
	WindowPtr	window;
	TextStyle	ts;

	// do nothing if there is no front window
	if ((window = FrontWindow ()) == nil) {
		return;
	}
	if (menuItem == kItemPlainText) {
		ts.tsFace = normal;
	}
	else {
      ts.tsFace = 1 << (menuItem - kItemBold);
	}
	// set the style of the selection
	
	WESetStyle(weDoFace + weDoToggleFace, &ts, GetWindowWE(window));
  

}
*/


/* ************************************************************************************************
DoColorChoice
************************************************************************************************ */
/*
void DoColorChoice(SInt16 menuItem)
{
	WindowPtr		window;
	MenuCRsrcPtr	pColors;
	SInt16			i;
	TextStyle		ts;
	// do nothing if there is no front window
	if ((window = FrontWindow()) == nil) {
		return;
	}
	if (menuItem <= kItemLastColor) {
		// find the color corresponding to the chosen menu item
		pColors = *(MenuCRsrcHandle)sColors;
		for (i = pColors->numEntries - 1; i >= 0; i --) {
			if (pColors -> mcEntryRecs [ i ] . mctItem == menuItem) {
				ts.tsColor = pColors -> mcEntryRecs [ i ] . mctRGB2;
				break;
			}
		}
	}
	else {
		return;
	}
	// set the color of the selection
	WESetStyle(weDoColor, &ts, GetWindowWE(window));



}

*/

/* ************************************************************************************************
DoAlignmentChoice
************************************************************************************************ */
void DoAlignmentChoice(SInt16 menuItem)
{
	WindowPtr	window;
	WEAlignment	alignment = weFlushDefault;

	if ((window = FrontWindow()) == nil) {
		return;
	}
	switch(menuItem) {
	
	case kItemAlignDefault:
		alignment = weFlushDefault;
		break;

	case kItemAlignLeft:
		alignment = weFlushLeft;
		break;

	case kItemCenter:
		alignment = weCenter;
		break;

	case kItemAlignRight:
		alignment = weFlushRight;
		break;

	case kItemJustify:
		alignment = weJustify;
		break;
	}
	/* set the alignment mode (this automatically redraws the text)
	*/
	WESetAlignment(alignment, GetWindowWE(window));
}


/* ************************************************************************************************
DoDirectionChoice
************************************************************************************************ */
void DoDirectionChoice(SInt16 menuItem)
{
	WindowPtr	window;
	WEDirection	direction = weDirDefault;

	if ((window = FrontWindow()) == nil) {
		return;
	}

	switch (menuItem) {
	
	case kItemDirectionDefault:
		direction = weDirDefault;
		break;

	case kItemDirectionLR:
		direction = weDirLeftToRight;
		break;

	case kItemDirectionRL:
		direction = weDirRightToLeft;
		break;
		
	}
	// set the primary line direction (this automatically redraws the text)
	WESetDirection(direction, GetWindowWE(window));
}


/* ************************************************************************************************
DoFeatureChoice
************************************************************************************************ */
void DoFeatureChoice(SInt16 menuItem)
{
	WindowPtr	window;
	WEReference	we;
	SInt32		threshold;

	if ((window = FrontWindow()) == nil)
		return;

	we = GetWindowWE(window);

	if (menuItem == kItemTabHooks) {
	
		// install or remove our custom tab hooks
		if (! WEIsTabHooks(we)) {
			// left-align the text (the hooks only work with left-aligned text)
			WESetAlignment(weFlushLeft, we);

			// force a LR primary direction
			// (the hooks don't work with bidirectional text anyway)
			WESetDirection(weDirLeftToRight, we);

			// install tab hooks
			WEInstallTabHooks(we);
		}
		else {
			// remove tab hooks
			WERemoveTabHooks(we);
		}

		// turn the cursor into a wristwatch
		SetCursor(* GetCursor(watchCursor));

		// recalculate link breaks and redraw the text
		WECalText(we);
		WEUpdate(nil, we);
	}
	else {
		switch (menuItem) {
		
		case kItemAutoScroll:
			WEFeatureFlag(weFAutoScroll, weBitToggle, we);
			break;

		case kItemOutlineHilite:
			WEFeatureFlag(weFOutlineHilite, weBitToggle, we);
			break;

		case kItemReadOnly:
			WEFeatureFlag(weFReadOnly, weBitToggle, we);
			break;

		case kItemIntCutAndPaste:
			WEFeatureFlag(weFIntCutAndPaste, weBitToggle, we);
			break;

		case kItemDragAndDrop:
			WEFeatureFlag(weFDragAndDrop, weBitToggle, we);
			break;

		case kItemTranslucentDrags:
			if (WEGetInfo(weTranslucencyThreshold, &threshold, we) == noErr) {
				threshold = kStandardTranslucencyThreshold - threshold;
				WESetInfo(weTranslucencyThreshold, &threshold, we);
			}
			break;

		case kItemOffscreenDrawing:
			WEFeatureFlag(weFDrawOffscreen, weBitToggle, we);
			break;
			
		}
	}
}


/* ************************************************************************************************
DoMenuChoice: The main function on RMenus.c, it is used to handle where to dispatch the event to
the corresponding procedure to handle the corresponding menus choice. Thus, if you need some Menus
choice, it is the best place to start with.
************************************************************************************************ */
void DoMenuChoice(SInt32 menuChoice, EventModifiers modifiers)
{
	SInt16	menuID, menuItem;

	// extract menu ID and menu item from menuChoice

	menuID = HiWord(menuChoice);
	menuItem = LoWord(menuChoice);

	// dispatch on menuID

	switch (menuID) {
	
	case kMenuApple:
		DoAppleChoice(menuItem);
		break;

	case kMenuFile:
		DoFileChoice(menuItem);
		break;

	case kMenuEdit:
		DoEditChoice(menuItem);
		break;
/*
	case kMenuFont:
		DoFontChoice(menuItem, modifiers);
		break;
*/
/*
	case kMenuSize:
		DoSizeChoice(menuItem);
		break;
*/
/*	case kMenuStyle:
		DoStyleChoice(menuItem);
		break;

	case kMenuColor:
		DoColorChoice(menuItem);
		break;
*/
	case kMenuFeatures:
		DoFeatureChoice(menuItem);
		break;

	case kMenuAlignment:
		DoAlignmentChoice(menuItem);
		break;

	case kMenuDirection:
		DoDirectionChoice(menuItem);
		break;
		
	case kWindows:
		doWindowsMenu(menuItem);
		break;
		
	}
	HiliteMenu(0);
}


/* ************************************************************************************************
InitializeMenus
************************************************************************************************ */
OSErr InitializeMenus(void)
{
	OSErr err = noErr;

	// build up the whole menu bar from the 'MBAR' resource
	SetMenuBar(GetNewMBar(kMenuBarID));

	// add names to the apple and Font menus
	AppendResMenu(GetMenuHandle(kMenuApple), kTypeDeskAccessory);
	AppendResMenu(GetMenuHandle(kMenuFont), kTypeFont);

	// insert the alignment and direction submenus into the hierarchical
	// portion of the menu list
	InsertMenu(GetMenu(kMenuAlignment), -1);
	InsertMenu(GetMenu(kMenuDirection), -1);

	// disable the "Drag and Drop Editing" item in the Features menu once and for all
	// if the Drag Manager isn't available
	if (! gHasDragAndDrop) {
		DisableItem(GetMenuHandle(kMenuFeatures), kItemDragAndDrop);
	}
	// disable the "Other" item in the Color menu if Color QuickDraw isn't available
	if (! gHasColorQD) {
		DisableItem(GetMenuHandle(kMenuColor), kItemOtherColor);
	}
	// load the menu color table for the color menu
	sColors = GetResource(kTypeMenuColorTable, kMenuColor);
	if ((err = ResError()) != noErr) {
		return err;
	}
	HNoPurge(sColors);
	// draw the menu bar
	DrawMenuBar();
	return err;
}


/* ************************************************************************************************
do_Print
Do_Print is a independent function, in here, if you set the hdl of picture, and call doPrinting,
you can print out a picture too.
You are not required to know how it work. (printing1.c , printing2.c, print.h)
************************************************************************************************ */

void do_Print(void){

	WindowPtr	window;
	WEReference	we;
	DevDesc		*dd;
	SInt16		WinIndex;
	GrafPtr		savePort;
	GrafPtr		picPort;
	Point		linesize;
	Point		*lp;
	
	linesize.h = 10;
	linesize.v = 7; 
	lp = &linesize;
	window = FrontWindow();
	we = GetWindowWE(window);
	// defaultPort = false;
	if (isGraphicWindow(window)) {
		PrintPicture = true;
		WinIndex = isGraphicWindow(window);
		dd =(DevDesc*)gGReference[WinIndex].devdesc;
		GetPort(&savePort);
		HLock((Handle) gPictureHdl);
		SetPort(window);
		gPictureHdl = OpenPicture(&(window->portRect));
		GetPort(&picPort);
		PicComment(SetLineWidth, 4, (char**)&lp);
		playDisplayList(dd);
		SetPort(picPort);         
		ClosePicture();
		SetPort(FrontWindow());
		//PrintPicture();
		printLoop();
		HUnlock((Handle) gPictureHdl);
		//doPrinting();
		SetPort(savePort);
	}
	else {
		GetPort(&savePort);
		PrintPicture = false;
		SetPort(FrontWindow());
		HLock((Handle) gTextHdl);
		/* Get the Handle of the Text which need to be print */
		gTextHdl = WEGetText(we);
		/* Extern Procedure */
		//doPrinting();
		printLoop();
		HUnlock((Handle) gTextHdl);
		SetPort(savePort);
	}
	defaultPort = true;
	DoActivate(false, FrontWindow());
	DoActivate(true, FrontWindow());
 	// SetPort(FrontWindow());
 	// DoUpdate(FrontWindow());
}



/* ************************************************************************************************
do_PageSetup
This function is a plugin function, it is work together with the do_Printing function.
************************************************************************************************ */
void do_PageSetup(void)
{
	// gInhibitPrintRecordsInfo = false;
	// gInhibitPrintStructuresInfo = false;
	// gPrintRecordInited = false;
	// gPrintStructureInited = false;
	SetPort(FrontWindow());
	doPrStyleDialog();
}


/* ************************************************************************************************
OpenSelection : Given a FSSpecPtr to either an application or a document, OpenSelection creates a
finder Open Selection Apple event for the object described by the FSSpec.
************************************************************************************************ */
OSErr OpenSelection(FSSpecPtr theDoc)
{
	AppleEvent			aeEvent;			// the event to create;
	AEDesc				myAddressDesc;		// descriptors for the 
	AEDesc				aeDirDesc;
	AEDesc				listElem;
	AEDesc				fileList;			// our list
	FSSpec				dirSpec;
	AliasHandle			dirAlias;			// alias to directory with our file
	AliasHandle			fileAlias;			// alias of the file itself
	ProcessSerialNumber	process;			// the finder's psn
	OSErr				myErr;				// duh

	// Get the psn of the Finder and create the target address for the .
	if(FindAProcess(kFinderSig,kSystemType,&process))
		return procNotFound;
	myErr = AECreateDesc(typeProcessSerialNumber,(Ptr) &process,
						 sizeof(process), &myAddressDesc);
	if(myErr)
		return myErr;

	// Create an empty 
	myErr = AECreateAppleEvent(kAEFinderEvents, kAEOpenSelection, 
				&myAddressDesc, kAutoGenerateReturnID, kAnyTransactionID, &aeEvent);
	if(myErr)
		return myErr;

	// Make an FSSpec and alias for the parent folder, and an alias for the file
	FSMakeFSSpec(theDoc->vRefNum,theDoc->parID,nil,&dirSpec);
	NewAlias(nil,&dirSpec,&dirAlias);
	NewAlias(nil,theDoc,&fileAlias);

	// Create the file list.
	if(myErr=AECreateList(nil,0,false,&fileList))
		return myErr;

	// Create the folder descriptor
	HLock((Handle)dirAlias);
	AECreateDesc(typeAlias, (Ptr) *dirAlias, GetHandleSize
				((Handle) dirAlias), &aeDirDesc);
	HUnlock((Handle)dirAlias);
	DisposeHandle((Handle)dirAlias);

	if((myErr = AEPutParamDesc(&aeEvent,keyDirectObject,&aeDirDesc)) == noErr) {
		AEDisposeDesc(&aeDirDesc);
		HLock((Handle)fileAlias);
		AECreateDesc(typeAlias, (Ptr)*fileAlias,
					GetHandleSize((Handle)fileAlias), &listElem);
		HUnlock((Handle)fileAlias);
		DisposeHandle((Handle)fileAlias);
		myErr = AEPutDesc(&fileList,0,&listElem);
	}
	if(myErr)
		return myErr;
	AEDisposeDesc(&listElem);

	if(myErr = AEPutParamDesc(&aeEvent,keySelection,&fileList))
		return myErr;

	myErr = AEDisposeDesc(&fileList);

	myErr = AESend(&aeEvent, nil,
				kAENoReply+kAEAlwaysInteract+kAECanSwitchLayer,
				kAENormalPriority, kAEDefaultTimeout, nil, nil);
	AEDisposeDesc(&aeEvent);
}


/* ************************************************************************************************
 FindAProcess
************************************************************************************************ */
OSErr FindAProcess(OSType typeToFind, OSType creatorToFind, ProcessSerialNumberPtr processSN)
{
	ProcessInfoRec	tempInfo;
	FSSpec			procSpec;
	Str31			processName;
	OSErr			myErr = noErr;

	// start at the beginning of the process list
	processSN->lowLongOfPSN = kNoProcess;
	processSN->highLongOfPSN = kNoProcess;

	// initialize the process information record
	tempInfo.processInfoLength = sizeof(ProcessInfoRec);
	tempInfo.processName = (StringPtr)&processName;
	tempInfo.processAppSpec = &procSpec;

	while((tempInfo.processSignature != creatorToFind ||
			tempInfo.processType != typeToFind) ||
			myErr != noErr)
	{
		myErr = GetNextProcess(processSN);
		if (myErr == noErr)
			GetProcessInformation(processSN, &tempInfo);
	}
	return(myErr);
}


/* ************************************************************************************************
assignPString : convert char* into unsigned char* with length howLong.
************************************************************************************************ */
void assignPString(unsigned char* input, char* buf, SInt16 howLong)
{
	SInt16	i;
	input[0] = howLong;
	for (i = 1; i <= howLong; i++){
		input[i] = buf[i - 1];
	}
}


/* ************************************************************************************************
ConsoleCopyAndPaste : 
A procedure which will be directly called when you choice Copy and Paste from the Menus.
************************************************************************************************ */
void ConsoleCopyAndPaste()
{
	WEReference	we;
	we = GetWindowWE(Console_Window);
	WECopy(we);
	ConsolePaste();
}

/* ************************************************************************************************
ConsolePaste : 
This procedure will be either called by procedure ConsoleCopyAndPaste or from the Menus Choice
We will read the text we have line by line.
The trick we use is that we will directly send a key event ("\r") to terminate the 
R_ReadConsole procedure after each line.
************************************************************************************************ */
void ConsolePaste()
{
	WEReference	we;
	SInt16		i;
	long		scrapOffset, rc;
	EventRecord	myEvent;
	char		TempChar;
   
	we = GetWindowWE(Console_Window);

	WESetSelection(WEGetTextLength(we), WEGetTextLength(we), we);   
	        
	myHandle = NewHandle(0);	/* allocate 0-length data area */

	rc = GetScrap(myHandle, 'TEXT', &scrapOffset);
	if (rc < 0) {
		/* . . . process the error . . . */
	}
	else {
		SetHandleSize(myHandle, rc + 1);	/* prepare for printf() */
		HLock(myHandle);
		(*myHandle)[rc] = 0;				/* make it ASCII */
		finalPaste = rc;
		for (i = 0; i <= rc; i++) {
			TempChar = (*myHandle)[i];
			if ((TempChar == '\r') || (i == finalPaste)){
				RnWrite(*myHandle, i);
				if (i != finalPaste) {
					myEvent.message = 140301;
					DoKeyDown(&myEvent);
				}
				curPaste = i;            
				break;
			}
		}
		finalPaste = rc;
		HUnlock(myHandle);
		if (rc != i) HaveContent = true;
		else DisposeHandle(myHandle);
	}
}

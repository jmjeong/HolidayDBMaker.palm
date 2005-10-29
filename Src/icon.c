/*
 * HolidayDBMaker.c
 *
 * Copyright (c) 2003-2005 Jaemok Jeong<jmjeong@gmail.com>
 *
 */
 
#include <PalmOS.h>
#include <PalmOSGlue.h>

#include "HolidayDBMaker.h"
#include "datebook.h"
#include "HolidayDBMaker_Rsc.h"

#include "lunar.h"

extern Char *noteTempBuffer;
extern DmOpenRef IconDB;

Boolean IconSelectHandleEvent(EventPtr eventP);

DmOpenRef OpenIconSet(const Char *name, Boolean canUseDefault);
Boolean IconSelectHandleEvent(EventPtr eventP);
void DrawFormANIcon(Int16 iconNumber, Int16 id);

void AdjustIconSelTrigger(Int16 idx);
void ProcessIconSelect();
Char* AdjustNote(Int16 iconNumber, Char* str);
UInt16 DisplayDialog( UInt16 formID, FormEventHandlerPtr handler );
void * GetObjectPtr(UInt16 objectID);

static Int16 LoadIcon(Int16 start);
static void IconFormScrollLines(Int16 lines);
static Int16 DrawANIcon(Int16 iconNumber, Int16 x, Int16 y);

// trick : only used in todopref.c
// 	(Bugs in FrmDoDialog : always return Cancel)
Boolean IconSelectCancelPressed;

static void ShowObject (FormPtr frm, UInt16 objectID)
{
	FrmShowObject (frm, FrmGetObjectIndex (frm, objectID));
}

static void HideObject (FormPtr frm, UInt16 objectID)
{
	FrmHideObject (frm, FrmGetObjectIndex (frm, objectID));
}

/*
 * FUNCTION: GetObjectPtr
 *
 * DESCRIPTION:
 *
 * This routine returns a pointer to an object in the current form.
 *
 * PARAMETERS:
 *
 * formId
 *     id of the form to display
 *
 * RETURNED:
 *     address of object as a void pointer
 */
void * GetObjectPtr(UInt16 objectID)
{
	FormType * frmP;

	frmP = FrmGetActiveForm();
	return FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID));
}

/*
 * OpenIconSet - returns a DmOpenRef to an icon set
 *
 * Arguments:
 *  name            = Optional name of icon set to open (can be NULL).
 *  canUseDefault   = Indicates if it's ok to open a default icon set if
 *                    the named icon set isn't available.
 *
 * Returns:
 *  NULL            = Unable to open an icon set.
 *  non-NULL        = DmOpenRef to the icon set database.
 *
 * Remarks:         If OpenIconSet() returns a valid DmOpenRef, the caller
 *                  is required to call DmCloseDatabase() to close it when
 *                  the caller is finished with it.
 */
DmOpenRef OpenIconSet(const Char *name, Boolean canUseDefault)
{
    DmOpenRef pdb = 0;

    // If 'name' was specified, try to open it.

    if (name && *name)
    {
        const UInt16 card = 0;
        const LocalID lid = DmFindDatabase(card, name);

        if (lid)
        {
            UInt32 type;
            UInt32 creator;

            // Double check the type and creator.

            DmDatabaseInfo(card, lid, 0, 0, 0, 0, 0, 0, 0, 0, 0, &type, &creator);

            if ('Rsrc' == type && 'Actn' == creator)
                pdb = DmOpenDatabase(card, lid, dmModeReadOnly);
        }
    }

    // Fall back to the default icon set if we need to.

    if (!pdb && canUseDefault)
        pdb = DmOpenDatabaseByTypeCreator('Rsrc', 'Actn', dmModeReadOnly);

    // At this point we may or may not have been able to open an icon set.
    // The return value may be NULL or it may point to an icon set.

    return pdb;
}

static Int16 DrawIconBitmap(UInt16 iconID, Coord x, Coord y)
{
	MemHandle pointH;
	BitmapPtr pbmp;
	
	if (IconDB) {
	 	pointH = DmGet1Resource('Tbmp', iconID);
		pbmp = (pointH) ? MemHandleLock(pointH) : NULL;

	    if (pbmp) {
	    	WinDrawBitmap(pbmp, x, y); 
	    	
	        MemHandleUnlock(pointH);
	        DmReleaseResource(pointH);
            return 1;
		}
	}

	return 0;
}

Int16 DrawANIcon(Int16 iconNumber, Int16 x, Int16 y)
{
	// icon handling
    Int16       curFont;
    Int16       height, adjust = 0;
    Int16       width = 0;

    curFont = FntSetFont(stdFont);
    height = FntCharHeight();
    FntSetFont(curFont);

    // 20 x 18;
    //
    adjust = 9;
    width = 12;
    y += (height - adjust)/2;
    
    if (DrawIconBitmap(iconNumber, x, y+1)) {
        return width;
    }
    else return 0;
}

// Graphics icon setting
//
void DrawFormANIcon(Int16 iconNumber, Int16 id)
{
    FormPtr frmP = FrmGetActiveForm();
    Int16 x, y;
    RectangleType r;
    UInt16 idx = FrmGetObjectIndex(frmP, id);

    FrmGetObjectPosition(frmP, idx, &x, &y);
    FrmGetObjectBounds(frmP, idx, &r);
    
    r.topLeft.x = x;
    r.topLeft.y = y;

    WinEraseRectangle(&r, 0);

    if (iconNumber >= 0) DrawANIcon(iconNumber, x+1, y);
}

void AdjustIconSelTrigger(Int16 id)
{
    FormPtr frmP = FrmGetActiveForm();
    RectangleType r;
    UInt16  idx = FrmGetObjectIndex(frmP, id);

    FrmGetObjectBounds(frmP, idx, &r);

    r.extent.x = 12;
    r.extent.y = 11;

    FrmSetObjectBounds(frmP, idx, &r);
}

UInt16 DisplayDialog( UInt16 formID, FormEventHandlerPtr handler ) 
{ 
	FormPtr active = FrmGetActiveForm();
    
	UInt16 activeID = ( active != NULL ) ? FrmGetFormId( active ) : 0;
	FormPtr dialog = FrmInitForm( formID );
	UInt16 buttonPressed = 0;
    
	FrmSetActiveForm( dialog ); 
	if( handler ){ 
		FrmSetEventHandler( dialog, handler ); 
	} 
	FrmPopupForm( formID ); 
	buttonPressed = FrmDoDialog( dialog ); 
	FrmReturnToForm( activeID );
    
	return buttonPressed;
} 

void ProcessIconSelect()
{
    if (IconDB) {
        DisplayDialog(IconSelectForm, IconSelectHandleEvent) ;
    }
    else {
        FrmCustomAlert(ErrorAlert,  "IconDB is not installed", "", "");
    }
}

static void IconFormScroll(Int16 newValue, Int16 oldValue)
{
	FormPtr 	frm = FrmGetActiveForm();

	if(oldValue != newValue) {
		LoadIcon(newValue*8);
	}
}

static void IconFormScrollLines(Int16 lines)
{
	ScrollBarPtr    barP;
    Int16           valueP, minP, maxP, pageSizeP;
    Int16           newValue;
	FormPtr			frm = FrmGetActiveForm();

    barP = GetObjectPtr(IconSelectScrollBar);
    SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

    //scroll up
    if(lines < 0)
    {
        //we are at the start
        if(valueP == minP)
            return;

        newValue = valueP + lines;
        if(newValue < minP)
            newValue = minP;
    }
    else
    {
        if(valueP == maxP)
            return;

        newValue = valueP + lines;
        if(newValue  > maxP) 
            newValue = maxP;
    }

    SclSetScrollBar(barP, newValue, minP, maxP, pageSizeP);
    IconFormScroll(newValue, valueP);
}

/*
 * GetIconByIndex - get an icon by index.
 *
 * Call this to get a BitmapPtr for the specified icon index number.  This
 * returns NULL if the resouce at the specified index is not an icon, or if
 * the specified index is out of range (greater than the number returned by
 * DmNumResources() for the icon set).
 */
static BitmapPtr GetIconByIndex(DmOpenRef pdb, UInt16 iconIndex)
{
    UInt32 typeRes;

    DmResourceInfo(pdb, iconIndex, &typeRes, 0, 0);

    if (bitmapRsc != typeRes)
        return NULL;

    return (BitmapPtr)DmGetResourceIndex(pdb, iconIndex);
}

static Int16 LoadIcon(Int16 start)
{
	FormPtr     frmP = FrmGetActiveForm();
	Int8	    i;
    Int16       num;
    Int16       iconButtonID = IconSelectIcon1Button;
    BitmapPtr   pbmp;
    MemHandle   h;
    Int16       x, y;
    RectangleType r;

    num = DmNumResources(IconDB);
    if (start > num) start = 0;
    
	for (i = 0; i < 9*8; i++) {
        UInt16 idx = FrmGetObjectIndex(frmP, iconButtonID + i);

        FrmGetObjectPosition(frmP, idx, &x, &y);
        FrmGetObjectBounds(frmP, idx, &r);

        r.topLeft.x = x;
        r.topLeft.y = y;

        WinEraseRectangle(&r, 0);

        if (i+start <num) {
            h = (MemPtr)GetIconByIndex(IconDB, i + start);
            pbmp = (h) ? MemHandleLock(h) : NULL;

            if (pbmp) {
                WinDrawBitmap(pbmp, x, y);
                                
                MemHandleUnlock(h);
                DmReleaseResource(h);
                ShowObject(frmP, iconButtonID + i);
            }
            else {
                HideObject(frmP, iconButtonID + i);
            }
        }
        else {
            HideObject(frmP, iconButtonID + i);
        }
	}
    return num;
}

Boolean IconSelectHandleEvent(EventPtr eventP)
{
   	Boolean handled = false;
   	FormPtr frmP = FrmGetActiveForm();
    Int16   num;
	ScrollBarPtr barP;

	switch (eventP->eType) 
    {
    case menuEvent:
        return handled;
			
    case frmOpenEvent:
        FrmDrawForm(frmP);
        num = LoadIcon(0);

        barP = GetObjectPtr(IconSelectScrollBar);
        if (num > 9*8) {
            SclSetScrollBar(barP, 0, 0, num/8-8, 9);
        }
        else {
            SclSetScrollBar(barP, 0, 0, 0, 9);
        }
        handled = true;
        break;
			
    case ctlSelectEvent:
    	IconSelectCancelPressed = false;
        if (eventP->data.ctlSelect.controlID >= IconSelectIcon1Button &&
            eventP->data.ctlSelect.controlID < IconSelectIcon1Button+9*8) {
            Int16   val, min, max, page;
            DmResID resID;

            SclGetScrollBar(GetObjectPtr(IconSelectScrollBar),
                            &val, &min, &max, &page);

            DmResourceInfo(IconDB, (val*8+ eventP->data.ctlSelect.controlID - IconSelectIcon1Button),
                           NULL, &resID, NULL);
			g_prefs.iconNumber = resID;                           
        }
        switch (eventP->data.ctlSelect.controlID) {
        case IconSelectNoneButton:
            g_prefs.iconNumber = -1;
            break;
        case IconSelectCancelButton:
        	IconSelectCancelPressed = true;
            break;
        }

        // FrmUpdateForm(NewTodoForm, frmRedrawUpdateCode+1);
        // FrmUpdateForm(NewEventForm, frmRedrawUpdateCode+1);
        FrmReturnToForm(0);
        
        handled = true;
        break;

	case sclRepeatEvent:
		IconFormScroll(eventP->data.sclRepeat.newValue, 
                       eventP->data.sclRepeat.value);
		break;

    case keyDownEvent: 
    {
		Int16		valueP, minP, maxP, pageSizeP;

		ScrollBarPtr barP = GetObjectPtr(IconSelectScrollBar);
		SclGetScrollBar(barP, &valueP, &minP, &maxP, &pageSizeP);

		switch (eventP->data.keyDown.chr) {
		case vchrPageUp:
		case vchrPrevField:
			IconFormScrollLines(-(pageSizeP-1));
			break;
		case vchrPageDown:
		case vchrNextField:
			IconFormScrollLines((pageSizeP-1));
			handled = true;
			break;
        }
        break;
    }
        
    default:
        break;
		
    }
	
	return handled;
}

Char* AdjustNote(Int16 iconNumber, Char* note)
{
    extern Char gAppErrStr[];
    Char *p, *q;

    noteTempBuffer[0] = 0;
    
    if (!note) {
        // 지금 가지고 있는 게 없을 경우
        StrPrintF(noteTempBuffer, "ICON: %d\n#AN\n", iconNumber);
    }
    else {
        // note field에 내용이 있는 경우
        if ( (p= StrStr(note, "#AN\n")) ) {
            // AN string을 가지고 있는 경우
            
            if (iconNumber < 0) {
                // 있는 ICON: 을 제거
                q = StrStr(note, "ICON:");

                if (q == note) {
                    q = StrStr(p, "\n");
                    StrNCopy(noteTempBuffer, q+1, 4096);
                }
                else {
                    StrNCopy(noteTempBuffer, note, q-note);
                    q = StrStr(q, "\n");
                    StrNCat(noteTempBuffer, q+1, 4096);
                }
            }
            else {
                // 있는 ICON: 변경
                q = StrStr(note, "ICON:");
                if (q) {
                    StrNCopy(noteTempBuffer, note, q-note);
                    q = StrStr(q, "\n") + 1;
                }
                else {
                    StrNCopy(noteTempBuffer, note, p-note);
                    q = p;
                }

                StrPrintF(gAppErrStr, "ICON: %d\n", iconNumber);
                StrNCat(noteTempBuffer, gAppErrStr, 4096);

                StrNCat(noteTempBuffer, q, 4096);
            }
        }
        else {
            StrPrintF(noteTempBuffer, "ICON: %d\n#AN\n", iconNumber);
            StrNCat(noteTempBuffer, note, 4096);
        }
        
        
    }
    return noteTempBuffer;
}

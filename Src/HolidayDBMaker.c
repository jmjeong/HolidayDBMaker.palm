/*
 * HolidayDBMaker.c
 *
 * main file for HolidayDBMaker
 *
 * This wizard-generated code is based on code adapted from the
 * stationery files distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */
 
#include <PalmOS.h>
#include <PalmOSGlue.h>

#include "HolidayDBMaker.h"
#include "HolidayDBMaker_Rsc.h"

/*********************************************************************
 * Global variables
 *********************************************************************/
/* 
 * g_prefs
 * cache for application preferences during program execution 
 */
HolidayDBMakerPreferenceType g_prefs;
DmOpenRef					 g_strDB;
DmOpenRef					 g_HolidayDB;
DateFormatType				 g_prefdfmts;


/*********************************************************************
 * Internal Constants
 *********************************************************************/

/* Define the minimum OS version we support */
#define ourMinVersion    sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0)
#define kPalmOS20Version sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0)

/*********************************************************************
 * Internal Functions
 *********************************************************************/

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

static void * GetObjectPtr(UInt16 objectID)
{
	FormType * frmP;

	frmP = FrmGetActiveForm();
	return FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID));
}


static Int16 DateCompare (DateType d1, DateType d2)
{
    UInt16 int1, int2;

    int1 = DateToInt(d1);
    int2 = DateToInt(d2);

    if (int1 > int2)
        return (1);
    else if (int1 < int2)
        return (-1);
    return 0;
}


//  SetFieldTextFromHandle - set txtH into field object text
//
static FieldPtr SetFieldTextFromHandle(FieldPtr fldP, MemHandle txtH)
{
    MemHandle   oldTxtH;
    
    oldTxtH  = FldGetTextHandle(fldP);

    // set the field's text to the new text.
    FldSetTextHandle(fldP, txtH);

    // free the handle AFTER we call FldSetTextHandle().
    if (oldTxtH)
        MemHandleFree((MemHandle)oldTxtH);

    return fldP;
}

// Allocates new handle and copies incoming string
//
static FieldPtr SetFieldTextFromStr(FieldPtr fld, Char * strP)
{
    MemHandle    txtH;

    // get some space in which to stash the string.
    txtH  = MemHandleNew(StrLen(strP) + 1);
    if (!txtH)
        return NULL;

    // copy the string to the locked handle.
    StrCopy((Char *)MemHandleLock(txtH), strP);

    // unlock the string handle.
    MemHandleUnlock(txtH);

    // set the field to the handle
    return SetFieldTextFromHandle(fld, txtH);
}

static void UpdateScrollBar(void)
{
    FormPtr          frm = FrmGetActiveForm();
    ScrollBarPtr     scroll;
    FieldPtr         field;
    UInt16           currentPosition;
    UInt16           textHeight;
    UInt16           fieldHeight;
    Int16            maxValue;

    field = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, MainField));
    FldGetScrollValues(field, &currentPosition, &textHeight, &fieldHeight);

    // if the field is 3 lines, and the text height is 4 lines
    // then we can scroll so that the first line is at the top 
    // (scroll position 0) or so the second line is at the top
    // (scroll postion 1). These two values are enough to see
    // the entire text.
    if (textHeight > fieldHeight)
        maxValue = textHeight - fieldHeight;
    else if (currentPosition)
        maxValue = currentPosition;
    else
        maxValue = 0;

    scroll = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, MainFieldScrollBar));

    // on a page scroll, want to overlap by one line (to provide context)
    SclSetScrollBar(scroll, currentPosition, 0, maxValue, fieldHeight - 1);
}

/*
 * FUNCTION: MainFormInit
 *
 * DESCRIPTION: This routine initializes the MainForm form.
 *
 * PARAMETERS:
 *
 * frm
 *     pointer to the MainForm form.
 */

static void MainFormInit(FormType *frmP)
{
	MemHandle recordH;
	Char* ptr;

	recordH = DmQueryRecord(g_strDB, 0);
	if (recordH) {
		ptr = MemHandleLock(recordH);
		
		SetFieldTextFromStr(GetObjectPtr(MainField), ptr);
		MemHandleUnlock(recordH);
	}
}

/*
 * FUNCTION: MainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:
 *
 * command
 *     menu item id
 */

static Boolean MainFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	switch (command)
	{
		case OptionsAboutHolidayDBMaker:
		{
			FormType * frmP;

			/* Clear the menu status from the display */
			MenuEraseStatus(0);

			/* Display the About Box. */
			frmP = FrmInitForm (AboutForm);
			FrmDoDialog (frmP);                    
			FrmDeleteForm (frmP);

			handled = true;
			break;
		}
		case OptionsHelp:
		{
			FrmHelp(HelpString);
			handled = true;
			break;
		}
		case OptionsPreferences:
		{
			FormType * frmP;
			ControlType * cbox = NULL;
			FieldType * field = NULL;			
			UInt16 controlID = 0;
			MemHandle handle = 0;
			char temp[50];
			FieldPtr fromField, toField;

			/* Clear the menu status from the display */
			MenuEraseStatus(0);

			/* Initialize the preference form. */
			frmP = FrmInitForm (PrefsForm);
			
			fromField = FrmGetObjectPtr(FrmGetFormPtr(PrefsForm), 
					FrmGetObjectIndex(FrmGetFormPtr(PrefsForm), PrefsFromField));
					
			toField = FrmGetObjectPtr(FrmGetFormPtr(PrefsForm), 
					FrmGetObjectIndex(FrmGetFormPtr(PrefsForm), PrefsToField));
										
			StrPrintF(temp, "%d", g_prefs.from);
			SetFieldTextFromStr(fromField, temp);
			
			StrPrintF(temp, "%d", g_prefs.to);
			SetFieldTextFromStr(toField, temp);
			
			controlID = FrmDoDialog (frmP);
			
			/* controlID contains the ID of the button used to dismiss the dialog */
			if (controlID == PrefsOKButton)
			{				
				if (FldDirty(fromField)) {
					char *p = FldGetTextPtr(fromField);
					g_prefs.from = StrAToI(p);
				}
	
				if (FldDirty(toField)) {
					char *p = FldGetTextPtr(toField);
					g_prefs.to = StrAToI(p);
				}
			}
			
			/* Clean up */
			FrmDeleteForm (frmP);

			handled = true;
			break;
		}

	}

	return handled;
}

static void ScrollLines(int numLinesToScroll, Boolean redraw)
{
    FormPtr        frm = FrmGetActiveForm();
    FieldPtr       field;

    field = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, MainField));
    if (numLinesToScroll < 0)
        FldScrollField(field, -numLinesToScroll, winUp);
    else
        FldScrollField(field, numLinesToScroll, winDown);

    // if there are blank lines at the end and we scroll up, FldScrollField
    // makes the blank lines disappear. Therefore, we've got to update
    // the scrollbar
    if ((FldGetNumberOfBlankLines(field) && numLinesToScroll < 0) || redraw)
        UpdateScrollBar();
}

static void PageScroll(WinDirectionType direction)
{
    FormPtr        frm = FrmGetActiveForm();
    FieldPtr       field;
    
    field = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, MainField));
    if (FldScrollable(field, direction)) {
        int linesToScroll = FldGetVisibleLines(field) - 1;

        if (direction == winUp)
            linesToScroll = -linesToScroll;
        ScrollLines(linesToScroll, true);
    }
}

static void SaveFieldDataToDB()
{
	MemHandle recordH;
	FieldPtr fld;
	char*	ptr = 0;
	MemPtr	s; 
	UInt16 index = 0;
	
	fld = GetObjectPtr(MainField);
	
	if (FldDirty(fld)) {
		FldCompactText(fld);
		ptr = FldGetTextPtr(fld);
		
		if (DmQueryRecord(g_strDB, 0)) {
			recordH = DmGetRecord(g_strDB, 0);
			ErrFatalDisplayIf(!recordH, "error");
			MemHandleResize(recordH, StrLen(ptr)+1);
		}
		else {
			recordH = DmNewRecord(g_strDB, &index, StrLen(ptr)+1);
		}
		s = MemHandleLock(recordH);
		DmWrite(s, 0, ptr, StrLen(ptr)+1 );
		
		MemHandleUnlock(recordH);
		
		DmReleaseRecord(g_strDB, index, true);
	}
	g_prefs.offset = FldGetInsPtPosition(fld);
}

static void CleanupHoliday(DmOpenRef dbP)
{
    UInt16 currindex = 0;
    MemHandle recordH = 0;

    if (dbP) {
        while (1) {
            recordH = DmQueryNextInCategory(dbP, &currindex, 
                                            dmAllCategories);
            if (!recordH) break;

            DmRemoveRecord(dbP, currindex);     // remove all traces
        }
    }
}


static Int16 CreateHolidayDB()
{
    UInt16 mode = 0;

    g_HolidayDB = DmOpenDatabaseByTypeCreator('DATA', holiFileCreator,
                                         mode | dmModeReadWrite);
    if (g_HolidayDB){
        CleanupHoliday(g_HolidayDB);
    }
    else {
        //
        // Create our database if it doesn't exist yet
        //
        if (DmCreateDatabase(0, "HolidayDB", holiFileCreator, 'DATA', false))
            return -1;
        g_HolidayDB = DmOpenDatabaseByTypeCreator('DATA', holiFileCreator,
                                             dmModeReadWrite);
        if (!g_HolidayDB) return -1;
    }

    return 0;
}

static void CloseHolidayDB()
{
    DmCloseDatabase(g_HolidayDB);
}   

static void PackHoliday(DateType* date, void* recordP)
{
    DmWrite(recordP, 0, (Char*)date, sizeof(DateType));
}

static Int16 HolidayComparePackedRecords(DateType *rec1,
                                     DateType *rec2,
                                     Int16 unusedInt,
                                     SortRecordInfoPtr unused1,
                                     SortRecordInfoPtr unused2,
                                     MemHandle appInfoH)
{
    return DateCompare(*rec1, *rec2);
}


static UInt16 HolidayFindSortPosition(DmOpenRef dbP, void* date)
{
    return DmFindSortPosition(dbP, (void*)date, 0,(DmComparF *)HolidayComparePackedRecords, 0);
}



static Int16 HolidayNewRecord(DmOpenRef dbP, DateType *r, UInt16 *index)
{
    MemHandle recordH;
    char* recordP;
    Int16 err;

    // 1) and 2) (make a new chunk with the correct size)
    recordH = DmNewHandle(dbP, (Int16)sizeof(DateType));
    if (recordH == NULL)
        return dmErrMemError;

    // 3) Copy the data from the unpacked record to the packed one.
    recordP = MemHandleLock(recordH);
    PackHoliday(r, recordP);

    MemPtrUnlock(recordP);

    // 4) attach in place
    err = DmAttachRecord(dbP, index, recordH, 0);
    if (err)
        MemHandleFree(recordH);

    return err;
}

static void MakeActualNewHolidayRecord(UInt16 day, UInt16 month, UInt16 year, Holiday hd)
{
    DateType r, *ptr;
    UInt16 index;
    MemHandle recordH;
    Boolean found = false;

    r.day = day;
    r.month = month;
    r.year = year - firstYear;

    index = DmFindSortPosition(g_HolidayDB, &r, NULL, (DmComparF*)HolidayComparePackedRecords, 0);
    if (index > 0) {
        recordH = DmQueryRecord(g_HolidayDB, index - 1);
        if (recordH) {
            ptr = MemHandleLock(recordH);
            if (DateCompare(*ptr, r) == 0) {
                found = true;
            }
            MemHandleUnlock(recordH);
        }
    }
    if (!found)
        HolidayNewRecord(g_HolidayDB, &r, &index);
}

static void MakeNewHolidayRecord(UInt16 day, UInt16 month, UInt16 year, Holiday hd)
{
    Int16 i;
    DateType temp;
    
    if (hd.duration_count == 0) 
        MakeActualNewHolidayRecord(day, month, year, hd);
    else {
        for (i = 0; i < hd.duration_count; i++) {
            temp.day = day;
            temp.month = month;
            temp.year = year - firstYear;

            DateAdjust(&temp, hd.duration[i]);

            MakeActualNewHolidayRecord(temp.day, temp.month, temp.year+firstYear, hd);
        }
    }
}

static Int16 AnalOneRecord(char* src, Holiday* hd)
{
    char *p, *q;
    char *s = src;
    char delimeter = '/';
    Char numOfDelimeter = 0;
    Int16 num[3], numIdx = 0;

    hd->flag.allBits = 0;
    hd->duration_count = 0;
    
    while (*src == ' ' || *src == '\t') src++;

    if (*src == ';') return true;

    // ; 이 있으면 그 담은 무시
    if ((q = StrChr(src, ';'))) *q = 0;

    if ((q = StrChr(src, '['))) {
        // [ 이 있는 경우 duration으로 판단 
        p = q;

        q = StrChr(src, ']');
        if (q) *q = 0;
        else return false;

        do {
            p++;
            hd->duration[hd->duration_count++] = StrAToI(p);
            if (hd->duration_count >= 10) break;
        } while ((p = StrChr(p, ',')));
    }

    if (StrNCaselessCompare(src, "@c", 2) == 0) {
        // @c 인 경우
        hd->flag.bits.dayofmonth = 1;
        
        p = StrChr(src, '(');
        if (!p) return false;
        q = StrChr(src, ')');
        if (!q) return false;
        else *q = 0;

        
        while (*++p == ' ')
            ;
        hd->month = StrAToI(p);
        
        p = StrChr(src, ',');
        if (!p) return false;
        while (*++p == ' ')
            ;
        hd->weekOfMonth = StrAToI(p);
        
        p = StrChr(p, ',');
        if (!p) return false;
        while (*++p == ' ')
            ;
        hd->dayOfWeek = StrAToI(p);

        p = StrChr(p, ',');
        if (!p) {
            hd->flag.bits.year = 0;
        }
        else {
            hd->flag.bits.year = 1;
            while (*++p == ' ') 
                ;
            hd->year = StrAToI(p);
        }

        if (hd->weekOfMonth > 5 || hd->weekOfMonth < -1
            || hd->dayOfWeek > 6 || hd->dayOfWeek < 0) return false;
        
        goto check;
    }
    else if (StrNCaselessCompare(src, "@easter", 7) == 0) 
    {
        hd->flag.bits.easter = 1;
        
        // @easter (easter sunday 계산)
        p = src + 7;
        q = StrChr(p, '(');
        if (q) {
            while (*++q == ' ')
                ;
            hd->year = StrAToI(q);
        }
        else hd->flag.bits.year = 0;

        goto check;
    }
                     
    if ((p = StrChr(s, ')'))) {
        if (*s == '-') hd->flag.bits.lunar = 1;
        else if (*s == '#') hd->flag.bits.lunar_leap = 1;
        else return false;

        s = p+1;
    }
    else hd->flag.bits.solar = 1;

    switch (g_prefdfmts) {
    case dfDMYWithDots:          // 31.12.95
    case dfYMDWithDots:          // 95.12.31
        delimeter = '.';
        break;
    case dfDMYWithDashes:        // 31-12-95
    case dfYMDWithDashes:        // 95-12-31
        delimeter = '-';
        break;
    case dfYMDWithSlashes:       // 95/12/31
    case dfDMYWithSlashes:       // 31/12/95
    case dfMDYWithSlashes:       // 12/31/95
    default:
        delimeter = '/';
        break;
    }

    p = s;
    while ((p = StrChr(p, delimeter)))
    {
        numOfDelimeter++;
        p++;
    }
    
    p = s;
    if (numOfDelimeter < 1 || numOfDelimeter > 2) return false;
    else if (numOfDelimeter == 2) {
        p = StrChr(p, delimeter);
        *p = 0; p++;
        num[numIdx++] = StrAToI(s);
        s = p;
    }
    
   p = StrChr(s, delimeter); *p = 0; p++;
    num[numIdx++] = StrAToI(s);
    s = p;

    if (*s == 0) hd->flag.bits.year = 0;
    else {
        hd->flag.bits.year = 1;
        num[numIdx] = StrAToI(s);
    }

    if (numOfDelimeter == 1) {
       	hd->flag.bits.year = 0;
    }

    switch (g_prefdfmts) {
    case dfDMYWithSlashes:       // 31/12/95
    case dfDMYWithDots:          // 31.12.95
    case dfDMYWithDashes:        // 31-12-95
        if (numOfDelimeter == 2) {
            hd->year = num[numIdx--];
        }
        hd->month = num[numIdx--];
        hd->day = num[numIdx];
        break;
    case dfYMDWithSlashes:       // 95/12/31
    case dfYMDWithDots:          // 95.12.31
    case dfYMDWithDashes:        // 95-12-31
        hd->day = num[numIdx--];
        hd->month = num[numIdx--];

        if (numOfDelimeter == 2) {
            hd->year = num[numIdx];
        }
        break;
    case dfMDYWithSlashes:       // 12/31/95
    default:
        if (numOfDelimeter == 2) {
            hd->year = num[numIdx--];
        }
        hd->day = num[numIdx--];
        hd->month = num[numIdx];
        break;
    }
    
 check:
    if (hd->month < 1 || hd->month > 12)
        return false;
    if (hd->day < 1 || hd->day > 31)
        return false;
    
    if (hd->flag.bits.year) {
        if (hd->year >= 0 && hd->year <= 31) hd->year += 2000;          // 2000
        else if (hd->year > 31 && hd->year <= 99) hd->year += 1900;     // 1900
    }
    
    return true;
}

static Boolean FindNearLunar(DateType *dt, DateType base, Boolean leapyes)
{
    int lunyear, lunmonth, lunday;
    DateTimeType rtVal;
    Int16 i;
    extern int lun2sol(int lyear, int lmonth, int lday, int leapyes,
                  DateTimeType *solar);


    lunmonth = dt->month; lunday = dt->day;

    for (i = -1; i < 5; i++) {
        lunyear = base.year + firstYear + i;

        if  (lun2sol(lunyear, lunmonth, lunday, leapyes, &rtVal)) continue;
         
        dt->year = rtVal.year - 1904;
        dt->month = rtVal.month; dt->day = rtVal.day;
    
        if (DateCompare(base, *dt) <= 0) return true;
    }
    return false;
}

static void CalcHoliday(Int16 *day, Int16 month, Int16 year, Int8 dayOfWeek, Int8 weekOfMonth)
{
    Int16 temp, lastday;

    if (weekOfMonth == -1) {
        lastday = DaysInMonth(month, year);
        temp = (DayOfWeek(month, lastday, year) - dayOfWeek + 7) % 7;
        *day = lastday - temp;
    }
    else {
        temp = (dayOfWeek - DayOfWeek(month, 1, year) + 7) % 7;
        *day = (7 * weekOfMonth) - 6 + temp;
    }
    return;
}

static void ProcessCalcHoliday(Holiday hd)
{
    Int16 i;
    Int16 day, month, year;

    month = hd.month;
    
    if (hd.flag.bits.year) {
        year = hd.year;

        CalcHoliday(&day, month, year, hd.dayOfWeek, hd.weekOfMonth);
        MakeNewHolidayRecord(day, month, year, hd);
    }
    else {
        for (i = g_prefs.from; i <= g_prefs.to; i++) {
            year = i;
            
            CalcHoliday(&day, month, year, hd.dayOfWeek, hd.weekOfMonth);
            MakeNewHolidayRecord(day, month, year, hd);
        }
    }
}

static void CalcEaster(Int16 *day, Int16 *month, Int16 year)
{
    Int16 a,b,c,d,e,f,g,h,i,k,l,m,p;

    a= year %19;
    b = year/100;
    c = year%100;
    d = b/4;
    e = b%4;
    f = (b+8)/25;
    g = (b-f+1)/3;
    h = (19*a+b-d-g+15)%30;
    i = c / 4;
    k = c % 4;
    l = (32+2*e+2*i-h-k)%7;
    m = (a+11*h+22*l)/451;
    p = (h+l-7*m+114)%31;
    *month = (h+l-7*m+114)/31;
    *day = p + 1;
}

static void ProcessEaster(Holiday hd)
{
    Int16 day, month, year, i;
    
    if (hd.flag.bits.year) {
        year = hd.year;

        CalcEaster(&day, &month, year);
        MakeNewHolidayRecord(day, month, year, hd);
    }
    else {
        for (i = g_prefs.from; i <= g_prefs.to; i++) {
            year = i;
            
            CalcEaster(&day, &month, year);
            MakeNewHolidayRecord(day, month, year, hd);
        }
    }
}

static void MakeNewEntry(Holiday hd)
{
    Int16 i;

    if (hd.flag.bits.dayofmonth) {
        ProcessCalcHoliday(hd);
    }
    else if (hd.flag.bits.easter) {
        ProcessEaster(hd);
    }
    else if (hd.flag.bits.solar) {
        if (hd.flag.bits.year) {
            if (hd.day <= DaysInMonth(hd.month, hd.year)) {
                // if exists
                MakeNewHolidayRecord(hd.day, hd.month, hd.year, hd);
            }
        }
        else {
            for (i = g_prefs.from; i <= g_prefs.to; i++) {
                if (hd.day <= DaysInMonth(hd.month, hd.year)) {
                    // if exists
                    MakeNewHolidayRecord(hd.day, hd.month, i, hd);
                }
            }
        }
    }
    else if (hd.flag.bits.lunar || hd.flag.bits.lunar_leap) {
        DateType r, dt;

        r.month = hd.month;
        r.day = hd.day;
        r.year = hd.year - firstYear;
        
        if (hd.flag.bits.year) {
        	dt = r;
            if (FindNearLunar(&dt, r, hd.flag.bits.lunar_leap)) {
                MakeNewHolidayRecord(dt.day, dt.month, dt.year + firstYear, hd);
            }
        }
        else {
            for (i = g_prefs.from; i <= g_prefs.to; i++) {
            
                r.year = i - firstYear;
        		dt = r;                
                if (FindNearLunar(&dt, r, hd.flag.bits.lunar_leap)) {
                    MakeNewHolidayRecord(dt.day, dt.month, dt.year + firstYear, hd);
                }
            }
        }
    }
}

static Boolean GenerateHoliday()
{
    char *p, *q;
    char *field = FldGetTextPtr(GetObjectPtr(MainField));
    char *str;
    Boolean ret = true;
    Int16   line = 0;
    Holiday hd;

	if (!field) return true;
 	str = MemPtrNew(StrLen(field)+1);
 	
    p = str;
    StrCopy(str, field);

    while ((q = StrChr(p, '\n'))) {
        *q = 0;

        line++;
        
        if (!AnalOneRecord(p, &hd)) {
            ret = false;
            break;
        }
        else MakeNewEntry(hd);
        
        p = q+1;
    }
    if (*p && ret) {
        line++;
        if (!AnalOneRecord(p, &hd)) {
            ret = false;
        }
        else MakeNewEntry(hd);
    }
    
    if (!ret) {
        // 에러가 난 행을 표시해 줌
        char temp[255];
        StrPrintF(temp, "Error in line %d", line);
        
        FrmCustomAlert(ErrorAlert, temp, "", "");
        FldSetInsPtPosition(GetObjectPtr(MainField), p - str);
        UpdateScrollBar();
    }
    else {
        FrmAlert(GeneratingHolidayAlert);
        
    }

    MemPtrFree(str);
    
    return ret;
}


/*
 * FUNCTION: MainFormHandleEvent
 *
 * DESCRIPTION:
 *
 * This routine is the event handler for the "MainForm" of this 
 * application.
 *
 * PARAMETERS:
 *
 * eventP
 *     a pointer to an EventType structure
 *
 * RETURNED:
 *     true if the event was handled and should not be passed to
 *     FrmHandleEvent
 */

static Boolean MainFormHandleEvent(EventType * eventP)
{
	Boolean handled = false;
	FormType * frmP;

	switch (eventP->eType) 
	{
		case menuEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			MainFormInit(frmP);
			if (g_prefs.offset >= 0) {
				FldSetInsPtPosition(GetObjectPtr(MainField), g_prefs.offset);
			}					
			FrmDrawForm(frmP);
			FrmSetFocus(frmP, FrmGetObjectIndex(frmP, MainField));
			UpdateScrollBar();
	
			handled = true;
			break;
            
        case frmUpdateEvent:
			/* 
			 * To do any custom drawing here, first call
			 * FrmDrawForm(), then do your drawing, and
			 * then set handled to true. 
			 */
			break;
			
		case ctlSelectEvent:
		{
			if (eventP->data.ctlSelect.controlID == MainGenerateButton)
			{
                CreateHolidayDB();
				GenerateHoliday();
                CloseHolidayDB();
				break;
			}

			break;
		}
		case fldChangedEvent:
		{
		   	UpdateScrollBar();
        	handled = true;

			break;
		}
    	case sclRepeatEvent:
    	{
       		ScrollLines(eventP->data.sclRepeat.newValue -
                    eventP->data.sclRepeat.value, false);
	        break;
	   	}
	   	
    	case keyDownEvent:
	        if (eventP->data.keyDown.chr == pageUpChr) {
	            PageScroll(winUp);
	            handled = true;
	        } else if (eventP->data.keyDown.chr == pageDownChr) {
	            PageScroll(winDown);
	            handled = true;
	        }
	        break;
		case appStopEvent:
			SaveFieldDataToDB();
		
			// handled = true;	
			break;        
	}
    
	return handled;
}

/*
 * FUNCTION: AppHandleEvent
 *
 * DESCRIPTION: 
 *
 * This routine loads form resources and set the event handler for
 * the form loaded.
 *
 * PARAMETERS:
 *
 * event
 *     a pointer to an EventType structure
 *
 * RETURNED:
 *     true if the event was handled and should not be passed
 *     to a higher level handler.
 */

static Boolean AppHandleEvent(EventType * eventP)
{
	UInt16 formId;
	FormType * frmP;

	if (eventP->eType == frmLoadEvent)
	{
		/* Load the form resource. */
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		/* 
		 * Set the event handler for the form.  The handler of the
		 * currently active form is called by FrmHandleEvent each
		 * time is receives an event. 
		 */
		switch (formId)
		{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;

		}
		return true;
	}

	return false;
}

/*
 * FUNCTION: AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.
 */

static void AppEventLoop(void)
{
	UInt16 error;
	EventType event;

	do 
	{
		/* change timeout if you need periodic nilEvents */
		EvtGetEvent(&event, evtWaitForever);

		if (! SysHandleEvent(&event))
		{
			if (! MenuHandleEvent(0, &event, &error))
			{
				if (! AppHandleEvent(&event))
				{
					FrmDispatchEvent(&event);
				}
			}
		}
	} while (event.eType != appStopEvent);
}

/*
 * FUNCTION: AppStart
 *
 * DESCRIPTION:  Get the current application's preferences.
 *
 * RETURNED:
 *     errNone - if nothing went wrong
 */

static Err AppStart(void)
{
	UInt16 prefsSize;
	UInt16 mode = dmModeReadWrite;
	SystemPreferencesType sysPrefs;
	
	/* Read the saved preferences / saved-state information. */
	prefsSize = sizeof(g_prefs);
	if (PrefGetAppPreferences(
		appFileCreator, appPrefID, &g_prefs, &prefsSize, true) == 
		noPreferenceFound)
	{
		/* no prefs; initialize pref struct with default values */
		g_prefs.from = 2003;
		g_prefs.to = 2006;
		g_prefs.offset = -1;
	}
	
	PrefGetPreferences(&sysPrefs);
	g_prefdfmts = sysPrefs.dateFormat;
	
	g_strDB = DmOpenDatabaseByTypeCreator('Temp', appFileCreator, mode);
	if (!g_strDB) {
		if (DmCreateDatabase(0, "HolidayTmpDB", appFileCreator, 'Temp', false))
			return -1;
			
		g_strDB = DmOpenDatabaseByTypeCreator('Temp', appFileCreator, mode);
	
	}
	
	return errNone;
}

/*
 * FUNCTION: AppStop
 *
 * DESCRIPTION: Save the current state of the application.
 */

static void AppStop(void)
{
	/* 
	 * Write the saved preferences / saved-state information.  This
	 * data will be saved during a HotSync backup. 
	 */
	PrefSetAppPreferences(
		appFileCreator, appPrefID, appPrefVersionNum, 
		&g_prefs, sizeof(g_prefs), true);
		
	DmCloseDatabase(g_strDB);		
        
	/* Close all the open forms. */
	FrmCloseAllForms();

}

/*
 * FUNCTION: RomVersionCompatible
 *
 * DESCRIPTION: 
 *
 * This routine checks that a ROM version is meet your minimum 
 * requirement.
 *
 * PARAMETERS:
 *
 * requiredVersion
 *     minimum rom version required
 *     (see sysFtrNumROMVersion in SystemMgr.h for format)
 *
 * launchFlags
 *     flags that indicate if the application UI is initialized
 *     These flags are one of the parameters to your app's PilotMain
 *
 * RETURNED:
 *     error code or zero if ROM version is compatible
 */

static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	/* See if we're on in minimum required version of the ROM or later. */
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
	{
		if ((launchFlags & 
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
		{
			FrmAlert (RomIncompatibleAlert);

			/* Palm OS versions before 2.0 will continuously relaunch this
			 * app unless we switch to another safe one. */
			if (romVersion < kPalmOS20Version)
			{
				AppLaunchWithCommand(
					sysFileCDefaultApp, 
					sysAppLaunchCmdNormalLaunch, NULL);
			}
		}

		return sysErrRomIncompatible;
	}

	return errNone;
}

/*
 * FUNCTION: PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 * 
 * PARAMETERS:
 *
 * cmd
 *     word value specifying the launch code. 
 *
 * cmdPB
 *     pointer to a structure that is associated with the launch code
 *
 * launchFlags
 *     word value providing extra information about the launch.
 *
 * RETURNED:
 *     Result of launch, errNone if all went OK
 */

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;

	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error) return (error);

	switch (cmd)
	{
		case sysAppLaunchCmdNormalLaunch:
			error = AppStart();
			if (error) 
				return error;

			/* 
			 * start application by opening the main form
			 * and then entering the main event loop 
			 */
			FrmGotoForm(MainForm);
			AppEventLoop();

			AppStop();
			break;
	}

	return errNone;
}

/*
 * HolidayDBMaker.h
 *
 * header file for HolidayDBMaker
 *
 * This wizard-generated code is based on code adapted from the
 * stationery files distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */
 
#ifndef HOLIDAYDBMAKER_H_
#define HOLIDAYDBMAKER_H_

/*********************************************************************
 * Internal Structures
 *********************************************************************/

typedef struct HolidayDBMakerPreferenceType
{
	Int16	from;
	Int16	to;
	Int16	offset;	// position
} HolidayDBMakerPreferenceType;

typedef union 
{
    struct 
    {
        unsigned int solar      :1;
        unsigned int lunar      :1;
        unsigned int lunar_leap :1;
        unsigned int year       :1;
        unsigned int easter     :1;
        unsigned int dayofmonth :1;
    } bits;
    Int16   allBits;
} EventFlag;


typedef struct HolidayStruct
{
    EventFlag   flag;
    Int8        day;
    Int8        month;
    Int16       year;

    Int8        dayOfWeek;
    Int8        weekOfMonth;
    
    Int16       duration_count;
    Int16       duration[10];
} Holiday;


/*********************************************************************
 * Global variables
 *********************************************************************/

extern HolidayDBMakerPreferenceType g_prefs;

/*********************************************************************
 * Internal Constants
 *********************************************************************/

#define appFileCreator			'HoLI'
#define holiFileCreator         'Holi'
#define appName					"HolidayDBMaker"
#define appVersionNum			0x01
#define appPrefID				0x00
#define appPrefVersionNum		0x01

#endif /* HOLIDAYDBMAKER_H_ */

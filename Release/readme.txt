== Overview ==

* Overview

This program generates HolidayDB.pdb file, which contains Holiday information. 
Currently WP+(http://wp.jmjeong.com) and 
MonthPlanner(http://wiki.jmjeong.com/wiki.cgi?MonthPlanner) use this data.

If the date is holiday, the background color is changed in these application.

If you are Palm program developer and want to use HolidayDB.pdb file, 
 please refer http://jmjeong.com/?HolidayDB

* Homepage : http://jmjeong.com/?HolidayDB/Maker
* You can submit your date for sharing in http://jmjeong.com/?HolidayDB/Input

* COPYRIGHT (c) 2004 Jaemok Jeong 
 HolidayDBMaker is free software. As such, it comes with NO WARRANTY OF ANY
 KIND. You are free to use and redistribute this software as long as this
 license is included.

== Input format

* General Rules

** All lines staring with ';' are comment, You can add comment with ';' in any
places of lines.
** Description string for Datebook notify is started with ':'.
** Each entry is divided with '\n'. You can enter one holiday entry in one line.
** You can specify the duration such as the day after or before holiday with '[' and ']'
  ex) 1/1[-1,0,1] ; A Day before New year, New Year, A Day after New Year.
** If year field is specified, it only occurs on that year. Or it is repeated on the years specified in Prefs.  

* Format

** Fixed Day(like New Year's Day, Christmas Day, Halloween)

It follows the system date format. If Date Format is 'M/D/Y', you can input
'10/2/2004'.

You can input lunar date with -) and #). -) means by lunar day, and #) means
by lunar leap day.

*** Examples

- New Year : 1/1
- Halloween : 10/31
- Christmas : 12/25

- Chuseok in Korea : -)8/15[-1,0,1]
- Seol in Korea : -)1/1[-1,0,1]

** Day of Month(Example: First Sunday of June; like Daylight Saving's Start/End, Martin Luther King's Day)

@c(month, week_of_month, day_of_week)
@c(month, week_of_month, day_of_week, year)

If week_of_month is -1, it means by the last week. day_of_week must be
between 0 and 6(Sun=0, ..., Sat=6).


*** Examples

- President's Day Observed(Third Monday in February) : @c(2, 3, 1)
- Labor Day Observed(First Monday in September) : @c(9, 1, 1)
- Columbus Day Observed(Second Monday in October) : @c(10, 2, 1)

** Easter related(x days before Easter Sunday; Like Easter Sunday)

@easter
@easter(year)

*** Examples

- Easter Sunday in 2004 : @easter(2004)
- Three and four days before Easter Sunday : @easter[-3,-4]

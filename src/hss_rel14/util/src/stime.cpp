/*
* Copyright (c) 2017 Sprint
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include "stime.h"


#define ft_strnicmp(str1, str2, count) __builtin_strncasecmp(str1, str2, count)
#define ft_localtime_s(a,b) localtime_r(b,a)
#define ft_gmtime_s(a,b) gmtime_r(b,a)
#define ft_sprintf_s __builtin_snprintf


static const char *_days[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char *_days_abbrev[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *_months[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
static const char *_months_abbrev[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

#define TM_YEAR_BASE   1900

#define DAYSPERLYEAR   366
#define DAYSPERNYEAR   365
#define DAYSPERWEEK    7

#define LEAPYEAR(year) (!((year) % 4) && (((year) % 100) || !((year) % 400)))

static char *_fmt(const char *format, const struct tm *t, char *pt, const char *ptlim, const struct timeval* tv);
static char *_conv(const int32_t n, const char *format, char *pt, const char *ptlim);
static char *_add(const char *str, char *pt, const char *ptlim);

inline long fttimezone()
{
    return timezone;
}

inline int32_t ftdaylight()
{
    return daylight;
}

size_t ftstrftime(char *s, size_t maxsize, const char *format, const struct tm *t, const struct timeval* tv)
{
  char *p;

  p = _fmt(((format == NULL) ? "%c" : format), t, s, s + maxsize, tv);
  if (p == s + maxsize) return 0;
  *p = '\0';
  return p - s;
}

static char *_fmt(const char *format, const struct tm *t, char *pt, const char *ptlim, const struct timeval* tv)
{
  for ( ; *format; ++format)
  {
    if (*format == '%')
    {
      if (*format == 'E')
        format++; // Alternate Era
      else if (*format == 'O')
        format++; // Alternate numeric symbols

      switch (*++format)
      {
        case '\0':
          --format;
          break;

        case '0': // milliseconds
            pt = _conv(tv->tv_usec / 1000, "%03d", pt, ptlim);
          continue;

        case '1': // microseconds
            pt = _conv(tv->tv_usec, "%06d", pt, ptlim);
            continue;

        case 'A':
          pt = _add((t->tm_wday < 0 || t->tm_wday > 6) ? "?" : _days[t->tm_wday], pt, ptlim);
          continue;

        case 'a':
          pt = _add((t->tm_wday < 0 || t->tm_wday > 6) ? "?" : _days_abbrev[t->tm_wday], pt, ptlim);
          continue;

        case 'B':
          pt = _add((t->tm_mon < 0 || t->tm_mon > 11) ? "?" : _months[t->tm_mon], pt, ptlim);
          continue;

        case 'b':
        case 'h':
          pt = _add((t->tm_mon < 0 || t->tm_mon > 11) ? "?" : _months_abbrev[t->tm_mon], pt, ptlim);
          continue;

        case 'C':
          pt = _conv((t->tm_year + TM_YEAR_BASE) / 100, "%02d", pt, ptlim);
          continue;

        case 'c':
          pt = _fmt("%a %b %e %H:%M:%S %Y", t, pt, ptlim, tv);
          continue;

        case 'D':
          pt = _fmt("%m/%d/%y", t, pt, ptlim, tv);
          continue;

        case 'd':
          pt = _conv(t->tm_mday, "%02d", pt, ptlim);
          continue;

        case 'e':
          pt = _conv(t->tm_mday, "%2d", pt, ptlim);
          continue;

        case 'F':
          pt = _fmt("%Y-%m-%d", t, pt, ptlim, tv);
          continue;

        case 'H':
          pt = _conv(t->tm_hour, "%02d", pt, ptlim);
          continue;

        case 'i':
          pt = _fmt("%Y-%m-%dT%H:%M:%S.%0", t, pt, ptlim, tv);
          continue;

        case 'I':
          pt = _conv((t->tm_hour % 12) ? (t->tm_hour % 12) : 12, "%02d", pt, ptlim);
          continue;

        case 'j':
          pt = _conv(t->tm_yday + 1, "%03d", pt, ptlim);
          continue;

        case 'k':
          pt = _conv(t->tm_hour, "%2d", pt, ptlim);
          continue;

        case 'l':
          pt = _conv((t->tm_hour % 12) ? (t->tm_hour % 12) : 12, "%2d", pt, ptlim);
          continue;

        case 'M':
          pt = _conv(t->tm_min, "%02d", pt, ptlim);
          continue;

        case 'm':
          pt = _conv(t->tm_mon + 1, "%02d", pt, ptlim);
          continue;

        case 'n':
          pt = _add("\n", pt, ptlim);
          continue;

        case 'p':
          pt = _add((t->tm_hour >= 12) ? "pm" : "am", pt, ptlim);
          continue;

        case 'R':
          pt = _fmt("%H:%M", t, pt, ptlim, tv);
          continue;

        case 'r':
          pt = _fmt("%I:%M:%S %p", t, pt, ptlim, tv);
          continue;

        case 'S':
          pt = _conv(t->tm_sec, "%02d", pt, ptlim);
          continue;

        case 's':
        {
          struct tm  tm;
          char buf[32];
          time_t mkt;

          tm = *t;
          mkt = mktime(&tm);
          ft_sprintf_s(buf, sizeof(buf), "%lu", mkt);
          pt = _add(buf, pt, ptlim);
          continue;
        }

        case 'T':
          pt = _fmt("%H:%M:%S", t, pt, ptlim, tv);
          continue;

        case 't':
          pt = _add("\t", pt, ptlim);
          continue;

        case 'U':
          pt = _conv((t->tm_yday + 7 - t->tm_wday) / 7, "%02d", pt, ptlim);
          continue;

        case 'u':
          pt = _conv((t->tm_wday == 0) ? 7 : t->tm_wday, "%d", pt, ptlim);
          continue;

        case 'V':  // ISO 8601 week number
        case 'G':  // ISO 8601 year (four digits)
        case 'g':  // ISO 8601 year (two digits)
        {
          int32_t  year;
          int32_t  yday;
          int32_t  wday;
          int32_t  w;

          year = t->tm_year + TM_YEAR_BASE;
          yday = t->tm_yday;
          wday = t->tm_wday;
          while (1)
          {
            int32_t  len;
            int32_t  bot;
            int32_t  top;

            len = LEAPYEAR(year) ? DAYSPERLYEAR : DAYSPERNYEAR;
            bot = ((yday + 11 - wday) % DAYSPERWEEK) - 3;
            top = bot - (len % DAYSPERWEEK);
            if (top < -3) top += DAYSPERWEEK;
            top += len;
            if (yday >= top)
            {
              ++year;
              w = 1;
              break;
            }
            if (yday >= bot)
            {
              w = 1 + ((yday - bot) / DAYSPERWEEK);
              break;
            }
            --year;
            yday += LEAPYEAR(year) ? DAYSPERLYEAR : DAYSPERNYEAR;
          }
          if (*format == 'V')
            pt = _conv(w, "%02d", pt, ptlim);
          else if (*format == 'g')
            pt = _conv(year % 100, "%02d", pt, ptlim);
          else
            pt = _conv(year, "%04d", pt, ptlim);

          continue;
        }

        case 'v':
          pt = _fmt("%e-%b-%Y", t, pt, ptlim, tv);
          continue;

        case 'W':
          pt = _conv((t->tm_yday + 7 - (t->tm_wday ? (t->tm_wday - 1) : 6)) / 7, "%02d", pt, ptlim);
          continue;

        case 'w':
          pt = _conv(t->tm_wday, "%d", pt, ptlim);
          continue;

        case 'X':
          pt = _fmt("%H:%M:%S", t, pt, ptlim, tv);
          continue;

        case 'x':
          pt = _fmt("%m/%d/%y", t, pt, ptlim, tv);
          continue;

        case 'y':
          pt = _conv((t->tm_year + TM_YEAR_BASE) % 100, "%02d", pt, ptlim);
          continue;

        case 'Y':
          pt = _conv(t->tm_year + TM_YEAR_BASE, "%04d", pt, ptlim);
          continue;

        case 'Z':
          pt = _add("?", pt, ptlim);
          continue;

        case 'z':
        {
          long absoff;
          if (fttimezone() >= 0)
          {
            absoff = fttimezone();
            pt = _add("+", pt, ptlim);
          }
          else
          {
            absoff = fttimezone();
            pt = _add("-", pt, ptlim);
          }
          pt = _conv(absoff / 3600, "%02d", pt, ptlim);
          pt = _conv((absoff % 3600) / 60, "%02d", pt, ptlim);

          continue;
        }

        case '+':
          pt = _fmt("%a, %d %b %Y %H:%M:%S %z", t, pt, ptlim, tv);
          continue;

        case '%':
        default:
          break;
      }
    }

    if (pt == ptlim) break;
    *pt++ = *format;
  }

  return pt;
}

static char *_conv(const int32_t n, const char *format, char *pt, const char *ptlim)
{
  char  buf[32];

  ft_sprintf_s(buf, sizeof(buf), format, n);
  return _add(buf, pt, ptlim);
}

static char *_add(const char *str, char *pt, const char *ptlim)
{
  while (pt < ptlim && (*pt = *str++) != '\0') ++pt;
  return pt;
}





////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


time_t timegm(struct tm *t)
{
    time_t tl, tb;
    struct tm *tg;

    tl = mktime (t);
    if (tl == -1)
    {
        t->tm_hour--;
        tl = mktime (t);
        if (tl == -1)
            return -1; /* can't deal with output from strptime */
        tl += 3600;
    }
    tg = gmtime (&tl);
    tg->tm_isdst = 0;
    tb = mktime (tg);
    if (tb == -1)
    {
        tg->tm_hour--;
        tb = mktime (tg);
        if (tb == -1)
            return -1; /* can't deal with output from gmtime */
        tb += 3600;
    }
    return (tl - (tb - tl));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/* Date string parsing */
#define DP_TIMESEP 0x01 /* Time separator ( _must_ remain 0x1, used as a bitmask) */
#define DP_DATESEP 0x02 /* Date separator */
#define DP_MONTH   0x04 /* Month name */
#define DP_AM      0x08 /* AM */
#define DP_PM      0x10 /* PM */

typedef struct tagDATEPARSE
{
    uint32_t dwCount;      /* Number of fields found so far (maximum 6) */
    uint32_t dwParseFlags; /* Global parse flags (DP_ Flags above) */
    uint32_t dwFlags[7];   /* Flags for each field */
    uint32_t dwValues[7];  /* Value of each field */
} DATEPARSE;

#define TIMEFLAG(i) ((dp.dwFlags[i] & DP_TIMESEP) << i)

#define IsLeapYear(y) (((y % 4) == 0) && (((y % 100) != 0) || ((y % 400) == 0)))

/* Determine if a day is valid in a given month of a given year */
static bool VARIANT_IsValidMonthDay(uint32_t day, uint32_t month, uint32_t year)
{
  static const unsigned char days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if (day && month && month < 13)
  {
    if (day <= days[month] || (month == 2 && day == 29 && IsLeapYear(year)))
      return true;
  }
  return false;
}

/* Possible orders for 3 numbers making up a date */
#define ORDER_MDY 0x01
#define ORDER_YMD 0x02
#define ORDER_YDM 0x04
#define ORDER_DMY 0x08
#define ORDER_MYD 0x10 /* Synthetic order, used only for funky 2 digit dates */

/* Determine a date for a particular locale, from 3 numbers */
static bool MakeDate(DATEPARSE *dp, uint32_t iDate, uint32_t offset, struct tm *ptm)
{
    uint32_t dwAllOrders, dwTry, dwCount = 0, v1, v2, v3;

    if (!dp->dwCount)
    {
        v1 = 30; /* Default to (Variant) 0 date part */
        v2 = 12;
        v3 = 1899;
        goto VARIANT_MakeDate_OK;
    }

    v1 = dp->dwValues[offset + 0];
    v2 = dp->dwValues[offset + 1];
    if (dp->dwCount == 2)
    {
        struct tm current;
        time_t t = time(NULL);
        ft_gmtime_s(&current, &t);
        v3 = current.tm_year + 1900;
    }
    else
        v3 = dp->dwValues[offset + 2];

    //TRACE("(%d,%d,%d,%d,%d)\n", v1, v2, v3, iDate, offset);

    /* If one number must be a month (Because a month name was given), then only
    * consider orders with the month in that position.
    * If we took the current year as 'v3', then only allow a year in that position.
    */
    if (dp->dwFlags[offset + 0] & DP_MONTH)
    {
        dwAllOrders = ORDER_MDY;
    }
    else if (dp->dwFlags[offset + 1] & DP_MONTH)
    {
        dwAllOrders = ORDER_DMY;
        if (dp->dwCount > 2)
            dwAllOrders |= ORDER_YMD;
    }
    else if (dp->dwCount > 2 && dp->dwFlags[offset + 2] & DP_MONTH)
    {
        dwAllOrders = ORDER_YDM;
    }
    else
    {
        dwAllOrders = ORDER_MDY|ORDER_DMY;
        if (dp->dwCount > 2)
              dwAllOrders |= (ORDER_YMD|ORDER_YDM);
    }

VARIANT_MakeDate_Start:
    //TRACE("dwAllOrders is 0x%08x\n", dwAllOrders);

    while (dwAllOrders)
    {
        uint32_t dwTemp;

        if (dwCount == 0)
        {
            /* First: Try the order given by iDate */
            switch (iDate)
            {
                case 0:  dwTry = dwAllOrders & ORDER_MDY; break;
                case 1:  dwTry = dwAllOrders & ORDER_DMY; break;
                default: dwTry = dwAllOrders & ORDER_YMD; break;
            }
        }
        else if (dwCount == 1)
        {
            /* Second: Try all the orders compatible with iDate */
            switch (iDate)
            {
                case 0:  dwTry = dwAllOrders & ~(ORDER_DMY|ORDER_YDM); break;
                case 1:  dwTry = dwAllOrders & ~(ORDER_MDY|ORDER_YMD|ORDER_MYD); break;
                default: dwTry = dwAllOrders & ~(ORDER_DMY|ORDER_YDM); break;
            }
        }
        else
        {
            /* Finally: Try any remaining orders */
            dwTry = dwAllOrders;
        }

        //TRACE("Attempt %d, dwTry is 0x%08x\n", dwCount, dwTry);

        dwCount++;
        if (!dwTry)
            continue;

#define DATE_SWAP(x,y) do { dwTemp = x; x = y; y = dwTemp; } while (0)

        if (dwTry & ORDER_MDY)
        {
            if (VARIANT_IsValidMonthDay(v2,v1,v3))
            {
                DATE_SWAP(v1,v2);
                goto VARIANT_MakeDate_OK;
            }
            dwAllOrders &= ~ORDER_MDY;
        }
        if (dwTry & ORDER_YMD)
        {
            if (VARIANT_IsValidMonthDay(v3,v2,v1))
            {
                DATE_SWAP(v1,v3);
                goto VARIANT_MakeDate_OK;
            }
            dwAllOrders &= ~ORDER_YMD;
        }
        if (dwTry & ORDER_YDM)
        {
            if (VARIANT_IsValidMonthDay(v2,v3,v1))
            {
                DATE_SWAP(v1,v2);
                DATE_SWAP(v2,v3);
                goto VARIANT_MakeDate_OK;
            }
            dwAllOrders &= ~ORDER_YDM;
        }
        if (dwTry & ORDER_DMY)
        {
            if (VARIANT_IsValidMonthDay(v1,v2,v3))
                goto VARIANT_MakeDate_OK;
            dwAllOrders &= ~ORDER_DMY;
        }
        if (dwTry & ORDER_MYD)
        {
            /* Only occurs if we are trying a 2 year date as M/Y not D/M */
            if (VARIANT_IsValidMonthDay(v3,v1,v2))
            {
                DATE_SWAP(v1,v3);
                DATE_SWAP(v2,v3);
                goto VARIANT_MakeDate_OK;
            }
            dwAllOrders &= ~ORDER_MYD;
        }
    }

    if (dp->dwCount == 2)
    {
        /* We couldn't make a date as D/M or M/D, so try M/Y or Y/M */
        v3 = 1; /* 1st of the month */
        dwAllOrders = ORDER_YMD|ORDER_MYD;
        dp->dwCount = 0; /* Don't return to this code path again */
        dwCount = 0;
        goto VARIANT_MakeDate_Start;
    }

    /* No valid dates were able to be constructed */
    return false;

VARIANT_MakeDate_OK:

    /* Check that the time part is ok */
    if (ptm->tm_hour > 23 || ptm->tm_min > 59 || ptm->tm_sec > 59)
        return false; //return DISP_E_TYPEMISMATCH;

    //TRACE("Time %d %d %d\n", st->wHour, st->wMinute, st->wSecond);
    if (ptm->tm_hour < 12 && (dp->dwParseFlags & DP_PM))
        ptm->tm_hour += 12;
    else if (ptm->tm_hour == 12 && (dp->dwParseFlags & DP_AM))
        ptm->tm_hour = 0;
    //TRACE("Time %d %d %d\n", st->wHour, st->wMinute, st->wSecond);

    ptm->tm_mday = v1;
    ptm->tm_mon = v2;
    ptm->tm_year = v3 < 30 ? 2000 + v3 : v3 < 100 ? 1900 + v3 : v3;
    //TRACE("Returning date %d/%d/%d\n", v1, v2, st->wYear);
    return true; //return S_OK;
}

bool DateFromStr(const char * strIn, struct timeval* ptvout, bool isLocal)
{
    static const char * tokens[] =
    {
        "January",      // LOCALE_SMONTHNAME1
        "February",     // LOCALE_SMONTHNAME2
        "March",        // LOCALE_SMONTHNAME3
        "April",        // LOCALE_SMONTHNAME4
        "May",          // LOCALE_SMONTHNAME5
        "June",         // LOCALE_SMONTHNAME6
        "July",         // LOCALE_SMONTHNAME7
        "August",       // LOCALE_SMONTHNAME8
        "September",    // LOCALE_SMONTHNAME9
        "October",      // LOCALE_SMONTHNAME10
        "November",     // LOCALE_SMONTHNAME11
        "December",     // LOCALE_SMONTHNAME12
        "",             // LOCALE_SMONTHNAME13
        "Jan",          // LOCALE_SABBREVMONTHNAME1
        "Feb",          // LOCALE_SABBREVMONTHNAME2
        "Mar",          // LOCALE_SABBREVMONTHNAME3
        "Apr",          // LOCALE_SABBREVMONTHNAME4
        "May",          // LOCALE_SABBREVMONTHNAME5
        "Jun",          // LOCALE_SABBREVMONTHNAME6
        "Jul",          // LOCALE_SABBREVMONTHNAME7
        "Aug",          // LOCALE_SABBREVMONTHNAME8
        "Sep",          // LOCALE_SABBREVMONTHNAME9
        "Oct",          // LOCALE_SABBREVMONTHNAME10
        "Nov",          // LOCALE_SABBREVMONTHNAME11
        "Dec",          // LOCALE_SABBREVMONTHNAME12
        "",             // LOCALE_SABBREVMONTHNAME13
        "Monday",       // LOCALE_SDAYNAME1
        "Tuesday",      // LOCALE_SDAYNAME2
        "Wednesday",    // LOCALE_SDAYNAME3
        "Thursday",     // LOCALE_SDAYNAME4
        "Friday",       // LOCALE_SDAYNAME5
        "Saturday",     // LOCALE_SDAYNAME6
        "Sunday",       // LOCALE_SDAYNAME7
        "Mon",          // LOCALE_SABBREVDAYNAME1
        "Tue",          // LOCALE_SABBREVDAYNAME2
        "Wed",          // LOCALE_SABBREVDAYNAME3
        "Thu",          // LOCALE_SABBREVDAYNAME4
        "Fri",          // LOCALE_SABBREVDAYNAME5
        "Sat",          // LOCALE_SABBREVDAYNAME6
        "Sun",          // LOCALE_SABBREVDAYNAME7
        "AM",           // LOCALE_S1159
        "PM"            // LOCALE_S2359
    };

    static const unsigned char ParseDateMonths[] =
    {
        1,2,3,4,5,6,7,8,9,10,11,12,13,
        1,2,3,4,5,6,7,8,9,10,11,12,13
    };

    unsigned int i;

    DATEPARSE dp;
    uint32_t dwDateSeps = 0, iDate = 0;

    if (!strIn)
        return false;

    ptvout->tv_sec = 0;
    ptvout->tv_usec = 0;

    memset(&dp, 0, sizeof(dp));

    /* Parse the string into our structure */
    while (*strIn)
    {
        if (dp.dwCount >= 7)
            break;

        if (isdigit(*strIn))
        {
            dp.dwValues[dp.dwCount] = strtoul(strIn, (char**)&strIn, 10);
            dp.dwCount++;
            strIn--;
        }
        else if (isalpha(*strIn))
        {
            bool bFound = false;

            for (i = 0; i < sizeof(tokens)/sizeof(tokens[0]); i++)
            {
                size_t dwLen = strlen(tokens[i]);
                if (dwLen && !ft_strnicmp(strIn, tokens[i], dwLen))
                {
                    if (i <= 25)
                    {
                        dp.dwValues[dp.dwCount] = ParseDateMonths[i];
                        dp.dwFlags[dp.dwCount] |= (DP_MONTH|DP_DATESEP);
                        dp.dwCount++;
                    }
                    else if (i > 39)
                    {
                        if (!dp.dwCount || dp.dwParseFlags & (DP_AM|DP_PM))
                            return false; //hRet = DISP_E_TYPEMISMATCH;
                        else
                        {
                            dp.dwFlags[dp.dwCount - 1] |= (i == 40 ? DP_AM : DP_PM);
                            dp.dwParseFlags |= (i == 40 ? DP_AM : DP_PM);
                        }
                    }
                    strIn += (dwLen - 1);
                    bFound = true;
                    break;
                }
            }

            if (!bFound)
            {
                if ((*strIn == 'a' || *strIn == 'A' || *strIn == 'p' || *strIn == 'P') &&
                    (dp.dwCount && !(dp.dwParseFlags & (DP_AM|DP_PM))))
                {
                    /* Special case - 'a' and 'p' are recognised as short for am/pm */
                    if (*strIn == 'a' || *strIn == 'A')
                    {
                        dp.dwFlags[dp.dwCount - 1] |= DP_AM;
                        dp.dwParseFlags |=  DP_AM;
                    }
                    else
                    {
                        dp.dwFlags[dp.dwCount - 1] |= DP_PM;
                        dp.dwParseFlags |=  DP_PM;
                    }
                    strIn++;
                }
                else if (*strIn == 'T')
                {
                   //ignore
                }
                else
                {
                    //TRACE("No matching token for %s\n", debugstr_w(strIn));
                    return false; //hRet = DISP_E_TYPEMISMATCH;
                    //break;
                }
            }
        }
        else if (*strIn == ':' ||  *strIn == '.')
        {
            if (!dp.dwCount || !strIn[1])
                return false; //hRet = DISP_E_TYPEMISMATCH;
            else
                dp.dwFlags[dp.dwCount - 1] |= DP_TIMESEP;
        }
        else if (*strIn == '-' || *strIn == '/')
        {
            dwDateSeps++;
            if (dwDateSeps > 2 || !dp.dwCount || !strIn[1])
                return false; //hRet = DISP_E_TYPEMISMATCH;
            else
                dp.dwFlags[dp.dwCount - 1] |= DP_DATESEP;
        }
        else if (*strIn == ',' || isspace(*strIn))
        {
            if (*strIn == ',' && !strIn[1])
                return false; //hRet = DISP_E_TYPEMISMATCH;
        }
        else
        {
            return false; //hRet = DISP_E_TYPEMISMATCH;
        }
        strIn++;
    }

    if (!dp.dwCount || dp.dwCount > 7 ||
        (dp.dwCount == 1 && !(dp.dwParseFlags & (DP_AM|DP_PM))))
        return false; //hRet = DISP_E_TYPEMISMATCH;

    struct tm t;
    int32_t milliseconds;
    uint32_t dwOffset = 0; /* Start of date fields in dp.dwValues */

    t.tm_wday = t.tm_hour = t.tm_min = t.tm_sec = milliseconds = 0;
    //st.wDayOfWeek = t.tm_hour = t.tm_min = t.tm_sec = st.wMilliseconds = 0;

    /* Figure out which numbers correspond to which fields.
     *
     * This switch statement works based on the fact that native interprets any
     * fields that are not joined with a time separator ('.' or ':') as date
     * fields. Thus we construct a value from 0-32 where each set bit indicates
     * a time field. This encapsulates the hundreds of permutations of 2-6 fields.
     * For valid permutations, we set dwOffset to point32_t to the first date field
     * and shorten dp.dwCount by the number of time fields found. The real
     * magic here occurs in MakeDate() above, where we determine what
     * each date number must represent in the context of iDate.
     */
    //TRACE("0x%08x\n", TIMEFLAG(0)|TIMEFLAG(1)|TIMEFLAG(2)|TIMEFLAG(3)|TIMEFLAG(4));

    switch (TIMEFLAG(0)|TIMEFLAG(1)|TIMEFLAG(2)|TIMEFLAG(3)|TIMEFLAG(4))
    {
        case 0x1: /* TT TTDD TTDDD */
            if (dp.dwCount > 3 &&
                ((dp.dwFlags[2] & (DP_AM|DP_PM)) || (dp.dwFlags[3] & (DP_AM|DP_PM)) ||
                (dp.dwFlags[4] & (DP_AM|DP_PM))))
                return false; //hRet = DISP_E_TYPEMISMATCH;
            else if (dp.dwCount != 2 && dp.dwCount != 4 && dp.dwCount != 5)
                return false; //hRet = DISP_E_TYPEMISMATCH;
            t.tm_hour = dp.dwValues[0];
            t.tm_min  = dp.dwValues[1];
            dp.dwCount -= 2;
            dwOffset = 2;
            break;

        case 0x3: /* TTT TTTDD TTTDDD */
            if (dp.dwCount > 4 &&
                ((dp.dwFlags[3] & (DP_AM|DP_PM)) || (dp.dwFlags[4] & (DP_AM|DP_PM)) ||
                (dp.dwFlags[5] & (DP_AM|DP_PM))))
                return false; //hRet = DISP_E_TYPEMISMATCH;
            else if (dp.dwCount != 3 && dp.dwCount != 5 && dp.dwCount != 6)
                return false; //hRet = DISP_E_TYPEMISMATCH;
            t.tm_hour   = dp.dwValues[0];
            t.tm_min = dp.dwValues[1];
            t.tm_sec = dp.dwValues[2];
            milliseconds = dp.dwValues[3];
            dwOffset = 3;
            dp.dwCount -= 3;
            break;

        case 0x4: /* DDTT */
            if (dp.dwCount != 4 ||
                (dp.dwFlags[0] & (DP_AM|DP_PM)) || (dp.dwFlags[1] & (DP_AM|DP_PM)))
                return false; //hRet = DISP_E_TYPEMISMATCH;
            t.tm_hour = dp.dwValues[2];
            t.tm_min  = dp.dwValues[3];
            dp.dwCount -= 2;
            break;

        case 0x0: /* T DD DDD TDDD TDDD */
            if (dp.dwCount == 1 && (dp.dwParseFlags & (DP_AM|DP_PM)))
            {
                t.tm_hour = dp.dwValues[0]; /* T */
                dp.dwCount = 0;
                break;
            }
            else if (dp.dwCount > 4 || (dp.dwCount < 3 && dp.dwParseFlags & (DP_AM|DP_PM)))
            {
                return false; //hRet = DISP_E_TYPEMISMATCH;
            }
            else if (dp.dwCount == 3)
            {
                if (dp.dwFlags[0] & (DP_AM|DP_PM)) /* TDD */
                {
                    dp.dwCount = 2;
                    t.tm_hour = dp.dwValues[0];
                    dwOffset = 1;
                    break;
                }
                if (dp.dwFlags[2] & (DP_AM|DP_PM)) /* DDT */
                {
                    dp.dwCount = 2;
                    t.tm_hour = dp.dwValues[2];
                    break;
                }
                else if (dp.dwParseFlags & (DP_AM|DP_PM))
                    return false; //hRet = DISP_E_TYPEMISMATCH;
            }
            else if (dp.dwCount == 4)
            {
                dp.dwCount = 3;
                if (dp.dwFlags[0] & (DP_AM|DP_PM)) /* TDDD */
                {
                    t.tm_hour = dp.dwValues[0];
                    dwOffset = 1;
                }
                else if (dp.dwFlags[3] & (DP_AM|DP_PM)) /* DDDT */
                {
                    t.tm_hour = dp.dwValues[3];
                }
                else
                    return false; //hRet = DISP_E_TYPEMISMATCH;
                break;
            }
            /* .. fall through .. */

        case 0x8: /* DDDTT */
            if ((dp.dwCount == 2 && (dp.dwParseFlags & (DP_AM|DP_PM))) ||
                (dp.dwCount == 5 && ((dp.dwFlags[0] & (DP_AM|DP_PM)) ||
                (dp.dwFlags[1] & (DP_AM|DP_PM)) || (dp.dwFlags[2] & (DP_AM|DP_PM)))) ||
                dp.dwCount == 4 || dp.dwCount == 6)
                return false; //hRet = DISP_E_TYPEMISMATCH;
            t.tm_hour   = dp.dwValues[3];
            t.tm_min = dp.dwValues[4];
            if (dp.dwCount == 5)
                dp.dwCount -= 2;
            break;

        case 0xC: /* DDTTT */
            if (dp.dwCount != 5 ||
                (dp.dwFlags[0] & (DP_AM|DP_PM)) || (dp.dwFlags[1] & (DP_AM|DP_PM)))
                return false; //hRet = DISP_E_TYPEMISMATCH;
            t.tm_hour   = dp.dwValues[2];
            t.tm_min = dp.dwValues[3];
            t.tm_sec = dp.dwValues[4];
            milliseconds = dp.dwValues[5];
            dp.dwCount -= 3;
            break;

        case 0x18: /* DDDTTT */
            if ((dp.dwFlags[0] & (DP_AM|DP_PM)) || (dp.dwFlags[1] & (DP_AM|DP_PM)) ||
                (dp.dwFlags[2] & (DP_AM|DP_PM)))
                return false; //hRet = DISP_E_TYPEMISMATCH;
            t.tm_hour   = dp.dwValues[3];
            t.tm_min = dp.dwValues[4];
            t.tm_sec = dp.dwValues[5];
            milliseconds = dp.dwValues[6];
            dp.dwCount -= 3;
            break;

        default:
            return false; //hRet = DISP_E_TYPEMISMATCH;
            break;
    }

    if (MakeDate(&dp, iDate, dwOffset, &t))
    {
        if (t.tm_year > 2037)
        {
            t.tm_year = 2037;
            t.tm_mon = 12;
            t.tm_hour = 23;
            t.tm_min = 59;
            t.tm_sec = 59;
            milliseconds = 0;
        }
        t.tm_year -= 1900;
        t.tm_mon--;
        t.tm_wday = 0;
        t.tm_yday = 0;
        if (isLocal)
        {
            t.tm_isdst = -1;
            ptvout->tv_sec = (long)mktime(&t);
        }
        else
        {
            t.tm_isdst = 0;
            ptvout->tv_sec = (long)timegm(&t);
        }
        ptvout->tv_usec = (long)(milliseconds * 1000);
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

STime::STime(int32_t year, int32_t mon, int32_t day, int32_t hour, int32_t min, int32_t sec, bool isLocal)
{
    timeval tv;
    struct tm t;

    memset(&t, 0, sizeof(t));

    t.tm_year = year - 1900;
    t.tm_mon = mon - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;

    if (isLocal)
    {
        t.tm_isdst = -1;
        tv.tv_sec = (long)mktime(&t);
    }
    else
    {
        t.tm_isdst = 0;
        tv.tv_sec = (long)timegm(&t);
    }

    tv.tv_usec = 0;

    set(tv);
}

int32_t STime::year()
{
   struct tm tms;
   time_t tv_sec  = (time_t)m_time.tv_sec;
    ft_localtime_s(&tms, &tv_sec);
    return tms.tm_year + 1900;
}

int32_t STime::month()
{
   struct tm tms;
   time_t tv_sec  = (time_t)m_time.tv_sec;
    ft_localtime_s(&tms, &tv_sec);
    return tms.tm_mon + 1;
}

int32_t STime::day()
{
   struct tm tms;
   time_t tv_sec  = (time_t)m_time.tv_sec;
    ft_localtime_s(&tms, &tv_sec);
    return tms.tm_mday;
}

int32_t STime::hour()
{
   struct tm tms;
   time_t tv_sec  = (time_t)m_time.tv_sec;
    ft_localtime_s(&tms, &tv_sec);
    return tms.tm_hour;
}

int32_t STime::minute()
{
   struct tm tms;
   time_t tv_sec  = (time_t)m_time.tv_sec;
    ft_localtime_s(&tms, &tv_sec);
    return tms.tm_min;
}

int32_t STime::second()
{
   struct tm tms;
   time_t tv_sec  = (time_t)m_time.tv_sec;
    ft_localtime_s(&tms, &tv_sec);
    return tms.tm_sec;
}

STime STime::Now()
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    return STime(tv.tv_sec, tv.tv_usec);
}

void STime::Format(std::string& dest, const char * fmt, bool local)
{
    char buf[2048];
    Format(buf, sizeof(buf), fmt, local);
    dest.assign(buf);
}

void STime::Format(char * dest, int32_t maxsize, const char * fmt, bool local)
{
    struct tm ts;
    time_t t = m_time.tv_sec;

    if (local)
        ft_localtime_s(&ts, &t);
    else
        ft_gmtime_s(&ts, &t);

    ftstrftime(dest, maxsize, fmt, &ts, &m_time);
}

bool STime::ParseDateTime(const char * pszDate, bool isLocal)
{
    return DateFromStr(pszDate, &m_time, isLocal);
}

void STime::getNTPTime(ntp_time_t &ntp) const
{
    ntp.second = m_time.tv_sec + 0x83AA7E80;
    ntp.fraction = (uint32_t)((double)(m_time.tv_usec+1) * (double)(1LL<<32) * 1.0e-6);
}

void STime::setNTPTime(const ntp_time_t &ntp)
{
    m_time.tv_sec = ntp.second - 0x83AA7E80;
    m_time.tv_usec = (uint32_t)((double)ntp.fraction * 1.0e6 / (double)(1LL<<32));
}

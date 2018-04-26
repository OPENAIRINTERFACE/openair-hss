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
#ifndef __STIME_H
#define __STIME_H

#include <sys/time.h>
#include <string>

size_t ftstrftime(char *s, size_t maxsize, const char *format, const struct tm *t, const struct timeval* tv);

struct ntp_time_t
{
    uint32_t second;
    uint32_t fraction;
};

class STime
{
public:
    STime()
    {
        *this = Now();
    }

    STime(const STime& a)
    {
        *this = a;
    }

    STime(int32_t sec, int32_t usec)
    {
        m_time.tv_sec = sec;
        m_time.tv_usec = usec;
    }

    STime(int64_t ms)
    {
       set( ms );
    }

    STime(int32_t year, int32_t mon, int32_t day, int32_t hour, int32_t min, int32_t sec, bool isLocal);

    ~STime()
    {
    }

    STime& operator=(const STime& a)
    {
        m_time.tv_sec = a.m_time.tv_sec;
        m_time.tv_usec = a.m_time.tv_usec;

        return *this;
    }

    bool operator==(const STime& a)
    {
        return ((m_time.tv_sec == a.m_time.tv_sec) && (m_time.tv_usec == a.m_time.tv_usec));
    }

    bool operator!=(const STime& a)
    {
        return ((m_time.tv_sec != a.m_time.tv_sec) || (m_time.tv_usec != a.m_time.tv_usec));
    }

    bool operator<(const STime& a)
    {
        if (m_time.tv_sec < a.m_time.tv_sec)
            return true;
        if (m_time.tv_sec == a.m_time.tv_sec)
        {
            if (m_time.tv_usec < a.m_time.tv_usec)
                return true;
        }

        return false;
    }

    bool operator>(const STime& a)
    {
        if (m_time.tv_sec > a.m_time.tv_sec)
            return true;
        if (m_time.tv_sec == a.m_time.tv_sec)
        {
            if (m_time.tv_usec > a.m_time.tv_usec)
                return true;
        }

        return false;
    }

    bool operator>=(const STime& a)
    {
        return !(*this < a);
    }

    bool operator<=(const STime& a)
    {
        return !(*this > a);
    }

    STime operator+(const STime& a)
    {
        return add(a.m_time);
    }

    STime operator+(const timeval& t)
    {
        return add(t);
    }

    STime operator-(const STime& a)
    {
        return subtract(a.m_time);
    }

    STime operator-(const timeval& t)
    {
        return subtract(t);
    }

    STime add(int32_t days, int32_t hrs, int32_t mins, int32_t secs, int32_t usecs)
    {
        timeval t;
        t.tv_sec =
            (days * 86400) +
            (hrs * 3600) +
            (mins * 60) +
            secs;
        t.tv_usec = usecs;

        return add(t);
    }

    STime add(const timeval& t)
    {
        STime nt(*this);

        nt.m_time.tv_sec += t.tv_sec;
        nt.m_time.tv_usec += t.tv_usec;
        if (nt.m_time.tv_usec >= 1000000)
        {
            nt.m_time.tv_usec -= 1000000;
            nt.m_time.tv_sec++;
        }

        return nt;
    }

    STime subtract(int32_t days, int32_t hrs, int32_t mins, int32_t secs, int32_t usecs)
    {
        timeval t;
        t.tv_sec =
            (days * 86400) +
            (hrs * 3600) +
            (mins * 60) +
            secs;
        t.tv_usec = usecs;

        return subtract(t);
    }

    STime subtract(const timeval& t)
    {
        STime nt(*this);

        nt.m_time.tv_sec -= t.tv_sec;
        nt.m_time.tv_usec -= t.tv_usec;
        if (nt.m_time.tv_usec < 0)
        {
            nt.m_time.tv_usec += 1000000;
            nt.m_time.tv_sec--;
        }

        return nt;
    }

    const timeval& getTimeVal()
    {
        return m_time;
    }

    STime &operator=(const timeval &a)
    {
       set(a);
       return *this;
    }

    STime &operator=(int64_t ms)
    {
       set(ms);
       return *this;
    }

    void set(const timeval& a)
    {
        m_time.tv_sec = a.tv_sec;
        m_time.tv_usec = a.tv_usec;
    }

    void set(int64_t ms)
    {
       m_time.tv_sec = ms / 1000;
       m_time.tv_usec = ( ms % 1000 ) * 1000;
    }

    int64_t getCassandraTimestmap()
    {
       return m_time.tv_sec * 1000 + (m_time.tv_usec / 1000);
    }

    void getNTPTime(ntp_time_t &ntp) const;
    void setNTPTime(const ntp_time_t &ntp);

    bool isValid() { return m_time.tv_sec != 0 || m_time.tv_usec != 0; }


//    LongLong operator - (const STime& a)
//    {
//        return ((m_time.tv_sec - a.m_time.tv_sec) * 1000000) + (m_time.tv_usec - a.m_time.tv_usec);
//    }

    int32_t year();
    int32_t month();
    int32_t day();
    int32_t hour();
    int32_t minute();
    int32_t second();

    static STime Now();
    void Format(std::string& dest, const char * fmt, bool local);
    void Format(char * dest, int32_t maxsize, const char * fmt, bool local);
    bool ParseDateTime(const char * pszDate, bool isLocal = true);

private:
    timeval m_time;
};

#endif // #define __STIME_H

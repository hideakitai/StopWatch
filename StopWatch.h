#pragma once
#ifndef STOPWATCH_H
#define STOPWATCH_H

#ifdef ARDUINO
#include <Arduino.h>
#define MICROS() micros()
#elif defined(OF_VERSION_MAJOR)
#include "ofMain.h"
#define MICROS() ofGetElapsedTimeMicros()
#else
#error THIS PLATFORM IS NOT SUPPORTED
#endif

class StopWatch
{
protected:

    static constexpr int64_t  UINT32_NUMERIC_LIMIT_MAX  {0x00000000FFFFFFFF};
    static constexpr uint32_t UINT32_NUMERIC_LIMIT_HALF {0x7FFFFFFF};

    bool running {false};
    bool prev_running {false};

    uint32_t prev_us32 {0};
    int64_t prev_us64 {0};
    int64_t origin {0};
    uint32_t ovf {0};
    int64_t offset {0};
    int64_t duration {0};

public:

    virtual ~StopWatch() {}

    inline void start()
    {
        startFromForUsec64(0, 0);
    }
    inline void startFrom(double from_sec)
    {
        startFromForUsec64(from_sec * 1000000., 0);
    }
    inline void startFromMsec(double from_ms)
    {
        startFromForUsec64(from_ms * 1000., 0);
    }
    inline void startFromUsec(double from_us)
    {
        startFromForUsec64(from_us, 0);
    }
    inline void startFor(double for_sec)
    {
        startFromForUsec64(0, for_sec * 1000000.);
    }
    inline void startForMsec(double for_ms)
    {
        startFromForUsec64(0, for_ms * 1000.);
    }
    inline void startForUsec(double for_us)
    {
        startFromForUsec64(0, for_us);
    }
    inline void startFromFor(double from_sec, double for_sec)
    {
        startFromForUsec64(from_sec * 1000000., for_sec * 1000000.);
    }
    inline void startFromForMsec(double from_ms, double for_ms)
    {
        startFromForUsec64(from_ms * 1000., for_ms * 1000.);
    }
    inline void startFromForUsec(double from_us, double for_us)
    {
        startFromForUsec64(from_us, for_us);
    }
    inline void startFromForUsec64(int64_t from_us, int64_t for_us)
    {
        running = true;
        prev_running = false;
        prev_us32 = MICROS();
        origin = prev_us64 = (int64_t)prev_us32;
        ovf = 0;
        offset = from_us;
        duration = for_us;
    }

    inline void stop()
    {
        running = false;
        prev_running = false;
        prev_us32 = 0;
        origin = prev_us64 = 0;
        ovf = 0;
        offset = 0;
        duration = 0;
    }

    inline void play()
    {
        if (isPausing())
        {
            running = true;
            uint32_t curr_us32 = MICROS();
            int64_t diff = 0;
            if (curr_us32 > prev_us32)
                diff = (int64_t)(curr_us32 - prev_us32);
            else
                // TODO: check
                diff = UINT32_NUMERIC_LIMIT_MAX - (int64_t)(prev_us32 - curr_us32);
            origin += diff;
            prev_us64 += diff;
            prev_us32 = curr_us32;
        }
        else if (isRunning())
            ;
        else
            start();
    }

    inline void pause()
    {
        if (isRunning())
        {
            microsec();
            running = false;
        }
    }

    inline void restart()
    {
        stop();
        start();
    }

    inline bool isRunning() const { return running; }
    inline bool isPausing() const { return (!running && (origin != 0)); }
    inline bool isStopping() const { return (!running && (origin == 0)); }

    inline bool hasStarted() const { return running && !prev_running; }
    inline bool hasFinished() const { return !running && prev_running; }

    inline int64_t usec64() { return microsec(); }
    inline double usec() { return (double)microsec(); }
    inline double msec() { return usec() * 0.001; }
    inline double sec() { return usec() * 0.000001; }

    inline double getOrigin() const { return (double)origin; }
    inline uint32_t getOverflow() const { return ovf; }
    inline double getOffset() const { return offset; }
    inline double getDuration() const { return duration; }
    inline double getRemainingTime() { return (double)duration - usec(); }
    inline double getRemainingLife() { return (double)duration - usec() / (double)duration; }

    inline void setOffsetUsec(int64_t us) { offset = us; }
    inline void setOffsetMsec(double ms) { setOffsetUsec(int64_t(1000. * ms)); }
    inline void setOffsetSec(double sec) { setOffsetUsec(int64_t(1000000. * sec)); }

    inline void addOffsetUsec(int64_t us) { setOffsetUsec(offset + us); }
    inline void addOffsetMsec(double ms) { addOffsetUsec(int64_t(1000. * ms)); }
    inline void addOffsetSec(double sec) { addOffsetUsec(int64_t(1000000. * sec)); }

    inline void setTimeUsec(int64_t u)
    {
        if (isStopping())
        {
            prev_us32 = MICROS();
            prev_us64 = (int64_t)prev_us32;
            origin = prev_us64 - u;
            offset = 0;
        }
        else if (isPausing())
        {
#if 0
            uint32_t diff_us32 = MICROS() - prev_us32;
            prev_us32 += diff_us32;
            prev_us64 += (int64_t)diff_us32;
            origin += (int64_t)diff_us32;
#else
            // Serial.print("prev = ");
            // Serial.println(usec());
            int64_t diff_us = (int64_t)MICROS() - (int64_t)prev_us32;
            if (diff_us >= 0)
            {
                // overflow
                if ((int64_t)UINT32_NUMERIC_LIMIT_MAX - (int64_t)prev_us32 < diff_us)
                {
                    prev_us32 += (uint32_t)diff_us; // overflow
                    ++ovf;
                }
                else
                {
                    prev_us32 += (uint32_t)diff_us;
                }
            }
            // overflow
            else
            {
                diff_us = (int64_t)MICROS() + ((int64_t)UINT32_NUMERIC_LIMIT_MAX - (int64_t)prev_us32);
                prev_us32 += (uint32_t)diff_us; // overflow
                ++ovf;
            }
            prev_us64 += diff_us;
            origin += diff_us;
#endif
            setOffsetUsec(u - elapsed());
            // Serial.print("curr = ");
            // Serial.println(usec());
        }
        else
        {
            setOffsetUsec(u - elapsed());
        }
    }
    inline void setTimeMsec(double m) { setTimeUsec(int64_t(m * 1000.)); }
    inline void setTimeSec(double s) { setTimeUsec(int64_t(s * 1000000.)); }


protected:

    inline int64_t microsec()
    {
        if      (isPausing())  return prev_us64 - origin + offset;
        else if (isStopping()) return 0;

        int64_t t = elapsed() + offset;
        if ((t >= duration) && (duration != 0))
        {
            // Serial.print("duration finished : ");
            // Serial.println((double)duration);
            stop();
            prev_running = true;
            return 0;
        }
        prev_running = isRunning();

        return t;
    }

    inline int64_t elapsed()
    {
        uint32_t curr_us32 = MICROS();

        if (curr_us32 < prev_us32) // check overflow and interrupt
        {
            if (MICROS() < prev_us32) // overflow
            {
                if (prev_us32 - MICROS() > UINT32_NUMERIC_LIMIT_HALF)
                {
                    ++ovf;
                }
                else
                {
                    // Serial.print("something is wrong, won't come here ");
                }
            }
            else // interrupted and changed prev_us after curr_us is captured
            {
                curr_us32 = MICROS(); // update curr_us
            }
        }
        prev_us32 = curr_us32;
        int64_t now = (int64_t)curr_us32 | ((int64_t)ovf << 32);
        prev_us64 = now;
        return now - origin;
    }
};

class IntervalCounter : public StopWatch
{
    double interval {0.};
    double cnt {0.};

protected:

    std::function<void(void)> func;

public:

    explicit IntervalCounter (double sec)
    : interval(sec * 1000000.)
    , cnt(0.)
    {}

    virtual ~IntervalCounter() {}

    inline void startForCount(double duration_count = 0.)
    {
        StopWatch::startForUsec((int64_t)(duration_count * interval));
        cnt = 0;
    }

    inline void stop()
    {
        StopWatch::stop();
        cnt = 0;
    }

    inline void restart()
    {
        IntervalCounter::stop();
        IntervalCounter::start();
    }

    inline bool isNext()
    {
        return update();
    }

    inline double count() { if (isPausing()) update(); return (double)cnt; }

    inline void setInterval(double interval_sec)
    {
        interval = interval_sec * 1000000.;
    }

    inline void setOffsetCount(double offset)
    {
        setOffsetUsec(interval * offset);
    }

    inline void addFunction(const std::function<void(void)>& f)
    {
        func = f;
    }

    inline bool hasFunction() const
    {
        return (bool)func;
    }

    inline bool update()
    {
        double prev_cnt = cnt;
        cnt = (double)usec() / interval;
        bool b = (cnt > 0) && (floor(cnt) > floor(prev_cnt));
        if (b && func) func();
        return b;
    }

};

class OneshotTimer : public IntervalCounter
{
public:

    OneshotTimer(const double sec, const std::function<void(void)>& f)
    : IntervalCounter(sec)
    {
        IntervalCounter::addFunction(f);
    }

    void start()
    {
        IntervalCounter::startForCount(1);
    }

    inline bool update()
    {
        if (usec64() == 0)
        {
            // Serial.println("usec64 = 0");
            if (hasFinished())
            {
                // Serial.println("finished");
                if  (hasFunction())
                {
                    // Serial.println("execute event!!!!!!!!!!!");
                    IntervalCounter::func();
                    return true;
                }
            }
        }
        return false;
    }
};

class FrameRateCounter : public IntervalCounter
{
    double fps {40.};
    bool is_one_start {false};

public:

    explicit FrameRateCounter(double fps)
    : IntervalCounter(1.0 / fps)
    , fps(fps)
    , is_one_start(false)
    {}

    virtual ~FrameRateCounter() {}

    inline double frame()
    {
        return is_one_start ? (count() + 1.) : count();
    }

    inline void setFrameRate(double rate)
    {
        fps = rate;
        setInterval(1. / fps);
    }

    inline void setFirstFrameToOne(bool b) { is_one_start = b; }

    double getFrameRate() const { return fps; }
    bool isFristFrameOne() const { return is_one_start; }

};



#endif // STOPWATCH_H

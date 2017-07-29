//
// Created by zhsyourai on 12/26/16.
//
#include <stdlib.h>
#include "time.h"
#include "support/time.h"

static double g_time_scale;
static uint64_t g_time_start;

static struct timespec timespec_from_gpr(zRPC_timespec gts) {
    struct timespec rv;
    rv.tv_sec = (time_t) gts.tv_sec;
    rv.tv_nsec = gts.tv_nsec;
    return rv;
}

static zRPC_timespec zRPC_from_timespec(struct timespec ts,
                                        zRPC_clock_type clock_type) {
    zRPC_timespec rv;
    rv.tv_sec = ts.tv_sec;
    rv.tv_nsec = (int32_t) ts.tv_nsec;
    rv.clock_type = clock_type;
    return rv;
}

/** maps zRPC_clock_type --> clockid_t for clock_gettime */
static const clockid_t clockid_for_zRPC_clock[] = {CLOCK_MONOTONIC,
                                                   CLOCK_REALTIME};

zRPC_timespec zRPC_now(zRPC_clock_type clock_type) {
    struct timespec now;
    if (clock_type == zRPC_CLOCK_PRECISE) {
        zRPC_timespec ret;
        ret = zRPC_now(zRPC_CLOCK_REALTIME);
        ret.clock_type = zRPC_CLOCK_PRECISE;
        return ret;
    } else {
        clock_gettime(clockid_for_zRPC_clock[clock_type], &now);
        return zRPC_from_timespec(now, clock_type);
    }
}

void zRPC_sleep_until(zRPC_timespec until) {
    zRPC_timespec now;
    zRPC_timespec delta;
    struct timespec delta_ts;
    int ns_result;

    for (;;) {
        /* We could simplify by using clock_nanosleep instead, but it might be
         * slightly less portable. */
        now = zRPC_now(until.clock_type);
        if (zRPC_time_cmp(until, now) <= 0) {
            return;
        }

        delta = zRPC_time_sub(until, now);
        delta_ts = timespec_from_gpr(delta);
        ns_result = nanosleep(&delta_ts, NULL);
        if (ns_result == 0) {
            break;
        }
    }
}

int zRPC_time_cmp(zRPC_timespec a, zRPC_timespec b) {
    int cmp = (a.tv_sec > b.tv_sec) - (a.tv_sec < b.tv_sec);
    if (cmp == 0) {
        cmp = (a.tv_nsec > b.tv_nsec) - (a.tv_nsec < b.tv_nsec);
    }
    return cmp;
}

zRPC_timespec zRPC_time_min(zRPC_timespec a, zRPC_timespec b) {
    return zRPC_time_cmp(a, b) < 0 ? a : b;
}

zRPC_timespec zRPC_time_max(zRPC_timespec a, zRPC_timespec b) {
    return zRPC_time_cmp(a, b) > 0 ? a : b;
}

zRPC_timespec zRPC_time_0(zRPC_clock_type type) {
    zRPC_timespec out;
    out.tv_sec = 0;
    out.tv_nsec = 0;
    out.clock_type = type;
    return out;
}

zRPC_timespec zRPC_time_inf_future(zRPC_clock_type type) {
    zRPC_timespec out;
    out.tv_sec = INT64_MAX;
    out.tv_nsec = 0;
    out.clock_type = type;
    return out;
}

zRPC_timespec zRPC_time_inf_past(zRPC_clock_type type) {
    zRPC_timespec out;
    out.tv_sec = INT64_MIN;
    out.tv_nsec = 0;
    out.clock_type = type;
    return out;
}

static zRPC_timespec to_seconds_from_sub_second_time(int64_t time_in_units,
                                                     int64_t units_per_sec,
                                                     zRPC_clock_type type) {
    zRPC_timespec out;
    if (time_in_units == INT64_MAX) {
        out = zRPC_time_inf_future(type);
    } else if (time_in_units == INT64_MIN) {
        out = zRPC_time_inf_past(type);
    } else {
        if (time_in_units >= 0) {
            out.tv_sec = time_in_units / units_per_sec;
        } else {
            out.tv_sec = (-((units_per_sec - 1) - (time_in_units + units_per_sec)) /
                          units_per_sec) -
                         1;
        }
        out.tv_nsec = (int32_t) ((time_in_units - out.tv_sec * units_per_sec) *
                                 zRPC_NS_PER_SEC / units_per_sec);
        out.clock_type = type;
    }
    return out;
}

static zRPC_timespec to_seconds_from_above_second_time(int64_t time_in_units,
                                                       int64_t secs_per_unit,
                                                       zRPC_clock_type type) {
    zRPC_timespec out;
    if (time_in_units >= INT64_MAX / secs_per_unit) {
        out = zRPC_time_inf_future(type);
    } else if (time_in_units <= INT64_MIN / secs_per_unit) {
        out = zRPC_time_inf_past(type);
    } else {
        out.tv_sec = time_in_units * secs_per_unit;
        out.tv_nsec = 0;
        out.clock_type = type;
    }
    return out;
}

zRPC_timespec zRPC_time_from_nanos(int64_t ns, zRPC_clock_type type) {
    return to_seconds_from_sub_second_time(ns, zRPC_NS_PER_SEC, type);
}

zRPC_timespec zRPC_time_from_micros(int64_t us, zRPC_clock_type type) {
    return to_seconds_from_sub_second_time(us, zRPC_US_PER_SEC, type);
}

zRPC_timespec zRPC_time_from_millis(int64_t ms, zRPC_clock_type type) {
    return to_seconds_from_sub_second_time(ms, zRPC_MS_PER_SEC, type);
}

zRPC_timespec zRPC_time_from_seconds(int64_t s, zRPC_clock_type type) {
    return to_seconds_from_sub_second_time(s, 1, type);
}

zRPC_timespec zRPC_time_from_minutes(int64_t m, zRPC_clock_type type) {
    return to_seconds_from_above_second_time(m, 60, type);
}

zRPC_timespec zRPC_time_from_hours(int64_t h, zRPC_clock_type type) {
    return to_seconds_from_above_second_time(h, 3600, type);
}

zRPC_timespec zRPC_time_add(zRPC_timespec a, zRPC_timespec b) {
    zRPC_timespec sum;
    int64_t inc = 0;
    sum.clock_type = a.clock_type;
    sum.tv_nsec = a.tv_nsec + b.tv_nsec;
    if (sum.tv_nsec >= zRPC_NS_PER_SEC) {
        sum.tv_nsec -= zRPC_NS_PER_SEC;
        inc++;
    }
    if (a.tv_sec == INT64_MAX || a.tv_sec == INT64_MIN) {
        sum = a;
    } else if (b.tv_sec == INT64_MAX ||
               (b.tv_sec >= 0 && a.tv_sec >= INT64_MAX - b.tv_sec)) {
        sum = zRPC_time_inf_future(sum.clock_type);
    } else if (b.tv_sec == INT64_MIN ||
               (b.tv_sec <= 0 && a.tv_sec <= INT64_MIN - b.tv_sec)) {
        sum = zRPC_time_inf_past(sum.clock_type);
    } else {
        sum.tv_sec = a.tv_sec + b.tv_sec;
        if (inc != 0 && sum.tv_sec == INT64_MAX - 1) {
            sum = zRPC_time_inf_future(sum.clock_type);
        } else {
            sum.tv_sec += inc;
        }
    }
    return sum;
}

zRPC_timespec zRPC_time_sub(zRPC_timespec a, zRPC_timespec b) {
    zRPC_timespec diff;
    int64_t dec = 0;
    if (b.clock_type == zRPC_TIMESPAN) {
        diff.clock_type = a.clock_type;
    } else {
        diff.clock_type = zRPC_TIMESPAN;
    }
    diff.tv_nsec = a.tv_nsec - b.tv_nsec;
    if (diff.tv_nsec < 0) {
        diff.tv_nsec += zRPC_NS_PER_SEC;
        dec++;
    }
    if (a.tv_sec == INT64_MAX || a.tv_sec == INT64_MIN) {
        diff = a;
    } else if (b.tv_sec == INT64_MIN ||
               (b.tv_sec <= 0 && a.tv_sec >= INT64_MAX + b.tv_sec)) {
        diff = zRPC_time_inf_future(zRPC_CLOCK_REALTIME);
    } else if (b.tv_sec == INT64_MAX ||
               (b.tv_sec >= 0 && a.tv_sec <= INT64_MIN + b.tv_sec)) {
        diff = zRPC_time_inf_past(zRPC_CLOCK_REALTIME);
    } else {
        diff.tv_sec = a.tv_sec - b.tv_sec;
        if (dec != 0 && diff.tv_sec == INT64_MIN + 1) {
            diff = zRPC_time_inf_past(zRPC_CLOCK_REALTIME);
        } else {
            diff.tv_sec -= dec;
        }
    }
    return diff;
}

int zRPC_time_similar(zRPC_timespec a, zRPC_timespec b, zRPC_timespec threshold) {
    int cmp_ab;

    cmp_ab = zRPC_time_cmp(a, b);
    if (cmp_ab == 0) return 1;
    if (cmp_ab < 0) {
        return zRPC_time_cmp(zRPC_time_sub(b, a), threshold) <= 0;
    } else {
        return zRPC_time_cmp(zRPC_time_sub(a, b), threshold) <= 0;
    }
}

int32_t zRPC_time_to_millis(zRPC_timespec t) {
    if (t.tv_sec >= 2147483) {
        if (t.tv_sec == 2147483 && t.tv_nsec < 648 * zRPC_NS_PER_MS) {
            return 2147483 * zRPC_MS_PER_SEC + t.tv_nsec / zRPC_NS_PER_MS;
        }
        return 2147483647;
    } else if (t.tv_sec <= -2147483) {
        /* TODO(ctiller): correct handling here (it's so far in the past do we
           care?)
        */
        return -2147483647;
    } else {
        return (int32_t) (t.tv_sec * zRPC_MS_PER_SEC + t.tv_nsec / zRPC_NS_PER_MS);
    }
}

double zRPC_timespec_to_micros(zRPC_timespec t) {
    return (double) t.tv_sec * zRPC_US_PER_SEC + t.tv_nsec * 1e-3;
}

zRPC_timespec zRPC_convert_clock_type(zRPC_timespec t, zRPC_clock_type clock_type) {
    if (t.clock_type == clock_type) {
        return t;
    }

    if (t.tv_nsec == 0) {
        if (t.tv_sec == INT64_MAX) {
            t.clock_type = clock_type;
            return t;
        }
        if (t.tv_sec == INT64_MIN) {
            t.clock_type = clock_type;
            return t;
        }
    }

    if (clock_type == zRPC_TIMESPAN) {
        return zRPC_time_sub(t, zRPC_now(t.clock_type));
    }

    if (t.clock_type == zRPC_TIMESPAN) {
        return zRPC_time_add(zRPC_now(clock_type), t);
    }

    return zRPC_time_add(zRPC_now(clock_type),
                         zRPC_time_sub(t, zRPC_now(t.clock_type)));
}

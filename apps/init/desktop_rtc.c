#include "desktop_rtc.h"
#include "desktop_ps2.h"

enum {
    CMOS_INDEX_PORT = 0x70,
    CMOS_DATA_PORT = 0x71,
    CMOS_WAIT_LIMIT = 100000u,
    DESKTOP_TIMEZONE_OFFSET_HOURS = 3u
};

static u32 bcd_to_binary(u32 value) {
    return ((value >> 4) * 10u) + (value & 0x0Fu);
}

static u32 cmos_read(u8 index) {
    ps2_port_write(CMOS_INDEX_PORT, index);
    return ps2_port_read(CMOS_DATA_PORT);
}

static void write_two_digits(char* out, u32 value) {
    out[0] = (char)('0' + ((value / 10u) % 10u));
    out[1] = (char)('0' + (value % 10u));
}

static void write_four_digits(char* out, u32 value) {
    out[0] = (char)('0' + ((value / 1000u) % 10u));
    out[1] = (char)('0' + ((value / 100u) % 10u));
    out[2] = (char)('0' + ((value / 10u) % 10u));
    out[3] = (char)('0' + (value % 10u));
}

static u32 is_leap_year(u32 year) {
    if ((year % 400u) == 0u) {
        return 1u;
    }
    if ((year % 100u) == 0u) {
        return 0u;
    }
    return (year % 4u) == 0u;
}

static u32 days_in_month(u32 month, u32 year) {
    switch (month) {
    case 1u: return 31u;
    case 2u: return is_leap_year(year) != 0u ? 29u : 28u;
    case 3u: return 31u;
    case 4u: return 30u;
    case 5u: return 31u;
    case 6u: return 30u;
    case 7u: return 31u;
    case 8u: return 31u;
    case 9u: return 30u;
    case 10u: return 31u;
    case 11u: return 30u;
    default: return 31u;
    }
}

static void advance_date(u32* day, u32* month, u32* year) {
    const u32 max_day = days_in_month(*month, *year);
    if (*day < max_day) {
        *day += 1u;
        return;
    }
    *day = 1u;
    if (*month < 12u) {
        *month += 1u;
        return;
    }
    *month = 1u;
    *year += 1u;
}

static void apply_timezone_offset(u32* hour, u32* day, u32* month, u32* year) {
    *hour += DESKTOP_TIMEZONE_OFFSET_HOURS;
    while (*hour >= 24u) {
        *hour -= 24u;
        advance_date(day, month, year);
    }
}

void desktop_rtc_init(desktop_rtc_state_t* state) {
    state->second = 0xFFFFFFFFu;
    state->minute = 0u;
    state->hour = 0u;
    state->day = 0u;
    state->month = 0u;
    state->year = 0u;
    state->time_text[0] = '0';
    state->time_text[1] = '0';
    state->time_text[2] = ':';
    state->time_text[3] = '0';
    state->time_text[4] = '0';
    state->time_text[5] = 0;
    state->date_text[0] = '0';
    state->date_text[1] = '0';
    state->date_text[2] = '.';
    state->date_text[3] = '0';
    state->date_text[4] = '0';
    state->date_text[5] = '.';
    state->date_text[6] = '0';
    state->date_text[7] = '0';
    state->date_text[8] = '0';
    state->date_text[9] = '0';
    state->date_text[10] = 0;
    state->date_text[11] = 0;
}

u32 desktop_rtc_update(desktop_rtc_state_t* state) {
    u32 second;
    u32 minute;
    u32 hour;
    u32 day;
    u32 month;
    u32 year;
    u32 reg_b;

    for (u32 spin = 0; spin < CMOS_WAIT_LIMIT; ++spin) {
        if ((cmos_read(0x0Au) & 0x80u) == 0u) {
            break;
        }
        if (spin + 1u == CMOS_WAIT_LIMIT) {
            return 0u;
        }
    }

    second = cmos_read(0x00u);
    minute = cmos_read(0x02u);
    hour = cmos_read(0x04u);
    day = cmos_read(0x07u);
    month = cmos_read(0x08u);
    year = cmos_read(0x09u);
    reg_b = cmos_read(0x0Bu);

    if ((reg_b & 0x04u) == 0u) {
        second = bcd_to_binary(second);
        minute = bcd_to_binary(minute);
        hour = bcd_to_binary(hour & 0x7Fu) | (hour & 0x80u);
        day = bcd_to_binary(day);
        month = bcd_to_binary(month);
        year = bcd_to_binary(year);
    }

    if ((reg_b & 0x02u) == 0u && (hour & 0x80u) != 0u) {
        hour = ((hour & 0x7Fu) + 12u) % 24u;
    } else {
        hour &= 0x7Fu;
    }

    year += 2000u;
    apply_timezone_offset(&hour, &day, &month, &year);
    if (second == state->second &&
        minute == state->minute &&
        hour == state->hour &&
        day == state->day &&
        month == state->month &&
        year == state->year) {
        return 0u;
    }

    state->second = second;
    state->minute = minute;
    state->hour = hour;
    state->day = day;
    state->month = month;
    state->year = year;

    write_two_digits(&state->time_text[0], hour);
    state->time_text[2] = ':';
    write_two_digits(&state->time_text[3], minute);
    state->time_text[5] = 0;

    write_two_digits(&state->date_text[0], day);
    state->date_text[2] = '.';
    write_two_digits(&state->date_text[3], month);
    state->date_text[5] = '.';
    write_four_digits(&state->date_text[6], year);
    state->date_text[10] = 0;
    return 1u;
}

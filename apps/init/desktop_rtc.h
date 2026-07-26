#ifndef VEX_INIT_DESKTOP_RTC_H
#define VEX_INIT_DESKTOP_RTC_H

typedef unsigned int u32;

typedef struct desktop_rtc_state {
    u32 second;
    u32 minute;
    u32 hour;
    u32 day;
    u32 month;
    u32 year;
    char time_text[6];
    char date_text[12];
} desktop_rtc_state_t;

void desktop_rtc_init(desktop_rtc_state_t* state);
u32 desktop_rtc_update(desktop_rtc_state_t* state);

#endif

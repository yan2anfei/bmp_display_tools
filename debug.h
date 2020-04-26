#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <syslog.h>

#ifdef LOG_TO_FILE
#define D(fmt, args...) syslog(LOG_DEBUG, fmt, ##args);
#define E(fmt, args...) syslog(LOG_ERR, fmt, ##args);
#else
#define D(fmt, args...) printf(fmt, ##args);
#define E(fmt, args...) printf(fmt, ##args);
#endif

#endif

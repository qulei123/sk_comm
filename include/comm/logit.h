/* syslog */

#ifndef _LOGIT_H_
#define _LOGIT_H_

#include <syslog.h>


#define LOG_OPTS                        (LOG_NOWAIT | LOG_PID)
#define LOG_FACILITY                    LOG_CRON

#define ERR(code, fmt, args...)         logit(LOG_ERR, code, fmt, ##args)
#define LOG(fmt, args...)               logit(LOG_NOTICE, 0, fmt, ##args)
#define INFO(fmt, args...)              logit(LOG_INFO, 0, fmt, ##args)
#define DBG(fmt, args...)               logit(LOG_DEBUG, 0, fmt, ##args)


void log_init(const char *prognm, int level, int use_syslog);
void log_exit(void);
int log_str2lvl(char *arg);
void logit(int severity, int syserr, const char *format, ...) __attribute__((format(printf, 3, 4)));


#endif /* _LOGIT_H_ */


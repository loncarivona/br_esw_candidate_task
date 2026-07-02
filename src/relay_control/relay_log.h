/**
 * @file relay_log.h
 *
 * @brief Optional compile-time debug logging for the relay controller.
 *
 * Define RELAY_LOG_ENABLED and link a platform backend (e.g. relay_log_host.c
 * on the host demo) to emit messages. When disabled, all RELAY_LOG_* macros
 * compile to no-ops with zero runtime and no stdio dependency.
 */

#ifndef RELAY_CONTROL_RELAY_LOG_H
#define RELAY_CONTROL_RELAY_LOG_H

#if defined(RELAY_LOG_ENABLED)

void RelayLog_Printf(const char *fmt, ...);
void RelayLog_Rawf(const char *fmt, ...);

#define RELAY_LOG_ERR(fmt, ...) RelayLog_Printf("[ERR] " fmt, ##__VA_ARGS__)
#define RELAY_LOG_WRN(fmt, ...) RelayLog_Printf("[WRN] " fmt, ##__VA_ARGS__)
#define RELAY_LOG_INF(fmt, ...) RelayLog_Printf("[INF] " fmt, ##__VA_ARGS__)
#define RELAY_LOG_RAW(fmt, ...) RelayLog_Rawf(fmt, ##__VA_ARGS__)

#else

#define RELAY_LOG_ERR(...) ((void)0)
#define RELAY_LOG_WRN(...) ((void)0)
#define RELAY_LOG_INF(...) ((void)0)
#define RELAY_LOG_RAW(...) ((void)0)

#endif /* RELAY_LOG_ENABLED */

#endif /* RELAY_CONTROL_RELAY_LOG_H */

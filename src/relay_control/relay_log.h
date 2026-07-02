/**
 * @file relay_log.h
 *
 * @brief Optional compile-time debug logging for the relay controller.
 *
 * Define LOG_ENABLED and link a platform backend (e.g. relay_log_host.c on the
 * host demo) to emit messages. When disabled, all LOG_* macros compile to no-ops
 * with zero runtime and no stdio dependency.
 */

#ifndef RELAY_CONTROL_RELAY_LOG_H
#define RELAY_CONTROL_RELAY_LOG_H

#if defined(LOG_ENABLED)

void RelayLog_Printf(const char *fmt, ...);

#define LOG_ERR(...) RelayLog_Printf("[ERR] " __VA_ARGS__)
#define LOG_WRN(...) RelayLog_Printf("[WRN] " __VA_ARGS__)
#define LOG_INF(...) RelayLog_Printf("[INF] " __VA_ARGS__)

#else

#define LOG_ERR(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define LOG_INF(...) ((void)0)

#endif /* LOG_ENABLED */

#endif /* RELAY_CONTROL_RELAY_LOG_H */

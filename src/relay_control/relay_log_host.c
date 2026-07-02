/**
 * @file relay_log_host.c
 *
 * @brief Host-only debug log backend (stdio). Link only in debug builds.
 */

#if defined(LOG_ENABLED)

#include "relay_control/relay_log.h"

#include <stdarg.h>
#include <stdio.h>

void RelayLog_Printf(const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  (void)vprintf(fmt, args);
  va_end(args);
}

#endif /* LOG_ENABLED */

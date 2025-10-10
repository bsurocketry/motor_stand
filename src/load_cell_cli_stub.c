/* Minimal stub for load_cell_cli on non-UNIX builds (Windows/MSYS2)
 * Provides the same symbol so GUI can build without POSIX sockets.
 */

#include <pthread.h>
#include "load_cell_cli.h"

/* Return a failure code and don't spawn any threads. This keeps the
 * signature compatible so the GUI can link, but no network client runs.
 */
pthread_t run_load_cell_cli(void (* callback)(output_data *, void * uarg),
                            void * uarg, lc_cli_opt behavior) {
    (void)callback; (void)uarg; (void)behavior;
    return (pthread_t)-1; /* indicate not running */
}

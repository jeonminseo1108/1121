/*---------------------------------------------------------------------------*/
/* snush.h                                                                   */
/* Author: Jongki Park, Kyoungsoo Park                                       */
/*---------------------------------------------------------------------------*/

#ifndef _SNUSH_H_
#define _SNUSH_H_

/* SIG_UNBLOCK & sigset_t */
#ifndef __USE_POSIX
#define __USE_POSIX
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_BG_PRO 16
#define MAX_FG_PRO 16

// TODO-start: data structures in snush.h
/**
 * BackgroundManager:
 * 
 * Structure to manage background processes.
 * 
 * Fields:
 *   - bg_array: Array storing the PIDs of currently running background processes.
 *   - bg_count: Integer representing the count of active background processes.
 * 
 * Notes:
 *   - `MAX_BG_PRO`: Maximum number of background processes allowed.
 *   - `bg_manager`: Global instance of BackgroundManager used to track background jobs.
 */
typedef struct {
    pid_t bg_array[MAX_BG_PRO];
    int bg_count;
} BackgroundManager;

extern BackgroundManager bg_manager;

//
// TODO-end: data structures in snush.h
//

#endif /* _SNUSH_H_ */

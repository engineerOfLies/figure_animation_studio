#ifndef __ACTION_INFO_H__
#define __ACTION_INFO_H__

#include "gf2d_windows.h"
#include "gf2d_actor.h"

Window *action_info(Window *parent,Action *action, Uint8 newAction);

/**
 * setters and getters
 */

/**
 * @brief called by child window to cleanup links
 */
void action_info_child_closed(Window *win);


#endif

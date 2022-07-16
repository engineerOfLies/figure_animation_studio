#ifndef __ACTION_EDITOR_H__
#define __ACTION_EDITOR_H__

#include "gf2d_windows.h"
#include "gf2d_actor.h"
#include "gf2d_figure.h"

Window *action_editor(Window *parent);

/**
 * setters and getters
 */
void action_editor_set_poseindex(Window *win,float poseindex);
void action_editor_set_armature(Window *win,Armature *armature);
void action_editor_set_figure(Window *win,Figure *figure);
void action_editor_set_frame_count(Window *win,Uint32 frameCount);
void action_editor_add_action(Window *win,Action *action);

void action_editor_update_selected_action(Window *win);

/**
 * @brief called by child window to cleanup links
 */
void action_editor_child_closed(Window *win);


#endif

#ifndef __ARMATURE_INSPECTOR_H__
#define __ARMATURE_INSPECTOR_H__

#include "gf2d_windows.h"
#include "gf2d_armature.h"
#include "gf2d_figure.h"

Window *armature_inspector(Window *parent);

/**
 * setters and getters
 */
void armature_inspector_set_bone(Window *win,Bone *bone);
void armature_inspector_set_armature(Window *win,Armature *armature);
void armature_inspector_set_poseindex(Window *win,Uint32 poseindex);
void armature_inspector_set_figure_instance(Window *win,FigureInstance *instance);
void armature_inspector_link_updated(Window *win,FigureLink *link);

/**
 * @brief called by child window to cleanup links
 */
void armature_inspector_child_closed(Window *win);


#endif

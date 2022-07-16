#ifndef __ARMATURE_EDITOR_H__
#define __ARMATURE_EDITOR_H__

#include "gf2d_windows.h"
#include "gf2d_armature.h"
#include "gf2d_figure.h"

/**
 * @brief create the armature editor window
 * @return NULL on error or a pointer to the window
 */
Window *armature_editor();

/**
 * Set and get values for the armature editor
 */
void armature_editor_set_armature(Window *win,Armature *armature);
Armature *armature_editor_get_armature(Window *win);
void armature_editor_subwindow_closed(Window *win);
void armature_editor_set_draw_scale(Window *win,Vector2D scale);
Uint32 armature_editor_next_pose(Window *win);
Uint32 armature_editor_prev_pose(Window *win);
void armature_editor_set_pose_index(Window *win, float index);
void armature_editor_set_pose_index_next(Window *win, Uint32 next);
float armature_editor_get_pose_index(Window *win);
void armature_editor_set_figure(Window *win,Figure *figure);
Figure *armature_editor_get_figure(Window *win);
void armature_editor_figure_instance_remove_link(Window *win,const char *bonename);
void armature_editor_figure_instance_add_link(Window *win,FigureLink *link);
void armature_editor_link_updated(Window *win,FigureLink *link);

/**
 * @brief get the selected bone
 */
Bone *armature_editor_get_selected_bone(Window *win);

/**
 * @brief select a given bone, deselect if NULL
 */
void armature_editor_select_bonepose(Window *win,BonePose *posebone);


#endif

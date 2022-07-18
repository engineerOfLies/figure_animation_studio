#include <stdio.h>

#include "simple_logger.h"

#include "gfc_types.h"
#include "gfc_shape.h"

#include "gf2d_graphics.h"
#include "gf2d_windows.h"
#include "gf2d_elements.h"
#include "gf2d_element_actor.h"
#include "gf2d_element_label.h"
#include "gf2d_element_entry.h"
#include "gf2d_mouse.h"
#include "gf2d_draw.h"
#include "gf2d_windows_common.h"

#include "armature_editor.h"
#include "figure_link_menu.h"
#include "action_editor.h"
#include "action_info.h"

typedef struct
{
    TextLine     nameText;
    TextLine     typeText;
    TextLine     startFrameText;
    TextLine     endFrameText;
    TextLine     frameRateText;
    Action      *action;
    Uint8        newAction;
}ActionInfoData;


int action_info_free(Window *win)
{
    ActionInfoData *data;
    if (!win)return 0;
    data = win->data;
    if (!data)return 0;
    if ((data->action != NULL)&&(data->newAction))
    {
        // I have a pointer to it, and it was new, so delete it
        free(data->action);
    }
    action_editor_update_selected_action(win->parent);
    gf2d_window_close_child(win->parent,win);
    free(data);
    return 0;
}

int action_info_draw(Window *win)
{
    return 0;
}

int action_info_validate_entries(Window *win,ActionInfoData* data)
{
    if (!data)return 0;
    if (!data->action)return 0;
    data->action->startFrame = atoi(data->startFrameText);
    data->action->endFrame = atoi(data->endFrameText);
    data->action->frameRate = atof(data->frameRateText);
    gfc_line_cpy(data->action->name,data->nameText);
    data->action->type = gf2d_actor_type_from_text(data->typeText);
    if (data->newAction)
    {
        action_editor_add_action(win->parent,data->action);
        data->action = NULL;
    }
    return 1;
}

int action_info_update(Window *win,List *updateList)
{
    int i,count;
    Element *e;
    ActionInfoData* data;
    if (!win)return 0;
    if (!updateList)return 0;
    data = (ActionInfoData*)win->data;
    if (!data)return 0;
    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;
        if (strcmp(e->name,"cancel") == 0)
        {
            gf2d_window_free(win);
            return 1;
        }
        if (strcmp(e->name,"ok") == 0)
        {
            if (!action_info_validate_entries(win,data))
            {
                window_alert("Error", "Bad entry for action", NULL,NULL);
            }
            else
            {
                gf2d_window_free(win);
            }
            return 1;
        }
    }
    return gf2d_window_mouse_in(win);
}


Window *action_info(Window *parent,Action *action, Uint8 newAction)
{
    ActionInfoData *data;
    Window *win;
    if (!action)return NULL;
    win = gf2d_window_load("menus/action_info.json");
    if (!win)
    {
        slog("failed to load action_info window");
        return NULL;
    }
    win->update = action_info_update;
    win->free_data = action_info_free;
    win->draw = action_info_draw;
    data = gfc_allocate_array(sizeof(ActionInfoData),1);
    win->data = data;
    win->parent = parent;
    data->action = action;
    data->newAction = newAction;
    gfc_line_sprintf(data->nameText,"%s",action->name);
    gf2d_element_entry_set_text_pointer(gf2d_window_get_element_by_name(win,"action_name"),data->nameText,GFCLINELEN);
    gfc_line_sprintf(data->startFrameText,"%i",action->startFrame);
    gf2d_element_entry_set_text_pointer(gf2d_window_get_element_by_name(win,"start_frame"),data->startFrameText,GFCLINELEN);
    gfc_line_sprintf(data->endFrameText,"%i",action->endFrame);
    gf2d_element_entry_set_text_pointer(gf2d_window_get_element_by_name(win,"end_frame"),data->endFrameText,GFCLINELEN);
    gfc_line_sprintf(data->frameRateText,"%.3f",action->frameRate);
    gf2d_element_entry_set_text_pointer(gf2d_window_get_element_by_name(win,"frame_rate"),data->frameRateText,GFCLINELEN);
    gfc_line_sprintf(data->typeText,"%s",gf2d_actor_type_to_text(action->type));
    gf2d_element_entry_set_text_pointer(gf2d_window_get_element_by_name(win,"action_type"),data->typeText,GFCLINELEN);

    return win;
}

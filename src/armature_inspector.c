#include <stdio.h>

#include "simple_logger.h"

#include "gfc_types.h"

#include "gf2d_graphics.h"
#include "gf2d_windows.h"
#include "gf2d_elements.h"
#include "gf2d_element_actor.h"
#include "gf2d_element_label.h"
#include "gf2d_mouse.h"

#include "windows_common.h"
#include "armature_editor.h"
#include "figure_link_menu.h"
#include "armature_inspector.h"

typedef struct
{
    TextLine        rename;
    Armature        *armature;
    FigureInstance  *instance;
    Uint32          poseindex;
    Bone            *bone;
    BonePose        *poseBone;
    Window          *win;
}ArmatureInspectorData;


void armature_inspector_child_closed(Window *win)
{
    if (!win)return;
    win->child = NULL;
}

int armature_inspector_free(Window *win)
{
    ArmatureInspectorData *data;
    if (!win)return 0;
    data = win->data;
    if (!data)return 0;
    free(data);
    return 0;
}

int armature_inspector_draw(Window *win)
{
    return 0;
}

void onScaleOk(void *Data)
{
    float scale;
    ArmatureInspectorData* data;
    if (!Data)return;
    data = Data;
    scale = atof(data->rename);
    if (scale == 0)
    {
        window_alert("Error","Cannot Scale to zero",NULL,NULL);
        return;
    }
    gf2d_armature_scale(data->armature,vector2d(scale,scale),vector2d(0,0));
}

void onBoneRenameOk(void *Data)
{
    ArmatureInspectorData* data;
    FigureLink *link;
    if (!Data)return;
    data = Data;
    if (!data->bone)return;
    if ((data->instance)&&(data->instance->figure))
    {
        link = gf2d_figure_link_get_by_bonename(data->instance->figure,data->bone->name);
        if (link)
        {
            gfc_line_cpy(link->bonename,data->rename);// keep the link in sync
        }
    }
    gfc_line_cpy(data->bone->name,data->rename);
    armature_inspector_set_bone(data->win,data->bone);
}

void onArmatureRenameOk(void *Data)
{
    ArmatureInspectorData* data;
    if (!Data)return;
    data = Data;
    gfc_line_cpy(data->armature->name,data->rename);
    armature_inspector_set_armature(data->win,data->armature);
}

void onFigureRenameOk(void *Data)
{
    ArmatureInspectorData* data;
    if (!Data)return;
    data = Data;
    if ((!data->instance)||(!data->instance->figure))return;
    gfc_line_cpy(data->instance->figure->name,data->rename);
    armature_inspector_set_figure_instance(data->win,data->instance);
}

void onRemoveLink(void *Data)
{
    Window *win;
    ArmatureInspectorData* data;
    win = Data;
    if ((!win)||(!win->data))return;
    data = win->data;
    if (!data->bone)return;
    armature_editor_figure_instance_remove_link(win->parent,data->bone->name);
}

void armature_inspector_link_updated(Window *win,FigureLink *link)
{
    if ((!win)||(!link))return;
    armature_editor_link_updated(win->parent,link);
}

int armature_inspector_update(Window *win,List *updateList)
{
    TextLine entryText;
    int i,count;
    Element *e;
    FigureLink *link;
    ArmatureInspectorData* data;
    if (!win)return 0;
    if (!updateList)return 0;
    data = (ArmatureInspectorData*)win->data;
    if (!data)return 0;
        
    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;
        if (strcmp(e->name,"bone_name_button") == 0)
        {
            if (!data->bone)break;
            gfc_line_cpy(data->rename,data->bone->name);
            window_text_entry("Rename Bone", data->rename, data, GFCLINELEN, onBoneRenameOk,NULL);
            return 1;
        }
        if (strcmp(e->name,"scale_armature") == 0)
        {
            if (!data->armature)break;
            gfc_line_sprintf(data->rename,"1.0");
            window_text_entry("Scale Armature", data->rename, data, GFCLINELEN, onScaleOk,NULL);
            return 1;
        }
        if (strcmp(e->name,"button_name_armature") == 0)
        {
            if (!data->armature)break;
            gfc_line_cpy(data->rename,data->armature->name);
            window_text_entry("Rename Armature", data->rename, data, GFCLINELEN, onArmatureRenameOk,NULL);
            return 1;
        }
        if (strcmp(e->name,"figure_name_button") == 0)
        {
            if ((!data->instance)||(!data->instance->figure))break;
            gfc_line_cpy(data->rename,data->instance->figure->name);
            window_text_entry("Rename Figure", data->rename, data, GFCLINELEN, onFigureRenameOk,NULL);
            return 1;
        }
        if (strcmp(e->name,"figure_link_button") == 0)
        {
            if (win->child)break;//child window already open
            if ((!data->armature)||(!data->instance)||(!data->instance->figure)||(!data->bone))break;
            //check if a link here already exists, if so ask to delete it?
            link = gf2d_figure_link_get_by_bonename(data->instance->figure,data->bone->name);
            if (!link)
            {
//                slog("attempting to add a new link");
                link = gf2d_figure_add_link_to_bone(data->instance->figure, data->bone->name,NULL,NULL);
                armature_editor_figure_instance_add_link(win->parent,link);
            }
            win->child = figure_link_menu(win,data->instance,link);

            //else make a new one and then pop up a dialog for actor and action to set it
            return 1;
        }
        if (strcmp(e->name,"figure_link_delete_button") == 0)
        {
            if (!data->bone)continue;
            if ((!data->instance)||(!data->instance->figure))continue;
            link = gf2d_figure_link_get_by_bonename(data->instance->figure,data->bone->name);
            if (!link)continue;
            gfc_line_sprintf(entryText,"Remove Link for %s",data->bone->name);
            window_yes_no(entryText, onRemoveLink,NULL,win);
        }
        
    }
    return gf2d_window_mouse_in(win);
}

void armature_inspector_set_poseindex(Window *win,Uint32 poseindex)
{
    TextLine text;
    ArmatureInspectorData* data;    
    if ((!win)||(!win->data))return;
    data = (ArmatureInspectorData*)win->data;
    data->poseindex = poseindex;
    gfc_line_sprintf(text,"%i",poseindex);
    gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"frame_number"),text);
}

void armature_inspector_set_figure_instance(Window *win,FigureInstance *instance)
{
    TextLine text;
    ArmatureInspectorData* data;    
    if ((!win)||(!win->data))return;
    data = (ArmatureInspectorData*)win->data;
    if ((instance == NULL)||(instance->figure == NULL))
    {
        data->instance = NULL;
        gfc_line_sprintf(text,"---");
        gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"figure_name"),text);
        return;
    }
    gfc_line_sprintf(text,"'%s'",instance->figure->name);
    gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"figure_name"),text);
    data->instance = instance;
}

void armature_inspector_set_armature(Window *win,Armature *armature)
{
    TextLine text;
    ArmatureInspectorData* data;    
    if ((!win)||(!win->data))return;
    data = (ArmatureInspectorData*)win->data;
    if (armature == NULL)
    {
        data->bone = NULL;
        gfc_line_sprintf(text,"---");
        gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"armature_name"),text);
        return;
    }
    data->armature = armature;
    gfc_line_sprintf(text,"'%s'",armature->name);
    gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"armature_name"),text);
}

void armature_inspector_set_bone(Window *win,Bone *bone)
{
    TextLine text;
    ArmatureInspectorData* data;    
    if ((!win)||(!win->data))return;
    data = (ArmatureInspectorData*)win->data;
    if (bone == NULL)
    {
        data->bone = NULL;
        gfc_line_sprintf(text,"--- : -");
        gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"bone_name"),text);
        gfc_line_sprintf(text,"Parent : <NONE>");
        gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"parent_name"),text);
        return;
    }
    data->bone = bone;
    gfc_line_sprintf(text,"%s : %i",bone->name,bone->index);
    gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"bone_name"),text);
    
    if (bone->parent)
    {
        gfc_line_sprintf(text,"Parent : %s",bone->parent->name);
    }
    else
    {
        gfc_line_sprintf(text,"Parent : <NONE>");
    }
    gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"parent_name"),text);
}


Window *armature_inspector(Window *parent)
{
    Window *win;
    ArmatureInspectorData* data;
    win = gf2d_window_load("menus/armature_inspector.json");
    if (!win)
    {
        slog("failed to load armature inspector");
        return NULL;
    }
    win->update = armature_inspector_update;
    win->free_data = armature_inspector_free;
    win->draw = armature_inspector_draw;
    data = (ArmatureInspectorData*)gfc_allocate_array(sizeof(ArmatureInspectorData),1);
    win->data = data;
    data->win = win;
    win->parent = parent;
    return win;
}


/*eol@eof*/

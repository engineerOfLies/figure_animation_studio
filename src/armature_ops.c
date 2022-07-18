#include <stdio.h>

#include "simple_logger.h"

#include "gfc_types.h"
#include "gfc_input.h"

#include "gf2d_graphics.h"
#include "gf2d_windows.h"
#include "gf2d_elements.h"
#include "gf2d_element_label.h"
#include "gf2d_mouse.h"
#include "gf2d_armature.h"
#include "gf2d_figure.h"
#include "gf2d_windows_common.h"

#include "armature_editor.h"

extern void exitGame();
extern void exitCheck();


typedef struct
{
    TextLine filename;
    Window *win;
}ArmatureOpsData;

void armature_ops_save_armature(Window *win, ArmatureOpsData* data);
void armature_ops_figure_save(Window *win);

void onArmatureLoadCancel(void *Data)
{
    ArmatureOpsData* data;
    if (!Data)return;
    data = Data;
    gfc_line_cpy(data->filename,"armatures/");
    return;
}

void onArmatureSaveOk(void *Data)
{
    Window *win;
    ArmatureOpsData* data;
    if (!Data)return;
    win = Data;
    data = win->data;
    if (!armature_editor_get_armature(win->parent))return;
    gf2d_armature_save(armature_editor_get_armature(win->parent), data->filename);
    gf2d_window_free(win);
}

void onFigureSaveOk(void *Data)
{
    Window *win;
    ArmatureOpsData* data;
    if (!Data)return;
    win = Data;
    if (!win->data)return;
    data = win->data;
    if (!armature_editor_get_figure(data->win->parent))return;
    gf2d_figure_save(armature_editor_get_figure(data->win->parent), data->filename);
    armature_ops_save_armature(win, data);
}

void onArmatureLoadOk(void *Data)
{
    ArmatureOpsData* data;
    if (!Data)return;
    data = Data;
    slog("loading armature %s",data->filename);
    armature_editor_set_armature(data->win->parent,gf2d_armature_load(data->filename));
    gf2d_window_free(data->win);
    return;
}

void onFigureLoadOk(void *Data)
{
    ArmatureOpsData* data;
    if (!Data)return;
    data = Data;
    slog("loading figure %s",data->filename);
    armature_editor_set_figure(data->win->parent,gf2d_figure_load(data->filename));
    gf2d_window_free(data->win);
    return;
}

int armature_ops_free(Window *win)
{
    ArmatureOpsData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    data = win->data;
    if (win->parent != NULL)
    {
        armature_editor_subwindow_closed(win->parent);
    }
    free(data);

    return 0;
}

int armature_ops_draw(Window *win)
{
    return 0;
}

void armature_ops_save_armature(Window *win, ArmatureOpsData* data)
{
    Armature *armature;
    if (armature_editor_get_armature(win->parent) != NULL)
    {
        armature = armature_editor_get_armature(data->win->parent);
        if (strlen(armature->filepath) > 0)
        {
            gfc_line_cpy(data->filename,armature->filepath);
        }
        else
        {
            gfc_line_cpy(data->filename,"armatures/");
        }
        window_text_entry("Save Armature", data->filename, win, GFCLINELEN, onArmatureSaveOk,NULL);
    }
}

void onSaveFigureFirst(void *Data)
{
    Window *win;
    if (!Data)return;
    win = Data;
    armature_ops_figure_save(win);
}

void OnSaveArmFirst(void *Data)
{
    Window *win;
    ArmatureOpsData* data;
    if (!Data)return;
    win = Data;
    data = win->data;
    armature_ops_save_armature(win, data);
}

void armature_ops_figure_save(Window *win)
{
    Figure *figure;
    ArmatureOpsData* data;
    if ((!win)||(!win->data))return;
    data = win->data;
    
    figure = armature_editor_get_figure(data->win->parent);
    if (strlen(figure->filename) > 0)
    {
        gfc_line_cpy(data->filename,figure->filename);
    }
    else
    {
        gfc_line_cpy(data->filename,"figures/");
    }
    window_text_entry("Save Figure", data->filename, win, GFCLINELEN, onFigureSaveOk,NULL);

}

int armature_ops_update(Window *win,List *updateList)
{
    int i,count;
    Element *e;
    Figure *figure;
    Armature *armature;
    ArmatureOpsData* data;
    if (!win)return 0;
    if (!updateList)return 0;
    data = (ArmatureOpsData*)win->data;
    if (!data)return 0;
        
    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;
        if (strcmp(e->name,"new_armature")==0)
        {
            armature = gf2d_armature_new();
            if (!armature)return 0;
            armature->poses = gfc_list_append(armature->poses,gf2d_armature_pose_new());
            gf2d_armature_add_bone(armature,NULL);
            armature_editor_set_armature(win->parent,armature);
            gf2d_window_free(win);
            return 1;
        }
        if (strcmp(e->name,"load_armature")==0)
        {
            gfc_line_cpy(data->filename,"armatures/");
            window_text_entry("Load Armature", data->filename, data, GFCLINELEN, onArmatureLoadOk,NULL);
            return 1;
        }       
        if (strcmp(e->name,"save_armature")==0)
        {
            armature_ops_save_armature(win, data);
            return 1;
        }
        if (strcmp(e->name,"new_figure")==0)
        {
            figure = gf2d_figure_new();
            if (!figure)return 0;
            armature_editor_set_figure(win->parent,figure);
            gf2d_window_free(win);
            return 1;
        }
        if (strcmp(e->name,"load_figure")==0)
        {
            gfc_line_cpy(data->filename,"figures/");
            window_text_entry("Load Figure", data->filename, data, GFCLINELEN, onFigureLoadOk,NULL);
            return 1;
        }       
        if (strcmp(e->name,"save_figure")==0)
        {
            if (armature_editor_get_figure(win->parent) != NULL)
            {
                figure = armature_editor_get_figure(data->win->parent);
                if (figure->armature)
                {
                    // we have an armature, but it hasn't been saved yet
                    if (strlen(figure->armature->filepath) <= 0)
                    {
                        window_yes_no("Save Armature First?",OnSaveArmFirst,onSaveFigureFirst,win);
                        return 1;
                    }
                }
                armature_ops_figure_save(win);
            }
            return 1;
        }
        if (strcmp(e->name,"quit")==0)
        {
            gf2d_window_free(win);
            exitCheck();
            return 1;
        }
    }
    if (win->parent)
    {
        if ((!gf2d_window_mouse_in(win)&&gf2d_mouse_button_pressed(0))||(gfc_input_command_pressed("cancel")))
        {
            gf2d_window_free(win);
            return 1;
        }
    }
    return 0;
}

void armature_ops_set_filename(Window *win,char *filename)
{
    ArmatureOpsData* data;
    if ((!win)||(!win->data))return;
    data = (ArmatureOpsData*)win->data;
    gfc_line_cpy(data->filename,filename);
}

Window *armature_ops(Window *parent)
{
    Window *win;
    ArmatureOpsData* data;
    win = gf2d_window_load("menus/armature_ops.json");
    if (!win)
    {
        slog("failed to load editor menu");
        return NULL;
    }
    win->update = armature_ops_update;
    win->free_data = armature_ops_free;
    win->draw = armature_ops_draw;
    data = (ArmatureOpsData*)gfc_allocate_array(sizeof(ArmatureOpsData),1);
    gfc_line_cpy(data->filename,"armatures/");
    win->data = data;
    data->win = win;
    win->parent = parent;
    return win;
}


/*eol@eof*/

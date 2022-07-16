#include <stdio.h>

#include "simple_logger.h"

#include "gfc_types.h"
#include "gfc_text.h"
#include "gfc_shape.h"

#include "gf2d_graphics.h"
#include "gf2d_windows.h"
#include "gf2d_elements.h"
#include "gf2d_element_label.h"
#include "gf2d_element_actor.h"
#include "gf2d_draw.h"
#include "gf2d_mouse.h"

#include "windows_common.h"
#include "armature_inspector.h"
#include "figure_link_menu.h"

static TextLine last_actor = {0};

typedef struct
{
    TextLine filename;
    Uint32  draw_order;
    FigureInstance *instance;
    FigureLink *link;
    TextLine actorname;
    TextLine action;
}FigureLinkData;

void figure_link_menu_set_actor(Window *win,const char *actorname,const char *actionname);


int figure_link_menu_free(Window *win)
{
    FigureLinkData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    data = win->data;
    armature_inspector_child_closed(win->parent);
    free(data);
    return 0;
}

int figure_link_menu_draw(Window *win)
{
    return 0;
}

void onActorChanged(void *Data)
{
    FigureLinkData* data;
    Window *win;
    if (!Data)return;
    win = Data;
    data = win->data;
    figure_link_menu_set_actor(win,data->filename,NULL);
}

void onActionChanged(void *Data)
{
    FigureLinkData* data;
    Window *win;
    if (!Data)return;
    win = Data;
    data = win->data;
    figure_link_menu_set_actor(win,NULL,data->filename);
}

void figure_link_menu_set_draw_order(Window *win)
{
    TextLine label;
    FigureLinkData *data;
    if (!win)return;
    if (!win->data)return;
    data = win->data;
    if ((!data->instance)||(!data->instance->figure)||(!data->link))return;
    data->draw_order = gfc_list_get_item_index(data->instance->figure->links,data->link);
//    slog("draw order for bone %s is: %i",data->link->bonename,data->draw_order);
    gfc_line_sprintf(label,"%i",data->draw_order);
    gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"draw_order"), label);
}

int figure_link_menu_update(Window *win,List *updateList)
{
    int i,count;
    Element *e;
    FigureLinkData* data;
    if (!win)return 0;
    if (!updateList)return 0;
    data = (FigureLinkData*)win->data;
    if (!data)return 0;
    
    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;        
        if (strcmp(e->name,"actor_button")==0)
        {
            if (strlen(data->actorname))
            {
                gfc_line_cpy(data->filename,data->actorname);
            }
            else
            {
                gfc_line_cpy(data->filename,"actors/");
            }
            window_text_entry("Load Background Actor", data->filename, win, GFCLINELEN, onActorChanged,NULL);
            return 1;
        }
        if (strcmp(e->name,"action_button")==0)
        {
            if (strlen(data->action))
            {
                gfc_line_cpy(data->filename,data->action);
            }
            else
            {
                gfc_line_clear(data->filename);
            }
            window_text_entry("Set Action", data->filename, win, GFCLINELEN, onActionChanged,NULL);
            return 1;
        }
        if (strcmp(e->name,"lower_draw_order")==0)
        {
            if (data->draw_order > 0)
            {
                gfc_list_swap_indices(data->instance->figure->links,data->draw_order, data->draw_order - 1);
                gfc_list_swap_indices(data->instance->instances,data->draw_order, data->draw_order - 1);
                figure_link_menu_set_draw_order(win);
            }
            return 1;
        }
        if (strcmp(e->name,"raise_draw_order")==0)
        {
            if ((data->draw_order + 1) < gfc_list_get_count(data->instance->figure->links))
            {
                gfc_list_swap_indices(data->instance->figure->links,data->draw_order, (data->draw_order + 1));
                gfc_list_swap_indices(data->instance->instances,data->draw_order, data->draw_order + 1);
                figure_link_menu_set_draw_order(win);
            }
            return 1;
        }
        if (strcmp(e->name,"OK")==0)
        {
            if ((strlen(data->actorname) == 0)||(strlen(data->action) == 0))
            {
                window_alert("Error", "missing data, nothing done", NULL,NULL);
                gf2d_window_free(win);
                return 1;            
            }
            gf2d_figure_link_set_actor(data->link,data->actorname,data->action);
            armature_inspector_link_updated(win->parent,data->link);
            gf2d_window_free(win);
            return 1;            
        }
        if (strcmp(e->name,"cancel")==0)
        {
            gf2d_window_free(win);
            return 1;
        }
    }
    return 1;
}

void figure_link_menu_set_actor(Window *win,const char *actorname,const char *actionname)
{
    FigureLinkData* data;
    Element *e;
    if (!win)return;
    data = win->data;
    if (!data)return;
    if (actorname)
    {
        gfc_line_cpy(data->actorname,actorname);
        if (actorname != last_actor)gfc_line_cpy(last_actor,actorname);
    }
    if (actionname)gfc_line_cpy(data->action,actionname);
    e = gf2d_window_get_element_by_name(win,"preview");
    if (!e)return;
    if ((actorname)&&(strlen(actorname)))
    {
        gf2d_element_actor_set_actor(e, actorname);
        gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"actorname"), actorname);
    }
    if ((actionname)&&(strlen(actionname)))
    {
        gf2d_element_actor_set_action(e, actionname);
        gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"actionname"), actionname);
    }
}

Window *figure_link_menu(Window *parent,FigureInstance *instance, FigureLink *link)
{
    Window *win;
    TextLine bonename;
    FigureLinkData* data;
    win = gf2d_window_load("menus/figure_link_menu.json");
    if ((!win)||(!link))
    {
        slog("failed to figure link menu");
        return NULL;
    }
    win->update = figure_link_menu_update;
    win->free_data = figure_link_menu_free;
    win->draw = figure_link_menu_draw;
    data = (FigureLinkData*)gfc_allocate_array(sizeof(FigureLinkData),1);
    win->data = data;
    win->parent = parent;
    
    data->instance = instance;
    data->link = link;
    
    gfc_line_sprintf(bonename,"Bone: %s",link->bonename);
    gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"bonename"), link->bonename);
    if (strlen(link->actorname) > 0)
    {
        figure_link_menu_set_actor(win,link->actorname,link->action);
    }
    else
    {
        if (strlen(last_actor) > 0)
        {
            figure_link_menu_set_actor(win,last_actor,NULL);
        }
    }
    figure_link_menu_set_draw_order(win);
    return win;
}


/*eol@eof*/

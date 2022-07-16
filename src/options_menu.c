#include <stdio.h>

#include "simple_logger.h"

#include "gfc_types.h"
#include "gf2d_graphics.h"
#include "gf2d_windows.h"
#include "gf2d_elements.h"
#include "gf2d_element_label.h"
#include "gf2d_draw.h"
#include "gf2d_mouse.h"

#include "windows_common.h"
#include "options_menu.h"

extern void exitGame();
extern void exitCheck();
extern void exitCheck();

typedef struct
{
    int selectedOption;
    Entity *player;
    TextLine filename;
}OptionsMenuData;

void onGameSaveCancel(void *Data)
{
}

void onGameSaveOk(void *Data)
{
}


int options_menu_free(Window *win)
{
    OptionsMenuData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    gf2d_window_close_child(win->parent,win);
    data = win->data;
    free(data);

    return 0;
}

int options_menu_draw(Window *win)
{
    return 0;
}

int options_menu_update(Window *win,List *updateList)
{
    int i,count;
    Element *e;
    OptionsMenuData* data;
    if (!win)return 0;
    if (!updateList)return 0;
    data = (OptionsMenuData*)win->data;
    if (!data)return 0;
    
    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;
        if (strcmp(e->name,"item_down")==0)
        {
            data->selectedOption++;
            if (data->selectedOption > 4)data->selectedOption = 1;
            gf2d_window_set_focus_to(win,gf2d_window_get_element_by_id(win,data->selectedOption));
            return 1;
        }
        else if (strcmp(e->name,"item_up")==0)
        {
            data->selectedOption--;
            if (data->selectedOption <=0 )data->selectedOption = 4;
            gf2d_window_set_focus_to(win,gf2d_window_get_element_by_id(win,data->selectedOption));
            return 1;
        }
        else if (strcmp(e->name,"savegame")==0)
        {
            //TODO:
            return 1;
        }
        else if (strcmp(e->name,"exitgame")==0)
        {
            exitCheck();
            return 1;
        }
    }
    return 0;
}


Window *options_menu(Entity *player)
{
    Window *win;
    OptionsMenuData* data;
    win = gf2d_window_load("menus/options_menu.json");
    if (!win)
    {
        slog("failed to load editor menu");
        return NULL;
    }
    win->update = options_menu_update;
    win->free_data = options_menu_free;
    win->draw = options_menu_draw;
    data = (OptionsMenuData*)gfc_allocate_array(sizeof(OptionsMenuData),1);
    win->data = data;
    data->player = player;
    data->selectedOption = 1;
    gf2d_window_set_focus_to(win,gf2d_window_get_element_by_id(win,data->selectedOption));
    return win;
}


/*eol@eof*/

#include <stdio.h>

#include "simple_logger.h"

#include "gfc_types.h"
#include "gfc_text.h"

#include "gf2d_font.h"
#include "gf2d_graphics.h"
#include "gf2d_windows.h"
#include "gf2d_elements.h"
#include "gf2d_element_list.h"
#include "gf2d_element_button.h"
#include "gf2d_element_label.h"
#include "gf2d_mouse.h"

#include "windows_common.h"
#include "key_value_list.h"

typedef struct
{
    TextLine    deleteKey;
    int         deleteIndex;
    TextLine    newKey;
    TextLine    newValue;
    SJson      *list;
}KeyValueListData;

void key_value_clear_items(Window *self);
void key_value_add_all_items(Window *self,SJson *keys);

int key_value_list_free(Window *win)
{
    gf2d_window_close_child(win->parent,win);
    gf2d_window_free(win->child);
    return 0;
}

int key_value_list_draw(Window *win)
{
    return 0;
}

void onNewKeyCancel(void *data)
{
    Window *win;
    if (!data)return;
    win = (Window *)data;
    win->child = NULL;
}

void onNewKeyOk(void *wdata)
{
    Window *win;
    KeyValueListData *data;
    SJson *old,*array;
    if (!wdata)return;
    win = (Window *)wdata;
    data = (KeyValueListData *)win->data;
//    slog("new data: %s / %s",data->newKey,data->newValue);
    old = sj_object_get_value(data->list,data->newKey);
    if (old)
    {
        if (sj_is_array(old))
        {
            sj_array_append(old,sj_new_str(data->newValue));
        }
        else
        {
            old = sj_copy(old);//make a backup, because the object will delete its'
            sj_object_delete_key(data->list,data->newKey);
            array = sj_array_new();
            sj_array_append(array,old);
            sj_array_append(array,sj_new_str(data->newValue));
            sj_object_insert(data->list,data->newKey,array);
        }
    }
    else
    {
        sj_object_insert(data->list,data->newKey,sj_new_str(data->newValue));
    }
    key_value_clear_items(win);
    key_value_add_all_items(win,data->list);
    win->child = NULL;
}

const char *key_value_get_key_by_index(SJson *list,int index)
{
    int i,j,d,c,k;
    SJson *key,*value;
    SJList *keys;
    if (!list)return NULL;
    keys = sj_object_get_keys_list(list);
    if (!keys)return NULL;
    c = sj_list_get_count(keys);
    for (i = 0,k = 0;i < c;i++)
    {
        key = sj_object_get_value(list,sj_list_get_nth(keys,i));
        if (!key)continue;
        if (sj_is_string(key))
        {
            if (k++ == index)
            {
                return sj_get_string_value(key);
            }
            continue;
        }
        if (sj_is_array(key))
        {
            d = sj_array_get_count(key);
            for (j = 0; j < d; j++)
            {
                value = sj_array_get_nth(key,j);
                if (sj_is_string(value))
                {
                    if (k++ == index)
                    {
                        return sj_get_string_value(value);
                    }
                }
            }
        }
    }
    // bad index
    return NULL;
}

void onKeyDelete(void *winData)
{
    int i,j,d,c,k;
    SJson *key,*value;
    SJList *keys;
    KeyValueListData *data;
    Window *win;
    if (!winData)return;
    win = (Window *)winData;
    data = (KeyValueListData *)win->data;
    win->child = NULL;

    if (!data->list)return;
    keys = sj_object_get_keys_list(data->list);
    if (!keys)return;
    c = sj_list_get_count(keys);
    for (i = 0,k = 0;i < c;i++)
    {
        key = sj_object_get_value(data->list,sj_list_get_nth(keys,i));
        if (!key)continue;
        if (sj_is_string(key))
        {
            if (k++ == data->deleteIndex)
            {
                sj_object_delete_key(data->list,sj_list_get_nth(keys,i));
                break;
            }
            continue;
        }
        if (sj_is_array(key))
        {
            d = sj_array_get_count(key);
            for (j = 0; j < d; j++)
            {
                value = sj_array_get_nth(key,j);
                if (sj_is_string(value))
                {
                    if (k++ == data->deleteIndex)
                    {//found it
                        sj_array_delete_nth(key,j);
                        goto onKeyDeleteDone;
                    }
                }
            }
        }
    }
onKeyDeleteDone:
    key_value_clear_items(win);
    key_value_add_all_items(win,data->list);
}

int key_value_list_update(Window *win,List *updateList)
{
    TextLine line;
    int key;
    int i,count;
    const char *keyName;
    Element *e;
    KeyValueListData* data;
    if (!win)return 0;
    if (!updateList)return 0;
    data = (KeyValueListData*)win->data;
    if (!data)return 0;
        
    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;
        if (strcmp(e->name,"close") == 0)
        {
            gf2d_window_free(win);
            return 1;
        }
        if (strcmp(e->name,"add") == 0)
        {
            if (!win->child)
            {
                gfc_line_clear(data->newKey);
                gfc_line_clear(data->newValue);
                win->child = window_key_value("Enter new Key and Value", data->newKey,data->newValue,win, 128,128, onNewKeyOk,onNewKeyCancel);
            }
            return 1;
        }
        if ((!win->child)&&(e->index >= 1000))
        {
            key = e->index - 1000;
            data->deleteIndex = key;
            keyName = key_value_get_key_by_index(data->list,key);
            gfc_line_cpy(data->deleteKey,keyName);
            gfc_line_sprintf(line,"Delete Key '%s'?",keyName);
            win->child = window_yes_no(line, onKeyDelete,onNewKeyCancel,win);
            return 1;
        }
    }
    return gf2d_window_mouse_in(win);
}

void key_value_add_item(Window *self, const char *item,int index)
{
    Element *list;
    Element *be,*le;
    
    LabelElement *label;

    if ((!self)||(!item))return;
    list = gf2d_window_get_element_by_name(self,"item_list");
    if (!list)
    {
        slog("window missing list element");
        return;
    }
    
    label = gf2d_element_label_new_full((char *)item,gfc_color8(255,255,255,255),FT_Small,LJ_Left,LA_Middle,0);

    be = gf2d_element_new_full(
        list,
        1000+index,
        (char*)item,
        gfc_rect(0,0,460,24),
        gfc_color(1,1,1,1),
        0,
        gfc_color(.5,.5,.5,1),1,self);
    le = gf2d_element_new_full(
        be,
        2000+index,
        (char*)item,
        gfc_rect(0,0,460,24),
        gfc_color(1,1,1,1),
        0,
        gfc_color(1,1,1,1),0,self);
    
    gf2d_element_make_label(le,label);
    gf2d_element_make_button(be,gf2d_element_button_new_full(le,NULL,gfc_color(1,1,1,1),gfc_color(0.9,0.9,0.9,1),0));
    gf2d_element_list_add_item(list,be);
}

void key_value_clear_items(Window *self)
{
    if (!self)return;
    gf2d_element_list_free_items(gf2d_window_get_element_by_name(self,"item_list"));
}

void key_value_add_all_items(Window *self,SJson *keys)
{
    TextLine newItem;
    SJList *list;
    SJson *key,*value;
    const char *item;
    int i,c;
    int j,d;
    int k;
    if ((!self)||(!keys))return;
    list = sj_object_get_keys_list(keys);
    if (!list)return;
    c = sj_list_get_count(list);
    k = 0;
    for (i = 0;i < c;i++)
    {
        item = sj_list_get_nth(list,i);
        if (!item)continue;
        key = sj_object_get_value(keys,item);
        if (sj_is_string(key))
        {
            gfc_line_sprintf(newItem,"%s : %s",item,sj_get_string_value(key));
            key_value_add_item(self, newItem,k++);
        }
        else if (sj_is_array(key))
        {
            d = sj_array_get_count(key);
            for (j = 0; j < d; j++)
            {
                value = sj_array_get_nth(key,j);
                if (!value)continue;
                gfc_line_sprintf(newItem,"%s : %s",item,sj_get_string_value(value));
                key_value_add_item(self, newItem,k++);
            }
        }
    }
    sj_list_delete(list);
}

Window *key_value_list(Window *parent,SJson *list)
{
    Window *win;
    KeyValueListData* data;
    win = gf2d_window_load("menus/key_value_list.json");
    if (!win)
    {
        slog("failed to load editor menu");
        return NULL;
    }
    win->update = key_value_list_update;
    win->free_data = key_value_list_free;
    win->draw = key_value_list_draw;
    data = (KeyValueListData*)gfc_allocate_array(sizeof(KeyValueListData),1);
    data->list = list;
    win->data = data;
    win->parent = parent;
    key_value_add_all_items(win,list);
    return win;
}

/*eol@eof*/

#include <stdio.h>

#include "simple_logger.h"

#include "gfc_types.h"
#include "gfc_shape.h"

#include "gf2d_graphics.h"
#include "gf2d_windows.h"
#include "gf2d_elements.h"
#include "gf2d_element_actor.h"
#include "gf2d_element_label.h"
#include "gf2d_mouse.h"
#include "gf2d_draw.h"
#include "gf2d_windows_common.h"

#include "armature_editor.h"
#include "figure_link_menu.h"
#include "action_info.h"
#include "action_editor.h"

static int max_draw_frames = 10;//how many frames can show up on the number line before we need to pan to see the rest

typedef enum
{
    AEA_None = 0,
    AEA_Pan,
    AEA_Play,
    AEA_Window,
    AEA_MAXACTIONS
}ActionEditorActions;

typedef struct
{
    float        timelineOffset;
    TextLine     rename;
    Action      *newAction;
    Armature    *armature;
    Figure      *figure;
    Action      *selectedAction;
    Uint32       poseindex;
    Uint32       frameCount;
    Window      *win;
    Vector2D     actionStart;
    Vector2D     panStart;
    ActionEditorActions action;
}ActionEditorData;

int action_editor_timeline_get_index_at(Window *win,Vector2D at);
int action_editor_timeline_get_nearest_index(Window *win,Vector2D at);


int action_editor_free(Window *win)
{
    ActionEditorData *data;
    if (!win)return 0;
    data = win->data;
    if (!data)return 0;
    free(data);
    return 0;
}

void action_editor_draw_segment(Vector2D a,Vector2D b, Color color)
{
    gf2d_draw_line(a,b,color);
    gf2d_draw_circle(a, 5, color);
    gf2d_draw_circle(b, 5, color);
}

Vector2D action_editor_get_timeline_beginning(Window *win)
{
    Vector2D beginning = {0,0};
    if (!win)
        return beginning;
    beginning = vector2d(win->dimensions.x+32,win->dimensions.y + 64);
    return beginning;
}

Vector2D action_editor_get_timeline_end(Window *win)
{
    Vector2D end = {0,0};
    if (!win)
        return end;
    end = vector2d(win->dimensions.x+win->dimensions.w - 32,win->dimensions.y + 64);
    return end;
}

float action_editor_get_timeline_step(Window *win)
{
    int count;
    Vector2D beginning,end;
    float length;
    ActionEditorData *data;
    if (!win)return 0;
    data = win->data;
    if (!data)return 0;
    if (!data->frameCount)return 0;
    count = MIN(max_draw_frames,data->frameCount);
    beginning = action_editor_get_timeline_beginning(win);
    end = action_editor_get_timeline_end(win);
    length = end.x - beginning.x;
    return length / (float)(count - 1);
}

void action_editor_add_action(Window *win,Action *action)
{
    ActionEditorData *data;
    if (!win)return;
    data = win->data;
    if (!data)return;
    if (!action)return;
    if (!data->armature)return;
    if (!data->armature->actions)
    {
        slog("action list has not been provided");
    }
    data->armature->actions = gfc_list_append(data->armature->actions,action);
}

float action_editor_get_timeline_frame_position(Window *win,Uint32 frame)
{
    float step;
    Vector2D beginning;
    ActionEditorData *data;
    if ((!win)||(!win->data))return 0;
    data = win->data;
    if (!data->frameCount)return 0;
    beginning = action_editor_get_timeline_beginning(win);
    step = action_editor_get_timeline_step(win);
    return beginning.x + (frame * step);
}

void action_editor_squence_draw(Window *win,Uint32 startFrame, Uint32 endFrame,Color color)
{
    Vector2D p1,p2;
    Vector2D beginning;
    Vector2D end;

    ActionEditorData *data;
    if ((!win)||(!win->data))return;
    data = win->data;

    beginning = action_editor_get_timeline_beginning(win);
    end = action_editor_get_timeline_end(win);
    p1 = vector2d(action_editor_get_timeline_frame_position(win,startFrame) + data->timelineOffset,beginning.y - 2);
    p2 = vector2d(action_editor_get_timeline_frame_position(win,endFrame) + data->timelineOffset,beginning.y - 2);
    if ((p1.x > end.x)||(p2.x < beginning.x))return; // can't draw, we are off the timeline
    if (p1.x < beginning.x)p1.x = beginning.x;//we clip the timeline, so start us at the beginning
    if (p2.x > end.x)p2.x = end.x;//we clip the timeline, so start us at the beginning
    action_editor_draw_segment(
        p1,
        p2,
        color);
}

void action_editor_action_draw(Window *win,Action *action,Color color)
{
    if (!win)return;
    if (!action)return;
    action_editor_squence_draw(win,action->startFrame, action->endFrame,color);
}

void action_editor_action_list_draw(Window *win)
{
    int i,c;
    TextLine text;
    Action *action;
    ActionEditorData *data;
    if (!win)return;
    data = win->data;
    if (!data)return;
    if (!data->armature)return;
    if (!data->armature->actions)return;
    c = gfc_list_get_count(data->armature->actions);
    for (i = 0; i < c; i++)
    {
        action = gfc_list_get_nth(data->armature->actions,i);
        if (!action)continue;
        if (data->selectedAction == action)
        {
            action_editor_action_draw(win,action,gfc_color8(100,255,255,255));
        }
        else
        {
            action_editor_action_draw(win,action,gfc_color8(0,200,200,255));
        }
    }
    if (data->selectedAction)
    {
        gfc_line_sprintf(text,"%s (%i -> %i @ %0.2f)",
                         data->selectedAction->name,
                         data->selectedAction->startFrame,
                         data->selectedAction->endFrame,
                         data->selectedAction->frameRate);
        gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"action_name"),text);
    }
    else
    {
        gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"action_name"),"<none>");
    }
}

void action_editor_timeline_draw(Window *win)
{
    TextLine text;
    Vector2D drawPosition;
    Vector2D beginning,end;
    float zeroline;
    int i;
    ActionEditorData *data;
    if ((!win)||(!win->data))return;
    data = win->data;

    //the line
    beginning = action_editor_get_timeline_beginning(win);
    end = action_editor_get_timeline_end(win);
    action_editor_draw_segment(beginning,end, gfc_color8(200,200,200,255));
    //the zero line
    zeroline = beginning.x + data->timelineOffset;
    if ((zeroline >= beginning.x)&&(zeroline <= end.x))
    {
        gf2d_draw_line(vector2d(zeroline,beginning.y - 10),vector2d(zeroline,beginning.y + 10), gfc_color8(255,255,255,255));
    }

    //current frame
    drawPosition = vector2d(action_editor_get_timeline_frame_position(win,data->poseindex) + data->timelineOffset,beginning.y);
    if ((drawPosition.x >= beginning.x)&&(drawPosition.x <= end.x))
    {
        gf2d_draw_circle(drawPosition, 10, gfc_color8(255,255,255,255));
    }
    
    // each frame
    for (i = 0;i < data->frameCount; i++)
    {
        drawPosition = vector2d(action_editor_get_timeline_frame_position(win,i) + data->timelineOffset,beginning.y);
        if ((drawPosition.x < beginning.x)||(drawPosition.x > end.x))continue;
        gf2d_draw_diamond(drawPosition, 8, gfc_color8(200,200,200,255));
    }
    
    i = action_editor_timeline_get_nearest_index(win,beginning);
    if (i != -1)
    {
        gfc_line_sprintf(text,"%i",i);
        gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"frame_start"),text);
    }
    i = action_editor_timeline_get_nearest_index(win,end);
    if (i != -1)
    {
        gfc_line_sprintf(text,"%i",i);
        gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"frame_end"),text);
    }
}

// return -1 on nothing, or the frame index otherwise
int action_editor_timeline_get_nearest_index(Window *win,Vector2D at)
{
    int i;
    float step;
    float bestDelta = 9001;
    int best = -1;
    Vector2D drawPosition;
    Vector2D beginning,end;
    ActionEditorData *data;
    if (!win)return -1;
    data = win->data;
    if (!data)return -1;
    if (!data->frameCount)return -1;
    beginning = action_editor_get_timeline_beginning(win);
    end = action_editor_get_timeline_end(win);
    step = action_editor_get_timeline_step(win);
    for (i = 0;i < data->frameCount; i++)
    {
        drawPosition = vector2d(beginning.x + (i * step) + data->timelineOffset,beginning.y);
        if ((drawPosition.x < beginning.x)||(drawPosition.x > end.x))continue;
        if (fabs(at.x - drawPosition.x) < bestDelta)
        {
            bestDelta = fabs(at.x - drawPosition.x);
            best = i;
        }
    }
    return best;
}

int action_editor_timeline_get_index_at(Window *win,Vector2D at)
{
    int i;
    float step;
    Vector2D drawPosition;
    Vector2D beginning,end;
    ActionEditorData *data;
    if (!win)return -1;
    data = win->data;
    if (!data)return -1;
    if (!data->frameCount)return -1;
    beginning = action_editor_get_timeline_beginning(win);
    end = action_editor_get_timeline_end(win);
    step = action_editor_get_timeline_step(win);
    for (i = 0;i < data->frameCount; i++)
    {
        drawPosition = vector2d(beginning.x + (i * step) + data->timelineOffset,beginning.y);
        if ((drawPosition.x < beginning.x)||(drawPosition.x > end.x))continue;
        if (gfc_point_in_cicle(at,gfc_circle(drawPosition.x,drawPosition.y,5)))
        {
            return i;
        }
    }
    return -1;
}

int action_editor_timeline_mouse_over(Window *win)
{
    Vector2D mouse;
    mouse = gf2d_mouse_get_position();
    return action_editor_timeline_get_index_at(win,mouse);
}

int action_editor_draw(Window *win)
{
    gf2d_window_draw(win);
    action_editor_timeline_draw(win);
    action_editor_action_list_draw(win);
    return 1;
}

void onAddNewFrame(void *Data)
{
    ActionEditorData* data;
    Window *win;
    if (!Data)return;
    win = Data;
    data = win->data;
    gf2d_armature_pose_add(data->armature);
    armature_editor_set_pose_index(win->parent, gf2d_armature_get_pose_count(data->armature) - 1);
    action_editor_set_frame_count(win,++data->frameCount);
}

void onDuplicateFrame(void *Data)
{
    ActionEditorData* data;
    Window *win;
    if (!Data)return;
    win = Data;
    data = win->data;
    gf2d_armature_pose_duplicate(data->armature,gf2d_armature_pose_get_by_index(data->armature,data->poseindex));
    armature_editor_set_pose_index(win->parent, gf2d_armature_get_pose_count(data->armature) - 1);
    action_editor_set_frame_count(win,++data->frameCount);
}

void onInsertDuplicateFrame(void *Data)
{
    Uint32 poseindex;
    ActionEditorData* data;
    Window *win;
    if (!Data)return;
    win = Data;
    data = win->data;
    poseindex = data->poseindex + 1;
    gf2d_armature_pose_duplicate_at_index(data->armature,gf2d_armature_pose_get_by_index(data->armature,data->poseindex),poseindex);
    armature_editor_set_pose_index(win->parent, poseindex);
    action_editor_set_frame_count(win,++data->frameCount);
    if (data->armature)
    {
        gf2d_action_list_frame_inserted(data->armature->actions,poseindex);
    }
}


void onInsertFrame(void *Data)
{
    Uint32 poseindex;
    ActionEditorData* data;
    Window *win;
    if (!Data)return;
    win = Data;
    data = win->data;
    if (!data->armature)
    poseindex = data->poseindex + 1;
    if (gf2d_armature_pose_add_at_index(data->armature,poseindex) == NULL)return;
    armature_editor_set_pose_index(win->parent,poseindex );
    action_editor_set_frame_count(win,++data->frameCount);
    if (data->armature)
    {
        gf2d_action_list_frame_inserted(data->armature->actions,poseindex);
    }
}

void onDeleteAction(void *Data)
{
    ActionEditorData* data;
    Window *win;
    if (!Data)return;
    win = Data;
    data = win->data;
    if (!data->armature)return;
    if (!data->selectedAction)return;
    gfc_list_delete_data(data->armature->actions,data->selectedAction);
}

void onDeleteFrame(void *Data)
{
    Uint32 prev_frame;
    ActionEditorData* data;
    Window *win;
    if (!Data)return;
    win = Data;
    data = win->data;
    prev_frame = data->poseindex;
    if (prev_frame > 0)prev_frame--;
    gf2d_armature_delete_pose_by_index(data->armature,data->poseindex);
    armature_editor_select_bonepose(win->parent,NULL);
    armature_editor_set_pose_index(win->parent, prev_frame);
    action_editor_set_frame_count(win,--data->frameCount);
    if (data->armature)
    {
        gf2d_action_list_frame_deleted(data->armature->actions,prev_frame + 1);
    }
}

void action_editor_set_armature(Window *win,Armature *armature)
{
    ActionEditorData* data;    
    if ((!win)||(!win->data))return;
    data = (ActionEditorData*)win->data;
    data->armature = armature;
    action_editor_set_frame_count(win,gf2d_armature_get_pose_count(armature));
}

void action_editor_set_frame_count(Window *win,Uint32 frameCount)
{
    ActionEditorData* data;    
    if ((!win)||(!win->data))return;
    data = (ActionEditorData*)win->data;
    data->frameCount = frameCount;
}

void action_editor_update_selected_action(Window *win)
{
    ActionEditorData* data;    
    if ((!win)||(!win->data))return;
    data = (ActionEditorData*)win->data;
    if (!data->armature)return;
    data->selectedAction = gf2d_action_list_get_action_by_frame(data->armature->actions,data->poseindex);

}

void action_editor_set_poseindex(Window *win,float poseindex)
{
    TextLine text;
    ActionEditorData* data;    
    if ((!win)||(!win->data))return;
    data = (ActionEditorData*)win->data;
    data->poseindex = poseindex;
    gfc_line_sprintf(text,"%3i/%3i",(int)poseindex,(data->frameCount - 1));
    gf2d_element_label_set_text(gf2d_window_get_element_by_name(win,"frame_number"),text);
    action_editor_update_selected_action(win);
}

void action_editor_set_figure(Window *win,Figure *figure)
{
    ActionEditorData* data;    
    if ((!win)||(!win->data))return;
    data = (ActionEditorData*)win->data;
    data->figure = figure;
    action_editor_update_selected_action(win);
}

int action_editor_update(Window *win,List *updateList)
{
    TextLine entryText;
    Vector2D delta;
    int i,count;
    int frame;
    float poseindex;
    Vector2D begin,end;
    Element *e;
    ActionEditorData* data;
    if ((!win)||(!win->data))return 0;
    if (!updateList)return 0;
    data = (ActionEditorData*)win->data;
    
    if ((data->action == AEA_Play)&&(data->selectedAction))
    {
        poseindex = armature_editor_get_pose_index(win->parent);
        if (gf2d_action_next_frame(data->selectedAction,&poseindex) == ART_END)
        {
            data->action = AEA_None;
        }
        armature_editor_set_pose_index(win->parent,poseindex);
        armature_editor_set_pose_index_next(win->parent, gf2d_action_next_frame_after(data->selectedAction,poseindex));
    }
    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;
        if (strcmp(e->name,"center_timeline") == 0)
        {
            begin = action_editor_get_timeline_beginning(win);
            end = action_editor_get_timeline_end(win);
            data->timelineOffset = -action_editor_get_timeline_frame_position(win,data->poseindex);
            data->timelineOffset += (end.x - begin.x)*0.5;
            continue;
        }
        if (strcmp(e->name,"play_action") == 0)
        {
            if (data->action == AEA_Play)
            {
                //stopping
                data->action = AEA_None;
                poseindex = (Uint32)armature_editor_get_pose_index(win->parent);//snap to integer
                armature_editor_set_pose_index(win->parent,poseindex);
            }
            else if (data->action == AEA_None)
            {
                if (!data->selectedAction)break;//can't play nothing
                armature_editor_set_pose_index(win->parent,data->selectedAction->startFrame);
                data->action = AEA_Play;
            }
        }
        if (data->action != AEA_None)continue;
        if (strcmp(e->name,"next_frame") == 0)
        {
            armature_editor_next_pose(win->parent);
            return 1;
        }
        if (strcmp(e->name,"prev_frame") == 0)
        {
            armature_editor_prev_pose(win->parent);
            return 1;
        }
        if (strcmp(e->name,"add_frame") == 0)
        {
            if (!data->armature)break;
            window_yes_no("add new frame?", onAddNewFrame,NULL,win);
            return 1;
        }
        if (strcmp(e->name,"add_action") == 0)
        {
            if (!data->armature)break;
            if (!win->child)
            {
                win->child = action_info(win,gf2d_action_new(),1);
            }
            return 1;
        }
        if (strcmp(e->name,"edit_action") == 0)
        {
            if (!data->armature)break;
            if (!data->selectedAction)break;
            if (!win->child)
            {
                win->child = action_info(win,data->selectedAction,0);
            }
            return 1;
        }
        if (strcmp(e->name,"delete_action") == 0)
        {
            if (!data->armature)break;
            if (!data->selectedAction)break;
            gfc_line_sprintf(entryText,"Delete Action %s?",data->selectedAction->name);
            window_yes_no(entryText, onDeleteAction,NULL,win);
            return 1;
        }
        if (strcmp(e->name,"delete_frame") == 0)
        {
            if (!data->armature)break;
            window_yes_no("delete current frame?", onDeleteFrame,NULL,win);
            return 1;
        }
        if (strcmp(e->name,"dup_frame") == 0)
        {
            if (!data->armature)break;
            window_yes_no("duplicate current frame?", onDuplicateFrame,NULL,win);
            return 1;
        }
        if (strcmp(e->name,"insert_dup_frame") == 0)
        {
            if (!data->armature)break;
            window_yes_no("Insert duplicate frame?", onInsertDuplicateFrame,NULL,win);
            return 1;
        }
        if (strcmp(e->name,"insert_frame") == 0)
        {
            if (!data->armature)break;
            window_yes_no("insert a new frame?", onInsertFrame,NULL,win);
            return 1;
        }
    }
    if (gf2d_window_mouse_in(win))
    {
        if ((gf2d_mouse_button_released(1))&&(data->action == AEA_Pan))
        {
            data->action = AEA_None;
        }
        else if((gf2d_mouse_button_held(1))&&(data->action == AEA_Pan))
        {// middle click drag
            vector2d_sub(delta,data->actionStart,gf2d_mouse_get_position());
            data->timelineOffset = data->panStart.x - delta.x;
        }
        else if ((data->action == AEA_None)&&(gf2d_window_mouse_in(win))&&(gf2d_mouse_button_pressed(1)))
        {// middle click
            data->action = AEA_Pan;
            data->actionStart = gf2d_mouse_get_position();
            data->panStart = vector2d(data->timelineOffset,0);
        }
        if ((data->action == AEA_None)&&(gf2d_window_mouse_in(win))&&(gf2d_mouse_button_pressed(2)))
        {// right click
            frame = action_editor_timeline_mouse_over(win);
            if (frame != -1)
            {
                armature_editor_set_pose_index(win->parent, frame);
            }
        }
    }
    return gf2d_window_mouse_in(win);
}

Window *action_editor(Window *parent)
{
    Window *win;
    ActionEditorData* data;
    win = gf2d_window_load("menus/action_editor.json");
    if (!win)
    {
        slog("failed to load armature inspector");
        return NULL;
    }
    win->update = action_editor_update;
    win->free_data = action_editor_free;
    win->draw = action_editor_draw;
    data = (ActionEditorData*)gfc_allocate_array(sizeof(ActionEditorData),1);
    win->data = data;
    data->win = win;
    win->parent = parent;
    return win;
}
/*eol@eof*/

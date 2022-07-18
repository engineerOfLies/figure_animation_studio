#include "simple_logger.h"
#include "simple_json.h"

#include "gfc_text.h"
#include "gfc_input.h"

#include "gf2d_graphics.h"
#include "gf2d_windows.h"
#include "gf2d_elements.h"
#include "gf2d_element_label.h"
#include "gf2d_draw.h"
#include "gf2d_font.h"
#include "gf2d_mouse.h"
#include "gf2d_actor.h"
#include "gf2d_armature.h"
#include "gf2d_figure.h"
#include "gf2d_windows_common.h"

#include "armature_editor.h"
#include "armature_ops.h"
#include "armature_inspector.h"
#include "action_editor.h"

typedef enum
{
    EM_Armature,
    EM_Pose,
    EM_Figure,
    EM_MAXMODES
}AE_EditorModes;

typedef enum
{
    EA_None = 0,
    EA_Grab,
    EA_Move,
    EA_Add,
    EA_Rotate,
    EA_Scale,
    EA_Clear,
    EA_Delete,
    EA_Pan,
    EA_ChildWindow,
    EA_MAXACTIONS
}AE_EditorActions;

typedef struct
{
    TextLine          armatureFile;
    Armature         *armature;
    Vector2D          drawCenter;
    Vector2D          drawScale;
    Vector2D          lock;
    Pose             *poseCopy;
    float             poseindex;
    Bone             *selectedBone;
    Bone              backupBone;           /**<kept for undo*/
    BonePose         *selectedPoseBone;
    BonePose          backupPoseBone;
    FigureInstance    figureInstance;
    FigureLink       *link;
    FigureLink        backupLink;
    TextLine          entryText;
    Actor            *background;
    Action           *backgroundAction;
    float             backgroundFrame;
    Uint8             ready;
    Uint8             mode;
    AE_EditorActions  action;
    Vector2D          scaleStart;//keep track of starting position
    Vector2D          actionStart;
    float             angleStart;
    Vector2D          panStart;
    Window           *subwindow;
    Window           *toolWindow;
    Window           *footerWindow;
    Window           *win;
}ArmatureEditorData;

void armature_editor_save_frame(ArmatureEditorData *data,Vector2D extent,const char *filename)
{//render the current frame to texture, then save that texture to file
    SDL_Texture *texture;
    Vector2D center;
    if ((!data)||(!filename))return;
    if ((!extent.x)||(!extent.y))return;
    texture = SDL_CreateTexture(gf2d_graphics_get_renderer(),
                                gf2d_graphics_get_image_format(),
                                SDL_TEXTUREACCESS_TARGET,
                                extent.x,
                                extent.y);
    if (!texture)
    {
        slog("failed to create render target texture for screenshot");
        return;
    }
    vector2d_scale(center,extent,0.5);
    SDL_SetRenderTarget(gf2d_graphics_get_renderer(),texture);
    SDL_SetRenderDrawColor(
        gf2d_graphics_get_renderer(), 
        0,0, 0, 0);

    SDL_RenderClear(gf2d_graphics_get_renderer());
    gf2d_figure_instance_draw(
        &data->figureInstance,
        center,
        NULL,
        0,
        NULL,
        1);
    gf2d_graphics_save_screenshot(filename);
    SDL_SetRenderTarget(gf2d_graphics_get_renderer(),NULL);
    SDL_DestroyTexture(texture);
}

void armature_editor_subwindow_closed(Window *win)
{
    ArmatureEditorData *data;
    if (!win)return;
    if (!win->data)return;
    data = win->data;
    data->action = EA_None;
    data->subwindow = NULL;
}

void armature_editor_set_mode(Window *win,ArmatureEditorData *data,int mode)
{
    if ((!win)||(!data))return;
    if (mode < 0)mode = EM_MAXMODES - 1;
    if (mode >= EM_MAXMODES)mode = 0;
    
    data->mode = mode;
}

int armature_editor_free(Window *win)
{
    ArmatureEditorData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    data = win->data;
    if (data->poseCopy)gf2d_armature_pose_free(data->poseCopy);
    if (data->subwindow)gf2d_window_free(data->subwindow);
    if (data->toolWindow)gf2d_window_free(data->toolWindow);    
    free(data);
    return 0;
}

void onPastePose(void *Data)
{
    Pose *dst;
    Window *win;
    ArmatureEditorData *data;
    if (!Data)return;
    win = Data;
    if (!win->data)return;
    data = win->data;
    if (!data->poseCopy)return;// nothing to do
    dst = gf2d_armature_pose_get_by_index(data->armature,data->figureInstance.frame);
    if (!dst)return; // something is wrong
    gf2d_armature_pose_copy(dst,data->poseCopy);
}

void onCopyPose(void *Data)
{
    Pose *src;
    Window *win;
    ArmatureEditorData *data;
    if (!Data)return;
    win = Data;
    if (!win->data)return;
    data = win->data;

    if (!data->armature)return;
    if (data->poseCopy != NULL)
    {
        gf2d_armature_pose_free(data->poseCopy);
        data->poseCopy = NULL;
    }
    src = gf2d_armature_pose_get_by_index(data->armature,data->figureInstance.frame);
    if (!src)return;
    data->poseCopy = gf2d_armature_pose_create_for(data->armature);
    gf2d_armature_pose_copy(data->poseCopy,src);
}

void onCancelAction(void *Data)
{
    Window *win;
    ArmatureEditorData *data;
    if (!Data)return;
    win = Data;
    if (!win->data)return;
    data = win->data;
    data->action = EA_None;
}

void onBackgroundOk(void *Data)
{
    Window *win;
    ArmatureEditorData *data;
    if (!Data)return;
    win = Data;
    if (!win->data)return;
    data = win->data;
    if (data->background)
    {
        gf2d_actor_free(data->background);
    }
    data->background = gf2d_actor_load(data->entryText);
    data->backgroundAction = gf2d_actor_get_action_by_index(data->background,0);
    data->backgroundFrame = 0;
}

void onBackgroundScale(void *Data)
{
    float scale;
    Window *win;
    ArmatureEditorData *data;
    if (!Data)return;
    win = Data;
    if (!win->data)return;
    data = win->data;
    scale = atof(data->entryText);
    if (scale == 0)
    {
        window_alert("Error","Cannot Scale to zero",NULL,NULL);
        return;
    }
    armature_editor_set_draw_scale(win,vector2d(scale,scale));
}

int armature_editor_draw(Window *win)
{
    char *mode = NULL;
    ArmatureEditorData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    data = win->data;
    
    gf2d_draw_line(
        vector2d(data->drawCenter.x,data->drawCenter.y - (50 * data->drawScale.y)),
        vector2d(data->drawCenter.x,data->drawCenter.y + (50 * data->drawScale.y)),
        gfc_color8(128,128,128,255));

    gf2d_draw_line(
        vector2d(data->drawCenter.x - (50 * data->drawScale.x),data->drawCenter.y),
        vector2d(data->drawCenter.x + (50 * data->drawScale.x),data->drawCenter.y),
        gfc_color8(128,128,128,255));

    if (data->background)
    {
        gf2d_actor_draw(
            data->background,
            data->backgroundFrame,
            data->drawCenter,
            &data->drawScale,
            NULL,
            NULL,
            NULL,
            NULL);

    }
    switch(data->mode)
    {
        case EM_Armature:
            gf2d_figure_instance_draw(
                &data->figureInstance,
                data->drawCenter,
                &data->drawScale,
                0,
                NULL,
                0);
            mode = "Armature Mode";
            gf2d_armature_draw_bones(data->armature,data->drawCenter,data->drawScale,0, gfc_color_hsl(30,0.5,0.5,255));
            if (data->selectedBone)
            {
                gf2d_armature_draw_bone(
                    data->selectedBone,
                    data->drawCenter,
                    data->drawScale,
                    0,
                    gfc_color_hsl(30,1,0.95,255));
            }
            break;
        case EM_Pose:
            gf2d_figure_instance_draw(
                &data->figureInstance,
                data->drawCenter,
                &data->drawScale,
                0,
                NULL,1);
            mode = "Pose Mode";
            gf2d_armature_draw_pose(
                data->armature,
                data->poseindex,
                data->drawCenter,
                data->drawScale,
                0,
                gfc_color_hsl(150,0.5,0.45,255));
            if (data->selectedPoseBone)
            {
                gf2d_armature_draw_pose_bone(
                    gf2d_armature_get_bone_pose(data->armature,data->poseindex, data->selectedPoseBone->bone->index),
                    data->drawCenter,
                    data->drawScale,
                    0,
                    gfc_color_hsl(150,1,0.95,255));
            }
            break;
        case EM_Figure:
            mode = "Figure Mode";
            gf2d_armature_draw_bones(data->armature,data->drawCenter,data->drawScale,0, gfc_color_hsl(240,0.5,0.45,255));
            if (data->selectedBone)
            {
                gf2d_armature_draw_bone(
                    data->selectedBone,
                    data->drawCenter,
                    data->drawScale,
                    0,
                    gfc_color_hsl(240,1,0.95,255));
            }//in figure mode, draw the figure last
            gf2d_figure_instance_draw(
                &data->figureInstance,
                data->drawCenter,
                &data->drawScale,
                0,
                NULL,1);
            break;

    }
    gf2d_element_label_set_text(gf2d_window_get_element_by_id(win,2),mode);
        
    return 0;
}

Uint32 armature_editor_prev_pose(Window *win)
{
    ArmatureEditorData *data;
    if (!win)return 0;
    data = (ArmatureEditorData*)win->data;
    if (!data)return 0;
    if (!data->armature)return 0;
    if (data->poseindex == 0)
    {
        armature_editor_set_pose_index(win, gf2d_armature_get_pose_count(data->armature) - 1);
    }
    else
    {
        armature_editor_set_pose_index(win, --data->poseindex);
    }
    return data->poseindex;
}

Uint32 armature_editor_next_pose(Window *win)
{
    ArmatureEditorData *data;
    if (!win)return 0;
    data = (ArmatureEditorData*)win->data;
    if (!data)return 0;
    if (!data->armature)return 0;
    armature_editor_set_pose_index(win, ++data->poseindex);
    return data->poseindex;
}

float armature_editor_get_pose_index(Window *win)
{
    ArmatureEditorData *data;
    if ((!win)||(!win->data))return 0;
    data = (ArmatureEditorData*)win->data;
    return data->poseindex;
}

void armature_editor_set_pose_index_next(Window *win, Uint32 next)
{
    ArmatureEditorData *data;
    if (!win)return;
    data = (ArmatureEditorData*)win->data;
    if (!data)return;
    if (!data->armature)return;
    if (next >= gfc_list_get_count(data->armature->poses))next = gfc_list_get_count(data->armature->poses)- 1;
    data->figureInstance.nextFrame = next;
}

void armature_editor_set_pose_index(Window *win, float index)
{
    ArmatureEditorData *data;
    if (!win)return;
    data = (ArmatureEditorData*)win->data;
    if (!data)return;
    if (index >= gf2d_armature_get_pose_count(data->armature))
    {
        index = 0;
    }
    data->poseindex = index;
    data->figureInstance.frame = index;
    if ((data->selectedPoseBone)&&( data->selectedPoseBone->bone))
    {
        data->selectedPoseBone = gf2d_armature_get_bone_pose(
                                    data->armature,
                                    index,
                                    data->selectedPoseBone->bone->index);
    }
    armature_inspector_set_poseindex(data->toolWindow,index);
    armature_editor_set_pose_index_next(win, (Uint32) data->poseindex);
    action_editor_set_poseindex(data->footerWindow,index);
}

void armature_editor_select_bone(Window *win,Bone *bone)
{
    ArmatureEditorData *data;
    if (!win)return;
    data = (ArmatureEditorData*)win->data;
    if (!data)return;
    data->selectedBone = bone;
    if ((bone)&&(data->figureInstance.figure))
    {
        data->link = gf2d_figure_link_get_by_bonename(data->figureInstance.figure,bone->name);
    }
    armature_inspector_set_bone(data->toolWindow,bone);
}

void armature_editor_select_bonepose(Window *win,BonePose *posebone)
{
    ArmatureEditorData *data;
    if (!win)return;
    data = (ArmatureEditorData*)win->data;
    if (!data)return;
    data->selectedPoseBone = posebone;
    if (data->selectedPoseBone)armature_editor_select_bone(win,data->selectedPoseBone->bone);
    else armature_editor_select_bone(win,NULL);
}

void onBoneReset(void *Data)
{
    Window *win;
    ArmatureEditorData *data;
    if (!Data)return;
    win = Data;
    if (!win->data)return;
    data = win->data;
    if (!data->selectedBone)return;
    gf2d_armature_bone_rotate(data->selectedBone, data->selectedBone->rootPosition, -data->selectedBone->baseAngle);
    gf2d_armature_bone_move(
        data->selectedBone,
        vector2d(
            -data->selectedBone->rootPosition.x,
            -data->selectedBone->rootPosition.y));
    if (data->selectedBone->parent)
    {
        gf2d_armature_bone_move(
            data->selectedBone,
            vector2d(
                data->selectedBone->parent->rootPosition.x,
                data->selectedBone->parent->rootPosition.y));
    }
    data->action = EA_None;
}

void onBonePoseReset(void *Data)
{
    Window *win;
    Vector2D delta;
    ArmatureEditorData *data;
    if (!Data)return;
    win = Data;
    if (!win->data)return;
    data = win->data;
    if (!data->selectedPoseBone)return;
    gf2d_armature_bonepose_rotate(
        data->armature,
        data->selectedPoseBone,
        data->poseindex,
        gf2d_armature_get_pose_bone_position(data->selectedPoseBone),
        -data->selectedPoseBone->angle);
    vector2d_negate(delta,data->selectedPoseBone->position);
    gf2d_armature_bonepose_move(data->armature, data->selectedPoseBone,data->poseindex, delta);
    data->action = EA_None;
}

void onLinkReset(void *Data)
{
    Window *win;
    ArmatureEditorData *data;
    if (!Data)return;
    win = Data;
    if (!win->data)return;
    data = win->data;
    if (!data->link)return;
    data->link->rotation = 0;
    vector2d_clear(data->link->offset);
    data->action = EA_None;
}

void onBoneDelete(void *Data)
{
    FigureLinkInstance *linkInstance;
    Window *win;
    ArmatureEditorData *data;
    if (!Data)return;
    win = Data;
    if (!win->data)return;
    data = win->data;
    
    linkInstance = gf2d_figure_instance_get_link(&data->figureInstance, data->selectedBone->name);
    if (linkInstance)
    {
        gf2d_figure_instance_remove_link(&data->figureInstance,linkInstance);
    }
    gf2d_armature_delete_bone(data->armature, data->selectedBone);
    armature_editor_select_bone(win,NULL);
    data->action = EA_None;
}

void armature_editor_link_updated(Window *win,FigureLink *link)
{
    FigureLinkInstance *linkInstance;
    ArmatureEditorData *data;
    if ((!win)||(!win->data))return;
    data = win->data;
    if (!link)return;
    linkInstance = gf2d_figure_instance_get_link(&data->figureInstance, link->bonename);
    gf2d_figure_link_instance_update_link(linkInstance);
}

void armature_editor_figure_instance_add_link(Window *win,FigureLink *link)
{
    ArmatureEditorData *data;
    if ((!win)||(!win->data))return;
    data = win->data;
    if (!link)return;
    gf2d_figure_instance_add_link(&data->figureInstance, link);
}

void armature_editor_figure_instance_remove_link(Window *win,const char *bonename)
{
    FigureLinkInstance *link;
    ArmatureEditorData *data;
    if ((!win)||(!win->data))return;
    data = win->data;
    if (!bonename)return;
    link = gf2d_figure_instance_get_link(&data->figureInstance,bonename);
    if (!link)return;
    gf2d_figure_instance_remove_link(&data->figureInstance,link);
}


int armature_editor_update(Window *win,List *updateList)
{
    TextLine question;
    int i,count;
    Element *e;
    Bone *newbone;
    float deltaAngle,mouseAngle;
    Vector2D delta,mouse,bonePosition,poseBonePosition;
    ArmatureEditorData *data;
    if (!win)return 0;
    data = (ArmatureEditorData*)win->data;
    if (!data)return 0;
    if (!updateList)return 0;
    if (data->subwindow != NULL)return 0;
    
    if (data->action == EA_None)
    {
        vector2d_clear(data->lock);
    }

    mouse = gf2d_mouse_get_position();
    
    if (gfc_input_mouse_wheel_up())//scale down/ zoom out
    {
        if (data->drawScale.x > .1)data->drawScale.x = data->drawScale.y = data->drawScale.x - 0.1;
        armature_editor_set_draw_scale(win,data->drawScale);
    }
    else if(gfc_input_mouse_wheel_down())//scale up/ zoom in
    {
        data->drawScale.x = data->drawScale.y = data->drawScale.x + 0.1;
        armature_editor_set_draw_scale(win,data->drawScale);
    }
    
    if ((gf2d_mouse_button_released(1))&&(data->action == EA_Pan))
    {
        data->action = EA_None;
    }
    if((gf2d_mouse_button_held(1))&&(data->action == EA_Pan))
    {
        vector2d_sub(delta,data->actionStart,gf2d_mouse_get_position());
        vector2d_sub(data->drawCenter,data->panStart,delta);
    }
    if ((gf2d_mouse_button_pressed(1))&&(data->action == EA_None))
    {
        data->action = EA_Pan;
        data->actionStart = gf2d_mouse_get_position();
        data->panStart = data->drawCenter;
    }


    switch(data->mode)
    {
        case EM_Figure:
            switch(data->action)
            {
                case EA_Move:
                    vector2d_sub(delta,mouse,data->actionStart);
                    delta.x /= data->drawScale.x;
                    delta.y /= data->drawScale.y;
                    if (data->lock.x)delta.y = 0;
                    if (data->lock.y)delta.x = 0;
                    vector2d_add(data->link->offset,data->link->offset,delta);
                    vector2d_copy(data->actionStart,mouse);
                    if (gf2d_mouse_button_pressed(0))
                    {
                        //make it real
                        data->action = EA_None;
                        return 1;
                    }
                    else if (gf2d_mouse_button_pressed(2))
                    {
                        memcpy(data->link,&data->backupLink,sizeof(FigureLink));
                        data->action = EA_None;
                        return 1;
                    }
                    break;
                case EA_Rotate:
                    vector2d_add(bonePosition,data->drawCenter,data->selectedBone->rootPosition);
                    deltaAngle = gf2d_mouse_get_angle_to(bonePosition) * GFC_DEGTORAD;
                    deltaAngle -= data->angleStart;
                    data->link->rotation += deltaAngle;
                    data->angleStart = gf2d_mouse_get_angle_to(bonePosition) * GFC_DEGTORAD;

                    if (gf2d_mouse_button_pressed(0))
                    {
                        //make it real
                        data->action = EA_None;
                        return 1;
                    }
                    else if (gf2d_mouse_button_pressed(2))
                    {
                        data->action = EA_None;
                        memcpy(data->link,&data->backupLink,sizeof(FigureLink));
                        return 1;
                    }
                    break;
                default:
                    if (gf2d_mouse_button_pressed(2))
                    {
                        vector2d_sub(delta,mouse,data->drawCenter);
                        armature_editor_select_bone(
                            win,
                            gf2d_armature_get_bone_by_position(data->armature,delta,data->drawScale,data->selectedBone));
                        return 1;
                    }
            }
            break;
        case EM_Pose:
            switch(data->action)
            {
                case EA_Move:
                    vector2d_sub(delta,mouse,data->actionStart);
                    delta.x /= data->drawScale.x;
                    delta.y /= data->drawScale.y;
                    if (data->lock.x)delta.y = 0;
                    if (data->lock.y)delta.x = 0;
                    gf2d_armature_bonepose_move(
                        data->armature, 
                        data->selectedPoseBone,
                        data->poseindex,
                        delta);
                    vector2d_copy(data->actionStart,mouse);
                    if (gf2d_mouse_button_pressed(0))
                    {
                        //make it real
                        data->action = EA_None;
                        return 1;
                    }
                    else if (gf2d_mouse_button_pressed(2))
                    {
                        vector2d_sub(delta,data->backupPoseBone.position,data->selectedPoseBone->position);
                        data->action = EA_None;
                        gf2d_armature_bonepose_move(
                            data->armature, 
                            data->selectedPoseBone,
                            data->poseindex,
                            delta);
                        return 1;
                    }
                    break;
                case EA_Grab:
                case EA_Rotate:
                    poseBonePosition = gf2d_armature_get_pose_bone_draw_position(data->selectedPoseBone,data->drawScale,0);
                    vector2d_add(bonePosition,poseBonePosition,data->drawCenter);

                    mouseAngle = gf2d_mouse_get_angle_to(bonePosition) * GFC_DEGTORAD;                     
                    deltaAngle = mouseAngle - data->angleStart;
                    
                    
                    gf2d_armature_bonepose_rotate(
                        data->armature,
                        data->selectedPoseBone,
                        data->poseindex,
                        gf2d_armature_get_pose_bone_position(data->selectedPoseBone),
                        deltaAngle);
                    
                    data->angleStart = mouseAngle;
                        
                    
                    if (gf2d_mouse_button_pressed(0))
                    {
                        //make it real
                        data->action = EA_None;
                        return 1;
                    }
                    else if (gf2d_mouse_button_pressed(2))
                    {
                        data->action = EA_None;
                        gf2d_armature_bonepose_rotate(
                            data->armature,
                            data->selectedPoseBone,
                            data->poseindex,
                            gf2d_armature_get_pose_bone_position(data->selectedPoseBone),
                            data->backupPoseBone.angle - data->selectedPoseBone->angle);
                        return 1;
                    }

                default:
                    if (gf2d_mouse_button_pressed(2))
                    {
                        vector2d_sub(delta,mouse,data->drawCenter);
                        armature_editor_select_bonepose(
                            win,
                            gf2d_armature_get_bonepose_by_position(
                                data->armature,
                                data->poseindex,
                                delta,
                                data->drawScale,
                                0,
                                data->selectedPoseBone));
                        return 1;
                    }
                    break;
            }
            break;
        case EM_Armature:
            switch(data->action)
            {
                case EA_Grab:
                    vector2d_sub(delta,mouse,data->actionStart);
                    delta.x /= data->drawScale.x;
                    delta.y /= data->drawScale.y;
                    gf2d_armature_bone_tip_move(data->selectedBone,delta);
                    vector2d_copy(data->actionStart,mouse);
                    if (gf2d_mouse_button_pressed(0))
                    {
                        //make it real
                        data->action = EA_None;
                        data->lock.x = data->lock.y = 0;
                        return 1;
                    }
                    else if (gf2d_mouse_button_pressed(2))
                    {
                        vector2d_sub(delta,gf2d_armature_get_bone_tip(&data->backupBone),gf2d_armature_get_bone_tip(data->selectedBone));
                        data->action = EA_None;
                        data->lock.x = data->lock.y = 0;
                        gf2d_armature_bone_tip_move(data->selectedBone,delta);
                        return 1;
                    }
                    break;
                case EA_Move:
                    vector2d_sub(delta,mouse,data->actionStart);
                    delta.x /= data->drawScale.x;
                    delta.y /= data->drawScale.y;
                    if (data->lock.x)delta.y = 0;
                    if (data->lock.y)delta.x = 0;
                    gf2d_armature_bone_move(data->selectedBone,delta);
                    vector2d_copy(data->actionStart,mouse);
                    if (gf2d_mouse_button_pressed(0))
                    {
                        //make it real
                        data->action = EA_None;
                        data->lock.x = data->lock.y = 0;
                        return 1;
                    }
                    else if (gf2d_mouse_button_pressed(2))
                    {
                        vector2d_sub(delta,data->backupBone.rootPosition,data->selectedBone->rootPosition);
                        data->action = EA_None;
                        data->lock.x = data->lock.y = 0;
                        gf2d_armature_bone_move(data->selectedBone,delta);
                        return 1;
                    }
                    break;
                case EA_Rotate:
                    vector2d_add(bonePosition,data->drawCenter,data->selectedBone->rootPosition);
                    deltaAngle = gf2d_mouse_get_angle_to(bonePosition) * GFC_DEGTORAD;
                    deltaAngle -= data->angleStart;
                    gf2d_armature_bone_rotate(data->selectedBone, data->selectedBone->rootPosition, deltaAngle);
                    data->angleStart = gf2d_mouse_get_angle_to(bonePosition) * GFC_DEGTORAD;

                    if (gf2d_mouse_button_pressed(0))
                    {
                        //make it real
                        data->action = EA_None;
                        return 1;
                    }
                    else if (gf2d_mouse_button_pressed(2))
                    {
                        data->action = EA_None;
                        gf2d_armature_bone_rotate(
                            data->selectedBone,
                            data->selectedBone->rootPosition,
                            data->backupBone.baseAngle - data->selectedBone->baseAngle);
                        return 1;
                    }
                    break;
                default:
                    if (gf2d_mouse_button_pressed(2))
                    {
                        vector2d_sub(delta,mouse,data->drawCenter);
                        armature_editor_select_bone(win,gf2d_armature_get_bone_by_position(data->armature,delta,data->drawScale,data->selectedBone));
                        data->lock.x = data->lock.y = 0;
                        return 1;
                    }
            }
            break;
    }
    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;
        if (strcmp(e->name,"delete")==0)
        {
            if (data->action != EA_None)continue;// can't switch modes while doing something
            if (data->mode == EM_Armature)
            {
                if (data->selectedBone)
                {
                    gfc_line_sprintf(question,"Delete Bone '%s'",data->selectedBone->name);
                    data->action = EA_Delete;
                    window_yes_no(question, onBoneDelete,onCancelAction,win);
                }
            }
            return 1;
        }
        if (strcmp(e->name,"flip")==0)
        {
            data->drawScale.x *= -1;
            return 1;
        }
        if (strcmp(e->name,"save_frame")==0)
        {
            armature_editor_save_frame(data,vector2d(512,512),"screenshots/figure.png");
            return 1;
        }
        if (strcmp(e->name,"lock_x")==0)
        {
            if ((data->action == EA_Move)||(data->action == EA_Grab))
            {
                data->lock.x = !(data->lock.x);
                return 1;
            }

        }
        if (strcmp(e->name,"lock_y")==0)
        {
            if ((data->action == EA_Move)||(data->action == EA_Grab))
            {
                data->lock.y = !(data->lock.y);
                return 1;
            }
        }
        if (strcmp(e->name,"mode_next")==0)
        {
            if (data->action != EA_None)continue;// can't switch modes while doing something
            armature_editor_set_mode(win,data,++data->mode);
            return 1;
        }
        if (strcmp(e->name,"move")==0)
        {
            if (data->action != EA_None)return 0;
            if (data->mode == EM_Armature)
            {
                if (!data->selectedBone)continue;
                memcpy(&data->backupBone,data->selectedBone,sizeof(Bone));//in case of backup
            }
            else if (data->mode == EM_Pose)
            {
                if (!data->selectedPoseBone)continue;
                memcpy(&data->backupPoseBone,data->selectedPoseBone,sizeof(BonePose));//in case of backup
            }
            else if (data->mode == EM_Figure)
            {
                if (!data->link)continue;
                memcpy(&data->backupLink,data->link,sizeof(FigureLink));//in case of backup
            }
            vector2d_copy(data->actionStart,mouse);
            data->action = EA_Move;
            return 1;
        }
        else if (strcmp(e->name,"file")==0)
        {
            if (data->subwindow == NULL)
            {
                data->action = EA_ChildWindow;
                data->subwindow = armature_ops(win);
            }
            return 1;
        }
        else if (strcmp(e->name,"background")==0)
        {
            if (data->background)
            {
                gfc_line_cpy(data->entryText,data->background->filename);
            }
            else gfc_line_cpy(data->entryText,"actors/");
            window_text_entry("Load Background Actor", data->entryText, win, GFCLINELEN, onBackgroundOk,NULL);
            return 1;
        }
        else if (strcmp(e->name,"scaleback")==0)
        {
            gfc_line_sprintf(data->entryText,"%.3f",data->drawScale.x);
            window_text_entry("Set Background draw scale", data->entryText, win, GFCLINELEN, onBackgroundScale,NULL);
            return 1;
        }
        else if (strcmp(e->name,"grab")==0)
        {
            if (data->action != EA_None)continue;
            if (data->mode == EM_Figure)continue;//grab is not supported by figure
            if (data->mode == EM_Armature)
            {
                if (!data->selectedBone)continue;
                memcpy(&data->backupBone,data->selectedBone,sizeof(Bone));//in case of backup
            }
            else if (data->mode == EM_Pose)
            {
                if (!data->selectedPoseBone)continue;
                memcpy(&data->backupPoseBone,data->selectedPoseBone,sizeof(BonePose));//in case of backup
                //take into account scale
                bonePosition = gf2d_armature_get_pose_bone_draw_position(data->selectedPoseBone,data->drawScale,0);
                vector2d_add(bonePosition,bonePosition,data->drawCenter);
                data->angleStart = gf2d_mouse_get_angle_to(bonePosition) * GFC_DEGTORAD;
                data->action = EA_Rotate;
                return 1;//grab is rotate for pose mode
            }
            vector2d_copy(data->actionStart,mouse);
            data->action = EA_Grab;
            return 1;
        }
        else if (strcmp(e->name,"clear_transform") == 0)
        {
            if (data->action != EA_None)continue;
            if (data->mode == EM_Armature)
            {
                if (!data->selectedBone)continue;
                data->action = EA_Clear;
                gfc_line_sprintf(question,"reset bone %s",data->selectedBone->name);
                window_yes_no(question, onBoneReset,onCancelAction,win);
            }
            if (data->mode == EM_Pose)
            {
                if (!data->selectedPoseBone)continue;
                data->action = EA_Clear;
                gfc_line_sprintf(question,"reset bone pose %s",data->selectedPoseBone->bone->name);
                window_yes_no(question, onBonePoseReset,onCancelAction,win);
            }
            if (data->mode == EM_Figure)
            {
                if (!data->link)continue;
                data->action = EA_Clear;
                gfc_line_sprintf(question,"reset bone link %s",data->link->bonename);
                window_yes_no(question, onLinkReset,onCancelAction,win);
            }
            return 1;
        }
        else if (strcmp(e->name,"duplicate") == 0)
        {
            if (data->action != EA_None)continue;
            if (data->mode == EM_Armature)
            {
                if (!data->armature)continue;
                if (!data->selectedBone)continue;
                newbone = gf2d_armature_duplicate_bone(data->armature, data->selectedBone);
                gf2d_armature_add_bone_to_parent(data->selectedBone->parent,newbone);
                data->selectedBone = newbone;
                memcpy(&data->backupBone,data->selectedBone,sizeof(Bone));//in case of backup
                vector2d_copy(data->actionStart,mouse);
                data->action = EA_Move;
                return 1;
            }
        }
        else if (strcmp(e->name,"zoom_reset")==0)
        {
            if (data->action != EA_None)continue;
            data->drawScale = vector2d(1,1);
            data->drawCenter = gf2d_graphics_get_resolution();
            vector2d_scale(data->drawCenter,data->drawCenter,0.5);
            if (data->toolWindow)
            {
                data->drawCenter.x -= data->toolWindow->dimensions.w / 2;
            }
        }
        else if (strcmp(e->name,"copy")==0)
        {
            if (data->action != EA_None)continue;
            if (data->mode == EM_Pose)
            {
                onCopyPose(win);
            }
        }
        else if (strcmp(e->name,"paste")==0)
        {
            if (data->action != EA_None)continue;
            if (data->mode == EM_Pose)
            {
                onPastePose(win);
            }
        }
        else if (strcmp(e->name,"add")==0)
        {
            if (!data->armature)continue;
            if (data->mode != EM_Armature)continue;//only able to add new bones in armature mode
            if (data->action != EA_None)continue;
            if (!data->selectedBone)continue;
            data->selectedBone = gf2d_armature_add_bone(data->armature,data->selectedBone);
            armature_inspector_set_bone(data->toolWindow,data->selectedBone);
            memcpy(&data->backupBone,data->selectedBone,sizeof(Bone));//in case of backup
            vector2d_copy(data->actionStart,mouse);
            data->action = EA_Grab;
            return 1;
        }
        else if (strcmp(e->name,"rotate")==0)
        {
            if (data->action != EA_None)continue;
            if (data->mode == EM_Armature)
            {
                if (!data->selectedBone)continue;
                memcpy(&data->backupBone,data->selectedBone,sizeof(Bone));//in case of backup
                vector2d_add(bonePosition,data->drawCenter,data->selectedBone->rootPosition);
            }
            else if (data->mode == EM_Pose)
            {
                if (!data->selectedPoseBone)continue;
                memcpy(&data->backupPoseBone,data->selectedPoseBone,sizeof(BonePose));//in case of backup
                //take into account scale
                bonePosition = gf2d_armature_get_pose_bone_draw_position(data->selectedPoseBone,data->drawScale,0);
                vector2d_add(bonePosition,bonePosition,data->drawCenter);
            }
            else if (data->mode == EM_Figure)
            {
                if (!data->link)continue;
                memcpy(&data->backupLink,data->link,sizeof(FigureLink));//in case of backup
                vector2d_add(bonePosition,data->drawCenter,data->selectedBone->rootPosition);
            }

            data->angleStart = gf2d_mouse_get_angle_to(bonePosition) * GFC_DEGTORAD;
            data->action = EA_Rotate;
            return 1;
        }
    }
    return 0;
}

void armature_editor_set_draw_scale(Window *win,Vector2D scale)
{
    TextLine text;
    ArmatureEditorData* data;
    if (!win)return;
    data = win->data;
    vector2d_copy(data->drawScale,scale);
    gfc_line_sprintf(text,"Draw Scale: %.2f",data->drawScale.x);
    gf2d_element_label_set_text(gf2d_window_get_element_by_id(win,611),text);
}

Armature *armature_editor_get_armature(Window *win)
{
    ArmatureEditorData* data;
    if (!win)return NULL;
    data = win->data;
    return data->armature;
}

Figure *armature_editor_get_figure(Window *win)
{
    ArmatureEditorData* data;
    if (!win)return NULL;
    data = win->data;
    return data->figureInstance.figure;
}

void armature_editor_set_figure(Window *win,Figure *figure)
{
    ArmatureEditorData* data;
    if (!win)return;
    data = win->data;
    if (!data)return;
    if (data->figureInstance.figure)
    {
        //cleanup old
        gf2d_figure_instance_free(&data->figureInstance);
    }
    gf2d_figure_instance_new(&data->figureInstance);
    data->figureInstance.figure = figure;
    if (data->armature)
    {
        if (data->figureInstance.figure->armature == NULL)
        {
            data->figureInstance.figure->armature = data->armature;
            gf2d_figure_instance_link(&data->figureInstance);
        }
        else
        {
            armature_editor_set_armature(win,data->figureInstance.figure->armature);
        }
    }
    else if (data->figureInstance.figure)
    {
        armature_editor_set_armature(win,data->figureInstance.figure->armature);
        gf2d_figure_instance_link(&data->figureInstance);
    }
    else if (data->figureInstance.figure == NULL)
    {
        action_editor_set_figure(data->footerWindow,NULL);
    }
    armature_inspector_set_figure_instance(data->toolWindow,&data->figureInstance);
    action_editor_set_figure(data->footerWindow,data->figureInstance.figure);
}

void armature_editor_set_armature(Window *win,Armature *armature)
{
    ArmatureEditorData* data;
    if (!win)return;
    data = win->data;
    if (!data)return;
    if (data->armature)
    {
        gf2d_armature_free(data->armature);
    }
    data->armature = armature;
    if (data->figureInstance.figure)
    {
        data->figureInstance.figure->armature = armature;
    }
    armature_inspector_set_armature(data->toolWindow,armature);
    action_editor_set_armature(data->footerWindow,data->armature);
    armature_editor_set_pose_index(win, 0);
}

Bone *armature_editor_get_selected_bone(Window *win)
{
    ArmatureEditorData* data;
    if (!win)return NULL;
    data = win->data;
    if (!data)return NULL;
    return data->selectedBone;
}

Window *armature_editor()
{
    Window *win;
    ArmatureEditorData* data;
    win = gf2d_window_load("menus/armature_editor.json");
    if (!win)
    {
        slog("failed to load editor menu");
        return NULL;
    }
    win->update = armature_editor_update;
    win->free_data = armature_editor_free;
    win->draw = armature_editor_draw;
    data = (ArmatureEditorData*)gfc_allocate_array(sizeof(ArmatureEditorData),1);
    gfc_line_cpy(data->armatureFile,"armatures/");
    data->mode = EM_Armature;
    data->action = EA_None;
    win->data = data;
    data->win = win;
    armature_editor_set_draw_scale(win,vector2d(1,1));
    data->toolWindow = armature_inspector(win);
    data->footerWindow = action_editor(win);
    data->drawCenter = gf2d_graphics_get_resolution();
    vector2d_scale(data->drawCenter,data->drawCenter,0.5);
    if (data->toolWindow)
    {
        data->drawCenter.x -= data->toolWindow->dimensions.w / 2;
    }
    return win;
}

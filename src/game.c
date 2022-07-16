#include <SDL.h>
#include <stdio.h>

#include <simple_logger.h>
#include "gfc_input.h"

#include "gf2d_graphics.h"
#include "gf2d_sprite.h"
#include "gf2d_windows.h"
#include "gf2d_entity.h"
#include "gf2d_font.h"
#include "gf2d_mouse.h"
#include "gf2d_draw.h"
#include "gf2d_armature.h"
#include "gf2d_figure.h"
#include "gf2d_lighting.h"
#include "gf2d_camera.h"

#include "windows_common.h"
#include "armature_editor.h"

static int _done = 0;
static int _begin = 0;
static int _paused = 0;
static Window *_quit = NULL;
extern int __DebugMode;


void init_all(int argc, char *argv[]);

void onCancel(void *data)
{
    _quit = NULL;
}

void beginGame()
{
    _begin = 1;
}

void onExit(void *data)
{
    _begin = 0;
    _done = 1;
    _quit = NULL;
}

void exitGame()
{
    _done = 1;
}

void exitCheck()
{
    if (_quit)return;
    _quit = window_yes_no("Exit?",onExit,onCancel,NULL);
}

int main(int argc, char * argv[])
{
    /*variable declarations*/
    int windowsUpdated = 0;
        
    init_all(argc,argv);
        
    armature_editor();
    
    /*main game loop*/
    while(!_done)
    {
        gfc_input_update();
        gf2d_mouse_update();
        /*update things here*/
        windowsUpdated = gf2d_windows_update_all();
                                
        gf2d_graphics_clear_screen();
        
        //UI elements last
        gf2d_windows_draw_all();
        
        gf2d_mouse_draw();
        

        gf2d_grahics_next_frame();// render current draw frame and skip to the next frame
        
        if ((gfc_input_command_down("exit"))&&(_quit == NULL))
        {
            exitCheck();
        }
        if (fpsMode)slog("Rendering at %f FPS",gf2d_graphics_get_frames_per_second());
    }
    slog("---==== END ====---");
    return 0;
}

void init_all(int argc, char *argv[])
{
    int i;
    int fullscreen = 0;
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i],"--fullscreen") == 0)
        {
            fullscreen = 1;
        }
        else if (strcmp(argv[i],"--fps") == 0)
        {
            fpsMode = 1;
        }
        else if (strcmp(argv[i],"--debug") == 0)
        {
            __DebugMode = 1;
        }
    }
    /*program initializtion*/
    init_logger("glyph.log");
    slog("---==== BEGIN ====---");
    gf2d_graphics_initialize(
        "Glyph's Descent",
        1280,
        960,
        1280,
        960,
        vector4d(0,0,0,255),
        fullscreen);
    gf2d_graphics_set_frame_delay(16);
    gfc_audio_init(256,16,4,1,1,1);
    gf2d_sprite_init(1024);
    gf2d_actor_init(128);
    gf2d_armature_init(1024);
    gf2d_font_init("config/font.cfg");
    gfc_input_init("config/input.cfg");
    gf2d_windows_init(128,"config/windows.cfg");
    gf2d_figure_init(1024);
    gf2d_mouse_load("actors/mouse.json");

    gf2d_camera_set_dimensions(0,0,1280,960);
    
    SDL_ShowCursor(SDL_DISABLE);
}
/*eol@eof*/

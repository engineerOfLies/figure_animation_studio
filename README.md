# Glyph's Descent
Working off of gameframework2d, this will be a 2d platforming adventure game.  The game will follow a young Goblin
as he explores the ruins of a once magical civilization.

# debug mode
Start the game with the argument '--debug' to display debug information for the collision system
Start the game with the argument '--fps' to get the frame rate written to standard out and log file

# editor mode
Start the game with the argument '--editor' to run the level editor
press F1 to call up the file options menu
arrow keys pan the view
Tab swaps between editing modes
## tile mode
Left Mouse Button adds the current tile
Right Mouse Button deletes tiles

## entity mode

Right Mouse Button Selects an entity
'a' deselects all
'g' will grab the entity for moving
'Delete' will delete a selected entity

# animation studio
start the game with the argument '--fas' to run the animation editor

it is a full featured 2D armature based animation tool.  I can save articulated figures for the game or export to sprite sheet or sequence.

# Build Process

Before you can build the example code we are providing for you, you will need to obtain the libraries required
by the source code
 - SDL2
 - SDL2_image
 - SDL2_mixer
 - SDL2_ttf
There are additional sub modules that are needed for this project to work as well, but they can be pulled right from within the project.
Performable from the following steps from the root of the cloned git repository within a terminal. 

Make sure you fetch submodules: `git submodule update --init --recursive`
Go into each submodule's src directory and type:
`make`
`make static`

Once each submodule has been made you can go into the base project src folder anre simply type:
`make`

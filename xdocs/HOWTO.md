# How To Make A Basic Game

THIS GUIDE IS SUBJECT TO CHANGE!

## Prerequisites

Git. All the dependencies listed in README.md, and clang capable of C++20.

## Setting up your build system

With RDM4001, we use meson. For the time being, you can link RDM4001 into your project by creating the directory `subprojects` and placing the rdm4001 repository within there, e.g. `subprojects/rdm4001`.

Then, you can create a project with a `meson.build`. An example `meson.build` would be

	project('mygame', 'cpp', default_options: ['cpp_std=c++20']) # RDM4001 requires to be compiled with C++20
	rdm4001_proj = subproject('rdm4001') # Load RDM4001 from your subprojects directory
	rdm4001_dep = rdm4001_proj.get_variable('rdm4001_dep')
	
	executable('mygame',
	    'main.cpp',
		dependencies: rdm4001_dep
	)

## Creating main.cpp

Once you have this, you can write your `main.cpp` file.

	#include <game.hpp> /* Include the game header, 
	where the class rdm::Game comes from. 
	That is what you inherit to make a game */
	
	using namespace rdm;
	
	class MyGame : public Game {
		virtual void initialize() {
			startClient(); /* If you do not call startClient, 
			the game will instantly shut down after initializing. 
			This tells the game to start up the graphics and sound engine. */
		}
		
		virtual void initializeClient() {
			/* You can get away with running your initialization code in 
			initialize for the time being, but it is a bad practice when 
			you want to get around to creating dedicated servers for your game. */
			
			/* You are free to do whatever you want using gfxEngine, or anything else here. */
		}
	};
	
	int main(int argc, char** argv) {
		rdm::Settings::singleton()->parseCommandLine(argv, argc); /* This parses 
		the command line, and loads the settings from 
		the executing directory/settings.json. If this is not called, 
		then the default settings will not be loaded. */
		
		MyGame mygame;
		mygame.mainLoop(); /* This will loop until the game stops. */
		rdm::Settings::singleton()->save(); /* This saves the settings to whatever 
		the settings load path is, usually just the executing directory/settings.json. */
	}

## Compiling

Because RDM4001 requires clang, sometimes your system will by default use GCC, which wont work for the time being. You can force meson to use clang by setting the environment variables CC and CXX to clang and clang++ respectively.

For example, compiling this project would have you do this:

	mkdir build
	cd build
	CC=clang CXX=clang++ meson setup
	ninja
	
Then your `mygame` should be in the directory. You have now made your first game.

RDM4001 requires that you have structure similar to its own. Copy over the `data` directory if you have any trouble with it finding certain files. At the time of writing, it requires the following data directories/files:

- dat1/ (material/shader directory)
- dat3/gui.json

These should be placed in (PROJECT ROOT)/data.

## Pointers

Some things you might find interesting are:

- rdm::putil::FpsController
- rdm::network::NetworkManager
- rdm::gfx::MeshCache
- rdm::gfx::BaseDevice
- rdm::gfx::Engine

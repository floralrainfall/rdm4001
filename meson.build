project('rdm4001', 'cpp', default_options: ['cpp_std=c++20'])

if get_option('vcpkg').enabled()
  vcpkg = find_program('vcpkg', required : true)
  run_command(vcpkg, 'install', '--x-wait-for-lock',
                                '--x-manifest-root=' + meson.current_source_dir())
endif

sdl2 = dependency('sdl2')
sdl_ttf = dependency('SDL2_ttf')
glm = dependency('glm')
openal = dependency('openal')
assimp = dependency('assimp')
enet = dependency('libenet')
# easy_profiler = dependency('easy_profiler')
bullet = dependency('Bullet')
boost_dep = dependency('boost', modules: ['program_options'])

common_proj = subproject('common')
common_dep = common_proj.get_variable('libcommon_dep')
obz_proj = subproject('obz', required: false)
obz_options = []
if not obz_proj.found()
  obz_dep = declare_dependency()
  obz_options = ['-DDISABLE_OBZ']
else
  obz_dep = obz_proj.get_variable('libobz_dep')
endif

glad_proj = subproject('glad')
glad_dep = glad_proj.get_variable('libglad_dep')

assembly_proj = subproject('assembly')

inc = include_directories(['.', '/usr/include/bullet'])

gamelib = static_library('game', [
  'game.cpp',
  'game.hpp',
  'graph.cpp',
  'graph.hpp',
  'input.cpp',
  'input.hpp',
  'scheduler.cpp',
  'scheduler.hpp',
  'settings.cpp',
  'settings.hpp',
  'signal.cpp',
  'signal.hpp',
  'world.cpp',
  'world.hpp',
  'physics.cpp',
  'physics.hpp',

  'putil/fpscontroller.cpp',
  'putil/fpscontroller.hpp',

  'network/bitstream.cpp',
  'network/bitstream.hpp',
  'network/network.cpp',
  'network/network.hpp',
  'network/entity.cpp',
  'network/entity.hpp',
  'network/player.cpp',
  'network/player.hpp',

  'gfx/gui/element.cpp',
  'gfx/gui/element.hpp',
  'gfx/gui/gui.cpp',
  'gfx/gui/gui.hpp',
  'gfx/gui/layout.cpp',
  'gfx/gui/layout.hpp',

  'gfx/camera.cpp',
  'gfx/camera.hpp',
  'gfx/engine.cpp',
  'gfx/engine.hpp',
  'gfx/entity.cpp',
  'gfx/entity.hpp',
  'gfx/heightmap.cpp',
  'gfx/heightmap.hpp',
  'gfx/material.cpp',
  'gfx/material.hpp',
  'gfx/mesh.cpp',
  'gfx/mesh.hpp',
  'gfx/stb_image.h',

  'gfx/base_context.cpp',
  'gfx/base_context.hpp',
  'gfx/base_device.cpp',
  'gfx/base_device.hpp',
  'gfx/base_types.cpp',
  'gfx/base_types.hpp',

  'gfx/gl_context.cpp',
  'gfx/gl_context.hpp',
  'gfx/gl_device.cpp',
  'gfx/gl_device.hpp',
  'gfx/gl_types.cpp',
  'gfx/gl_types.hpp',
], cpp_args: obz_options, include_directories: [inc], dependencies: [sdl_ttf, boost_dep, enet, common_dep, glad_dep, obz_dep, sdl2, glm, openal, assimp, bullet])

inc2 = include_directories('wawaworld')

executable('launcher', [
  'launcher.cpp'
], include_directories: [inc, inc2], dependencies: [common_dep], link_with: gamelib)

executable('raymarcher', [
  'raymarcher/main.cpp',
  'raymarcher/rgame.hpp',
  'raymarcher/rgame.cpp'
], include_directories: [inc, inc2], dependencies: [common_dep, sdl2, glm], link_with: gamelib)

b3geometry_dep = declare_dependency(link_args : ['-lBullet3Geometry', '-lBullet3Common'] ) 
executable('wawaworld', [
  'wawaworld/main.cpp',
  'wawaworld/map.cpp',
  'wawaworld/map.hpp',
  'wawaworld/wgame.cpp',
  'wawaworld/wgame.hpp',
  'wawaworld/worldspawn.cpp',
  'wawaworld/worldspawn.hpp'
], include_directories: [inc, inc2], dependencies: [common_dep, sdl2, glm, b3geometry_dep], link_with: gamelib)

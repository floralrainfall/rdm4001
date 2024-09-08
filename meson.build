project('rdm4001', 'cpp', default_options: ['cpp_std=c++20'])

sdl2 = dependency('sdl2')
cglm = dependency('cglm')
openal = dependency('openal')
assimp = dependency('assimp')
easy_profiler = dependency('easy_profiler')

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

inc = include_directories('.')

gamelib = static_library('game', [
  'game.cpp',
  'game.hpp',
  'graph.cpp',
  'graph.hpp',
  'input.cpp',
  'input.hpp',
  'scheduler.cpp',
  'scheduler.hpp',
  'signal.cpp',
  'signal.hpp',
  'world.cpp',
  'world.hpp',

  'gfx/gui/element.cpp',
  'gfx/gui/element.hpp',
  'gfx/gui/gui.cpp',
  'gfx/gui/gui.hpp',
  'gfx/gui/layout.cpp',
  'gfx/gui/layout.hpp',

  'gfx/engine.cpp',
  'gfx/engine.hpp',
  'gfx/entity.cpp',
  'gfx/entity.hpp',
  'gfx/material.cpp',
  'gfx/material.hpp',
  'gfx/mesh.cpp',
  'gfx/mesh.hpp',

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
], cpp_args: obz_options, include_directories: inc, dependencies: [common_dep, glad_dep, obz_dep, sdl2, cglm, openal, assimp, easy_profiler])

inc2 = include_directories('wawaworld')

executable('wawaworld', [
  'wawaworld/main.cpp',
  'wawaworld/wgame.cpp',
  'wawaworld/wgame.hpp'
], include_directories: [inc, inc2], dependencies: [common_dep], link_with: gamelib)
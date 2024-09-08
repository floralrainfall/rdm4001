project('common', 'cpp')

inc = include_directories('.')
libcommon = static_library('common',
  ['filesystem.cpp',
   'filesystem.hpp',
   'guid.cpp',
   'guid.hpp',
   'json.hpp',
   'logging.cpp',
   'logging.hpp',
   'random.cpp',
   'random.hpp'],
  include_directories: inc)
libcommon_dep = declare_dependency(include_directories: inc, link_with: libcommon)
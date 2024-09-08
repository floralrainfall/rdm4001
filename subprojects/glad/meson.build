project('glad', 'c')

inc = include_directories('include')
libglad = static_library('glad',
  ['src/glad.c',
   'include/glad/glad.h',
   'include/KHR/khrplatform.h'],
  include_directories: inc)

libglad_dep = declare_dependency(include_directories: inc, link_with: libglad)
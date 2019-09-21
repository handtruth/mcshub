project('mcshub', 'cpp',
  version : run_command('git', 'describe', '--abbrev=0', '--tags').stdout().strip(),
  default_options : ['warning_level=3',
                     'cpp_std=c++17'])

group = 'com.handtruth.mc'
maintainer = 'ktlo <ktlo@handtruth.com>'

modules = [
  'paket-cpp',
  'ekutils'
]

######################################

module_deps = []

foreach module : modules
  module_deps += dependency(module, fallback : [module, 'dep'])
endforeach

module_deps += meson.get_compiler('cpp').find_library('stdc++fs')
module_deps += dependency('threads')

cmake = import('cmake')
yamlcpp_proj = cmake.subproject('yaml-cpp', cmake_options : [
  '-DYAML_CPP_BUILD_TESTS=OFF'
])
module_deps += yamlcpp_proj.dependency('yaml-cpp')

cppres_exe = find_program('cpp-res', required : false)
if (not cppres_exe.found())
  cppres_proj = subproject('cpp-res')
  cppres_exe = cppres_proj.get_variable('exe')
endif

subdir('res')
subdir('src')
subdir('test')

cppcheck = custom_target(meson.project_name() + '_cppcheck_internal',
  output : meson.project_name() + '_cppcheck.log',
  input : sources + test_files + files(['src'/'main.cpp']),
  command : [
    'cppcheck',
    '--enable=all',
    '-I', meson.current_source_dir() / 'include',
    '-I', meson.current_source_dir() / 'src',
    '@INPUT@',
#   '--project=compile_commands.json',
    '--output-file=@OUTPUT@'
])

run_target('cppcheck', command : ['cat', cppcheck.full_path()], depends : cppcheck)

if get_option('systemd')
  install_data('res'/'config'/'mcshub.yml', install_dir : 'etc'/'mcshub')
  install_data('etc'/'mcshub.service', install_dir : 'etc'/'systemd'/'system')
endif

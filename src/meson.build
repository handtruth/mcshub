sources = files([
  'client.cpp',
  'hosts_db.cpp',
  'manager.cpp',
  'mc_pakets.cpp',
  'mcshub.cpp',
  'prog_args.cpp',
  'response_props.cpp',
  'sclient.cpp',
  'settings.cpp',
  'thread_controller.cpp'
])

src = include_directories('.')

sources += configure_file(output : 'config.cpp', input : 'config.cpp.in',
  configuration : {
  'group' : group,
  'project' : meson.project_name(),
  'version' : meson.project_version(),
  'build' : run_command('git', 'describe').stdout().strip(),
  'maintainer' : maintainer,
})

static_lib = static_library(meson.project_name(), [sources, res_header, res_source], dependencies: module_deps)
exe = executable(meson.project_name(), ['main.cpp', res_header], link_with : static_lib, install : true, dependencies : module_deps)
mcping = executable('mcping', 'pingtool.cpp', link_with : static_lib, install : true, dependencies : module_deps)
run_target('run', command : [exe] + get_option('run_args'), depends : exe)

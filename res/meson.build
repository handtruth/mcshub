resources = files([
  'config/fallback/login.json',
  'config/fallback/status.json',
  'config/mcshub.yml',
  'config/mcsman/login.json',
  'config/mcsman/status.json'
])

res_header = custom_target(
  'res_header',
  output : 'resources.hpp',
  input : resources,
  command : [cppres_exe,
    '-header-only',
    '-path', meson.current_source_dir(),
    '-header', '@OUTPUT@',
    '-tree', '--', '@INPUT@'
  ]
)

res_source = custom_target(
  'res_source',
  output : 'resources.cpp',
  input : resources,
  command : [cppres_exe,
    '-source-only',
    '-path', meson.current_source_dir(),
    '-source', '@OUTPUT@',
    '-tree', '--', '@INPUT@'
  ]
)

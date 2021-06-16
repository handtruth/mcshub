test_names = [
  'args',
  'distributed_test',
  'config',
  'paket',
  #'status',
  'vars',
  'fetch_status',
  'yaml',
  'test_strparts'
]

test_files = []

foreach test_name : test_names
  test_files += files(test_name + '.cpp')
  test_exe = executable(test_name + '.test', [test_files[-1], res_header], link_with : static_lib, include_directories : src, dependencies : module_deps)
  test(meson.project_name() + '-' + test_name, test_exe, suite : 'unit')
endforeach

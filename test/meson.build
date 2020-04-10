test_names = [
  'args',
#  'config',
  'paket',
  'status',
  'vars',
  'fetch_status'
]

test_files = []

foreach test_name : test_names
  test_files += files(test_name + '.cpp')
  test_exe = executable(test_name + '.test', [test_files[-1], res_header], link_with : static_lib, include_directories : src, dependencies : module_deps)
  test(test_name, test_exe)
endforeach

sh = find_program('sh')

#TODO:
#test('serve100clients', sh, args : [files('serve100clients.sh'), exe])

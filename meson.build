project('Dyppyl', 'cpp')

extra_args = [
    '-fpermissive',
    '-Wno-literal-suffix',
    '-Wno-non-virtual-dtor',
    '-Wno-sign-compare'
]

executable('demo', 'src/main.cpp',
        override_options : ['cpp_std=c++2a'],
        cpp_args : extra_args)

test_files = [
    'LL1.cpp',
    'LR0.cpp',
    'NonLL1.cpp',
    'NonLR0.cpp',
    'Production.cpp',
    'Regex.cpp',
    'SmallLL1.cpp'
]

foreach t : test_files
    relative = 'DyppylTest' / t
    test_exe = executable(t, relative,
                             override_options : ['cpp_std=c++2a'],
                             cpp_args : extra_args,
                             include_directories : include_directories('deps/include', 'src/'))
    test(t, test_exe)
endforeach
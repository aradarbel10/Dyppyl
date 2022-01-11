project('pascal compiler', 'cpp')
executable('pascal', 'main.cpp',
    override_options : ['cpp_std=c++2a'],
    cpp_args : [
        '-fpermissive',
        '-Wno-literal-suffix',
        '-Wno-non-virtual-dtor',
        '-Wno-sign-compare'
    ],
    include_directories : include_directories('../../src/'))
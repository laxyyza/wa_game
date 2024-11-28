coregame_src = files(
    'src/coregame.c',
    'src/cg_map.c',
)
coregame_include = include_directories('include/')

include_dir = [
    coregame_include,
    cutils_include,
    ght_include,
]
deps = [m_dep]

libcoregame_client = library('coregame_client', coregame_src, 
    include_directories: include_dir,
    dependencies: deps,
    c_args: ['-DCG_CLIENT']
)

coregame_src += files('src/sbsm.c')

libcoregame_server = library('coregame_server', coregame_src, 
    include_directories: include_dir,
    dependencies: deps,
    c_args: ['-DCG_SERVER']
)

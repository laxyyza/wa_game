coregame_src = files(
    'src/coregame.c',
    'src/cg_map.c',
)
coregame_include = include_directories('include/')
inc_dir = [coregame_include, cutils_include, ght_include]
deps = [m_dep]

coregame_client_args = ['-DCG_CLIENT']
coregame_server_args = ['-DCG_SERVER']

libcoregame_client = library('coregame_client', coregame_src, 
    include_directories: inc_dir,
    dependencies: deps,
    c_args: coregame_client_args
)

coregame_src += files('src/sbsm.c')

libcoregame_server = library('coregame_server', coregame_src, 
    include_directories: inc_dir,
    dependencies: deps,
    c_args: coregame_server_args
)

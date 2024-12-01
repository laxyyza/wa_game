coregame_src = files(
    'src/coregame.c',
    'src/cg_map.c',
)
coregame_include = include_directories('include/')

libcoregame_client = library('coregame_client', coregame_src, 
    include_directories: [
        coregame_include,
        cutils_include,
        ght_include,
    ],
    dependencies: [
        m_dep
    ],
    c_args: ['-DCG_CLIENT']
)

coregame_src += files('src/sbsm.c')

libcoregame_server = library('coregame_server', coregame_src, 
    include_directories: [
        coregame_include,
        cutils_include,
        ght_include,
    ],
    dependencies: [
        m_dep
    ],
    c_args: ['-DCG_SERVER']
)

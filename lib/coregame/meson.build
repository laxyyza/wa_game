coregame_src = files(
    'src/coregame.c',
)
coregame_include = include_directories('include/')

libcoregame = library('coregame', coregame_src, 
    include_directories: [
        coregame_include,
        cutils_include,
        ght_include,
    ],
    dependencies: [
        m_dep
    ]
)

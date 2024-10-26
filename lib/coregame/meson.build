coregame_src = files(
    'src/array.c',
    'src/coregame.c',
)
coregame_include = include_directories('include/')

libcoregame = library('coregame', coregame_src, 
    include_directories: [
        coregame_include,
        ght_include,
    ]
)

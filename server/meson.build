server_src = files(
    'src/main.c',
    'src/server.c',
    'src/client.c',
    'src/event.c',
    'src/server_init.c',
    'src/server_game.c',
)
server_include = include_directories('include/')

executable('server', server_src, 
    include_directories: [
        server_include,
        ght_include,
        coregame_include,
        ssp_include,
        netdef_include,
        cutils_include,
    ],
    link_with: [
        libssp,
        libnetdef,
        libcoregame,
        libcutils,
    ]
)

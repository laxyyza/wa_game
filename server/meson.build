server_src = files(
    'src/main.c',
    'src/server.c',
    'src/client.c',
    'src/event.c',
)
server_include = include_directories('include/')

executable('server', server_src, 
    include_directories: [
        server_include,
        ght_include,
        coregame_include,
        ssp_include,
        netdef_include,
    ],
    link_with: [
        libssp,
        libnetdef
    ]
)

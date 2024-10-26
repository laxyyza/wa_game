server_src = files(
    'src/main.c',
    'src/server.c',
)
server_include = include_directories('include/')

executable('server', server_src, 
    include_directories: [
        server_include,
        ght_include,
        coregame_include
    ]
)

netdef_src = files(
    'src/netdef.c'
)

netdef_include = include_directories('include/')

libnetdef = library('netdef', netdef_src, 
    include_directories: [
        netdef_include,
        coregame_include,
        ssp_include,
        ght_include,
        cutils_include,
    ]
)

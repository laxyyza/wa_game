cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: true)

subdir('lib/wa/wa')
subdir('lib/glad')
include_dir = include_directories('include/')

src = files(
    'src/main.c',
    'src/app.c',
    'src/opengl.c',
    'src/vertbuf.c',
    'src/idxbuf.c',
    'src/vertlayout.c',
    'src/vertarray.c',
    'src/shader.c',
    'src/renderer.c',
    'src/stb_image.c',
    'src/texture.c',
    'src/rect.c',
    'src/mat.c',
    'src/file.c',
    'src/gui/gui.c',
    'src/util.c',
    'src/loading.c',
    'src/player.c',
)

executable('wa_game', src, 
    include_directories: [
        wa_include, 
        include_dir,
        ght_include,
        glad_include,
        coregame_include,
    ], 
    dependencies: [
        m_dep,
    ],
    link_with: [
        libwa,
        libght,
        libglad,
        libcoregame,
    ],
    link_args: ['-Wl,-rpath=./lib']
)

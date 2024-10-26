cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: true)

subdir('lib/wa/wa')
subdir('lib/ght/ght')
subdir('lib/glad')
include_dir = include_directories('include/')

src = files(
    'src/main.c',
    'src/app.c',
    'src/opengl.c',
    'src/vertbuf.c',
    'src/idxbuf.c',
    'src/array.c',
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
)

executable('wa_opengl', src, 
    include_directories: [
        wa_include, 
        include_dir,
        ght_include,
        glad_include,
    ], 
    dependencies: [
        m_dep,
    ],
    link_with: [
        libwa,
        libght,
        libglad
    ],
    link_args: ['-Wl,-rpath=./lib']
)

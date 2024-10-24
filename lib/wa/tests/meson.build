opengl_src = files(
    'opengl.c'
)

simple_src = files(
    'simple.c'
)

executable('wa_simple', simple_src,
    include_directories: [wa_include],
    link_with: [libwa],
    dependencies: [gl, glew]
)

executable('wa_opengl', opengl_src,
    include_directories: [wa_include],
    link_with: [libwa],
    dependencies: [gl, glew]
)

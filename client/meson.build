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
    'src/player.c',
    'src/client_net.c',
    'src/app_wayland.c',
    'src/state.c',
    'src/main_menu.c',
    'src/map_editor.c',
    'src/game.c',
    'src/game_ui.c',
    'src/game_draw.c',
    'src/game_net_events.c',
    'src/progress_bar.c',
)
deps = [m_dep]

if meson.is_cross_build()
    winsock_dep = cc.find_library('ws2_32', required: true)
    deps += winsock_dep
else
    deps += wa_deps
endif

executable('wa_game', src, 
    include_directories: [
        wa_include, 
        include_dir,
        ght_include,
        glad_include,
        coregame_include,
        netdef_include,
        ssp_include,
        cutils_include,
    ], 
    dependencies: deps,
    link_with: [
        libwa,
        libght,
        libglad,
        libcoregame_client,
        libnetdef,
        libssp,
        libcutils,
    ],
    link_args: ['-Wl,-rpath=./lib'],
    c_args: coregame_client_args
)

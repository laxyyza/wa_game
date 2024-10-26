wa_src = [
    'src/wa_log.c',
    'src/wa.c',
]

message('Building WA for ', host_machine.system(), host_machine.cpu())

wa_deps = []

if meson.is_cross_build()
    add_global_arguments(
        '-Wno-error=cast-function-type',
        language: 'c'
    )
    add_project_arguments(
        '-D_WA_WIN32',
        language: 'c'
    )
    wa_src += [
        'src/wa_win32.c',
        'src/wa_win32_keymap.c',
        'src/wa_win32_cursor.c',
    ]
    gl = dependency('gl', required: true)
    wa_deps += gl
else
    add_project_arguments(
        '-D_WA_WAYLAND',
        language: 'c'
    )
    wa_src += [
        'src/wa_log.c',
        'src/wa_wayland.c',
        'src/wa_wayland_input.c',
        'src/wa_wayland_cursor.c',
        'src/wa_wayland_egl.c',
        'src/wa_wayland_xkbcommon.c',
    ]

    # Find Wayland dependencies
    wayland_client = dependency('wayland-client', required: true)
    wayland_egl = dependency('wayland-egl', required: true)
    wayland_protocols = dependency('wayland-protocols', required: true)

    # Find OpenGL and EGL dependencies
    egl = dependency('egl', required: true)

    # Find xkbcommon
    xkbcommon = dependency('xkbcommon', required: true)

    wa_deps = [wayland_client, wayland_egl, egl, xkbcommon]

    subdir('wayland-protocols/')

    wa_deps += client_protos
endif


wa_include = include_directories('include/')
wa_src = files(wa_src)


public_headers = [
    'include/wa.h', 
    'include/wa_event.h'
]
install_headers(public_headers, install_dir: 'include/wa')

libwa = library('wa', wa_src,
    include_directories: wa_include,
    dependencies: wa_deps,
    install: true
)

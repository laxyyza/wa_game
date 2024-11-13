project('wa_game', 'c',
    default_options: [
        'werror=true',
        'warning_level=2',
        'default_library=static',
        'c_std=c2x'
    ],
    version: '0.1'
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: true)

subdir('lib/')
subdir('client/')
if not meson.is_cross_build()
    subdir('server/')
endif

project('wa_game', 'c',
    default_options: [
        'werror=true',
        'warning_level=2',
        'default_library=static',
        'c_std=c23'
    ],
    version: run_command('cat', join_paths(meson.project_source_root(), 'VERSION'), check: true).stdout().strip(),
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required: true)

add_project_arguments(
  '-DVERSION="' + meson.project_version() + '"',
  '-DBUILD_TYPE="' + get_option('buildtype') + '"',
  language : 'c'
)

subdir('lib/')
subdir('client/')
if not meson.is_cross_build()
    subdir('server/')
endif

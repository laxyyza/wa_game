wl_protcol_dir = wayland_protocols.get_variable('pkgdatadir')

wayland_scanner = find_program('wayland-scanner')

wayland_scanner_code = generator(
    wayland_scanner,
    output: '@BASENAME@-protocol.c',
    arguments: ['private-code', '@INPUT@', '@OUTPUT@']
)

wayland_scanner_client = generator(
    wayland_scanner,
    output: '@BASENAME@.h',
    arguments: ['client-header', '@INPUT@', '@OUTPUT@']
)

xdg_shell_proto = 'stable/xdg-shell/xdg-shell.xml'
tearing_control_proto = 'staging/tearing-control/tearing-control-v1.xml'
cursor_shape_proto = 'staging/cursor-shape/cursor-shape-v1.xml'
tablet_tool_proto = 'stable/tablet/tablet-v2.xml'
xdg_decoration_proto = 'unstable/xdg-decoration/xdg-decoration-unstable-v1.xml'

client_protocols = [
    [wl_protcol_dir, xdg_shell_proto],
    [wl_protcol_dir, xdg_decoration_proto],
    [wl_protcol_dir, tearing_control_proto],
    [wl_protcol_dir, cursor_shape_proto],
    [wl_protcol_dir, tablet_tool_proto],
]

client_proto_src = []
client_proto_headers = []

foreach p : client_protocols
    xml = join_paths(p)
    client_proto_src += wayland_scanner_code.process(xml)
    client_proto_headers += wayland_scanner_client.process(xml)
endforeach

lib_client_protos = static_library(
    'client_protos',
    client_proto_src + client_proto_headers,
    dependencies: [wayland_client]
)

client_protos = declare_dependency(
    link_with: lib_client_protos,
    sources: client_proto_headers
)

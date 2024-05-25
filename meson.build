project(
	'xsocket', 'c',
	default_options: {
		'b_ndebug': 'if-release',
		'buildtype': 'release',
		'warning_level': '1',
		'strip': true
	},
	version: '1.0')

add_project_arguments('-D_GNU_SOURCE', language: 'c')

if get_option('debug')
	add_project_arguments('-D_DEBUG', language: 'c')
endif

install_headers('xsocket.h')

systemd = dependency('systemd', required: false)
libsystemd = dependency('libsystemd', required: false)
python = import('python').find_installation('python3', required: false)

if systemd.found()
	install_data('xsocket.service',
		install_dir: systemd.get_variable(pkgconfig: 'systemd_system_unit_dir'))

	install_data('xsocket.sysusers',
		install_dir: systemd.get_variable(pkgconfig: 'sysusers_dir'),
		rename: 'xsocket.conf')
endif

if libsystemd.found()
	add_project_arguments('-DXSOCKET_SYSTEMD', language: 'c')
endif

xsocket_common = static_library(
	'xsocket_common',
	'cleanup.c',
	'socket.c',
	'protocol.c',
	gnu_symbol_visibility: 'hidden')

xsocket_library = shared_library(
	'xsocket',
	'xsocket.c',
	gnu_symbol_visibility: 'hidden',
	link_with: xsocket_common,
	install: true)

shared_library(
	'xbind',
	'hook.c',
	'switch.c',
	gnu_symbol_visibility: 'hidden',
	link_with: [xsocket_common, xsocket_library],
	install: true)

executable(
	'xsocket-server',
	'server.c',
	dependencies: libsystemd,
	gnu_symbol_visibility: 'hidden',
	link_with: xsocket_common,
	install: true)

if python.found() and python.language_version().version_compare('>=3.12')
	python.install_sources('xsocket.py')
endif

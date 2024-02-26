project(
	'xsocket', 'c',
	default_options: {
		'b_ndebug': 'if-release',
		'buildtype': 'release',
		'warning_level': '1',
		'strip': true
	},
	version: '0.3')

add_project_arguments('-D_GNU_SOURCE', language: 'c')

if get_option('debug')
	add_project_arguments('-D_DEBUG', language: 'c')
endif

install_headers('xsocket.h')

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
	gnu_symbol_visibility: 'hidden',
	link_with: [xsocket_common, xsocket_library],
	install: true)

executable(
	'xsocket-server',
	'server.c',
	gnu_symbol_visibility: 'hidden',
	link_with: xsocket_common,
	install: true)

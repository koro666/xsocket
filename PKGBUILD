pkgname='xsocket'
pkgver=0.8
pkgrel=1
pkgdesc='Cross-namespace socket library'
arch=('x86_64' 'i686' 'aarch64' 'armv7h')
makedepends=('meson')
source=(
	'meson.build'
	'system.h'
	'cleanup.c'
	'cleanup.h'
	'socket.c'
	'socket.h'
	'protocol.c'
	'protocol.h'
	'xsocket.c'
	'xsocket.h'
	'hook.c'
	'hook.h'
	'switch.c'
	'switch.h'
	'server.c'
	'server.h'
	'xsocket.service'
)
sha256sums=(
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
)

build() {
	arch-meson build
	meson compile -C build
}

package() {
	meson install -C build --destdir "$pkgdir"
}

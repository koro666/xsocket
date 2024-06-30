pkgname='xsocket'
pkgver=1.1
pkgrel=1
pkgdesc='Cross-namespace socket library'
arch=('x86_64' 'i686' 'aarch64' 'armv7h')
url='https://github.com/koro666/xsocket'
makedepends=('meson')
optdepends=('python: python support')
source=(
	'meson.build'
	'system.h'
	'cleanup.c'
	'cleanup.h'
	'socket.c'
	'socket.h'
	'address.c'
	'address.h'
	'protocol.c'
	'protocol.h'
	'xsocket.c'
	'xsocket.h'
	'hook.c'
	'hook.h'
	'option.c'
	'option.h'
	'switch.c'
	'switch.h'
	'server.c'
	'server.h'
	'xsocket.service'
	'xsocket.sysusers'
	'xsocket.py'
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
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
	'SKIP'
)

prepare() {
	local _src
	local _dst

	for _src in "${source[@]}"
	do
		if [[ -L "$_src" ]]
		then
			_dst=$(readlink "$_src")
			unlink "$_src"
			ln -- "$_dst" "$_src"
		fi
	done
}

build() {
	arch-meson build
	meson compile -C build
}

package() {
	meson install -C build --destdir "$pkgdir"
}

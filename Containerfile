# container for clean builds
#  binary:  podman build --target=build --output=type=tar,dest=xsocket.tar .
#  package: podman build --target=package --output=. .

FROM archlinux:latest AS base

RUN \
--mount=type=cache,id=pacman,target=/var/cache/pacman/pkg \
--mount=type=cache,id=pacman.db,target=/var/lib/pacman/sync \
pacman -Su --noconfirm && \
pacman -S --noconfirm archlinux-keyring && \
pacman -Sy --noconfirm && \
pacman -S --noconfirm base-devel python meson

RUN \
useradd -d /var/empty -M -s /usr/bin/nologin builder && \
mkdir /build /output && \
chown builder:builder /build /output

USER builder:builder
WORKDIR /build
COPY --chown=builder:builder . /build

FROM base AS builder
RUN \
meson setup build --prefix=/ && \
meson compile -C build && \
DESTDIR=/output meson install -C build

FROM scratch AS build
COPY --from=builder --chown=0:0 /output /

FROM base AS packager
RUN PKGDEST=/output makepkg

FROM scratch AS package
COPY --from=packager --chown=0:0 /output /

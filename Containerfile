# container for clean builds
#  archlinux binary:  podman build --build-arg=DISTRO=archlinux --target=build --output=type=tar,dest=xsocket.tar .
#  archlinux package: podman build --build-arg=DISTRO=archlinux --target=package --output=. .
#  alpine binary:     podman build --build-arg=DISTRO=alpine --target=build --output=type=tar,dest=xsocket.tar .

ARG DISTRO=archlinux

FROM archlinux:latest AS archlinux

RUN \
--mount=type=cache,id=pacman,target=/var/cache/pacman/pkg \
--mount=type=cache,id=pacman.db,target=/var/lib/pacman/sync \
pacman -Su --noconfirm && \
pacman -S --noconfirm archlinux-keyring && \
pacman -Sy --noconfirm && \
pacman -S --noconfirm base-devel python meson

FROM alpine:3.19 AS alpine

RUN \
--mount=type=cache,id=apk,target=/etc/apk/cache \
apk upgrade --clean-protected && \
apk add shadow alpine-sdk python3 meson

FROM ${DISTRO} AS base

RUN \
useradd -d /var/empty -M -s /sbin/nologin builder && \
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

FROM base AS packager-archlinux
RUN PKGDEST=/output makepkg

FROM base AS packager-alpine
# TODO:
RUN false

FROM packager-${DISTRO} AS packager

FROM scratch AS package
COPY --from=packager --chown=0:0 /output /

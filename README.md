# `xsocket`: cross-namespace socket creation

`xsocket` is a set of tools to allow processes from isolated network namespaces to create sockets into other network namespaces without needing to call `setns` which requires elevated privileges.

It works by forwarding the request to create a socket to a small server process which runs in the desired target namespace.

## Build

This project uses [Meson](https://mesonbuild.com/).

```
meson setup build
meson compile -C build
meson install -C build
```

Alternatively, if on Archlinux, just run `makepkg` to build a proper package.

## Usage

Make sure the server is running, either by running `xsocket-server` directly or using the `xsocket.service` systemd unit.

The default listen path is `/run/xsocket/default`, but a different path can be passed as the sole argument to `xsocket-server`. Abstract sockets are supported when prefixed with `@`.

There are two main ways to use this tool.

### Programmatically

`xsocket`-aware code can include `<xsocket.h>` and link with `libxsocket.so`, then call the following function:

```c
int xsocket(const char* path, int domain, int type, int, protocol);
```

The `path` parameter is the filesystem path to the server socket, in the same format as specified to `xsocket-server`. If set to `NULL`, it will use the default path.

The three remaining parameters have the same meaning as with `socket (2)`.

### Injection

The `libxbind.so` library can be injected into unaware processes using `LD_PRELOAD` to forward selected listening ports to the target network namespace.

The `XBIND` environment variable is a space separated list of ports which should be forwarded, or the special value `*` for all ports. No distinction is made between IPv4 vs IPv6, or TCP vs UDP.

The `XSOCKET` environment variable can be set to point to an alternate control socket. If unset, it will use the default path.

The following example demonstrates remote listening using `nc`:

```
LD_PRELOAD=libxbind.so XBIND=3000 nc -l -p 3000
```

For systemd services, an add-on file (`/etc/systemd/system/servicename.d/xbind.conf`) can be used to configure specific listening ports:

```
[Unit]
Wants=xsocket.service
After=xsocket.service

[Service]
ExecStartPre=+/usr/bin/setfacl -n -m u:serviceuser:rwx,m::rwx %t/xsocket
Environment=LD_PRELOAD=libxbind.so
Environment=XBIND=1234
```

Replace the `serviceuser` username and port number accordingly.

## Security

There is none. Any process with access to the control socket can ask it to create fresh sockets of any kind. Therefore, any security comes externally in the form of strict permissions on the socket, restrictions on the server process itself, and proper firewall rules.

## How it Works

When `xsocket` is called to create a socket, it connects to the `xsocket-server` instance through its UNIX socket and sends a message containing the requested domain, type and protocol.

Upon reception of such a message, the server creates the requested socket, then passes it back using `SCM_RIGHTS` to the client. In case of failure, an error code is returned instead.

The `libxbind.so` library works by intercepting the `bind` call, creating a new socket of the same domain, type and protocol as the original, copying all known socket options, and then duplicating it over the original file descriptor. Special care is taken that the non-blocking and close-on-exec flags of the original socket are preserved.

All calls to `setsockopt` are also tracked to keep track of which socket options have been used, in order to minimize the amount of blind copying of socket options.

## Thanks

- [rd235](https://github.com/rd235) for all their awesome network namespace tools;
- [libsdsock](https://github.com/ryancdotorg/libsdsock) for the technical inspiration to make the `libxbind.so` library.

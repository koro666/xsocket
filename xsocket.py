import ctypes as _ctypes
import os as _os
import socket as _socket
import typing as _typing

__version__ = '1.1'
__all__ = ['xsocket']

_libxsocket = _ctypes.CDLL('libxsocket.so', use_errno=True)

_xsocket = _libxsocket.xsocket
_xsocket.argtypes = [_ctypes.c_char_p, _ctypes.c_int, _ctypes.c_int, _ctypes.c_int]
_xsocket.restype = _ctypes.c_int

def xsocket(
	path: _typing.Union[_typing.AnyStr, _os.PathLike[_typing.AnyStr], None] = None,
	domain: _typing.Union[_socket.AddressFamily, int] = _socket.AF_INET,
	type: _typing.Union[_socket.SocketKind, int] = _socket.SOCK_STREAM,
	protocol: int = 0
) -> _socket.socket:
	bpath: _typing.Optional[bytes] = None
	if path is not None:
		bpath = _os.fsencode(path)

	fd = _xsocket(bpath, domain, type, protocol)
	if fd >= 0:
		return _socket.socket(fileno=fd)

	errno = _ctypes.get_errno()
	raise OSError(errno, _os.strerror(errno))

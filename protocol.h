#pragma once

extern const char xsocket_default_address[];

#define XS_PROTOCOL_REQUEST  0x58533031
#define XS_PROTOCOL_RESPONSE 0x58533032

struct xs_protocol_request
{
	uint32_t signature;
	int32_t domain;
	int32_t type;
	int32_t protocol;
};

struct xs_protocol_response
{
	uint32_t signature;
	int32_t error;
};

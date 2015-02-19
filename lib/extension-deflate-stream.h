#ifndef EXTENSION_DEFLATE_STREAM_H
#define EXTENSION_DEFLATE_STREAM_H
#include <zlib.h>
#include "private-libwebsockets.h"
#define LWS_ZLIB_WINDOW_BITS 15
#define LWS_ZLIB_MEMLEVEL 8

struct lws_ext_deflate_stream_conn {
	z_stream zs_in;
	z_stream zs_out;
	int remaining_in;
	unsigned char buf_in[LWS_MAX_SOCKET_IO_BUF];
	unsigned char buf_out[LWS_MAX_SOCKET_IO_BUF];
};

extern int lws_extension_callback_deflate_stream(
		struct libwebsocket_context *context,
		struct libwebsocket_extension *ext,
		struct libwebsocket *wsi,
		enum libwebsocket_extension_callback_reasons reason,
					      void *user, void *in, size_t len);

#ifdef __cplusplus
class Extension_Deflate_Stream: public libwebsocket_extension
{
	public:
	Extension_Deflate_Stream(const char* name)
	{
		this->name = name;
		this->per_session_data_size = sizeof(struct lws_ext_deflate_stream_conn);
		this->per_context_private_data = NULL;
	};
	virtual ~Extension_Deflate_Stream()
	{

	};


	virtual int callback(
		struct libwebsocket_context *context,
		struct libwebsocket_extension *ext,
		struct libwebsocket *wsi,
		enum libwebsocket_extension_callback_reasons reason,
					      void *user, void *in, size_t len);
};
#endif
#endif // EXTENSION_DEFLATE_STREAM_H

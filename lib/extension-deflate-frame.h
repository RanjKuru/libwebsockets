#ifndef EXTENSION_DEFLATE_FRAME_H
#define EXTENSION_DEFLATE_FRAME_H
#include <zlib.h>
#include "private-libwebsockets.h"
#define LWS_ZLIB_WINDOW_BITS 15
#define LWS_ZLIB_MEMLEVEL 8

struct lws_ext_deflate_frame_conn {
	z_stream zs_in;
	z_stream zs_out;
	size_t buf_pre_used;
	size_t buf_pre_length;
	size_t buf_in_length;
	size_t buf_out_length;
	int compressed_out;
	unsigned char *buf_pre;
	unsigned char *buf_in;
	unsigned char *buf_out;
};

extern int lws_extension_callback_deflate_frame(
		struct libwebsocket_context *context,
		struct libwebsocket_extension *ext,
		struct libwebsocket *wsi,
		enum libwebsocket_extension_callback_reasons reason,
		void *user, void *in, size_t len);

#ifdef __cplusplus
class Extension_Deflate_Frame: public libwebsocket_extension
{
public:
	Extension_Deflate_Frame(const char* name)
	{
		this->name = name;
		this->per_session_data_size = sizeof(struct lws_ext_deflate_frame_conn);
		this->per_context_private_data = NULL;
	};
	virtual ~Extension_Deflate_Frame()
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

#endif // EXTENSION_DEFLATE_FRAME_H

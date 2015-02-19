#ifndef EXTENSION_C
#define EXTENSION_C
#include "private-libwebsockets.h"

#ifdef LWS_EXT_DEFLATE_STREAM
#define EXTENSION_SIZE 1
#include "extension-deflate-stream.h"
#else
#define EXTENSION_SIZE 2
#include "extension-deflate-frame.h"
#endif

LWS_VISIBLE void
lws_context_init_extensions(struct lws_context_creation_info *info,
				    struct libwebsocket_context *context)
{
	context->extensions = info->extensions;
	lwsl_info(" LWS_MAX_EXTENSIONS_ACTIVE: %u\n", LWS_MAX_EXTENSIONS_ACTIVE);
}

LWS_VISIBLE struct libwebsocket_extension **libwebsocket_get_internal_extensions()
{
	struct libwebsocket_extension **libwebsocket_internal_extensions = lws_init_extensions(EXTENSION_SIZE);
#ifdef LWS_EXT_DEFLATE_STREAM
	const char* name = "deflate-stream";
#ifdef __cplusplus
	libwebsocket_internal_extensions[0] = new Extension_Deflate_Stream(name);
#else
	libwebsocket_internal_extensions[0] = lws_build_extension(name, lws_extension_callback_deflate_stream, sizeof(struct lws_ext_deflate_stream_conn), NULL);
#endif
#else
	const char* name0 = "x-webkit-deflate-frame";
	const char* name1 = "deflate-frame";
#ifdef __cplusplus
	libwebsocket_internal_extensions[0] = new Extension_Deflate_Frame(name0);
	libwebsocket_internal_extensions[1] = new Extension_Deflate_Frame(name1);
#else
	libwebsocket_internal_extensions[0] = lws_build_extension(name0, lws_extension_callback_deflate_frame, sizeof(struct lws_ext_deflate_frame_conn), NULL);
	libwebsocket_internal_extensions[1] = lws_build_extension(name1, lws_extension_callback_deflate_frame, sizeof(struct lws_ext_deflate_frame_conn), NULL);
#endif
#endif
	return libwebsocket_internal_extensions;
}


/* 0 = nobody had nonzero return, 1 = somebody had positive return, -1 = fail */

int lws_ext_callback_for_each_active(struct libwebsocket *wsi, int reason,
				void *arg, int len)
{
	int n, m, handled = 0;

	for (n = 0; n < wsi->count_active_extensions; n++) {
		m = wsi->active_extensions[n]->callback(
			wsi->protocol->owning_server,
			wsi->active_extensions[n], wsi,
			(enum libwebsocket_extension_callback_reasons)reason,
			wsi->active_extensions_user[n],
			arg, len);
		if (m < 0) {
			lwsl_ext(
			 "Extension '%s' failed to handle callback %d!\n",
				      wsi->active_extensions[n]->name, reason);
			return -1;
		}
		if (m > handled)
			handled = m;
	}
	
	return handled;
}

int lws_ext_callback_for_each_extension_type(
		struct libwebsocket_context *context, struct libwebsocket *wsi,
				int reason, void *arg, int len)
{
	int n = 0, i = 0, m, handled = 0;

	while (context->extensions[i] && !handled) {
		m = context->extensions[i]->callback(context, context->extensions[i], wsi, (enum libwebsocket_extension_callback_reasons)reason,
						(void *)(long)n, arg, len);

		if (m < 0) {
			lwsl_ext(
			 "Extension '%s' failed to handle callback %d!\n",
				      wsi->active_extensions[n]->name, reason);
			return -1;
		}
		if (m)
			handled = 1;

		i++;
		n++;
	}

	return 0;
}

int
lws_issue_raw_ext_access(struct libwebsocket *wsi,
						 unsigned char *buf, size_t len)
{
	int ret;
	struct lws_tokens eff_buf;
	int m;
	int n = 0;

	eff_buf.token = (char *)buf;
	eff_buf.token_len = len;

	/*
	 * while we have original buf to spill ourselves, or extensions report
	 * more in their pipeline
	 */

	ret = 1;
	while (ret == 1) {

		/* default to nobody has more to spill */

		ret = 0;

		/* show every extension the new incoming data */
		m = lws_ext_callback_for_each_active(wsi,
			       LWS_EXT_CALLBACK_PACKET_TX_PRESEND, &eff_buf, 0);
		if (m < 0)
			return -1;
		if (m) /* handled */
			ret = 1;

		if ((char *)buf != eff_buf.token)
			/*
			 * extension recreated it:
			 * need to buffer this if not all sent
			 */
			wsi->u.ws.clean_buffer = 0;

		/* assuming they left us something to send, send it */

		if (eff_buf.token_len) {
			n = lws_issue_raw(wsi, (unsigned char *)eff_buf.token,
							    eff_buf.token_len);
			if (n < 0) {
				lwsl_info("closing from ext access\n");
				return -1;
			}

			/* always either sent it all or privately buffered */
			if (wsi->u.ws.clean_buffer)
				len = n;
		}

		lwsl_parser("written %d bytes to client\n", n);

		/* no extension has more to spill?  Then we can go */

		if (!ret)
			break;

		/* we used up what we had */

		eff_buf.token = NULL;
		eff_buf.token_len = 0;

		/*
		 * Did that leave the pipe choked?
		 * Or we had to hold on to some of it?
		 */

		if (!lws_send_pipe_choked(wsi) && !wsi->truncated_send_len)
			/* no we could add more, lets's do that */
			continue;

		lwsl_debug("choked\n");

		/*
		 * Yes, he's choked.  Don't spill the rest now get a callback
		 * when he is ready to send and take care of it there
		 */
		libwebsocket_callback_on_writable(
					     wsi->protocol->owning_server, wsi);
		wsi->extension_data_pending = 1;
		ret = 0;
	}

	return len;
}

int
lws_any_extension_handled(struct libwebsocket_context *context,
			  struct libwebsocket *wsi,
			  enum libwebsocket_extension_callback_reasons r,
						       void *v, size_t len)
{
	int n;
	int handled = 0;

	/* maybe an extension will take care of it for us */

	for (n = 0; n < wsi->count_active_extensions && !handled; n++) {
		if (!wsi->active_extensions[n])
			continue;

		handled |= wsi->active_extensions[n]->callback(context,
			wsi->active_extensions[n], wsi,
			r, wsi->active_extensions_user[n], v, len);
	}

	return handled;
}
#endif // EXTENSION_C

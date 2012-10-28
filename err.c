#define _GNU_SOURCE
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"

struct evb_err {
	char *message;
	enum evb_err_code code;
};

struct evb_err *evb_err_new(void)
{
	struct evb_err *err;

	err = malloc(sizeof(struct evb_err));
	if (!err)
		return NULL;

	err->message = calloc(1, sizeof(char));
	if (!err->message) {
		free(err);
		return NULL;
	}

	evb_err_clear(err);

	return err;
}

void evb_err_free(struct evb_err *const err)
{
	free(err->message);
	free(err);
}

void evb_err_clear(struct evb_err *const err)
{
        /* Safe: message is always null terminated */
	memset(err->message, 0, strlen(err->message));
	err->code = EVB_ERR_CODE_UNKNOWN;
}

int evb_err_set(struct evb_err *const err, enum evb_err_code code,
		const char *const message_format, ...)
{
	char *message;
	va_list ap;

	va_start(ap, message_format);

	if (vasprintf(&message, message_format, ap) == -1) {
		va_end(ap);
		return -1;
	}

	va_end(ap);

	if (err->message)
		free(err->message);

	err->message = message;
	err->code = code;

	return 0;
}

enum evb_err_code evb_err_get_code(const struct evb_err *const err)
{
	return err->code;
}

const char *evb_err_get_message(const struct evb_err *const err)
{
	return err->message;
}

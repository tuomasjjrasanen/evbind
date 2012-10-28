#include <stdbool.h>

enum evb_err_code {
	EVB_ERR_CODE_NONE,
	EVB_ERR_CODE_SYS,
	EVB_ERR_CODE_UNKNOWN,
	EVB_ERR_CODE_UDEV
};

struct evb_err;

struct evb_err *evb_err_new(void);
void evb_err_free(struct evb_err *err);

void evb_err_clear(struct evb_err *err);
int evb_err_set(struct evb_err *err, enum evb_err_code code,
		const char *message_format, ...);
enum evb_err_code evb_err_get_code(const struct evb_err *err);
const char *evb_err_get_message(const struct evb_err *err);

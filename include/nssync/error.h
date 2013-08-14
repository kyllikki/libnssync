/*
 * This file is part of libnssync
 *
 * Copyright 20013 Vincent Sanders <vince@netsurf-browser.org>
 *
 * Released under MIT licence (see COPYING file)
 */

#ifndef NSSYNC_ERROR_H
#define NSSYNC_ERROR_H

enum nssync_error {
	NSSYNC_ERROR_OK = 0, /* operationhad no error */
	NSSYNC_ERROR_NOMEM, /* operation failed on memory allocation */
	NSSYNC_ERROR_REGISTRATION, /* registration faliure */
	NSSYNC_ERROR_INVAL, /* invalid parameter */
	NSSYNC_ERROR_VERSION, /* unsupported protocol version */
	NSSYNC_ERROR_PROTOCOL, /* protocol error (unexpected format etc.) */
	NSSYNC_ERROR_HMAC, /* HMAC mismatch */
	NSSYNC_ERROR_FETCH, /* https fetch error */
};

typedef enum nssync_error nssync_error;

#endif

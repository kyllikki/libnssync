
#ifndef NSSYNC_ERROR_H
#define NSSYNC_ERROR_H

enum nssync_error {
	NSSYNC_ERROR_OK = 0, /* operationhad no error */
	NSSYNC_ERROR_NOMEM, /* operation failed on memory allocation */
	NSSYNC_ERROR_AUTH, /* authorisation faliure */
	NSSYNC_ERROR_INVAL, /* invalid parameter */
	NSSYNC_ERROR_VERSION, /* unsupported protocol version */
	NSSYNC_ERROR_PROTOCOL, /* protocol error (unexpected format etc.) */
};

#endif

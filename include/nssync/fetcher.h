/*
 * This file is part of libnssync
 *
 * Copyright 20013 Vincent Sanders <vince@netsurf-browser.org>
 *
 * Released under MIT licence (see COPYING file)
 */

#ifndef NSSYNC_FETCH_H
#define NSSYNC_FETCH_H

#include <stdint.h>

#include <nssync/error.h>


enum nssync_fetcher_flags {
	NSSYNC_FETCHER_SYNC = 0,
	NSSYNC_FETCHER_ASYNC = 1,
};

struct nssync_fetcher_fetch;

/** retrive data from a uri.
 *
 * All inputs and outputs are provided to the fetcher routine within a
 *   nssync_fetcher_fetch structure. If additional context is required
 *   in the completion callback it is expected that this structure
 *   will be the first element in a wraping structure and cast as
 *   appropriate.
 *
 * The uri, username and password allow for a uri to be retrived with
 *   authentication.
 *
 * The data block may be provided or if NULL be allocated by the
 *   fetcher and should be a heap block with the length stored in
 *   data_size. The fether should set how much of the block is actually
 *   used in data_used.
 *
 * The fetcher routine must call the completion callback having set
 *   the result code. The completion callback should be used to complete
 *   the fetch. Usually this will include freeing the fetch structure.
 *
 * If asyncronous operation is requested by the caller the fetcher may
 *   use what ever mecanism is apropriate to cause the fetch operation
 *   to be started and return immediately to the caller with the
 *   NSSYNC_ERROR_RETRY code. Upon completion of the fetch (sucessful or
 *   otherwise) the completion will be triggered allowing for release of
 *   resources or flagging error status etc.
 */
typedef enum nssync_error(nssync_fetcher)(struct nssync_fetcher_fetch *fetch);

struct nssync_fetcher_fetch {
	enum nssync_fetcher_flags flags; /**< flags affecting the fetch */

	char *url; /**< url being retrived */
	char *username; /**< authentication username */
	char *password; /**< authentication password */

	void *data; /**< retrived data is stored. */
	size_t data_size; /**< size of data allocation */
	size_t data_used; /**< amount of allocation used for retrived data */

	nssync_fetcher *completion; /**< called upon completion of fetch */
	nssync_error result; /**< fetch result */
};


nssync_fetcher nssync_fetcher_curl;

#endif

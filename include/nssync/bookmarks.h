/*
 * This file is part of libnssync
 *
 * Copyright 20013 Vincent Sanders <vince@netsurf-browser.org>
 *
 * Released under MIT licence (see COPYING file)
 */

#ifndef NSSYNC_BOOKMARKS_H
#define NSSYNC_BOOKMARKS_H

struct nssync_sync_bookmarks;

enum nssync_error nssync_bookmarks_new(struct nssync_sync *sync, struct nssync_sync_bookmarks **sync_bookmarks);
enum nssync_error nssync_bookmarks_free(struct nssync_sync_bookmarks *sync_bookmarks);

#endif

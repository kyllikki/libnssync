/*
 * Copyright 2013 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnssync, http://www.netsurf-browser.org/
 *
 * Released under the Expat MIT License (see COPYING),
 *
 */

struct nssync_registration;

enum nssync_error nssync_registration_new(const char *server, const char *account, const char *password, struct nssync_registration **registration_out);

enum nssync_error nssync_registration_free(struct nssync_registration *registration);

char *nssync_registration_get_storage_server(struct nssync_registration *registration);
char *nssync_registration_get_username(struct nssync_registration *registration);
char *nssync_registration_get_password(struct nssync_registration *registration);

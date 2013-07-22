struct nssync_auth;

enum nnsync_error nssync_auth_new(const char *server, const char *account, const char *password, struct nssync_auth **auth_out);

enum nnsync_error nssync_auth_free(struct nssync_auth *auth);

char *nssync_auth_get_storage_server(struct nssync_auth *auth);
char *nssync_auth_get_username(struct nssync_auth *auth);
char *nssync_auth_get_password(struct nssync_auth *auth);


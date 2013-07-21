struct nssync_storage;
struct nssync_storage_obj;

struct nssync_storage *nssync_storage_new_auth(struct nssync_auth *auth, const char *pathname);
int nssync_storage_free(struct nssync_storage *store);

struct nssync_storage_obj *nssync_storage_obj_fetch(struct nssync_storage *store, const char *path);

int nssync_storage_obj_free(struct nssync_storage_obj *obj);

uint8_t *nssync_storage_obj_payload(struct nssync_storage_obj *obj);

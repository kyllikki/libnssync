struct nssync_storage;
struct nssync_storage_obj;

int nssync_storage_new(struct nssync_auth *auth, const char *pathname, struct nssync_storage **store_out);
int nssync_storage_free(struct nssync_storage *store);

int nssync_storage_obj_fetch(struct nssync_storage *store, const char *collection, const char *object, struct nssync_storage_obj **obj_out);

int nssync_storage_obj_free(struct nssync_storage_obj *obj);

uint8_t *nssync_storage_obj_payload(struct nssync_storage_obj *obj);

struct nssync_storage;
struct nssync_storage_obj;

struct nssync_storage *nssync_storage_new(const char *server, const char *pathname, const char* username, const char*password);
int nssync_storage_free(struct nssync_storage *store);

struct nssync_storage_obj *nssync_storage_obj_fetch(struct nssync_storage *store, const char *path);

int nssync_storage_obj_free(struct nssync_storage_obj *obj);

uint8_t *nssync_storage_obj_payload(struct nssync_storage_obj *obj);

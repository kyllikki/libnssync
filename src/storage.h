struct nssync_storage;
struct nssync_storage_obj;

/** create a new storage state for retriving objects */
nssync_error nssync_storage_new(struct nssync_registration *registration, const char *pathname, nssync_fetcher *fetcher, struct nssync_storage **store_out);
nssync_error nssync_storage_free(struct nssync_storage *store);

/** fetch storage object from storage server */
enum nssync_error nssync_storage_obj_fetch(struct nssync_storage *store, struct nssync_crypto_keybundle *keybundle, const char *collection, const char *object, struct nssync_storage_obj **obj_out);

int nssync_storage_obj_free(struct nssync_storage_obj *obj);

uint8_t *nssync_storage_obj_payload(struct nssync_storage_obj *obj);

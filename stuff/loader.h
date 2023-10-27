struct loader;

struct loader *loader_load(char *design_id);

int loader_is_done(struct loader *loader);

void loader_get(struct loader *loader, struct fcsim_arena *arena);

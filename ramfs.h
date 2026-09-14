#ifndef RAMFS_H
#define RAMFS_H

#include "vfs.h"

void        ramfs_init(void);
vfs_node_t *ramfs_create_file(vfs_node_t *parent, const char *name);
vfs_node_t *ramfs_create_dir (vfs_node_t *parent, const char *name);

#endif
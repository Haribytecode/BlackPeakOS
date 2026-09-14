#ifndef VFS_H
#define VFS_H
#include <stdint.h>
#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02
#define VFS_CHARDEV 0x04
struct vfs_node;

typedef struct vfs_ops{
    uint32_t (*read)(struct vfs_node *node,uint32_t offset,uint32_t size,uint8_t *buf);
    uint32_t (*write)(struct vfs_node *node,uint32_t offset,uint32_t size,uint8_t *buf);
    void     (*open)(struct vfs_node *node,uint32_t flags); 
    void     (*close)(struct vfs_node *node);
    struct vfs_node *(*readdir)(struct vfs_node *node,uint32_t index);
    struct vfs_node *(*finddir)(struct vfs_node *node,const char *name);
}vfs_ops_t;

typedef struct vfs_node{
    char name[64];
    uint32_t flags;
    uint32_t size;
    uint32_t inode;
    vfs_ops_t *ops;
    void *internal;
}vfs_node_t;

void vfs_init(void);
void vfs_mount(vfs_node_t *node);
vfs_node_t *vfs_root(void);
vfs_node_t *vfs_lookup(const char *path);

#endif

#include "vfs.h"
#include "console.h"
static vfs_node_t *root=0;

void vfs_init(void){
    root=0;
    kprint("VFS INITIALISED\n");
}
//at initial vfs is not mounted to any fs,

void vfs_mount(vfs_node_t *node){
    if(!node) return; //if node is empty return
    //else
    root=node;
    kprint("VFS MOUNTED: ");
    kprint(node->name); //printing the name of the root of the file system 
    kprint("\n");
}
vfs_node_t *vfs_root(void){
    return root;
}

vfs_node_t *vfs_lookup(const char *path)
{
    if (!root || !path) return 0;

    const char *p = path;
    if (*p == '/') p++;
    if (*p == '\0') return root;

    vfs_node_t *cur = root;
    char component[64];

    while (*p) {
        int i = 0;
        while (p[i] && p[i] != '/' && i < 63) {
            component[i] = p[i];
            i++;
        }
        component[i] = '\0';

        p += i;
        if (*p == '/') p++;

        if (!cur->ops || !cur->ops->finddir) return 0;
        cur = cur->ops->finddir(cur, component);
        if (!cur) return 0;
    }
    return cur;
}
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

vfs_node_t *vfs_lookup(const char *path){
    if(!root || !path) return 0; //if root node is empty or the path is empty i.e no directory flow there,then return 0
    if(path[0]=='/') path++; //means the first one we move just 
    if(path[0]=='\0') return root; //marks the end 
    vfs_node_t *cur=root;
    char component[64];
    while(*path){ //keeps processing until reaches end of component
        int i=0;
        while(path[i] && path[i]!='/' && i<63){  //just checking valid,inside ? ,also no overflow
            component[i]=path[i];
            i++;
        }
        //in c marking the end of string as \0 is mandatory as afterthat only it is a valid string
        component[i]='\0';
        path+=i;
        if(*path=='/') path++;
        if(!cur->ops || !cur->ops->finddir) return 0; //cur represents / the root compoenet egfile/folder
        cur=cur->ops->finddir(cur,component); //eg cur=/ then component=etc
        if(!cur) return 0;
    }
    return cur;

}
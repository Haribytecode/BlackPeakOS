#include "ramfs.h"
#include "heap.h"
#include "console.h"
#include "uart.h"
#define RAMFS_MAX_CHILDREN 64
#define RAMFS_MAX_FILESIZE 2048

typedef struct ramfs_dir {
    vfs_node_t *children[RAMFS_MAX_CHILDREN];
    uint32_t    count;
} ramfs_dir_t;

typedef struct ramfs_file {
    uint8_t *data;
} ramfs_file_t;

static uint32_t    ramfs_read   (vfs_node_t *n, uint32_t off, uint32_t sz, uint8_t *buf);
static uint32_t    ramfs_write  (vfs_node_t *n, uint32_t off, uint32_t sz, uint8_t *buf);
static vfs_node_t *ramfs_finddir(vfs_node_t *n, const char *name);
static vfs_node_t *ramfs_readdir(vfs_node_t *n, uint32_t index);

static vfs_ops_t ramfs_file_ops = {
    .read = ramfs_read, .write = ramfs_write,
    .open = 0, .close = 0, .readdir = 0, .finddir = 0,
};
static vfs_ops_t ramfs_dir_ops = {
    .read = 0, .write = 0,
    .open = 0, .close = 0, .readdir = ramfs_readdir, .finddir = ramfs_finddir,
};

static void str_copy(char *dst, const char *src, uint32_t max)
{
    uint32_t i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static int str_eq(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == '\0' && *b == '\0';
}

static vfs_node_t *alloc_node(const char *name, uint32_t flags)
{
    vfs_node_t *n = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!n) return 0;
    uint8_t *p = (uint8_t *)n;
    for (uint32_t i = 0; i < sizeof(vfs_node_t); i++) p[i] = 0;

    str_copy(n->name, name, sizeof(n->name));
    n->flags = flags;

    if (flags == VFS_DIRECTORY) {
        ramfs_dir_t *d = (ramfs_dir_t *)kmalloc(sizeof(ramfs_dir_t));
        if (!d) return 0;
        d->count = 0;
        for (uint32_t i = 0; i < RAMFS_MAX_CHILDREN; i++) d->children[i] = 0;
        n->ops = &ramfs_dir_ops;
        n->internal = d;
    }      else {
        ramfs_file_t *f = (ramfs_file_t *)kmalloc(sizeof(ramfs_file_t));
        if (!f) return 0;
        f->data = (uint8_t *)kmalloc(RAMFS_MAX_FILESIZE);
        if (f->data) {
            for (uint32_t i = 0; i < RAMFS_MAX_FILESIZE; i++) f->data[i] = 0;
        }
        n->ops = &ramfs_file_ops;
        n->internal = f;
    }
    return n;
}

static uint32_t ramfs_read(vfs_node_t *n, uint32_t off, uint32_t sz, uint8_t *buf)
{
    ramfs_file_t *f = (ramfs_file_t *)n->internal;
    if (!f || !f->data || off >= n->size) return 0;
    uint32_t avail = n->size - off;
    if (sz > avail) sz = avail;
    for (uint32_t i = 0; i < sz; i++) buf[i] = f->data[off + i];
    return sz;
}

static uint32_t ramfs_write(vfs_node_t *n, uint32_t off, uint32_t sz, uint8_t *buf)
{
    if (!n || !buf) return 0;
    if (off >= RAMFS_MAX_FILESIZE) return 0;
    if (sz > RAMFS_MAX_FILESIZE - off) sz = RAMFS_MAX_FILESIZE - off;

    ramfs_file_t *f = (ramfs_file_t *)n->internal;
    if (!f || !f->data) return 0;

    uint8_t *dst = f->data + off;
    for (uint32_t i = 0; i < sz; i++) dst[i] = buf[i];

    if (off + sz > n->size) n->size = off + sz;
    return sz;
}
static vfs_node_t *ramfs_finddir(vfs_node_t *n, const char *name)
{
    ramfs_dir_t *d = (ramfs_dir_t *)n->internal;
    if (!d) return 0;
    for (uint32_t i = 0; i < d->count; i++)
        if (d->children[i] && str_eq(d->children[i]->name, name))
            return d->children[i];
    return 0;
}

static vfs_node_t *ramfs_readdir(vfs_node_t *n, uint32_t idx)
{
    ramfs_dir_t *d = (ramfs_dir_t *)n->internal;
    if (!d || idx >= d->count) return 0;
    return d->children[idx];
}

void ramfs_attach(vfs_node_t *parent, vfs_node_t *child)
{
    ramfs_dir_t *d = (ramfs_dir_t *)parent->internal;
    if (!d || d->count >= RAMFS_MAX_CHILDREN) return;
    d->children[d->count++] = child;
}

vfs_node_t *ramfs_create_file(vfs_node_t *parent, const char *name)
{
    vfs_node_t *n = alloc_node(name, VFS_FILE);
    if (!n) return 0;
    if (parent) ramfs_attach(parent, n);
    return n;
}

vfs_node_t *ramfs_create_dir(vfs_node_t *parent, const char *name)
{
    vfs_node_t *n = alloc_node(name, VFS_DIRECTORY);
    if (!n) return 0;
    if (parent) ramfs_attach(parent, n);
    return n;
}

void ramfs_init(void)
{
    vfs_node_t *r = ramfs_create_dir(0, "/");
    if (!r) {
        kprint("RAMFS: root failed\n");
        return;
    }
    vfs_mount(r);
    kprint("RAMFS ready\n");
}
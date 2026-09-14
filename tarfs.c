#include "tarfs.h"
#include "ramfs.h"
#include "heap.h"
#include "console.h"

extern uint8_t initrd_start[];
extern uint8_t initrd_end[];

#define TAR_BLOCK_SIZE 512

typedef struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
} tar_header_t;

static uint32_t parse_octal(const char *s, uint32_t len)
{
    uint32_t v = 0;
    uint32_t i = 0;
    while (i < len && (s[i] == ' ' || s[i] == '0')) i++;
    while (i < len && s[i] >= '0' && s[i] <= '7') {
        v = v * 8 + (s[i] - '0');
        i++;
    }
    return v;
}

static int is_zero_block(const uint8_t *p)
{
    for (int i = 0; i < TAR_BLOCK_SIZE; i++)
        if (p[i] != 0) return 0;
    return 1;
}

static uint32_t tarfs_read(vfs_node_t *n, uint32_t off, uint32_t sz, uint8_t *buf)
{
    if (!n || !n->internal) return 0;
    uint8_t *data = (uint8_t *)n->internal;
    if (off >= n->size) return 0;
    uint32_t avail = n->size - off;
    if (sz > avail) sz = avail;
    for (uint32_t i = 0; i < sz; i++) buf[i] = data[off + i];
    return sz;
}

static vfs_node_t *tarfs_finddir(vfs_node_t *n, const char *name)
{
    if (!n || !n->ops || !n->ops->readdir) return 0;
    for (uint32_t i = 0; ; i++) {
        vfs_node_t *c = n->ops->readdir(n, i);
        if (!c) return 0;
        const char *a = c->name;
        const char *b = name;
        while (*a && *b && *a == *b) { a++; b++; }
        if (*a == '\0' && *b == '\0') return c;
    }
}

static vfs_node_t *tarfs_readdir(vfs_node_t *n, uint32_t idx)
{
    if (!n || !n->internal) return 0;
    vfs_node_t **list = (vfs_node_t **)n->internal;
    if (!list[idx]) return 0;
    return list[idx];
}

static vfs_ops_t tarfs_file_ops = {
    .read = tarfs_read, .write = 0,
    .open = 0, .close = 0,
    .readdir = 0, .finddir = 0,
};

static vfs_ops_t tarfs_dir_ops = {
    .read = 0, .write = 0,
    .open = 0, .close = 0,
    .readdir = tarfs_readdir, .finddir = tarfs_finddir,
};

static void str_copy_n(char *dst, const char *src, uint32_t max)
{
    uint32_t i = 0;
    while (i < max - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void tarfs_init(void)
{
    /* Root node is /initrd */
    vfs_node_t *root = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!root) { kprint("TARFS: root alloc fail\n"); return; }
    uint8_t *rp = (uint8_t *)root;
    for (uint32_t i = 0; i < sizeof(vfs_node_t); i++) rp[i] = 0;
    str_copy_n(root->name, "initrd", sizeof(root->name));
    root->flags = VFS_DIRECTORY;
    root->ops = &tarfs_dir_ops;

    /* Directory entry list — up to 64 files */
    vfs_node_t **list = (vfs_node_t **)kmalloc(sizeof(vfs_node_t *) * 64);
    if (!list) { kprint("TARFS: list alloc fail\n"); return; }
    for (int i = 0; i < 64; i++) list[i] = 0;
    root->internal = list;

    /* Walk the tar */
    
    uint8_t *p = initrd_start;
    uint32_t count = 0;

    while (p + TAR_BLOCK_SIZE <= initrd_end) {
        if (is_zero_block(p)) break;

        tar_header_t *h = (tar_header_t *)p;
        uint32_t size = parse_octal(h->size, 12);

        if (h->typeflag == '0' || h->typeflag == 0) {
            /* Regular file */
            vfs_node_t *f = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
            if (!f) break;
            uint8_t *fp = (uint8_t *)f;
            for (uint32_t i = 0; i < sizeof(vfs_node_t); i++) fp[i] = 0;

            str_copy_n(f->name, h->name, sizeof(f->name));
            f->flags = VFS_FILE;
            f->size = size;
            f->ops = &tarfs_file_ops;
            f->internal = p + TAR_BLOCK_SIZE;   /* file data */

            if (count < 63) list[count++] = f;
            list[count] = 0;
        }

        /* advance: header + data (rounded up to 512) */
        uint32_t data_blocks = (size + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE;
        p += TAR_BLOCK_SIZE + (data_blocks * TAR_BLOCK_SIZE);
    }
    
    /* Attach to VFS root */
    vfs_node_t *vfs_r = vfs_root();
    if (vfs_r) ramfs_attach(vfs_r, root);

    kprint("TARFS ready, files: ");
    kprint_dec(count);
    kprint("\n");
}
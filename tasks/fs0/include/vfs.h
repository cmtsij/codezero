#ifndef __VFS_H__
#define __VFS_H__

#include <fs.h>
#include <lib/malloc.h>
#include <l4/lib/list.h>
#include <memfs/memfs.h>

/*
 * FIXME:
 * These ought to be strings and split/comparison functions should
 * always use strings because formats like UTF-8 wouldn't work.
 */
#define VFS_STR_SEP		'/'
#define VFS_STR_PARDIR		".."
#define VFS_STR_CURDIR		"."
#define VFS_STR_XATDIR		"...."

/*
 * This is a temporary replacement for page cache support provided by mm0.
 * Normally mm0 tracks all vnode pages, but this is used to track pages in
 * directory vnodes, which are normally never mapped by tasks.
 */
extern struct memfs_superblock *memfs_superblock;
static inline void *vfs_alloc_dirpage(void)
{
	/*
	 * Urgh, we allocate from the block cache of memfs to store generic vfs directory
	 * pages. This is currently the quickest we can allocate page-aligned memory.
	 */
	return memfs_alloc_block(memfs_superblock);
}

static inline void vfs_free_dirpage(void *block)
{
	memfs_free_block(memfs_superblock, block);
}

static inline struct dentry *vfs_alloc_dentry(void)
{
	struct dentry *d = kzalloc(sizeof(struct dentry));

	INIT_LIST_HEAD(&d->child);
	INIT_LIST_HEAD(&d->children);
	INIT_LIST_HEAD(&d->vref);

	return d;
}

static inline void vfs_free_dentry(struct dentry *d)
{
	return kfree(d);
}

static inline struct vnode *vfs_alloc_vnode(void)
{
	struct vnode *v = kzalloc(sizeof(struct vnode));

	INIT_LIST_HEAD(&v->dentries);
	INIT_LIST_HEAD(&v->state_list);

	return v;
}

static inline void vfs_free_vnode(struct vnode *v)
{
	kfree(v);
}

static inline struct superblock *vfs_alloc_superblock(void)
{
	struct superblock *sb = kmalloc(sizeof(struct superblock));
	INIT_LIST_HEAD(&sb->list);

	return sb;
}

struct vfs_mountpoint {
	struct superblock *sb;	/* The superblock of mounted filesystem */
	struct vnode *pivot;	/* The dentry upon which we mount */
};

extern struct vfs_mountpoint vfs_root;

int vfs_mount_root(struct superblock *sb);
struct vnode *generic_vnode_lookup(struct vnode *thisnode, char *path);
struct vnode *vfs_lookup(struct superblock *sb, char *path);

#endif /* __VFS_H__ */
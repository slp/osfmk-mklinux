/*
 * linux/fs/hfs/dir_dbl.c
 *
 * Copyright (C) 1995-1997  Paul H. Hargrove
 * This file may be distributed under the terms of the GNU Public License.
 *
 * This file contains the inode_operations and file_operations
 * structures for HFS directories.
 *
 * Based on the minix file system code, (C) 1991, 1992 by Linus Torvalds
 *
 * "XXX" in a comment is a note to myself to consider changing something.
 *
 * In function preconditions the term "valid" applied to a pointer to
 * a structure means that the pointer is non-NULL and the structure it
 * points to has all fields initialized to consistent values.
 */

#include "hfs.h"
#include <linux/hfs_fs_sb.h>
#include <linux/hfs_fs_i.h>
#include <linux/hfs_fs.h>

/*================ Forward declarations ================*/

static int dbl_lookup(struct inode *, const char *, int, struct inode **);
static int dbl_readdir(struct inode *, struct file *, void *, filldir_t);
static int dbl_create(struct inode *, const char *, int, int, struct inode **);
static int dbl_mkdir(struct inode *, const char *, int, int);
static int dbl_mknod(struct inode *, const char *, int, int, int);
static int dbl_unlink(struct inode *, const char *, int);
static int dbl_rmdir(struct inode *, const char *, int);
static int dbl_rename(struct inode *, const char *, int,
		      struct inode *, const char *, int, int);

/*================ Global variables ================*/

#define DOT_LEN			1
#define DOT_DOT_LEN		2
#define ROOTINFO_LEN		8
#define PCNT_ROOTINFO_LEN	9

const struct hfs_name hfs_dbl_reserved1[] = {
	{DOT_LEN,		"."},
	{DOT_DOT_LEN,		".."},
	{0,			""},
};

const struct hfs_name hfs_dbl_reserved2[] = {
	{ROOTINFO_LEN,		"RootInfo"},
	{PCNT_ROOTINFO_LEN,	"%RootInfo"},
	{0,			""},
};

#define DOT		(&hfs_dbl_reserved1[0])
#define DOT_DOT		(&hfs_dbl_reserved1[1])
#define ROOTINFO	(&hfs_dbl_reserved2[0])
#define PCNT_ROOTINFO	(&hfs_dbl_reserved2[1])

static struct file_operations hfs_dbl_dir_operations = {
	NULL,			/* lseek - default */
	hfs_dir_read,		/* read - invalid */
	NULL,			/* write - bad */
	dbl_readdir,		/* readdir */
	NULL,			/* select - default */
	NULL,			/* ioctl - default */
	NULL,			/* mmap - none */
	NULL,			/* no special open code */
	NULL,			/* no special release code */
	file_fsync,		/* fsync - default */
        NULL,			/* fasync - default */
        NULL,			/* check_media_change - none */
        NULL			/* revalidate - none */
};

struct inode_operations hfs_dbl_dir_inode_operations = {
	&hfs_dbl_dir_operations,/* default directory file-ops */
	dbl_create,		/* create */
	dbl_lookup,		/* lookup */
	NULL,			/* link */
	dbl_unlink,		/* unlink */
	NULL,			/* symlink */
	dbl_mkdir,		/* mkdir */
	dbl_rmdir,		/* rmdir */
	dbl_mknod,		/* mknod */
	dbl_rename,		/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	NULL,			/* readpage */
	NULL,			/* writepage */
	NULL,			/* bmap */
	NULL,			/* truncate */
	NULL,			/* permission */
	NULL			/* smap */
};

/*================ File-local functions ================*/

/*
 * is_hdr()
 */
static int is_hdr(struct inode *dir, const char *name, int len)
{
	int retval = 0;

	if (name[0] == '%') {
		struct hfs_cat_entry *entry = HFS_I(dir)->entry;
		struct hfs_cat_entry *victim;
		struct hfs_name cname;
		struct hfs_cat_key key;

		hfs_nameout(dir, &cname, name+1, len-1);
		hfs_cat_build_key(entry->cnid, &cname, &key);
		if ((victim = hfs_cat_get(entry->mdb, &key))) {
			hfs_cat_put(victim);
			retval = 1;
		}
	}
	return retval;
}

/*
 * dbl_lookup()
 *
 * This is the lookup() entry in the inode_operations structure for
 * HFS directories in the AppleDouble scheme.  The purpose is to
 * generate the inode corresponding to an entry in a directory, given
 * the inode for the directory and the name (and its length) of the
 * entry.
 */
static int dbl_lookup(struct inode * dir, const char * name,
		      int len, struct inode ** result)
{
	struct hfs_name cname;
	struct hfs_cat_entry *entry;
	struct hfs_cat_key key;
	struct inode *inode = NULL;
	
	if (!dir || !S_ISDIR(dir->i_mode)) {
		goto done;
	}

	entry = HFS_I(dir)->entry;
	
	if (len && !name) {
		*result = NULL;
		iput(dir);
		return -EINVAL;
	}

	/* Perform name-mangling */
	hfs_nameout(dir, &cname, name, len);
 
	/* Check for "." */
	if (hfs_streq(&cname, DOT)) {
		/* this little trick skips the iget and iput */
		*result = dir;
		return 0;
	}

	/* Check for "..". */
	if (hfs_streq(&cname, DOT_DOT)) {
		inode = __hfs_iget(hfs_cat_parent(entry), HFS_DBL_DIR, 0);
		goto done;
	}

	/* Check for "%RootInfo" if in the root directory. */
	if ((entry->cnid == htonl(HFS_ROOT_CNID)) &&
	    hfs_streq(&cname, PCNT_ROOTINFO)) {
		++entry->count; /* __hfs_iget() eats one */
		inode = __hfs_iget(entry, HFS_DBL_HDR, 0);
		goto done;
	}

	/* Do an hfs_iget() on the mangled name. */
	hfs_cat_build_key(entry->cnid, &cname, &key);
	inode = hfs_iget(entry->mdb, &key, HFS_DBL_NORM);

	/* Try as a header if not found and first character is '%' */
	if (!inode && (name[0] == '%')) {
		hfs_nameout(dir, &cname, name+1, len-1);
		hfs_cat_build_key(entry->cnid, &cname, &key);
		inode = hfs_iget(entry->mdb, &key, HFS_DBL_HDR);
	}
	
done:
	iput(dir);
	*result = inode;
	return inode ? 0 : -ENOENT;
}

/*
 * dbl_readdir()
 *
 * This is the readdir() entry in the file_operations structure for
 * HFS directories in the AppleDouble scheme.  The purpose is to
 * enumerate the entries in a directory, given the inode of the
 * directory and a (struct file *), the 'f_pos' field of which
 * indicates the location in the directory.  The (struct file *) is
 * updated so that the next call with the same 'dir' and 'filp'
 * arguments will produce the next directory entry.  The entries are
 * returned in 'dirent', which is "filled-in" by calling filldir().
 * This allows the same readdir() function be used for different
 * formats.  We try to read in as many entries as we can before
 * filldir() refuses to take any more.
 *
 * XXX: In the future it may be a good idea to consider not generating
 * metadata files for covered directories since the data doesn't
 * correspond to the mounted directory.	 However this requires an
 * iget() for every directory which could be considered an excessive
 * amount of overhead.	Since the inode for a mount point is always
 * in-core this is another argument for a call to get an inode if it
 * is in-core or NULL if it is not.
 */
static int dbl_readdir(struct inode * dir, struct file * filp,
		       void * dirent, filldir_t filldir)
{
	struct hfs_brec brec;
        struct hfs_cat_entry *entry;

	if (!dir || !dir->i_sb || !S_ISDIR(dir->i_mode)) {
		return -EBADF;
	}

        entry = HFS_I(dir)->entry;

	if (filp->f_pos == 0) {
		/* Entry 0 is for "." */
		if (filldir(dirent, DOT->Name, DOT_LEN, 0, dir->i_ino)) {
			return 0;
		}
		filp->f_pos = 1;
	}

	if (filp->f_pos == 1) {
		/* Entry 1 is for ".." */
		if (filldir(dirent, DOT_DOT->Name, DOT_DOT_LEN, 1,
			    hfs_get_hl(entry->key.ParID))) {
			return 0;
		}
		filp->f_pos = 2;
	}

	if (filp->f_pos < (dir->i_size - 1)) {
                hfs_u32 cnid;
                hfs_u8 type;

		if (hfs_cat_open(entry, &brec) ||
		    hfs_cat_next(entry, &brec, (filp->f_pos - 1) >> 1,
				 &cnid, &type)) {
			return 0;
		}

		while (filp->f_pos < (dir->i_size - 1)) {
			ino_t ino;
			int is_hdr = (filp->f_pos & 1);
			unsigned int len;
			unsigned char tmp_name[HFS_NAMEMAX + 1];

			if (is_hdr) {
				ino = ntohl(cnid) | HFS_DBL_HDR;
				tmp_name[0] = '%';
				len = 1 + hfs_namein(dir, tmp_name + 1,
				    &((struct hfs_cat_key *)brec.key)->CName);
			} else {
				if (hfs_cat_next(entry, &brec, 1,
							&cnid, &type)) {
					return 0;
				}
				ino = ntohl(cnid);
				len = hfs_namein(dir, tmp_name,
				    &((struct hfs_cat_key *)brec.key)->CName);
			}

			if (filldir(dirent, tmp_name, len, filp->f_pos, ino)) {
				hfs_cat_close(entry, &brec);
				return 0;
			}
			++filp->f_pos;
		}
		hfs_cat_close(entry, &brec);
	}

	if (filp->f_pos == (dir->i_size - 1)) {
		if (entry->cnid == htonl(HFS_ROOT_CNID)) {
			/* In root dir last entry is for "%RootInfo" */
			if (filldir(dirent, PCNT_ROOTINFO->Name,
				    PCNT_ROOTINFO_LEN, filp->f_pos,
				    ntohl(entry->cnid) | HFS_DBL_HDR)) {
				return 0;
			}
		}
		++filp->f_pos;
	}

	return 0;
}

/*
 * dbl_create()
 *
 * This is the create() entry in the inode_operations structure for
 * AppleDouble directories.  The purpose is to create a new file in
 * a directory and return a corresponding inode, given the inode for
 * the directory and the name (and its length) of the new file.
 */
static int dbl_create(struct inode * dir, const char * name, int len,
		      int mode, struct inode **result)
{
	int error;

	*result = NULL;
	if (is_hdr(dir, name, len)) {
		iput(dir);
		error = -EEXIST;
	} else {
		error = hfs_create(dir, name, len, mode, result);
	}
	return error;
}

/*
 * dbl_mkdir()
 *
 * This is the mkdir() entry in the inode_operations structure for
 * AppleDouble directories.  The purpose is to create a new directory
 * in a directory, given the inode for the parent directory and the
 * name (and its length) of the new directory.
 */
static int dbl_mkdir(struct inode * parent, const char * name,
		     int len, int mode)
{
	int error;

	if (is_hdr(parent, name, len)) {
		iput(parent);
		error = -EEXIST;
	} else {
		error = hfs_mkdir(parent, name, len, mode);
	}
	return error;
}

/*
 * dbl_mknod()
 *
 * This is the mknod() entry in the inode_operations structure for
 * regular HFS directories.  The purpose is to create a new entry
 * in a directory, given the inode for the parent directory and the
 * name (and its length) and the mode of the new entry (and the device
 * number if the entry is to be a device special file).
 */
static int dbl_mknod(struct inode *dir, const char *name,
		     int len, int mode, int rdev)
{
	int error;

	if (is_hdr(dir, name, len)) {
		iput(dir);
		error = -EEXIST;
	} else {
		error = hfs_mknod(dir, name, len, mode, rdev);
	}
	return error;
}

/*
 * dbl_unlink()
 *
 * This is the unlink() entry in the inode_operations structure for
 * AppleDouble directories.  The purpose is to delete an existing
 * file, given the inode for the parent directory and the name
 * (and its length) of the existing file.
 */
static int dbl_unlink(struct inode * dir, const char * name, int len)
{
	int error;

	++dir->i_count;
	error = hfs_unlink(dir, name, len);
	if ((error == -ENOENT) && is_hdr(dir, name, len)) {
		error = -EPERM;
	}
	iput(dir);
	return error;
}

/*
 * dbl_rmdir()
 *
 * This is the rmdir() entry in the inode_operations structure for
 * AppleDouble directories.  The purpose is to delete an existing
 * directory, given the inode for the parent directory and the name
 * (and its length) of the existing directory.
 */
static int dbl_rmdir(struct inode * parent, const char * name, int len)
{
	int error;

	++parent->i_count;
	error = hfs_rmdir(parent, name, len);
	if ((error == -ENOENT) && is_hdr(parent, name, len)) {
		error = -ENOTDIR;
	}
	iput(parent);
	return error;
}

/*
 * dbl_rename()
 *
 * This is the rename() entry in the inode_operations structure for
 * AppleDouble directories.  The purpose is to rename an existing
 * file or directory, given the inode for the current directory and
 * the name (and its length) of the existing file/directory and the
 * inode for the new directory and the name (and its length) of the
 * new file/directory.
 */
static int dbl_rename(struct inode *old_dir, const char *old_name, int old_len,
		      struct inode *new_dir, const char *new_name, int new_len,
		      int must_be_dir)
{
	int error;

	if (is_hdr(new_dir, new_name, new_len)) {
		error = -EPERM;
	} else {
		++old_dir->i_count;
		++new_dir->i_count;
		error = hfs_rename(old_dir, old_name, old_len,
				   new_dir, new_name, new_len, must_be_dir);
		if ((error == -ENOENT) && !must_be_dir &&
		    is_hdr(old_dir, old_name, old_len)) {
			error = -EPERM;
		}
	}
	iput(old_dir);
	iput(new_dir);
	return error;
}

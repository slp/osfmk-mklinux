/*
 * linux/fs/hfs/dir_nat.c
 *
 * Copyright (C) 1995-1997  Paul H. Hargrove
 * This file may be distributed under the terms of the GNU Public License.
 *
 * This file contains the inode_operations and file_operations
 * structures for HFS directories.
 *
 * Based on the minix file system code, (C) 1991, 1992 by Linus Torvalds
 *
 * The source code distributions of Netatalk, versions 1.3.3b2 and
 * 1.4b2, were used as a specification of the location and format of
 * files used by Netatalk's afpd.  No code from Netatalk appears in
 * hfs_fs.  hfs_fs is not a work ``derived'' from Netatalk in the
 * sense of intellectual property law.
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

static int nat_lookup(struct inode *, const char *, int, struct inode **);
static int nat_readdir(struct inode *, struct file *, void *, filldir_t);
static int nat_rmdir(struct inode *, const char *, int);
static int nat_hdr_unlink(struct inode *, const char *, int);
static int nat_hdr_rename(struct inode *, const char *, int,
			  struct inode *, const char *, int, int);

/*================ Global variables ================*/

#define DOT_LEN			1
#define DOT_DOT_LEN		2
#define DOT_APPLEDOUBLE_LEN	12
#define DOT_PARENT_LEN		7

const struct hfs_name hfs_nat_reserved1[] = {
	{DOT_LEN,		"."},
	{DOT_DOT_LEN,		".."},
	{DOT_APPLEDOUBLE_LEN,	".AppleDouble"},
	{DOT_PARENT_LEN,	".Parent"},
	{0,			""},
};

const struct hfs_name hfs_nat_reserved2[] = {
	{0,			""},
};

#define DOT		(&hfs_nat_reserved1[0])
#define DOT_DOT		(&hfs_nat_reserved1[1])
#define DOT_APPLEDOUBLE	(&hfs_nat_reserved1[2])
#define DOT_PARENT	(&hfs_nat_reserved1[3])

static struct file_operations hfs_nat_dir_operations = {
	NULL,			/* lseek - default */
	hfs_dir_read,		/* read - invalid */
	NULL,			/* write - bad */
	nat_readdir,		/* readdir */
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

struct inode_operations hfs_nat_ndir_inode_operations = {
	&hfs_nat_dir_operations,/* default directory file-ops */
	hfs_create,		/* create */
	nat_lookup,		/* lookup */
	NULL,			/* link */
	hfs_unlink,		/* unlink */
	NULL,			/* symlink */
	hfs_mkdir,		/* mkdir */
	nat_rmdir,		/* rmdir */
	hfs_mknod,		/* mknod */
	hfs_rename,		/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	NULL,			/* readpage */
	NULL,			/* writepage */
	NULL,			/* bmap */
	NULL,			/* truncate */
	NULL,			/* permission */
	NULL			/* smap */
};

struct inode_operations hfs_nat_hdir_inode_operations = {
	&hfs_nat_dir_operations,/* default directory file-ops */
	hfs_create,		/* create */
	nat_lookup,		/* lookup */
	NULL,			/* link */
	nat_hdr_unlink,		/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	nat_hdr_rename,		/* rename */
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
 * nat_lookup()
 *
 * This is the lookup() entry in the inode_operations structure for
 * HFS directories in the Netatalk scheme.  The purpose is to generate
 * the inode corresponding to an entry in a directory, given the inode
 * for the directory and the name (and its length) of the entry.
 */
static int nat_lookup(struct inode * dir, const char * name,
		      int len, struct inode ** result)
{
	ino_t dtype;
	struct hfs_name cname;
	struct hfs_cat_entry *entry;
	struct hfs_cat_key key;
	struct inode *inode = NULL;

	if (!dir || !S_ISDIR(dir->i_mode)) {
		goto done;
	}

	entry = HFS_I(dir)->entry;
	dtype = HFS_ITYPE(dir->i_ino);

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
		struct hfs_cat_entry *parent;

		if (dtype != HFS_NAT_NDIR) {
			/* Case for ".." in ".AppleDouble" */
			parent = entry;
			++entry->count; /* __hfs_iget() eats one */
		} else {
			/* Case for ".." in a normal directory */
			parent = hfs_cat_parent(entry);
		}
		inode = __hfs_iget(parent, HFS_NAT_NDIR, 0);
		goto done;
	}

	/* Check for ".AppleDouble" if in a normal directory,
	   and for ".Parent" in ".AppleDouble". */
	if (dtype==HFS_NAT_NDIR) {
		/* Check for ".AppleDouble" */
		if (hfs_streq(&cname, DOT_APPLEDOUBLE)) {
			++entry->count; /* __hfs_iget() eats one */
			inode = __hfs_iget(entry, HFS_NAT_HDIR, 1);
			goto done;
		}
	} else if (dtype==HFS_NAT_HDIR) {
		if (hfs_streq(&cname, DOT_PARENT)) {
			++entry->count; /* __hfs_iget() eats one */
			inode = __hfs_iget(entry, HFS_NAT_HDR, 0);
			goto done;
		}
	}

	/* Do an hfs_iget() on the mangled name. */
	hfs_cat_build_key(entry->cnid, &cname, &key);
	inode = hfs_iget(entry->mdb, &key, HFS_I(dir)->file_type);

	/* Don't return a header file for a directory other than .Parent */
	if (inode && (dtype == HFS_NAT_HDIR) &&
	    (HFS_I(inode)->entry != entry) &&
	    (HFS_I(inode)->entry->type == HFS_CDR_DIR)) {
		iput(inode);
		inode = NULL;
	}

done:
	iput(dir);
	*result = inode;
	return inode ? 0 : -ENOENT;
}

/*
 * nat_readdir()
 *
 * This is the readdir() entry in the file_operations structure for
 * HFS directories in the netatalk scheme.  The purpose is to
 * enumerate the entries in a directory, given the inode of the
 * directory and a struct file which indicates the location in the
 * directory.  The struct file is updated so that the next call with
 * the same dir and filp will produce the next directory entry.	 The
 * entries are returned in dirent, which is "filled-in" by calling
 * filldir().  This allows the same readdir() function be used for
 * different dirent formats.  We try to read in as many entries as we
 * can before filldir() refuses to take any more.
 *
 * Note that the Netatalk format doesn't have the problem with
 * metadata for covered directories that exists in the other formats,
 * since the metadata is contained within the directory.
 */
static int nat_readdir(struct inode * dir, struct file * filp,
		       void * dirent, filldir_t filldir)
{
	ino_t type;
	int skip_dirs;
	struct hfs_brec brec;
        struct hfs_cat_entry *entry;

	if (!dir || !dir->i_sb || !S_ISDIR(dir->i_mode)) {
		return -EBADF;
	}

        entry = HFS_I(dir)->entry;
	type = HFS_ITYPE(dir->i_ino);
	skip_dirs = (type == HFS_NAT_HDIR);

	if (filp->f_pos == 0) {
		/* Entry 0 is for "." */
		if (filldir(dirent, DOT->Name, DOT_LEN, 0, dir->i_ino)) {
			return 0;
		}
		filp->f_pos = 1;
	}

	if (filp->f_pos == 1) {
		/* Entry 1 is for ".." */
		hfs_u32 cnid;

		if (type == HFS_NAT_NDIR) {
			cnid = hfs_get_nl(entry->key.ParID);
		} else {
			cnid = entry->cnid;
		}

		if (filldir(dirent, DOT_DOT->Name,
			    DOT_DOT_LEN, 1, ntohl(cnid))) {
			return 0;
		}
		filp->f_pos = 2;
	}

	if (filp->f_pos < (dir->i_size - 1)) {
		hfs_u32 cnid;
		hfs_u8 type;

	    	if (hfs_cat_open(entry, &brec) ||
		    hfs_cat_next(entry, &brec, filp->f_pos - 2, &cnid, &type)) {
			return 0;
		}
		while (filp->f_pos < (dir->i_size - 1)) {
			if (hfs_cat_next(entry, &brec, 1, &cnid, &type)) {
				return 0;
			}
			if (!skip_dirs || (type != HFS_CDR_DIR)) {
				ino_t ino;
				unsigned int len;
				unsigned char tmp_name[HFS_NAMEMAX];

				ino = ntohl(cnid) | HFS_I(dir)->file_type;
				len = hfs_namein(dir, tmp_name,
				    &((struct hfs_cat_key *)brec.key)->CName);
				if (filldir(dirent, tmp_name, len,
					    filp->f_pos, ino)) {
					hfs_cat_close(entry, &brec);
					return 0;
				}
			}
			++filp->f_pos;
		}
		hfs_cat_close(entry, &brec);
	}

	if (filp->f_pos == (dir->i_size - 1)) {
		if (type == HFS_NAT_NDIR) {
			/* In normal dirs entry 2 is for ".AppleDouble" */
			if (filldir(dirent, DOT_APPLEDOUBLE->Name,
				    DOT_APPLEDOUBLE_LEN, filp->f_pos,
				    ntohl(entry->cnid) | HFS_NAT_HDIR)) {
				return 0;
			}
		} else if (type == HFS_NAT_HDIR) {
			/* In .AppleDouble entry 2 is for ".Parent" */
			if (filldir(dirent, DOT_PARENT->Name,
				    DOT_PARENT_LEN, filp->f_pos,
				    ntohl(entry->cnid) | HFS_NAT_HDR)) {
				return 0;
			}
		}
		++filp->f_pos;
	}

	return 0;
}

/*
 * nat_rmdir()
 *
 * This is the rmdir() entry in the inode_operations structure for
 * Netatalk directories.  The purpose is to delete an existing
 * directory, given the inode for the parent directory and the name
 * (and its length) of the existing directory.
 *
 * We handle .AppleDouble and call hfs_rmdir() for all other cases.
 */
static int nat_rmdir(struct inode *parent, const char *name, int len)
{
	struct hfs_cat_entry *entry = HFS_I(parent)->entry;
	struct hfs_name cname;
	int error;

	hfs_nameout(parent, &cname, name, len);
	if (hfs_streq(&cname, DOT_APPLEDOUBLE)) {
		if (!HFS_SB(parent->i_sb)->s_afpd) {
			/* Not in AFPD compatibility mode */
			error = -EPERM;
		} else if (entry->u.dir.files || entry->u.dir.dirs) {
			/* AFPD compatible, but the directory is not empty */
			error = -ENOTEMPTY;
		} else {
			/* AFPD compatible, so pretend to succeed */
			error = 0;
		}
		iput(parent);
	} else {
		error = hfs_rmdir(parent, name, len);
	}
	return error;
}

/*
 * nat_hdr_unlink()
 *
 * This is the unlink() entry in the inode_operations structure for
 * Netatalk .AppleDouble directories.  The purpose is to delete an
 * existing file, given the inode for the parent directory and the name
 * (and its length) of the existing file.
 *
 * WE DON'T ACTUALLY DELETE HEADER THE FILE.
 * In non-afpd-compatible mode:
 *   We return -EPERM.
 * In afpd-compatible mode:
 *   We return success if the file exists or is .Parent.
 *   Otherwise we return -ENOENT.
 */
static int nat_hdr_unlink(struct inode *dir, const char *name, int len)
{
	struct hfs_cat_entry *entry = HFS_I(dir)->entry;
	int error = 0;

	if (!HFS_SB(dir->i_sb)->s_afpd) {
		/* Not in AFPD compatibility mode */
		error = -EPERM;
	} else {
		struct hfs_name cname;

		hfs_nameout(dir, &cname, name, len);
		if (!hfs_streq(&cname, DOT_PARENT)) {
			struct hfs_cat_entry *victim;
			struct hfs_cat_key key;

			hfs_cat_build_key(entry->cnid, &cname, &key);
			victim = hfs_cat_get(entry->mdb, &key);

			if (victim) {
				/* pretend to succeed */
				hfs_cat_put(victim);
			} else {
				error = -ENOENT;
			}
		}
	}
	iput(dir);
	return error;
}

/*
 * nat_hdr_rename()
 *
 * This is the rename() entry in the inode_operations structure for
 * Netatalk header directories.  The purpose is to rename an existing
 * file given the inode for the current directory and the name 
 * (and its length) of the existing file and the inode for the new
 * directory and the name (and its length) of the new file/directory.
 *
 * WE NEVER MOVE ANYTHING.
 * In non-afpd-compatible mode:
 *   We return -EPERM.
 * In afpd-compatible mode:
 *   If the source header doesn't exist, we return -ENOENT.
 *   If the destination is not a header directory we return -EPERM.
 *   We return success if the destination is also a header directory
 *    and the header exists or is ".Parent".
 */
static int nat_hdr_rename(struct inode *old_dir, const char *old_name,
			  int old_len, struct inode *new_dir,
			  const char *new_name, int new_len, int must_be_dir)
{
	struct hfs_cat_entry *entry = HFS_I(old_dir)->entry;
	int error = 0;

	if (!HFS_SB(old_dir->i_sb)->s_afpd) {
		/* Not in AFPD compatibility mode */
		error = -EPERM;
	} else {
		struct hfs_name cname;

		hfs_nameout(old_dir, &cname, old_name, old_len);
		if (!hfs_streq(&cname, DOT_PARENT)) {
			struct hfs_cat_entry *victim;
			struct hfs_cat_key key;

			hfs_cat_build_key(entry->cnid, &cname, &key);
			victim = hfs_cat_get(entry->mdb, &key);

			if (victim) {
				/* pretend to succeed */
				hfs_cat_put(victim);
			} else {
				error = -ENOENT;
			}
		}

		if (!error && (HFS_ITYPE(new_dir->i_ino) != HFS_NAT_HDIR)) {
			error = -EPERM;
		}
	}
	iput(old_dir);
	iput(new_dir);
	return error;
}

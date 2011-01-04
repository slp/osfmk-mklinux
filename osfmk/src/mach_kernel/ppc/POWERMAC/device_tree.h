/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

#ifndef __DEVICE_TREE__
#define __DEVICE_TREE__


/*
-------------------------------------------------------------------------------
 Foundation Types
-------------------------------------------------------------------------------
*/
enum {
	kDTPathNameSeparator	= '/'				/* 0x2F */
};


/* Property Name Definitions (Property Names are C-Strings)*/
enum {
	kDTMaxPropertyNameLength=31	/* Max length of Property Name (terminator not included) */
};

typedef char DTPropertyNameBuf[32];


/* Entry Name Definitions (Entry Names are C-Strings)*/
enum {
	kDTMaxEntryNameLength		= 31	/* Max length of a C-String Entry Name (terminator not included) */
};

/* length of DTEntryNameBuf = kDTMaxEntryNameLength +1*/
typedef char DTEntryNameBuf[32];


/* Entry*/
typedef struct OpaqueDTEntry* DTEntry;

/* Entry Iterator*/
typedef struct OpaqueDTEntryIterator* DTEntryIterator;

/* Property Iterator*/
typedef struct OpaqueDTPropertyIterator* DTPropertyIterator;


/* status values*/
enum {
		kError = -1,
		kIterationDone = 0,
		kSuccess = 1
};

/*

Structures for a Flattened Device Tree
 */

#define kPropNameLength	32

typedef struct DeviceTreeNodeProperty {
    char		name[kPropNameLength];	// NUL terminated property name
    unsigned long	length;		// Length (bytes) of folloing prop value
//  unsigned long	value[1];	// Variable length value of property
					// Padded to a multiple of a longword?
} DeviceTreeNodeProperty;

typedef struct OpaqueDTEntry {
    unsigned long	nProperties;	// Number of props[] elements (0 => end)
    unsigned long	nChildren;	// Number of children[] elements
//  DeviceTreeNodeProperty	props[];// array size == nProperties
//  DeviceTreeNode	children[];	// array size == nChildren
} DeviceTreeNode;


#ifndef	__MWERKS__
/*
-------------------------------------------------------------------------------
 Device Tree Calls
-------------------------------------------------------------------------------
*/

/*
-------------------------------------------------------------------------------
 Entry Handling
-------------------------------------------------------------------------------
*/
/* Compare two Entry's for equality. */
extern int DTEntryIsEqual(const DTEntry ref1, const DTEntry ref2);

/*
-------------------------------------------------------------------------------
 LookUp Entry by Name
-------------------------------------------------------------------------------
*/
/*
 Lookup Entry
 Locates an entry given a specified subroot (searchPoint) and path name.  If the
 searchPoint pointer is NULL, the path name is assumed to be an absolute path
 name rooted to the root of the device tree.
*/
extern int DTLookupEntry(const DTEntry searchPoint, const char *pathName, DTEntry *foundEntry);

/*
-------------------------------------------------------------------------------
 Entry Iteration
-------------------------------------------------------------------------------
*/
/*
 An Entry Iterator maintains three variables that are of interest to clients.
 First is an "OutermostScope" which defines the outer boundry of the iteration.
 This is defined by the starting entry and includes that entry plus all of it's
 embedded entries. Second is a "currentScope" which is the entry the iterator is
 currently in. And third is a "currentPosition" which is the last entry returned
 during an iteration.

 Create Entry Iterator
 Create the iterator structure. The outermostScope and currentScope of the iterator
 are set to "startEntry".  If "startEntry" = NULL, the outermostScope and
 currentScope are set to the root entry.  The currentPosition for the iterator is
 set to "nil".
*/
extern int DTCreateEntryIterator(const DTEntry startEntry, DTEntryIterator *iterator);

/* Dispose Entry Iterator*/
extern int DTDisposeEntryIterator(DTEntryIterator iterator);

/*
 Enter Child Entry
 Move an Entry Iterator into the scope of a specified child entry.  The
 currentScope of the iterator is set to the entry specified in "childEntry".  If
 "childEntry" is nil, the currentScope is set to the entry specified by the
 currentPosition of the iterator.
*/
extern int DTEnterEntry(DTEntryIterator iterator, DTEntry childEntry);

/*
 Exit to Parent Entry
 Move an Entry Iterator out of the current entry back into the scope of it's parent
 entry. The currentPosition of the iterator is reset to the current entry (the
 previous currentScope), so the next iteration call will continue where it left off.
 This position is returned in parameter "currentPosition".
*/
extern int DTExitEntry(DTEntryIterator iterator, DTEntry *currentPosition);

/*
 Iterate Entries 
 Iterate and return entries contained within the entry defined by the current
 scope of the iterator.  Entries are returned one at a time. When
 int == kIterationDone, all entries have been exhausted, and the
 value of nextEntry will be Nil. 
*/
extern int DTIterateEntries(DTEntryIterator iterator, DTEntry *nextEntry);

/*
 Restart Entry Iteration
 Restart an iteration within the current scope.  The iterator is reset such that
 iteration of the contents of the currentScope entry can be restarted. The
 outermostScope and currentScope of the iterator are unchanged. The currentPosition
 for the iterator is set to "nil".
*/
extern int DTRestartEntryIteration(DTEntryIterator iterator);

/*
-------------------------------------------------------------------------------
 Get Property Values
-------------------------------------------------------------------------------
*/
/*
 Get the value of the specified property for the specified entry.  

 Get Property
*/
extern int DTGetProperty(const DTEntry entry, const char *propertyName, void **propertyValue, int *propertySize);

/*
-------------------------------------------------------------------------------
 Iterating Properties
-------------------------------------------------------------------------------
*/
/*
 Create Property Iterator
 Create the property iterator structure. The target entry is defined by entry.
*/

extern int DTCreatePropertyIterator(const DTEntry entry,
					DTPropertyIterator *iterator);

/* Dispose Property Iterator*/
extern int DTDisposePropertyIterator(DTPropertyIterator iterator);

/*
 Iterate Properites
 Iterate and return properties for given entry.  
 When int == kIterationDone, all properties have been exhausted.
*/

extern int DTIterateProperties(DTPropertyIterator iterator,
						char **foundProperty);

/*
 Restart Property Iteration
 Used to re-iterate over a list of properties.  The Property Iterator is
 reset to the beginning of the list of properties for an entry.
*/

extern int DTRestartPropertyIteration(DTPropertyIterator iterator);

/*
 * BEGINNING OF LINUX-PMAC COMPATIBILITY ROUTINES AND STRUCTURES
 * (Note, not all of the structures, variables and routines
 *  found in linux-pmac are implemented here.)
 */

typedef struct reg_property reg_property_t;

struct reg_property {
	unsigned int address;
	unsigned int size;
};

typedef struct property property_t;

struct property {
	char		*name;		/* Name of property */
	int		length;		/* Length of property */
	unsigned char	*value;		/* data */
	property_t	*next;		/* Next property item */
};

typedef struct device_node device_node_t;

struct device_node {
	char		*name;		/* Name of this device */
	char		*type;		/* type */
	int		n_addrs;	/* Number of addresses */
	reg_property_t *addrs;		/* Address list */
	int		n_intrs;	/* Number of interrupts */
	unsigned long	*intrs;		/* Interrupt list */
	char		*full_name;	/* Full path name (not implemented) */
	property_t	*properties;	/* Property list */
	device_node_t	*parent;	/* pointer to node device */
	device_node_t	*child;		/* pointer to children devices */
	device_node_t	*sibling;	/* pointer to sibling device */
	device_node_t	*next;		/* Next like device (not implemented) */
	device_node_t	*allnext;	/* next node in the great world of device nodes */
};

struct device_node *find_devices(const char *name);
struct device_node *find_type(const char *type);
unsigned char *get_property(device_node_t *node, const char *name,
							unsigned long *lenp);
unsigned char *find_property(const char *device, const char *propname,
							unsigned long *lenp);
void ofw_init(void *);

#endif /* __MWERKS__ */
#endif /* __DEVICE_TREE__ */


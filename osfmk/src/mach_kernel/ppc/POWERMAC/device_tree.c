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

#include <ppc/misc_protos.h>
#include <kern/kalloc.h>
#include <ppc/boot.h>
#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/pci.h>

#define round_long(x)	(((x) + 3) & -4)
#define next_prop(x)	((DeviceTreeNodeProperty *) (((int)x) + sizeof(DeviceTreeNodeProperty) + round_long(x->length)))

/* Entry*/
typedef DeviceTreeNode *RealDTEntry;

typedef struct DTSavedScope {
	struct DTSavedScope * nextScope;
	RealDTEntry scope;
	RealDTEntry entry;
	unsigned long index;		
} *DTSavedScopePtr;

/* Entry Iterator*/
typedef struct OpaqueDTEntryIterator {
	RealDTEntry outerScope;
	RealDTEntry currentScope;
	RealDTEntry currentEntry;
	DTSavedScopePtr savedScope;
	unsigned long currentIndex;		
} *RealDTEntryIterator;

/* Property Iterator*/
typedef struct OpaqueDTPropertyIterator {
	RealDTEntry entry;
	DeviceTreeNodeProperty *currentProperty;
	unsigned long currentIndex;
} *RealDTPropertyIterator;

static RealDTEntry DTRootNode;
static int DTInitialized = 0;

void DTInit(void *base);

/*
 * Support Routines
 */
static RealDTEntry
skipProperties(RealDTEntry entry)
{
	DeviceTreeNodeProperty *prop;
	int k;

	if (entry == NULL || entry->nProperties == 0) {
		return NULL;
	} else {
		prop = (DeviceTreeNodeProperty *) (entry + 1);
		for (k = 0; k < entry->nProperties; k++) {
			prop = next_prop(prop);
		}
	}
	return ((RealDTEntry) prop);
}

static RealDTEntry
skipTree(RealDTEntry root)
{
	RealDTEntry entry;
	int k;

	entry = skipProperties(root);
	if (entry == NULL) {
		return NULL;
	}
	for (k = 0; k < root->nChildren; k++) {
		entry = skipTree(entry);
	}
	return entry;
}

static RealDTEntry
GetFirstChild(RealDTEntry parent)
{
	return skipProperties(parent);
}

static RealDTEntry
GetNextChild(RealDTEntry sibling)
{
	return skipTree(sibling);
}

static const char *
GetNextComponent(const char *cp, char *bp)
{
	while (*cp != 0) {
		if (*cp == kDTPathNameSeparator) {
			cp++;
			break;
		}
		*bp++ = *cp++;
	}
	*bp = 0;
	return cp;
}

static RealDTEntry
FindChild(RealDTEntry cur, char *buf)
{
	RealDTEntry	child;
	unsigned long	index;
	char *		str;
	int		dummy;

	if (cur->nChildren == 0) {
		return NULL;
	}
	index = 1;
	child = GetFirstChild(cur);
	while (1) {
		if (DTGetProperty(child, "name", (void **)&str, &dummy) != kSuccess) {
			break;
		}
		if (strcmp(str, buf) == 0) {
			return child;
		}
		if (index >= cur->nChildren) {
			break;
		}
		child = GetNextChild(child);
		index++;
	}
	return NULL;
}


/*
 * External Routines
 */
void
DTInit(void *base)
{
	DTRootNode = (RealDTEntry) base;
	DTInitialized = 1;
}

int
DTEntryIsEqual(const DTEntry ref1, const DTEntry ref2)
{
	/* equality of pointers */
	return (ref1 == ref2);
}

int
DTLookupEntry(const DTEntry searchPoint, const char *pathName, DTEntry *foundEntry)
{
	DTEntryNameBuf	buf;
	RealDTEntry	cur;
	const char *	cp;

	if (!DTInitialized) {
		return kError;
	}
	if (searchPoint == NULL) {
		cur = DTRootNode;
	} else {
		cur = searchPoint;
	}
	cp = pathName;
	if (*cp == kDTPathNameSeparator) {
		cp++;
		if (*cp == 0) {
			*foundEntry = cur;
			return kSuccess;
		}
	}
	do {
		cp = GetNextComponent(cp, buf);

		/* Check for done */
		if (*buf == 0) {
			if (*cp == 0) {
				*foundEntry = cur;
				return kSuccess;
			}
			break;
		}

		cur = FindChild(cur, buf);

	} while (cur != NULL);

	return kError;
}

int
DTCreateEntryIterator(const DTEntry startEntry, DTEntryIterator *iterator)
{
	RealDTEntryIterator iter;

	if (!DTInitialized) {
		return kError;
	}

	iter = (RealDTEntryIterator) kalloc(sizeof(struct OpaqueDTEntryIterator));
	if (startEntry != NULL) {
		iter->outerScope = (RealDTEntry) startEntry;
		iter->currentScope = (RealDTEntry) startEntry;
	} else {
		iter->outerScope = DTRootNode;
		iter->currentScope = DTRootNode;
	}
	iter->currentEntry = NULL;
	iter->savedScope = NULL;
	iter->currentIndex = 0;

	*iterator = iter;
	return kSuccess;
}

int
DTDisposeEntryIterator(DTEntryIterator iterator)
{
	RealDTEntryIterator iter = iterator;
	DTSavedScopePtr scope;

	while ((scope = iter->savedScope) != NULL) {
		iter->savedScope = scope->nextScope;
		kfree((vm_offset_t) scope, sizeof(struct DTSavedScope));
	}
	kfree((vm_offset_t) iterator, sizeof(struct OpaqueDTEntryIterator));
	return kSuccess;
}

int
DTEnterEntry(DTEntryIterator iterator, DTEntry childEntry)
{
	RealDTEntryIterator iter = iterator;
	DTSavedScopePtr newScope;

	if (childEntry == NULL) {
		return kError;
	}
	newScope = (DTSavedScopePtr) kalloc(sizeof(struct DTSavedScope));
	newScope->nextScope = iter->savedScope;
	newScope->scope = iter->currentScope;
	newScope->entry = iter->currentEntry;
	newScope->index = iter->currentIndex;		

	iter->currentScope = childEntry;
	iter->currentEntry = NULL;
	iter->savedScope = newScope;
	iter->currentIndex = 0;

	return kSuccess;
}

int
DTExitEntry(DTEntryIterator iterator, DTEntry *currentPosition)
{
	RealDTEntryIterator iter = iterator;
	DTSavedScopePtr newScope;

	newScope = iter->savedScope;
	if (newScope == NULL) {
		return kError;
	}
	iter->savedScope = newScope->nextScope;
	iter->currentScope = newScope->scope;
	iter->currentEntry = newScope->entry;
	iter->currentIndex = newScope->index;
	*currentPosition = iter->currentEntry;

	kfree((vm_offset_t) newScope, sizeof(struct DTSavedScope));

	return kSuccess;
}

int
DTIterateEntries(DTEntryIterator iterator, DTEntry *nextEntry)
{
	RealDTEntryIterator iter = iterator;

	if (iter->currentIndex >= iter->currentScope->nChildren) {
		*nextEntry = NULL;
		return kIterationDone;
	} else {
		iter->currentIndex++;
		if (iter->currentIndex == 1) {
			iter->currentEntry = GetFirstChild(iter->currentScope);
		} else {
			iter->currentEntry = GetNextChild(iter->currentEntry);
		}
		*nextEntry = iter->currentEntry;
		return kSuccess;
	}
}

int
DTRestartEntryIteration(DTEntryIterator iterator)
{
	RealDTEntryIterator iter = iterator;
#if 0
	// This commented out code allows a second argument (outer)
	// which (if true) causes restarting at the outer scope
	// rather than the current scope.
	DTSavedScopePtr scope;

	if (outer) {
		while ((scope = iter->savedScope) != NULL) {
			iter->savedScope = scope->nextScope;
			kfree((vm_offset_t) scope, sizeof(struct DTSavedScope));
		}
		iter->currentScope = iter->outerScope;
	}
#endif
	iter->currentEntry = NULL;
	iter->currentIndex = 0;
	return kSuccess;
}

int
DTGetProperty(const DTEntry entry, const char *propertyName, void **propertyValue, int *propertySize)
{
	DeviceTreeNodeProperty *prop;
	int k;

	if (entry == NULL || entry->nProperties == 0) {
		return kError;
	} else {
		prop = (DeviceTreeNodeProperty *) (entry + 1);
		for (k = 0; k < entry->nProperties; k++) {
			if (strcmp(prop->name, propertyName) == 0) {
				*propertyValue = (void *) (((int)prop)
						+ sizeof(DeviceTreeNodeProperty));
				*propertySize = prop->length;
				return kSuccess;
			}
			prop = next_prop(prop);
		}
	}
	return kError;
}

int
DTCreatePropertyIterator(const DTEntry entry, DTPropertyIterator *iterator)
{
	RealDTPropertyIterator iter;

	iter = (RealDTPropertyIterator) kalloc(sizeof(struct OpaqueDTPropertyIterator));
	iter->entry = entry;
	iter->currentProperty = NULL;
	iter->currentIndex = 0;

	*iterator = iter;
	return kSuccess;
}

int
DTDisposePropertyIterator(DTPropertyIterator iterator)
{
	kfree((vm_offset_t)iterator, sizeof(struct OpaqueDTPropertyIterator));
	return kSuccess;
}

int
DTIterateProperties(DTPropertyIterator iterator, char **foundProperty)
{
	RealDTPropertyIterator iter = iterator;

	if (iter->currentIndex >= iter->entry->nProperties) {
		*foundProperty = NULL;
		return kIterationDone;
	} else {
		iter->currentIndex++;
		if (iter->currentIndex == 1) {
			iter->currentProperty = (DeviceTreeNodeProperty *) (iter->entry + 1);
		} else {
			iter->currentProperty = next_prop(iter->currentProperty);
		}
		*foundProperty = iter->currentProperty->name;
		return kSuccess;
	}
}

int
DTRestartPropertyIteration(DTPropertyIterator iterator)
{
	RealDTPropertyIterator iter = iterator;

	iter->currentProperty = NULL;
	iter->currentIndex = 0;
	return kSuccess;
}


/*
 * Construct linux-pmac style OFW entries for ease of porting
 * drivers to and from MkLinux.
 *
 * NOTE - this is called VERY earily on, before the VM init and
 * the video console driver init.
 */

device_node_t	*ofw_node_list = NULL;
device_node_t	*ofw_root_node = NULL;

struct pci_address {
	unsigned long	addr_hi;
	unsigned long	addr_mid;
	unsigned long	addr_low;
};

typedef struct pci_address pci_address_t;

struct pci_reg_property {
	pci_address_t	 addr;
	unsigned long	size_hi;
	unsigned long 	size_low;
};

typedef struct pci_reg_property pci_reg_property_t;

void ofw_construct_node(char **bufPtr,
			device_node_t *parent, device_node_t **sib,
			device_node_t **nodelist, unsigned long asic_base);

static char *ofw_first_avail = NULL;

static void * ofw_alloc(size_t size)
{
	char	*data;

	size = (size+3) & ~3;	/* Round it up */
	data = ofw_first_avail;

	ofw_first_avail += size;

	return data;
}

void
ofw_init(void *_args)
{
	struct boot_args *args = _args;
	char		*first_avail;
	char		*tree = (char *) args->deviceTreeBuffer;
	device_node_t	*list = NULL;

	/* Check to make sure the boot loader actually did pass in something */
	if (args->Version < kBootHaveOFWVersion || args->deviceTreeSize == 0)
		return;

	/* Guru MkLinux note -
	 *
	 * If the booter could not construct a device tree,
	 * then the flatten tree will not be traversed
	 * due to a property count of zero on the first
	 * entry. (This assumes the booter did the
	 * right thing and zeroed the flatten tree)
	 */

	/* Init Apple style of OFW */

	DTInit((void *) args->deviceTreeBuffer);

	ofw_first_avail = (char *) ((args->first_avail+3) & ~3);

	ofw_construct_node(&tree, NULL, &ofw_root_node, &list, 0);

	args->first_avail = ( ( ( (unsigned int) ofw_first_avail )+3) & ~3);

	powermac_scan_bridges(&args->first_avail);

}

void
ofw_construct_node(char **bufPtr, device_node_t *parent, device_node_t **sib,
			device_node_t **nodelist, unsigned long asic_base)
{
	DeviceTreeNode *bufNode = (DeviceTreeNode *) *bufPtr;
	DeviceTreeNodeProperty	*bufProp;
	device_node_t		*node, *kid = NULL;
	property_t		*prop;
	reg_property_t		*reg;
	pci_reg_property_t	*pciOFW;
	char			*propData;
	int			p, i;
	long			*seenAAPL = NULL;
	int			seenAssigned = FALSE;

	if (bufNode->nProperties == 0)
		return;


	node = (device_node_t *) ofw_alloc(sizeof(*node));
	bzero((char *) node, sizeof(*node));

	node->parent = parent;
	prop = NULL;
	*bufPtr = (char *) (bufNode + 1);

	if (*nodelist)
		(*nodelist)->allnext = node;
	else
		ofw_node_list = node;

	*nodelist = node;

	if (*sib)
		(*sib)->sibling = node;

	*sib = node;

	if (parent && parent->child == NULL)
		parent->child = node;

	for (p = 0; p < bufNode->nProperties; p++) {
		// Iterate through the list of properties.

		bufProp = (DeviceTreeNodeProperty *) *bufPtr;
		propData = (char *) (bufProp+1);
		*bufPtr = (char *) (next_prop(bufProp));

		if (prop) {
			prop->next = ofw_alloc(sizeof(property_t));
			prop = prop->next;
		} else {
			prop = ofw_alloc(sizeof(property_t));
			node->properties = prop;
		}

		bzero((char *) prop, sizeof(*prop));

		prop->value = (unsigned char *) propData;
		prop->length = bufProp->length;
		prop->name = bufProp->name;


		/*
		 * Look through each property and build
		 * up a convience shopping list for the node
		 */
		if (strcmp(bufProp->name, "name") == 0) {
			node->name = ofw_alloc(prop->length+1);
			strncpy(node->name, propData, prop->length);
			node->name[prop->length] = 0;
		/* Kind of device - dbdma, scsi, pram, etc */
		} else if (strcmp(bufProp->name, "device_type") == 0)
			node->type = propData;
		/* Look for assigned addresses */
		else  if(strcmp(bufProp->name, "assigned-addresses") == 0) {
			seenAssigned = TRUE;
			node->addrs = reg = (reg_property_t *) ofw_alloc(bufProp->length);
			pciOFW = (pci_reg_property_t *) propData;
			
			for (i = 0; i < bufProp->length; reg++, pciOFW++) {
				reg->address = pciOFW->addr.addr_low;
				reg->size = pciOFW->size_low;
				i += sizeof(pci_reg_property_t);
			}

			node->n_addrs = i / sizeof(pci_reg_property_t);
		/* Look for register address */
		} else if (strcmp(bufProp->name, "reg") == 0) {
			/* If the assigned-addresses property has
			 * already been seen, don't bother with
			*/
			if (seenAssigned)
				continue;

			node->n_addrs = bufProp->length/sizeof(reg_property_t);

			node->addrs = (reg_property_t *)ofw_alloc(bufProp->length);
			bcopy_nc(propData, (char*)node->addrs, bufProp->length);

			/* Adjust the addresses - most are relative to
			 * the asic which contains them
			 */
			for (i = 0; i < node->n_addrs; i++) 
				node->addrs[i].address +=  asic_base;
		} else if (strcmp(bufProp->name, "AAPL,interrupts") == 0) {
			node->n_intrs = prop->length / sizeof(long);
			node->intrs = (unsigned long *) prop->value;
		} else if (strcmp(bufProp->name, "AAPL,address") == 0) 
			seenAAPL = (long *) propData;

	}

#if 0
	/*
	 * Override the register address values with Apple
	 * defined ones if the property was seen.
	 */

	if (seenAAPL && !seenAssigned && node->n_addrs) {
		for (i = 0; i < node->n_addrs; i++) 
			node->addrs[i].address = seenAAPL[i];
	}
#endif

	/* Is there a better way? */
	if (node->type && node->n_addrs && strcmp(node->type, "dbdma") == 0)
		asic_base = node->addrs[0].address;

	kid = NULL;
	parent = node;
	for (i = 0; i < bufNode->nChildren; i++) 
		ofw_construct_node(bufPtr, parent, &kid, nodelist, asic_base);
}

int	ofw_debug = FALSE;

device_node_t *
find_devices(const char *name)
{
	device_node_t	*node, *head = NULL, *prevp;

	if (ofw_debug)
		printf("<device %s>", name);

	for (node = ofw_node_list; node; node = node->allnext) {
		if (node->name && strcmp(node->name, name) == 0) {
			if (ofw_debug)
				printf("<found %s/%s> ", node->parent->name ? node->parent->name : "??", node->name ? node->name : "??");
			if (head == NULL) 
				head = node;
			else
				prevp->next = node;

			prevp = node;
		}
	}

	if (ofw_debug)
		printf("<finished>");

	if (head)
		prevp->next = NULL;

	return head;
}

device_node_t *
find_type(const char *type)
{
	device_node_t	*node, *head = NULL, *prevp;

	if (ofw_debug)
		printf("<type %s> ", type);
	for (node = ofw_node_list; node; node = node->allnext) 
		if (node->type && strcmp(node->type, type) == 0) {
			if (ofw_debug)
				printf("<found %s/%s> ", node->parent->name ? node->parent->name : "??", node->name ? node->name : "??");

			if (head == NULL) 
				head = node;
			 else
				prevp->next = node;

			prevp = node;
		}

	if (ofw_debug)
		printf("<finished>");

	if (head)
		prevp->next = NULL;

	return  head;
}

unsigned char *
get_property(device_node_t *node, const char *name, unsigned long *size)
{
	property_t	*prop;

	for (prop = node->properties; prop; prop = prop->next) {
		if (strcmp(prop->name, name) == 0) {
			if (size)
				*size = prop->length;

			return prop->value;
		}
	}

	if (size)
		*size = 0;
	return NULL;
}

unsigned char *
find_property(const char *device, const char *propname, unsigned long *size)
{
	device_node_t	*node;

	if ((node = find_devices(device)) == NULL)
		return	NULL;

	return get_property(node, propname, size);
}

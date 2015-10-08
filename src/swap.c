/*
 * This file is part of DGD, https://github.com/dworkin/dgd
 * Copyright (C) 1993-2010 Dworkin B.V.
 * Copyright (C) 2010-2012 DGD Authors (see the commit log for details)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

# define INCLUDE_FILE_IO
# include "dgd.h"
# include "swap.h"

typedef struct _header_ {	/* swap slot header */
    struct _header_ *prev;	/* previous in swap slot list */
    struct _header_ *next;	/* next in swap slot list */
    sector sec;			/* the sector that uses this slot */
    sector swap;		/* the swap sector (if any) */
    bool dirty;			/* has the swap slot been written to? */
} header;

static char *swapfile;			/* swap file name */
static int swap;			/* swap file descriptor */
static int dump, dump2;			/* snapshot descriptors */
static char *mem;			/* swap slots in memory */
static sector *map, *smap;		/* sector map, swap free map */
static sector mfree, sfree;		/* free sector lists */
static char *cbuf;			/* sector buffer */
static sector cached;			/* sector currently cached in cbuf */
static header *first, *last;		/* first and last swap slot */
static header *lfree;			/* free swap slot list */
static long slotsize;			/* sizeof(header) + size of sector */
static unsigned int sectorsize;		/* size of sector */
static unsigned int restoresecsize;	/* size of sector in restore file */
static sector swapsize, cachesize;	/* # of sectors in swap and cache */
static sector nsectors;			/* total swap sectors */
static sector nfree;			/* # free sectors */
static sector ssectors;			/* sectors actually in swap file */
static sector sbarrier;			/* swap sector barrier */
static bool swapping;			/* currently using a swapfile? */

/*
 * NAME:	swap->init()
 * DESCRIPTION:	initialize the swap device
 */
bool sw_init(char *file, unsigned int total, unsigned int cache, unsigned int secsize)
{
    header *h;
    sector i;

    /* allocate and initialize all tables */
    swapfile = file;
    swapsize = total;
    cachesize = cache;
    sectorsize = secsize;
    slotsize = sizeof(header) + secsize;

    /* sanity check */
    if (cache >= total || ((cache * slotsize) / cache) != slotsize) {
	P_message("Config error: swap cache too big\012"); /* LF */
	return 0;
    }

    mem = ALLOC(char, slotsize * cache);
    map = ALLOC(sector, total);
    smap = ALLOC(sector, total);
    cbuf = ALLOC(char, secsize);
    cached = SW_UNUSED;

    /* 0 sectors allocated */
    nsectors = 0;
    ssectors = 0;
    sbarrier = 0;
    nfree = 0;

    /* init free sector maps */
    mfree = SW_UNUSED;
    sfree = SW_UNUSED;
    lfree = h = (header *) mem;
    for (i = cache - 1; i > 0; --i) {
	h->sec = SW_UNUSED;
	h->next = (header *) ((char *) h + slotsize);
	h = h->next;
    }
    h->sec = SW_UNUSED;
    h->next = (header *) NULL;

    /* no swap slots in use yet */
    first = (header *) NULL;
    last = (header *) NULL;

    swap = dump = -1;
    swapping = TRUE;
    return 1;
}

/*
 * NAME:	swap->finish()
 * DESCRIPTION:	clean up swapfile
 */
void sw_finish()
{
    if (swap >= 0) {
	char buf[STRINGSZ];

	P_close(swap);
	P_unlink(path_native(buf, swapfile));
    }
    if (dump >= 0) {
	P_close(dump);
    }
}

/*
 * NAME:	create()
 * DESCRIPTION:	create the swap file
 */
static void sw_create()
{
    char buf[STRINGSZ], *p;

    memset(cbuf, '\0', sectorsize);
    p = path_native(buf, swapfile);
    P_unlink(p);
    swap = P_open(p, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0600);
    if (swap < 0 || P_write(swap, cbuf, sectorsize) < 0) {
	fatal("cannot create swap file \"%s\"", swapfile);
    }
}

/*
 * NAME:	swap->newv()
 * DESCRIPTION:	initialize a new vector of sectors
 */
void sw_newv(sector *vec, unsigned int size)
{
    while (mfree != SW_UNUSED) {
	/* reuse a previously deleted sector */
	if (size == 0) {
	    return;
	}
	mfree = map[*vec = mfree];
	map[*vec++] = SW_UNUSED;
	--nfree;
	--size;
    }

    while (size > 0) {
	/* allocate a new sector */
	if (nsectors == swapsize) {
	    fatal("out of sectors");
	}
	map[*vec++ = nsectors++] = SW_UNUSED;
	--size;
    }
}

/*
 * NAME:	swap->wipev()
 * DESCRIPTION:	wipe a vector of sectors
 */
void sw_wipev(sector *vec, unsigned int size)
{
    sector sec, i;
    header *h;

    vec += size;
    while (size > 0) {
	sec = *--vec;
	i = map[sec];
	if (i < cachesize && (h=(header *) (mem + i * slotsize))->sec == sec) {
	    i = h->swap;
	    h->swap = SW_UNUSED;
	} else {
	    map[sec] = SW_UNUSED;
	}
	if (i != SW_UNUSED && i >= sbarrier) {
	    /*
	     * free sector in swap file
	     */
	    smap[i] = sfree;
	    sfree = i;
	}
	--size;
    }
}

/*
 * NAME:	swap->delv()
 * DESCRIPTION:	delete a vector of swap sectors
 */
void sw_delv(sector *vec, unsigned int size)
{
    sector sec, i;
    header *h;

    /*
     * note: sectors must have been wiped before being deleted!
     */
    vec += size;
    while (size > 0) {
	sec = *--vec;
	i = map[sec];
	if (i < cachesize && (h=(header *) (mem + i * slotsize))->sec == sec) {
	    /*
	     * remove the swap slot from the first-last list
	     */
	    if (h != first) {
		h->prev->next = h->next;
	    } else {
		first = h->next;
		if (first != (header *) NULL) {
		    first->prev = (header *) NULL;
		}
	    }
	    if (h != last) {
		h->next->prev = h->prev;
	    } else {
		last = h->prev;
		if (last != (header *) NULL) {
		    last->next = (header *) NULL;
		}
	    }
	    /*
	     * put the cache slot in the free cache slot list
	     */
	    h->sec = SW_UNUSED;
	    h->next = lfree;
	    lfree = h;
	}

	/*
	 * put sec in free sector list
	 */
	map[sec] = mfree;
	mfree = sec;
	nfree++;

	--size;
    }
}

/*
 * NAME:	swap->load()
 * DESCRIPTION:	reserve a swap slot for sector sec. If fill == TRUE, load it
 *		from the swap file if appropriate.
 */
static header *sw_load(sector sec, bool restore, bool fill)
{
    header *h;
    sector load, save;

    load = map[sec];
    if (load >= cachesize ||
	(h=(header *) (mem + load * slotsize))->sec != sec) {
	/*
	 * the sector is either unused or in the swap file
	 */
	if (lfree != (header *) NULL) {
	    /*
	     * get swap slot from the free swap slot list
	     */
	    h = lfree;
	    lfree = h->next;
	} else {
	    /*
	     * No free slot available, use the last one in the swap slot list
	     * instead.
	     */
	    h = last;
	    last = h->prev;
	    if (last != (header *) NULL) {
		last->next = (header *) NULL;
	    } else {
		first = (header *) NULL;
	    }
	    save = h->swap;
	    if (h->dirty) {
		/*
		 * Dump the sector to swap file
		 */

		if (save == SW_UNUSED || save < sbarrier) {
		    /*
		     * allocate new sector in swap file
		     */
		    if (sfree == SW_UNUSED) {
			if (ssectors == SW_UNUSED) {
			    fatal("out of sectors");
			}
			save = ssectors++;
		    } else {
			save = sfree;
			sfree = smap[save];
		    }
		}

		if (swap < 0) {
		    sw_create();
		}
		P_lseek(swap, (off_t) (save + 1L) * sectorsize, SEEK_SET);
		if (P_write(swap, (char *) (h + 1), sectorsize) < 0) {
		    fatal("cannot write swap file");
		}
	    }
	    map[h->sec] = save;
	}
	h->sec = sec;
	h->swap = load;
	h->dirty = FALSE;
	/*
	 * The slot has been reserved. Update map.
	 */
	map[sec] = ((intptr_t) h - (intptr_t) mem) / slotsize;

	if (load != SW_UNUSED) {
	    if (restore) {
		/*
		 * load the sector from the snapshot
		 */
		P_lseek(dump, (off_t) (load + 1L) * sectorsize, SEEK_SET);
		if (P_read(dump, (char *) (h + 1), sectorsize) <= 0) {
		    fatal("cannot read snapshot");
		}
	    } else if (fill) {
		/*
		 * load the sector from the swap file
		 */
		P_lseek(swap, (off_t) (load + 1L) * sectorsize, SEEK_SET);
		if (P_read(swap, (char *) (h + 1), sectorsize) <= 0) {
		    fatal("cannot read swap file");
		}
	    }
	} else if (fill) {
	    /* zero-fill new sector */
	    memset(h + 1, '\0', sectorsize);
	}
    } else {
	/*
	 * The sector already had a slot. Remove it from the first-last list.
	 */
	if (h != first) {
	    h->prev->next = h->next;
	} else {
	    first = h->next;
	}
	if (h != last) {
	    h->next->prev = h->prev;
	} else {
	    last = h->prev;
	    if (last != (header *) NULL) {
		last->next = (header *) NULL;
	    }
	}
    }
    /*
     * put the sector at the head of the first-last list
     */
    h->prev = (header *) NULL;
    h->next = first;
    if (first != (header *) NULL) {
	first->prev = h;
    } else {
	last = h;	/* last was NULL too */
    }
    first = h;

    return h;
}

/*
 * NAME:	swap->readv()
 * DESCRIPTION:	read bytes from a vector of sectors
 */
void sw_readv(char *m, sector *vec, Uint size, Uint idx)
{
    unsigned int len;

    vec += idx / sectorsize;
    idx %= sectorsize;
    do {
	len = (size > sectorsize - idx) ? sectorsize - idx : size;
	memcpy(m, (char *) (sw_load(*vec++, FALSE, TRUE) + 1) + idx, len);
	idx = 0;
	m += len;
    } while ((size -= len) > 0);
}

/*
 * NAME:	swap->writev()
 * DESCRIPTION:	write bytes to a vector of sectors
 */
void sw_writev(char *m, sector *vec, Uint size, Uint idx)
{
    header *h;
    unsigned int len;

    vec += idx / sectorsize;
    idx %= sectorsize;
    do {
	len = (size > sectorsize - idx) ? sectorsize - idx : size;
	h = sw_load(*vec++, FALSE, (len != sectorsize));
	h->dirty = TRUE;
	memcpy((char *) (h + 1) + idx, m, len);
	idx = 0;
	m += len;
    } while ((size -= len) > 0);
}

/*
 * NAME:	swap->dreadv()
 * DESCRIPTION:	restore bytes from a vector of sectors in snapshot
 */
void sw_dreadv(char *m, sector *vec, Uint size, Uint idx)
{
    header *h;
    unsigned int len;

    vec += idx / sectorsize;
    idx %= sectorsize;
    do {
	len = (size > sectorsize - idx) ? sectorsize - idx : size;
	h = sw_load(*vec++, TRUE, FALSE);
	h->swap = SW_UNUSED;
	memcpy(m, (char *) (h + 1) + idx, len);
	idx = 0;
	m += len;
    } while ((size -= len) > 0);
}

/*
 * NAME:	swap->conv()
 * DESCRIPTION:	restore converted bytes from a vector of sectors in snapshot
 */
void sw_conv(char *m, sector *vec, Uint size, Uint idx)
{
    unsigned int len;

    vec += idx / restoresecsize;
    idx %= restoresecsize;
    do {
	len = (size > restoresecsize - idx) ? restoresecsize - idx : size;
	if (*vec != cached) {
	    P_lseek(dump, (off_t) (map[*vec] + 1L) * restoresecsize, SEEK_SET);
	    if (P_read(dump, cbuf, restoresecsize) <= 0) {
		fatal("cannot read snapshot");
	    }
	    map[cached = *vec] = SW_UNUSED;
	}
	vec++;
	memcpy(m, cbuf + idx, len);
	idx = 0;
	m += len;
    } while ((size -= len) > 0);
}

/*
 * NAME:	swap->conv2()
 * DESCRIPTION:	restore bytes from a vector of sectors in secondary snapshot
 */
void sw_conv2(char *m, sector *vec, Uint size, Uint idx)
{
    unsigned int len;

    vec += idx / restoresecsize;
    idx %= restoresecsize;
    do {
	len = (size > restoresecsize - idx) ? restoresecsize - idx : size;
	if (*vec != cached) {
	    P_lseek(dump2, (off_t) (map[*vec] + 1L) * restoresecsize,
		    SEEK_SET);
	    if (P_read(dump2, cbuf, restoresecsize) <= 0) {
		fatal("cannot read secondary snapshot");
	    }
	    map[cached = *vec] = SW_UNUSED;
	}
	vec++;
	memcpy(m, cbuf + idx, len);
	idx = 0;
	m += len;
    } while ((size -= len) > 0);
}

/*
 * NAME:	swap->mapsize()
 * DESCRIPTION:	count the number of sectors required for size bytes + a map
 */
sector sw_mapsize(unsigned int size)
{
    sector i, n;

    /* calculate the number of sectors required */
    n = 0;
    for (;;) {
	i = (size + n * sizeof(sector) + sectorsize - 1) / sectorsize;
	if (n == i) {
	    return n;
	}
	n = i;
    }
}

/*
 * NAME:	swap->count()
 * DESCRIPTION:	return the number of sectors presently in use
 */
sector sw_count()
{
    return nsectors - nfree;
}


typedef struct {
    Uint secsize;		/* size of swap sector */
    sector nsectors;		/* # sectors */
    sector ssectors;		/* # swap sectors */
    sector nfree;		/* # free sectors */
    sector mfree;		/* free sector list */
} dump_header;

static char dh_layout[] = "idddd";

/*
 * NAME:	sector_compare
 * DESCRIPTION: used by qsort to compare entries
 */
int sector_compare(const void *pa, const void *pb)
{
    sector a = *(sector *)pa;
    sector b = *(sector *)pb;

    if (a > b) {
	return 1;
    } else if (a < b) {
	return -1;
    } else {
	return 0;
    }
}

/*
 * NAME:	swap->trim()
 * DESCRIPTION:	trim free sectors from the end of the sector map
 */
void sw_trim()
{
    sector npurge;
    sector *entries;
    sector i;
    sector j;

    if (!nfree) {
	/* nothing to trim */
	return;
    }

    npurge = 0;
    entries = ALLOC(sector, nfree);

    j = mfree;

    /* 1. prepare a list of free sectors */
    for (i = 0; i < nfree; i++) {
	entries[i] = j;
	j = map[j];
    }

    /* 2. sort indices from low to high */
    qsort(entries, nfree, sizeof(sector), sector_compare);

    /* 3. trim the object table */
    while (nfree > 0 && entries[nfree - 1] == nsectors - 1) {
	npurge++;
	nsectors--;
	nfree--;
    }

    memset(map + nsectors, '\0', npurge * sizeof(sector));

    /* 4. relink remaining free sectors from low to high */
    j = SW_UNUSED;

    for (i = 0; i < nfree; i++) {
	uindex n = entries[nfree - i - 1];
	map[n] = j;
	j = n;
    }

    mfree = j;
    FREE(entries);
}

/*
 * NAME:	swap->dump()
 * DESCRIPTION:	create snapshot
 */
int sw_dump(char *snapshot, bool keep)
{
    header *h;
    sector sec;
    char buffer[STRINGSZ + 4], buf1[STRINGSZ], buf2[STRINGSZ], *p, *q;
    sector n;

    if (swap < 0) {
	sw_create();
    }

    /* flush the cache and adjust sector map */
    for (h = last; h != (header *) NULL; h = h->prev) {
	sec = h->swap;
	if (h->dirty) {
	    /*
	     * Dump the sector to swap file
	     */
	    if (sec == SW_UNUSED || sec < sbarrier) {
		/*
		 * allocate new sector in swap file
		 */
		if (sfree == SW_UNUSED) {
		    if (ssectors == SW_UNUSED) {
			fatal("out of sectors");
		    }
		    sec = ssectors++;
		} else {
		    sec = sfree;
		    sfree = smap[sec];
		}
		h->swap = sec;
	    }
	    P_lseek(swap, (off_t) (sec + 1L) * sectorsize, SEEK_SET);
	    if (P_write(swap, (char *) (h + 1), sectorsize) < 0) {
		fatal("cannot write swap file");
	    }
	}
	map[h->sec] = sec;
    }

    sw_trim();

    if (dump >= 0 && !keep) {
	P_close(dump);
	dump = -1;
    }
    if (swapping) {
	p = path_native(buf1, snapshot);
	sprintf(buffer, "%s.old", snapshot);
	q = path_native(buf2, buffer);
	P_unlink(q);
	P_rename(p, q);

	/* move to snapshot */
	P_close(swap);
	q = path_native(buf2, swapfile);
	if (P_rename(q, p) < 0) {
	    int old;

	    /*
	     * The rename failed.  Attempt to copy the snapshot instead.
	     * This will take a long, long while, so keep the swapfile and
	     * snapshot on the same file system if at all possible.
	     */
	    old = P_open(q, O_RDWR | O_BINARY, 0);
	    swap = P_open(p, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0600);
	    if (old < 0 || swap < 0) {
		fatal("cannot move swap file");
	    }
	    /* copy initial sector */
	    if (P_read(old, cbuf, sectorsize) <= 0) {
		fatal("cannot read swap file");
	    }
	    if (P_write(swap, cbuf, sectorsize) < 0) {
		fatal("cannot write snapshot");
	    }
	    /* copy swap sectors */
	    for (n = ssectors; n > 0; --n) {
		if (P_read(old, cbuf, sectorsize) <= 0) {
		    fatal("cannot read swap file");
		}
		if (P_write(swap, cbuf, sectorsize) < 0) {
		    fatal("cannot write snapshot");
		}
	    }
	    P_close(old);
	} else {
	    /*
	     * The rename succeeded; reopen the new snapshot.
	     */
	    swap = P_open(p, O_RDWR | O_BINARY, 0);
	    if (swap < 0) {
		fatal("cannot reopen snapshot");
	    }
	}
    }

    /* write map */
    P_lseek(swap, (off_t) (ssectors + 1L) * sectorsize, SEEK_SET);
    if (P_write(swap, (char *) map, nsectors * sizeof(sector)) < 0) {
	fatal("cannot write sector map to snapshot");
    }

    /* fix the sector map */
    for (h = last; h != (header *) NULL; h = h->prev) {
	map[h->sec] = ((intptr_t) h - (intptr_t) mem) / slotsize;
	h->dirty = FALSE;
    }

    return swap;
}

/*
 * NAME:	swap->dump2()
 * DESCRIPTION:	finish snapshot
 */
void sw_dump2(char *header, int size, bool incr)
{
    static off_t prev;
    register off_t sectors;
    Uint offset;
    dump_header dh;
    char save[4];

    memset(cbuf, '\0', sectorsize);

    if (!swapping || incr) {
	/* extend */
	sectors = P_lseek(swap, 0, SEEK_CUR);
	offset = sectors % sectorsize;
	sectors /= sectorsize;
	if (offset != 0) {
	    if (P_write(swap, cbuf, sectorsize - offset) < 0) {
		fatal("cannot extend swap file");
	    }
	    sectors++;
	}
    }

    if (swapping) {
	P_lseek(swap, 0, SEEK_SET);
	prev = 0;
    }

    /* write header */
    memcpy(cbuf, header, size);
    dh.secsize = sectorsize;
    dh.nsectors = nsectors;
    dh.ssectors = ssectors;
    dh.nfree = nfree;
    dh.mfree = mfree;
    memcpy(cbuf + sectorsize - sizeof(dump_header), &dh, sizeof(dump_header));
    if (P_write(swap, cbuf, sectorsize) < 0) {
	fatal("cannot write snapshot header");
    }

    if (!swapping) {
	/* let the previous header refer to the current one */
	save[0] = sectors >> 24;
	save[1] = sectors >> 16;
	save[2] = sectors >> 8;
	save[3] = sectors;
	P_lseek(swap, prev * sectorsize + size - sizeof(save), SEEK_SET);
	if (P_write(swap, save, sizeof(save)) < 0) {
	    fatal("cannot write offset");
	}
	prev = sectors;
    }

    if (incr) {
	/* incremental snapshot */
	if (swapping) {
	    --sectors;
	}
	if (sectors > SW_UNUSED) {
	    sectors = SW_UNUSED;
	}
	sbarrier = ssectors = sectors;
	swapping = FALSE;
    } else {
	/* full snapshot */
	dump = swap;
	swap = -1;
	sbarrier = ssectors = 0;
	swapping = TRUE;
	restoresecsize = sectorsize;
    }
    sfree = SW_UNUSED;
    cached = SW_UNUSED;
}

/*
 * NAME:	swap->restore()
 * DESCRIPTION:	restore snapshot
 */
void sw_restore(int fd, unsigned int secsize)
{
    dump_header dh;

    /* restore swap header */
    P_lseek(fd, -(off_t) (conf_dsize(dh_layout) & 0xff), SEEK_CUR);
    conf_dread(fd, (char *) &dh, dh_layout, (Uint) 1);
    if (dh.secsize != secsize) {
	error("Wrong sector size (%d)", dh.secsize);
    }
    if (dh.nsectors > swapsize) {
	error("Too many sectors in restore file (%d)", dh.nsectors);
    }
    restoresecsize = secsize;
    if (secsize > sectorsize) {
	cbuf = REALLOC(cbuf, char, 0, secsize);
    }

    /* seek beyond swap sectors */
    P_lseek(fd, (off_t) (dh.ssectors + 1L) * secsize, SEEK_SET);

    /* restore swap map */
    conf_dread(fd, (char *) map, "d", (Uint) dh.nsectors);
    nsectors = dh.nsectors;
    mfree = dh.mfree;
    nfree = dh.nfree;

    dump = fd;
}

/*
 * NAME:	swap->restore2()
 * DESCRIPTION:	restore secondary snapshot
 */
void sw_restore2(int fd)
{
    dump2 = fd;
}

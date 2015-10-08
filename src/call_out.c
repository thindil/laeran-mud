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
# include "str.h"
# include "array.h"
# include "object.h"
# include "xfloat.h"
# include "interpret.h"
# include "data.h"
# include "call_out.h"

# define CYCBUF_SIZE	128		/* cyclic buffer size, power of 2 */
# define CYCBUF_MASK	(CYCBUF_SIZE - 1) /* cyclic buffer mask */
# define SWPERIOD	60		/* swaprate buffer size */

typedef struct {
    uindex handle;	/* callout handle */
    uindex oindex;	/* index in object table */
    Uint time;		/* when to call */
    uindex htime;	/* when to call, high word */
    uindex mtime;	/* when to call in milliseconds */
} call_out;

static char co_layout[] = "uuiuu";

# define count		time
# define last		htime
# define prev		htime
# define next		mtime

static call_out *cotab;			/* callout table */
static uindex cotabsz;			/* callout table size */
static uindex queuebrk;			/* queue brk */
static uindex cycbrk;			/* cyclic buffer brk */
static uindex flist;			/* free list index */
static uindex nzero;			/* # immediate callouts */
static uindex nshort;			/* # short-term callouts, incl. nzero */
static uindex running;			/* running callouts */
static uindex immediate;		/* immediate callouts */
static uindex cycbuf[CYCBUF_SIZE];	/* cyclic buffer of callout lists */
static Uint timestamp;			/* cycbuf start time */
static Uint timeout;			/* time of first callout in cycbuf */
static Uint timediff;			/* stored/actual time difference */
static Uint cotime;			/* callout time */
static unsigned short comtime;		/* callout millisecond time */
static Uint swaptime;			/* last swap count timestamp */
static Uint swapped1[SWPERIOD];		/* swap info for last minute */
static Uint swapped5[SWPERIOD];		/* swap info for last five minutes */
static Uint swaprate1;			/* swaprate per minute */
static Uint swaprate5;			/* swaprate per 5 minutes */

/*
 * NAME:	call_out->init()
 * DESCRIPTION:	initialize callout handling
 */
bool co_init(unsigned int max)
{
    if (max != 0) {
	/* only if callouts are enabled */
	cotab = ALLOC(call_out, max + 1);
	cotab[0].time = 0;	/* sentinel for the heap */
	cotab[0].mtime = 0;
	cotab++;
	flist = 0;
	timestamp = timeout = 0;
	timediff = 0;
    }
    running = immediate = 0;
    memset(cycbuf, '\0', sizeof(cycbuf));
    cycbrk = cotabsz = max;
    queuebrk = 0;
    nzero = nshort = 0;
    cotime = 0;

    swaptime = P_time();
    memset(swapped1, '\0', sizeof(swapped1));
    memset(swapped5, '\0', sizeof(swapped5));
    swaprate1 = swaprate5 = 0;

    return TRUE;
}

/*
 * NAME:	enqueue()
 * DESCRIPTION:	put a callout in the queue
 */
static call_out *enqueue(Uint t, unsigned short m)
{
    uindex i, j;
    call_out *l;

    /*
     * create a free spot in the heap, and sift it upward
     */
# ifdef DEBUG
    if (queuebrk == cycbrk) {
	fatal("callout table overflow");
    }
# endif
    i = ++queuebrk;
    l = cotab - 1;
    for (j = i >> 1; l[j].time > t || (l[j].time == t && l[j].mtime > m);
	 i = j, j >>= 1) {
	l[i] = l[j];
    }

    l = &l[i];
    l->time = t;
    l->mtime = m;
    return l;
}

/*
 * NAME:	dequeue()
 * DESCRIPTION:	remove a callout from the queue
 */
static void dequeue(uindex i)
{
    Uint t;
    short m;
    uindex j;
    call_out *l;

    l = cotab - 1;
    i++;
    t = l[queuebrk].time;
    m = l[queuebrk].mtime;
    if (t < l[i].time) {
	/* sift upward */
	for (j = i >> 1; l[j].time > t || (l[j].time == t && l[j].mtime > m);
	     i = j, j >>= 1) {
	    l[i] = l[j];
	}
    } else if (i <= UINDEX_MAX / 2) {
	/* sift downward */
	for (j = i << 1; j < queuebrk; i = j, j <<= 1) {
	    if (l[j].time > l[j + 1].time ||
		(l[j].time == l[j + 1].time && l[j].mtime > l[j + 1].mtime)) {
		j++;
	    }
	    if (t < l[j].time || (t == l[j].time && m <= l[j].mtime)) {
		break;
	    }
	    l[i] = l[j];
	}
    }
    /* put into place */
    l[i] = l[queuebrk--];
}

/*
 * NAME:	newcallout()
 * DESCRIPTION:	allocate a new callout for the cyclic buffer
 */
static call_out *newcallout(uindex *list, Uint t)
{
    uindex i;
    call_out *co, *first, *last;

    if (flist != 0) {
	/* get callout from free list */
	i = flist;
	flist = cotab[i].next;
    } else {
	/* allocate new callout */
# ifdef DEBUG
	if (cycbrk == queuebrk || cycbrk == 1) {
	    fatal("callout table overflow");
	}
# endif
	i = --cycbrk;
    }
    nshort++;
    if (t == 0) {
	nzero++;
    }

    co = &cotab[i];
    if (*list == 0) {
	/* first one in list */
	*list = i;
	co->count = 1;

	if (t != 0 && (timeout == 0 || t < timeout)) {
	    timeout = t;
	}
    } else {
	/* add to list */
	first = &cotab[*list];
	last = (first->count == 1) ? first : &cotab[first->last];
	last->next = i;
	first->count++;
	first->last = i;
    }
    co->prev = co->next = 0;

    return co;
}

/*
 * NAME:	freecallout()
 * DESCRIPTION:	remove a callout from the cyclic buffer
 */
static void freecallout(uindex *cyc, uindex j, uindex i, Uint t)
{
    call_out *l, *first;

    --nshort;
    if (t == 0) {
	--nzero;
    }

    l = cotab;
    first = &l[*cyc];
    if (i == j) {
	if (first->count == 1) {
	    *cyc = 0;

	    if (t != 0 && t == timeout) {
		if (nshort != nzero) {
		    while (cycbuf[t & CYCBUF_MASK] == 0) {
			t++;
		    }
		    timeout = t;
		} else {
		    timeout = 0;
		}
	    }
	} else {
	    *cyc = first->next;
	    l[first->next].count = first->count - 1;
	    if (first->count != 2) {
		l[first->next].last = first->last;
	    }
	}
    } else {
	--first->count;
	if (i == first->last) {
	    l[j].prev = l[j].next = 0;
	    if (first->count != 1) {
		first->last = j;
	    }
	} else {
	    l[j].next = l[i].next;
	}
    }

    l += i;
    l->handle = 0;	/* mark as unused */
    if (i == cycbrk) {
	/*
	 * callout at the edge
	 */
	while (++cycbrk != cotabsz && (++l)->handle == 0) {
	    /* followed by free callout */
	    if (cycbrk == flist) {
		/* first in the free list */
		flist = l->next;
	    } else {
		/* connect previous to next */
		cotab[l->prev].next = l->next;
		if (l->next != 0) {
		    /* connect next to previous */
		    cotab[l->next].prev = l->prev;
		}
	    }
	}
    } else {
	/* add to free list */
	if (flist != 0) {
	    /* link next to current */
	    cotab[flist].prev = i;
	}
	/* link to next */
	l->next = flist;
	flist = i;
    }
}

/*
 * NAME:	call_out->decode()
 * DESCRIPTION:	decode millisecond time
 */
Uint co_decode(Uint time, unsigned short *mtime)
{
    *mtime = time & 0xffff;
    time = ((timestamp - timediff) & 0xffffff00L) + ((time >> 16) & 0xff);
    if (time + timediff < timestamp) {
	time += 0x100;
    }
    return time;
}

/*
 * NAME:	call_out->time()
 * DESCRIPTION:	get the current (adjusted) time
 */
Uint co_time(unsigned short *mtime)
{
    Uint t;

    if (cotime != 0) {
	*mtime = comtime;
	return cotime;
    }

    t = P_mtime(mtime) - timediff;
    if (t < timestamp) {
	/* clock turned back? */
	t = timestamp;
	*mtime = 0;
    } else if (timestamp < t) {
	if (running == 0) {
	    if (timeout == 0 || timeout > t) {
		timestamp = t;
	    } else if (timestamp < timeout) {
		timestamp = timeout - 1;
	    }
	}
	if (t > timestamp + 60) {
	    /* lot of lag? */
	    t = timestamp + 60;
	    *mtime = 0;
	}
    }

    comtime = *mtime;
    return cotime = t + timediff;
}

/*
 * NAME:	call_out->check()
 * DESCRIPTION:	check if, and how, a new callout can be added
 */
Uint co_check(unsigned int n, Int delay, unsigned int mdelay, Uint *tp,
	unsigned short *mp, uindex **qp)
{
    Uint t;
    unsigned short m;

    if (cotabsz == 0) {
	/*
	 * call_outs are disabled
	 */
	*qp = (uindex *) NULL;
	return 0;
    }

    if (queuebrk + (uindex) n == cycbrk || cycbrk - (uindex) n == 1) {
	error("Too many callouts");
    }

    if (delay == 0 && (mdelay == 0 || mdelay == 0xffff)) {
	/*
	 * immediate callout
	 */
	if (nshort == 0 && queuebrk == 0 && n == 0) {
	    co_time(mp);	/* initialize timestamp */
	}
	*qp = &immediate;
	*tp = t = 0;
	*mp = 0xffff;
    } else {
	/*
	 * delayed callout
	 */
	t = co_time(mp) - timediff;
	if (t + delay + 1 <= t) {
	    error("Too long delay");
	}
	t += delay;
	if (mdelay != 0xffff) {
	    m = *mp + mdelay;
	    if (m >= 1000) {
		m -= 1000;
		t++;
	    }
	} else {
	    m = 0xffff;
	}

	if (mdelay == 0xffff && t < timestamp + CYCBUF_SIZE) {
	    /* use cyclic buffer */
	    *qp = &cycbuf[t & CYCBUF_MASK];
	} else {
	    /* use queue */
	    *qp = (uindex *) NULL;
	}
	*tp = t;
	*mp = m;
    }

    return t;
}

/*
 * NAME:	call_out->new()
 * DESCRIPTION:	add a callout
 */
void co_new(unsigned int oindex, unsigned int handle, Uint t,
	unsigned int m, uindex *q)
{
    call_out *co;

    if (q != (uindex *) NULL) {
	co = newcallout(q, t);
    } else {
	if (m == 0xffff) {
	    m = 0;
	}
	co = enqueue(t, m);
    }
    co->handle = handle;
    co->oindex = oindex;
}

/*
 * NAME:	rmshort()
 * DESCRIPTION:	remove a short-term callout
 */
static bool rmshort(uindex *cyc, uindex i, uindex handle, Uint t)
{
    uindex j, k;
    call_out *l;

    k = *cyc;
    if (k != 0) {
	/*
	 * this time-slot is in use
	 */
	l = cotab;
	if (l[k].oindex == i && l[k].handle == handle) {
	    /* first element in list */
	    freecallout(cyc, k, k, t);
	    return TRUE;
	}
	if (l[*cyc].count != 1) {
	    /*
	     * list contains more than 1 element
	     */
	    j = k;
	    k = l[j].next;
	    do {
		if (l[k].oindex == i && l[k].handle == handle) {
		    /* found it */
		    freecallout(cyc, j, k, t);
		    return TRUE;
		}
		j = k;
	    } while ((k = l[j].next) != 0);
	}
    }
    return FALSE;
}

/*
 * NAME:	call_out->remaining()
 * DESCRIPTION:	return the time remaining before a callout expires
 */
Int co_remaining(Uint t, unsigned short *m)
{
    Uint time;
    unsigned short mtime;

    time = co_time(&mtime);

    if (t != 0) {
	t += timediff;
	if (*m == 0xffff) {
	    if (t > time) {
		return t - time;
	    }
	} else if (t == time && *m > mtime) {
	    *m -= mtime;
	} else if (t > time) {
	    if (*m < mtime) {
		--t;
		*m += 1000;
	    }
	    *m -= mtime;
	    return t - time;
	} else {
	    *m = 0xffff;
	}
    }

    return 0;
}

/*
 * NAME:	call_out->del()
 * DESCRIPTION:	remove a callout
 */
void co_del(unsigned int oindex, unsigned int handle, Uint t, unsigned int m)
{
    call_out *l;

    if (m == 0xffff) {
	/*
	 * try to find the callout in the cyclic buffer
	 */
	if (t > timestamp && t < timestamp + CYCBUF_SIZE &&
	    rmshort(&cycbuf[t & CYCBUF_MASK], oindex, handle, t)) {
	    return;
	}
    }

    if (t <= timestamp) {
	/*
	 * possible immediate callout
	 */
	if (rmshort(&immediate, oindex, handle, 0) ||
	    rmshort(&running, oindex, handle, 0)) {
	    return;
	}
    }

    /*
     * Not found in the cyclic buffer; it <must> be in the queue.
     */
    l = cotab;
    for (;;) {
# ifdef DEBUG
	if (l == cotab + queuebrk) {
	    fatal("failed to remove callout");
	}
# endif
	if (l->oindex == oindex && l->handle == handle) {
	    dequeue(l - cotab);
	    return;
	}
	l++;
    }
}

/*
 * NAME:	call_out->list()
 * DESCRIPTION:	adjust callout delays in array
 */
void co_list(array *a)
{
    value *v, *w;
    unsigned short i;
    Uint t;
    unsigned short m;
    xfloat flt1, flt2;

    for (i = a->size, v = a->elts; i != 0; --i, v++) {
	w = &v->u.array->elts[2];
	if (w->type == T_INT) {
	    t = w->u.number;
	    m = 0xffff;
	} else {
	    GET_FLT(w, flt1);
	    t = flt1.low;
	    m = flt1.high;
	}
	t = co_remaining(t, &m);
	if (m == 0xffff) {
	    PUT_INTVAL(w, t);
	} else {
	    flt_itof(t, &flt1);
	    flt_itof(m, &flt2);
	    flt_mult(&flt2, &thousandth);
	    flt_add(&flt1, &flt2);
	    PUT_FLTVAL(w, flt1);
	}
    }
}

/*
 * NAME:	call_out->expire()
 * DESCRIPTION:	collect callouts to run next
 */
static void co_expire()
{
    call_out *co, *first, *last;
    uindex handle, oindex, i, *cyc;
    Uint t;
    unsigned short m;

    t = P_mtime(&m) - timediff;
    if ((timeout != 0 && timeout <= t) ||
	(queuebrk != 0 &&
	 (cotab[0].time < t || (cotab[0].time == t && cotab[0].mtime <= m)))) {
	while (timestamp < t) {
	    timestamp++;

	    /*
	     * from queue
	     */
	    while (queuebrk != 0 && cotab[0].time < timestamp) {
		handle = cotab[0].handle;
		oindex = cotab[0].oindex;
		dequeue(0);
		co = newcallout(&immediate, 0);
		co->handle = handle;
		co->oindex = oindex;
	    }

	    /*
	     * from cyclic buffer list
	     */
	    cyc = &cycbuf[timestamp & CYCBUF_MASK];
	    i = *cyc;
	    if (i != 0) {
		*cyc = 0;
		if (immediate == 0) {
		    immediate = i;
		} else {
		    first = &cotab[immediate];
		    last = (first->count == 1) ? first : &cotab[first->last];
		    last->next = i;
		    first->count += cotab[i].count;
		    first->last = (cotab[i].count == 1) ? i : cotab[i].last;
		}
		nzero += cotab[i].count;
	    }
	}

	/*
	 * from queue
	 */
	while (queuebrk != 0 &&
	       (cotab[0].time < t ||
		(cotab[0].time == t && cotab[0].mtime <= m))) {
	    handle = cotab[0].handle;
	    oindex = cotab[0].oindex;
	    dequeue(0);
	    co = newcallout(&immediate, 0);
	    co->handle = handle;
	    co->oindex = oindex;
	}

	if (timeout <= timestamp) {
	    if (nshort != nzero) {
		for (t = timestamp; cycbuf[t & CYCBUF_MASK] == 0; t++) ;
		timeout = t;
	    } else {
		timeout = 0;
	    }
	}
    }

    /* handle swaprate */
    while (swaptime < t) {
	++swaptime;
	swaprate1 -= swapped1[swaptime % SWPERIOD];
	swapped1[swaptime % SWPERIOD] = 0;
	if (swaptime % 5 == 0) {
	    swaprate5 -= swapped5[swaptime % (5 * SWPERIOD) / 5];
	    swapped5[swaptime % (5 * SWPERIOD) / 5] = 0;
	}
    }
}

/*
 * NAME:	call_out->call()
 * DESCRIPTION:	call expired callouts
 */
void co_call(frame *f)
{
    uindex i, handle;
    object *obj;
    string *str;
    int nargs;
#ifdef CO_THROTTLE
#   if (CO_THROTTLE < 1)
#	error Invalid CO_THROTTLE setting
#   endif
    int quota;

    quota = CO_THROTTLE;
#endif

    if (running == 0) {
	co_expire();
	running = immediate;
	immediate = 0;
    }

    if (running != 0) {
	/*
	 * callouts to do
	 */
	while (ec_push((ec_ftn) errhandler)) {
	    endthread();
	}
#ifdef CO_THROTTLE
	while ((i=running) != 0 && (quota-- > 0)) {
#else
	while ((i=running) != 0) {
#endif
	    handle = cotab[i].handle;
	    obj = OBJ(cotab[i].oindex);
	    freecallout(&running, i, i, 0);

	    str = d_get_call_out(o_dataspace(obj), handle, f, &nargs);
	    if (i_call(f, obj, (array *) NULL, str->text, str->len, TRUE,
		       nargs)) {
		/* function exists */
		i_del_value(f->sp++);
	    }
	    str_del((f->sp++)->u.string);
	    endthread();
	}
	ec_pop();
    }
}

/*
 * NAME:	call_out->info()
 * DESCRIPTION:	give information about callouts
 */
void co_info(uindex *n1, uindex *n2)
{
    *n1 = nshort;
    *n2 = queuebrk;
}

/*
 * NAME:	call_out->delay()
 * DESCRIPTION:	return the time until the next timeout
 */
Uint co_delay(Uint rtime, unsigned int rmtime, unsigned short *mtime)
{
    Uint t;
    unsigned short m;

    if (nzero != 0) {
	/* immediate */
	*mtime = 0;
	return 0;
    }
    if ((rtime | timeout | queuebrk) == 0) {
	/* infinite */
	*mtime = 0xffff;
	return 0;
    }
    if (rtime != 0) {
	rtime -= timediff;
    }
    if (timeout != 0 && (rtime == 0 || timeout <= rtime)) {
	rtime = timeout;
	rmtime = 0;
    }
    if (queuebrk != 0 &&
	(rtime == 0 || cotab[0].time < rtime ||
	 (cotab[0].time == rtime && cotab[0].mtime <= rmtime))) {
	rtime = cotab[0].time;
	rmtime = cotab[0].mtime;
    }
    if (rtime != 0) {
	rtime += timediff;
    }

    t = co_time(&m);
    cotime = 0;
    if (t > rtime || (t == rtime && m >= rmtime)) {
	/* immediate */
	*mtime = 0;
	return 0;
    }
    if (m > rmtime) {
	m -= 1000;
	t++;
    }
    *mtime = rmtime - m;
    return rtime - t;
}

/*
 * NAME:	call_out->swapcount()
 * DESCRIPTION:	keep track of the number of objects swapped out
 */
void co_swapcount(unsigned int count)
{
    swaprate1 += count;
    swaprate5 += count;
    swapped1[swaptime % SWPERIOD] += count;
    swapped5[swaptime % (SWPERIOD * 5) / 5] += count;
    cotime = 0;
}

/*
 * NAME:	call_out->swaprate1()
 * DESCRIPTION:	return the number of objects swapped out per minute
 */
long co_swaprate1()
{
    return swaprate1;
}

/*
 * NAME:	call_out->swaprate5()
 * DESCRIPTION:	return the number of objects swapped out per 5 minutes
 */
long co_swaprate5()
{
    return swaprate5;
}


typedef struct {
    uindex cotabsz;		/* callout table size */
    uindex queuebrk;		/* queue brk */
    uindex cycbrk;		/* cyclic buffer brk */
    uindex flist;		/* free list index */
    uindex nshort;		/* # of short-term callouts */
    uindex running;		/* running callouts */
    uindex immediate;		/* immediate callouts list */
    unsigned short hstamp;	/* timestamp high word */
    unsigned short hdiff;	/* timediff high word */
    Uint timestamp;		/* time the last alarm came */
    Uint timediff;		/* accumulated time difference */
} dump_header;

static char dh_layout[] = "uuuuuuussii";

typedef struct {
    uindex cotabsz;		/* callout table size */
    uindex queuebrk;		/* queue brk */
    uindex cycbrk;		/* cyclic buffer brk */
    uindex flist;		/* free list index */
    uindex nshort;		/* # of short-term callouts */
    uindex running;		/* running callouts */
    uindex immediate;		/* immediate callouts list */
    Uint timestamp;		/* time the last alarm came */
    Uint timediff;		/* accumulated time difference */
} old_header;

static char oh_layout[] = "uuuuuuuii";

typedef struct {
    uindex cotabsz;		/* callout table size */
    uindex queuebrk;		/* queue brk */
    uindex cycbrk;		/* cyclic buffer brk */
    uindex flist;		/* free list index */
    uindex nshort;		/* # of short-term callouts */
    uindex nlong0;		/* # of long-term callouts and imm. callouts */
    Uint timestamp;		/* time the last alarm came */
    Uint timediff;		/* accumulated time difference */
} conv_header;

static char ch_layout[] = "uuuuuuii";

typedef struct {
    uindex list;	/* list */
    uindex last;	/* last in list */
} cbuf;

static char cb_layout[] = "uu";

typedef struct {
    uindex handle;	/* callout handle */
    uindex oindex;	/* index in object table */
    Uint time;		/* when to call */
    uindex mtime;	/* when to call in milliseconds */
} conv_callout;

static char cco_layout[] = "uuiu";

typedef struct {
    uindex handle;	/* callout handle */
    uindex oindex;	/* index in object table */
    Uint time;		/* when to call */
} dump_callout;

static char dco_layout[] = "uui";

/*
 * NAME:	call_out->dump()
 * DESCRIPTION:	dump callout table
 */
bool co_dump(int fd)
{
    dump_header dh;
    unsigned short m;

    /* update timestamp */
    co_time(&m);
    cotime = 0;

    /* fill in header */
    dh.cotabsz = cotabsz;
    dh.queuebrk = queuebrk;
    dh.cycbrk = cycbrk;
    dh.flist = flist;
    dh.nshort = nshort;
    dh.running = running;
    dh.immediate = immediate;
    dh.hstamp = 0;
    dh.hdiff = 0;
    dh.timestamp = timestamp;
    dh.timediff = timediff;

    /* write header and callouts */
    return (P_write(fd, (char *) &dh, sizeof(dump_header)) > 0 &&
	    (queuebrk == 0 ||
	     P_write(fd, (char *) cotab, queuebrk * sizeof(call_out)) > 0) &&
	    (cycbrk == cotabsz ||
	     P_write(fd, (char *) (cotab + cycbrk),
		     (cotabsz - cycbrk) * sizeof(call_out)) > 0) &&
	    P_write(fd, (char *) cycbuf, CYCBUF_SIZE * sizeof(uindex)) > 0);
}

/*
 * NAME:	call_out->restore()
 * DESCRIPTION:	restore callout table
 */
void co_restore(int fd, Uint t, int conv, int conv2, int conv_time)
{
    uindex n, i, offset, last;
    call_out *co;
    uindex *cb;
    uindex buffer[CYCBUF_SIZE];
    unsigned short m;

    /* read and check header */
    timediff = t;
    if (conv2) {
	conv_header ch;

	conf_dread(fd, (char *) &ch, ch_layout, (Uint) 1);
	queuebrk = ch.queuebrk;
	offset = cotabsz - ch.cotabsz;
	cycbrk = ch.cycbrk + offset;
	flist = ch.flist;
	nshort = ch.nshort;
	nzero = ch.nlong0 - ch.queuebrk;
	timestamp = ch.timestamp;
	t = -ch.timediff;
    } else if (conv_time) {
	old_header oh;

	conf_dread(fd, (char *) &oh, oh_layout, (Uint) 1);
	queuebrk = oh.queuebrk;
	offset = cotabsz - oh.cotabsz;
	cycbrk = oh.cycbrk + offset;
	flist = oh.flist;
	nshort = oh.nshort;
	running = oh.running;
	immediate = oh.immediate;
	timestamp = oh.timestamp;
	t = -oh.timediff;
    } else {
	dump_header dh;

	conf_dread(fd, (char *) &dh, dh_layout, (Uint) 1);
	queuebrk = dh.queuebrk;
	offset = cotabsz - dh.cotabsz;
	cycbrk = dh.cycbrk + offset;
	flist = dh.flist;
	nshort = dh.nshort;
	running = dh.running;
	immediate = dh.immediate;
	timestamp = dh.timestamp;
	t = 0;
    }
    timestamp += t;
    timediff -= timestamp;
    if (queuebrk > cycbrk || cycbrk == 0) {
	error("Restored too many callouts");
    }

    /* read tables */
    n = queuebrk + cotabsz - cycbrk;
    if (n != 0) {
	if (conv) {
	    dump_callout *dc;

	    dc = ALLOCA(dump_callout, n);
	    conf_dread(fd, (char *) dc, dco_layout, (Uint) n);

	    for (co = cotab, i = queuebrk; i != 0; co++, --i) {
		co->handle = dc->handle;
		co->oindex = dc->oindex;
		if (dc->time >> 24 == 1) {
		    co->time = co_decode(dc->time, &m);
		    co->mtime = m;
		} else {
		    co->time = dc->time + t;
		    co->mtime = 0;
		}
		dc++;
	    }
	    for (co = cotab + cycbrk, i = cotabsz - cycbrk; i != 0; co++, --i) {
		co->handle = dc->handle;
		co->oindex = dc->oindex;
		co->next = dc->time;
		dc++;
	    }
	    AFREE(dc - n);
	} else if (conv2) {
	    conv_callout *dc;

	    dc = ALLOCA(conv_callout, n);
	    conf_dread(fd, (char *) dc, cco_layout, (Uint) n);

	    for (co = cotab, i = queuebrk; i != 0; co++, --i) {
		co->handle = dc->handle;
		co->oindex = dc->oindex;
		co->time = dc->time + t;
		co->mtime = dc->mtime;
		dc++;
	    }
	    for (co = cotab + cycbrk, i = cotabsz - cycbrk; i != 0; co++, --i) {
		co->handle = dc->handle;
		co->oindex = dc->oindex;
		co->next = dc->time;
		dc++;
	    }
	    AFREE(dc - n);
	} else {
	    conf_dread(fd, (char *) cotab, co_layout, (Uint) queuebrk);
	    conf_dread(fd, (char *) (cotab + cycbrk), co_layout,
		       (Uint) (cotabsz - cycbrk));

	    for (co = cotab, i = queuebrk; i != 0; co++, --i) {
		co->time += t;
	    }
	}
    }
    if (conv2) {
	cbuf cbuffer[CYCBUF_SIZE];

	/* convert free list */
	for (i = flist; i != 0; i = cotab[i].next) {
	    i += offset;
	    cotab[i].prev = cotab[i].oindex;
	}

	conf_dread(fd, (char *) cbuffer, cb_layout, (Uint) CYCBUF_SIZE);

	/* convert cyclic buffer lists */
	for (i = 0, cb = buffer; i < CYCBUF_SIZE; i++, cb++) {
	    *cb = cbuffer[i].list;
	    if (*cb != 0) {
		n = 1;
		last = *cb;
		while (cotab[last + offset].next != 0) {
		    last = cotab[last + offset].next;
		    n++;
		}
		cotab[*cb + offset].count = n;
		cotab[*cb + offset].last = last;
		cotab[last + offset].prev = 0;
	    }
	}
    } else {
	conf_dread(fd, (char *) buffer, "u", (Uint) CYCBUF_SIZE);
    }

    /* cycle around cyclic buffer */
    t &= CYCBUF_MASK;
    memcpy(cycbuf + t, buffer,
	   (unsigned int) (CYCBUF_SIZE - t) * sizeof(uindex));
    memcpy(cycbuf, buffer + CYCBUF_SIZE - t, (unsigned int) t * sizeof(uindex));

    if (conv2) {
	/* fix immediate callouts */
	if (nzero != 0) {
	    cb = &cycbuf[timestamp & CYCBUF_MASK];
	    immediate = *cb + offset;
	    if (cotab[immediate].count == nzero) {
		*cb = 0;
	    } else {
		for (i = nzero - 1, last = *cb; i != 0; --i) {
		    last = cotab[last + offset].next;
		}
		*cb = cotab[last + offset].next;
		cotab[*cb + offset].count = cotab[immediate].count - nzero;
		n = cotab[immediate].last;
		cotab[*cb + offset].last = n;
		cotab[n + offset].prev = 0;
		cotab[n + offset].next = 0;

		cotab[immediate].count = nzero;
		cotab[immediate].last = last;
		cotab[last + offset].prev = 0;
		cotab[last + offset].next = 0;
	    }
	}
    } else {
	nzero = 0;
	if (running != 0) {
	    running += offset;
	    nzero += cotab[running].count;
	}
	if (immediate != 0) {
	    immediate += offset;
	    nzero += cotab[immediate].count;
	}
    }

    if (offset != 0) {
	/* patch callout references */
	if (flist != 0) {
	    flist += offset;
	}
	for (i = CYCBUF_SIZE, cb = cycbuf; i > 0; --i, cb++) {
	    if (*cb != 0) {
		*cb += offset;
	    }
	}
	for (i = cotabsz - cycbrk, co = cotab + cycbrk; i > 0; --i, co++) {
	    if (co->prev != 0) {
		co->prev += offset;
	    }
	    if (co->next != 0) {
		co->next += offset;
	    }
	}
    }

    /* restart callouts */
    if (nshort != nzero) {
	for (t = timestamp; cycbuf[t & CYCBUF_MASK] == 0; t++) ;
	timeout = t;
    }
}

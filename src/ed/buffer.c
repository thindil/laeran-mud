/*
 * This file is part of DGD, https://github.com/dworkin/dgd
 * Copyright (C) 1993-2010 Dworkin B.V.
 * Copyright (C) 2010 DGD Authors (see the commit log for details)
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

# include "ed.h"
# include "buffer.h"

/*
 * This file defines the basic editing operations.
 */

/*
 * NAME:	editbuf->new()
 * DESCRIPTION:	create a new edit buffer
 */
editbuf *eb_new(char *tmpfile)
{
    editbuf *eb;

    eb = ALLOC(editbuf, 1);
    eb->lb = lb_new((linebuf *) NULL, tmpfile);
    eb->buffer = (block) 0;
    eb->lines = 0;

    return eb;
}

/*
 * NAME:	editbuf->del()
 * DESCRIPTION:	delete an edit buffer
 */
void eb_del(editbuf *eb)
{
    lb_del(eb->lb);
    FREE(eb);
}

/*
 * NAME:	editbuf->clear()
 * DESCRIPTION:	reinitialize an edit buffer
 */
void eb_clear(editbuf *eb)
{
    lb_new(eb->lb, (char *) NULL);
    eb->buffer = (block) 0;
    eb->lines = 0;
}

/*
 * NAME:	editbuf->add()
 * DESCRIPTION:	add a new block of lines to the edit buffer after a given line.
 *		If this line is 0 the block is inserted before the other lines
 *		in the edit buffer.
 */
void eb_add(editbuf *eb, Int ln, char *(*getline) ())
{
    block b;

    b = bk_new(eb->lb, getline);
    if (b != (block) 0) {
	Int size;

	size = eb->lines + bk_size(eb->lb, b);
	if (size < 0) {
	    error("Too many lines");
	}

	if (ln == 0) {
	    if (eb->lines == 0) {
		eb->buffer = b;
	    } else {
		eb->buffer = bk_cat(eb->lb, b, eb->buffer);
	    }
	} else if (ln == eb->lines) {
	    eb->buffer = bk_cat(eb->lb, eb->buffer, b);
	} else {
	    block head, tail;

	    bk_split(eb->lb, eb->buffer, ln, &head, &tail);
	    eb->buffer = bk_cat(eb->lb, bk_cat(eb->lb, head, b), tail);
	}

	eb->lines = size;
    }
}

/*
 * NAME:	editbuf->delete()
 * DESCRIPTION:	delete a subrange of lines in the edit buffer
 */
block eb_delete(editbuf *eb, Int first, Int last)
{
    block head, mid, tail;
    Int size;

    size = last - first + 1;

    if (last < eb->lines) {
	bk_split(eb->lb, eb->buffer, last, &mid, &tail);
	if (first > 1) {
	    bk_split(eb->lb, mid, first - 1, &head, &mid);
	    eb->buffer = bk_cat(eb->lb, head, tail);
	} else {
	    eb->buffer = tail;
	}
    } else {
	mid = eb->buffer;
	if (first > 1) {
	    bk_split(eb->lb, mid, first - 1, &head, &mid);
	    eb->buffer = head;
	} else {
	    eb->buffer = (block) 0;
	}
    }
    eb->lines -= size;

    return mid;
}

/*
 * NAME:	editbuf->change()
 * DESCRIPTION:	change a subrange of lines in the edit buffer
 */
void eb_change(editbuf *eb, Int first, Int last, block b)
{
    Int size;
    block head, tail;

    size = eb->lines - (last - first + 1);
    if (b != (block) 0) {
	size += bk_size(eb->lb, b);
	if (size < 0) {
	    error("Too many lines");
	}
    }

    if (last < eb->lines) {
	if (first > 1) {
	    bk_split(eb->lb, eb->buffer, first - 1, &head, (block *) NULL);
	    bk_split(eb->lb, eb->buffer, last, (block *) NULL, &tail);
	    if (b != (block) 0) {
		b = bk_cat(eb->lb, bk_cat(eb->lb, head, b), tail);
	    } else {
		b = bk_cat(eb->lb, head, tail);
	    }
	} else {
	    bk_split(eb->lb, eb->buffer, last, (block *) NULL, &tail);
	    if (b != (block) 0) {
		b = bk_cat(eb->lb, b, tail);
	    } else {
		b = tail;
	    }
	}
    } else if (first > 1) {
	bk_split(eb->lb, eb->buffer, first - 1, &head, (block *) NULL);
	if (b != (block) 0) {
	    b = bk_cat(eb->lb, head, b);
	} else {
	    b = head;
	}
    }
    eb->buffer = b;
    eb->lines = size;
}

/*
 * NAME:	editbuf->yank()
 * DESCRIPTION:	return a subrange block of the edit buffer
 */
block eb_yank(editbuf *eb, Int first, Int last)
{
    block head, mid, tail;

    if (last < eb->lines) {
	bk_split(eb->lb, eb->buffer, last, &mid, &tail);
    } else {
	mid = eb->buffer;
    }
    if (first > 1) {
	bk_split(eb->lb, mid, first - 1, &head, &mid);
    }

    return mid;
}

/*
 * NAME:	editbuf->put()
 * DESCRIPTION:	put a block after a line in the edit buffer. The block is
 *		supplied immediately.
 */
void eb_put(editbuf *eb, Int ln, block b)
{
    Int size;

    size = eb->lines + bk_size(eb->lb, b);
    if (size < 0) {
	error("Too many lines");
    }

    if (ln == 0) {
	if (eb->lines == 0) {
	    eb->buffer = b;
	} else {
	    eb->buffer = bk_cat(eb->lb, b, eb->buffer);
	}
    } else if (ln == eb->lines) {
	eb->buffer = bk_cat(eb->lb, eb->buffer, b);
    } else {
	block head, tail;

	bk_split(eb->lb, eb->buffer, ln, &head, &tail);
	eb->buffer = bk_cat(eb->lb, bk_cat(eb->lb, head, b), tail);
    }

    eb->lines = size;
}

/*
 * NAME:	editbuf->range()
 * DESCRIPTION:	output a subrange of the edit buffer, without first making
 *		a subrange block for it
 */
void eb_range(editbuf *eb, Int first, Int last, void (*putline) (char*), int reverse)
{
    bk_put(eb->lb, eb->buffer, first - 1, last - first + 1, putline, reverse);
}

/*
 * Routines to add lines to a block in pieces. It would be nice if bk_new could
 * be used for this, but this is only possible if the editor functions as a
 * stand-alone program.
 * Lines are stored in a local buffer, which is flushed into a block when full.
 */

static editbuf *eeb;	/* editor buffer */

/*
 * NAME:	add_line()
 * DESCRIPTION:	return the next line from the lines buffer
 */
static char *add_line()
{
    editbuf *eb;

    eb = eeb;
    if (eb->szlines > 0) {
	char *p;
	int len;

	len = strlen(p = eb->llines) + 1;
	eb->llines += len;
	eb->szlines -= len;
	return p;
    }
    return (char *) NULL;
}

/*
 * NAME:	flush_line()
 * DESCRIPTION:	flush the lines buffer into a block
 */
static void flush_line(editbuf *eb)
{
    block b;

    eb->llines = eb->llbuf;
    eeb = eb;
    b = bk_new(eb->lb, add_line);
    if (eb->flines == (block) 0) {
	eb->flines = b;
    } else {
	eb->flines = bk_cat(eb->lb, eb->flines, b);
    }
}

/*
 * NAME:	editbuf->startblock()
 * DESCRIPTION:	start a block of lines
 */
void eb_startblock(editbuf *eb)
{
    eb->flines = (block) 0;
    eb->szlines = 0;
}

/*
 * NAME:	editbuf->addblock()
 * DESCRIPTION:	add a line to the current block of lines
 */
void eb_addblock(editbuf *eb, char *text)
{
    int len;

    len = strlen(text) + 1;

    if (eb->szlines + len >= sizeof(eb->llines)) {
	flush_line(eb);
    }
    memcpy(eb->llbuf + eb->szlines, text, len);
    eb->szlines += len;
}

/*
 * NAME:	editbuf->endblock()
 * DESCRIPTION:	finish the current block
 */
void eb_endblock(editbuf *eb)
{
    if (eb->szlines > 0) {
	flush_line(eb);
    }
}

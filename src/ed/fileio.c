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

# define INCLUDE_FILE_IO
# include "ed.h"
# include "buffer.h"
# include "path.h"
# include "fileio.h"

/*
 * The file I/O operations of the editor.
 */

static int ffd;			/* read/write file descriptor */
static char *buffer;		/* file buffer */
static char *bufp;		/* buffer pointer */
static unsigned int inbuf;	/* # bytes in buffer */
static char *lbuf;		/* line buffer */
static char *lbuflast;		/* end of line buffer */
static io *iostat;		/* I/O status */
static char filename[STRINGSZ];	/* file name */

/*
 * NAME:	get_line()
 * DESCRIPTION:	read a line from the input, return as '\0'-terminated string
 *		without '\n'
 */
static char *get_line()
{
    char c, *p, *bp;
    int i;

    if (iostat->ill) {
	/* previous line was incomplete, therefore the last */
	return (char *) NULL;
    }

    p = lbuf;
    bp = bufp;
    i = inbuf;
    do {
	if (i == 0) {	/* buffer empty */
	    i = P_read(ffd, buffer, BUF_SIZE);
	    if (i <= 0) {
		/* eof or error */
		if (i < 0) {
		    error("error while reading file \"/%s\"", filename);
		}
		if (p == lbuf) {
		    return (char *) NULL;
		} else {
		    p++;	/* make room for terminating '\0' */
		    iostat->ill = TRUE;
		    break;
		}
	    }
	    bp = buffer;
	}
	--i;
	c = *bp++;
	if (c == '\0') {
	    iostat->zero++;	/* skip zeroes */
	} else {
	    if (p == lbuflast && c != LF) {
		iostat->split++;
		i++;
		--bp;
		c = LF;
	    }
	    *p++ = c;
	}
    } while (c != LF);	/* eoln */

    iostat->lines++;
    iostat->chars += p - lbuf;	/* including terminating '\0' */
    bufp = bp;
    inbuf = i;
    *--p = '\0';
    return lbuf;
}

/*
 * NAME:	io_load()
 * DESCRIPTION:	append block read from file after a line
 */
bool io_load(editbuf *eb, char *fname, Int l, io *iobuf)
{
    char b[MAX_LINE_SIZE], buf[BUF_SIZE];
    struct stat sbuf;

    /* open file */
    if (path_ed_read(filename, fname) == (char *) NULL ||
	P_stat(filename, &sbuf) < 0 || (sbuf.st_mode & S_IFMT) != S_IFREG) {
	return FALSE;
    }

    ffd = P_open(filename, O_RDONLY | O_BINARY, 0);
    if (ffd < 0) {
	return FALSE;
    }

    /* initialize buffers */
    buffer = buf;
    inbuf = 0;
    lbuf = b;
    lbuflast = &b[MAX_LINE_SIZE - 1];

    /* initialize statistics */
    iostat = iobuf;
    iostat->lines = 0;
    iostat->chars = 0;
    iostat->zero = 0;
    iostat->split = 0;
    iostat->ill = FALSE;

    /* add the block to the edit buffer */
    if (ec_push((ec_ftn) NULL)) {
	P_close(ffd);
	error((char *) NULL);	/* pass on error */
    }
    eb_add(eb, l, get_line);
    ec_pop();
    P_close(ffd);

    return TRUE;
}

/*
 * NAME:	put_line()
 * DESCRIPTION:	write a line to a file
 */
static void put_line(char *text)
{
    unsigned int len;

    len = strlen(text);
    iostat->lines += 1;
    iostat->chars += len + 1;
    while (inbuf + len >= BUF_SIZE) {	/* flush buffer */
	if (inbuf != BUF_SIZE) {	/* room left for a piece of line */
	    unsigned int chunk;

	    chunk = BUF_SIZE - inbuf;
	    memcpy(buffer + inbuf, text, chunk);
	    text += chunk;
	    len -= chunk;
	}
	if (P_write(ffd, buffer, BUF_SIZE) != BUF_SIZE) {
	    error("error while writing file \"/%s\"", filename);
	}
	inbuf = 0;
    }
    if (len > 0) {			/* piece of line left */
	memcpy(buffer + inbuf, text, len);
	inbuf += len;
    }
    buffer[inbuf++] = LF;
}

/*
 * NAME:	io_save()
 * DESCRIPTION:	write a range of lines to a file
 */
bool io_save(editbuf *eb, char *fname, Int first, Int last, int append, io *iobuf)
{
    char buf[BUF_SIZE];
    struct stat sbuf;

    if (path_ed_write(filename, fname) == (char *) NULL ||
	(P_stat(filename, &sbuf) >= 0 && (sbuf.st_mode & S_IFMT) != S_IFREG))
    {
	return FALSE;
    }
    /* create file */
    ffd = P_open(filename,
		 (append) ? O_CREAT | O_APPEND | O_WRONLY | O_BINARY :
			    O_CREAT | O_TRUNC | O_WRONLY | O_BINARY,
	      0664);
    if (ffd < 0) {
	return FALSE;
    }

    /* initialize buffer */
    buffer = buf;
    inbuf = 0;

    /* initialize statistics */
    iostat = iobuf;
    iostat->lines = 0;
    iostat->chars = 0;
    iostat->zero = 0;
    iostat->split = 0;
    iostat->ill = FALSE;

    /* write range */
    if (ec_push((ec_ftn) NULL)) {
	P_close(ffd);
	error((char *) NULL);	/* pass on error */
    }
    eb_range(eb, first, last, put_line, FALSE);
    if (P_write(ffd, buffer, inbuf) != inbuf) {
	error("error while writing file \"/%s\"", filename);
    }
    ec_pop();
    P_close(ffd);

    return TRUE;
}

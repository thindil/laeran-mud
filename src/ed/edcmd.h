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

# include "regexp.h"
# include "buffer.h"
# include "vars.h"

/* status flags */
# define CB_NOIMAGE	0x0001
# define CB_INSERT	0x0002
# define CB_CHANGE	0x0004
# define CB_GLOBAL	0x0008
# define CB_EXCL	0x0010
# define CB_NUMBER	0x0020
# define CB_LIST	0x0040

/* indentor */
# define CB_PPCONTROL	0x0080
# define CB_COMMENT	0x0100
# define CB_JSKEYWORD	0x0200

/* substitutor */
# define CB_CURRENTBLK	0x0080
# define CB_SKIPPED	0x0100
# define CB_GLOBSUBST	0x0200
# define CB_UPPER	0x0400
# define CB_LOWER	0x0800
# define CB_TUPPER	0x1000
# define CB_TLOWER	0x2000

typedef struct {
    char *cmd;			/* command to do */
    editbuf *edbuf;		/* edit buffer */
    rxbuf *regexp;		/* current regular expression */
    vars *vars;			/* variables */
    jmp_buf env;		/* environment to jump back to after search */
    bool reverse;		/* reverse search */
    bool ignorecase;		/* ignore case */

    short flags;		/* status flags */
    Int edit;			/* number of edits on file */

    Int this;			/* current line number */
    Int othis;			/* current line number after last operation */
    Int first;			/* first line number of current range */
    Int last;			/* last line number of current range */

    Int a_addr;			/* argument address */
    char a_buffer;		/* argument buffer */

    Int lineno;			/* current line number in internal operations */
    char *buffer;		/* buffer for internal operations */
    int buflen;			/* size of buffer */

    /* globals */
    rxbuf *glob_rx;		/* global regexp */
    Int glob_next;		/* next line affected in global */
    Int glob_size;		/* # lines affected in global */

    /* indenting and shifting */
    char *stack, *stackbot;	/* token stack */
    int *ind;			/* indent stack */
    char quote;			/* ' or " */
    short shift;		/* shift amount */

    /* substituting */
    Int offset;			/* offset in lines */
    Int *moffset;		/* mark offsets */

    Int mark[26];		/* line numbers of marks */
    block buf;			/* default yank buffer */
    block zbuf[26];		/* named buffers */

    char fname[STRINGSZ];	/* current filename */

    block undo;			/* undo block */
    Int uthis;			/* current line number after undo */
    Int umark[26];		/* marks after undo */

    char search[STRINGSZ];	/* pattern to search for */
    char replace[STRINGSZ];	/* string to replace with */
} cmdbuf;

# define RET_QUIT	1
# define RET_FLAGS	2

extern cmdbuf *cb_new     (char*);
extern void    cb_del     (cmdbuf*);
extern bool    cb_command (cmdbuf*, char*);
extern int     cb_edit	  (cmdbuf*);

extern cmdbuf *ccb;

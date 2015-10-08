/*
 * This file is part of DGD, https://github.com/dworkin/dgd
 * Copyright (C) 1993-2010 Dworkin B.V.
 * Copyright (C) 2010,2012 DGD Authors (see the commit log for details)
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

# include "dgd.h"
# include "str.h"
# include "array.h"
# include "object.h"
# include "xfloat.h"
# include "interpret.h"
# include "data.h"
# include "path.h"
# include "node.h"
# include "compile.h"

/*
 * NAME:	path->resolve()
 * DESCRIPTION:	resolve a path
 */
char *path_resolve(char *buf, char *file)
{
    char *p, *q, *d;

    strncpy(buf, file, STRINGSZ - 1);
    buf[STRINGSZ - 1] = '\0';
    d = p = q = buf;
    for (;;) {
	if (*p == '/' || *p == '\0') {
	    /* reached a directory separator */
	    if (q - 1 == d && d[0] == '.') {
		/* . */
		q = d;
	    } else if (q - 2 == d && d[0] == '.' && d[1] == '.') {
		/* .. */
		q = d;
		if (q != buf) {
		    for (--q; q != buf && *--q != '/'; ) ;
		}
	    }
	    if (q != buf) {
		if (q[-1] == '/') {
		    /* // or path/ */
		    --q;
		}
		*q++ = *p;
	    }
	    d = q;
	    if (*p == '\0') {
		break;
	    }
	    p++;
	} else {
	    *q++ = *p++;
	}
    }

    if (q == buf) {
	/* "" -> "." */
	*q++ = '.';
	*q = '\0';
    }
    return buf;
}

/*
 * NAME:	path_string()
 * DESCRIPTION:	check and resolve a string path
 */
char *path_string(char *buf, char *file, unsigned int len)
{
    if (len >= STRINGSZ || strlen(file) != len) {
	return (char *) NULL;
    }
    return path_resolve(buf, file);
}

/*
 * NAME:	path->from()
 * DESCRIPTION:	resolve a (possibly relative) path
 */
char *path_from(char *buf, char *from, char *file)
{
    char buf2[STRINGSZ];

    if (file[0] != '/' && strlen(from) + strlen(file) < STRINGSZ - 4) {
	sprintf(buf2, "%s/../%s", from, file);
	file = buf2;
    }
    return path_resolve(buf, file);
}

/*
 * NAME:	path->ed_read()
 * DESCRIPTION:	resolve an editor read file path
 */
char *path_ed_read(char *buf, char *file)
{
    frame *f;

    f = cframe;
    if (OBJR(f->oindex)->flags & O_DRIVER) {
	return path_resolve(buf, file);
    } else {
	PUSH_STRVAL(f, str_new(file, (long) strlen(file)));
	call_driver_object(f, "path_read", 1);
	if (f->sp->type != T_STRING) {
	    i_del_value(f->sp++);
	    return (char *) NULL;
	}
	path_resolve(buf, f->sp->u.string->text);
	str_del((f->sp++)->u.string);
	return buf;
    }
}

/*
 * NAME:	path->ed_write()
 * DESCRIPTION:	resolve an editor write file path
 */
char *path_ed_write(char *buf, char *file)
{
    frame *f;

    f = cframe;
    if (OBJR(f->oindex)->flags & O_DRIVER) {
	return path_resolve(buf, file);
    } else {
	PUSH_STRVAL(f, str_new(file, (long) strlen(file)));
	call_driver_object(f, "path_write", 1);
	if (f->sp->type != T_STRING) {
	    i_del_value(f->sp++);
	    return (char *) NULL;
	}
	path_resolve(buf, f->sp->u.string->text);
	str_del((f->sp++)->u.string);
	return buf;
    }
}

/*
 * NAME:	path->include()
 * DESCRIPTION:	resolve an include path
 */
char *path_include(char *buf, char *from, char *file, string ***strs, int *nstr)
{
    frame *f;
    int i;
    value *v;
    string **str;

    *strs = NULL;
    *nstr = 0;
    if (c_autodriver()) {
	return path_from(buf, from, file);
    }

    f = cframe;
    PUSH_STRVAL(f, str_new(from, strlen(from)));
    PUSH_STRVAL(f, str_new(file, (long) strlen(file)));
    if (!call_driver_object(f, "include_file", 2)) {
	f->sp++;
	return path_from(buf, from, file);
    }

    if (f->sp->type == T_STRING) {
	/* simple path */
	path_resolve(buf, f->sp->u.string->text);
	str_del((f->sp++)->u.string);
	return buf;
    } else if (f->sp->type == T_ARRAY) {
	/*
	 * Array of strings.  Check that the array does indeed contain only
	 * strings, then return it.
	 */
	i = f->sp->u.array->size;
	if (i != 0) {
	    v = d_get_elts(f->sp->u.array);
	    while ((v++)->type == T_STRING) {
		if (--i == 0) {
		    *nstr = i = f->sp->u.array->size;
		    str = ALLOC(string*, i);
		    do {
			str_ref(*str++ = (--v)->u.string);
		    } while (--i != 0);
		    *strs = str;
		    arr_del((f->sp++)->u.array);

		    /* return the untranslated path, as well */
		    return path_from(buf, from, file);
		}
	    }
	}
    }

    i_del_value(f->sp++);
    return (char *) NULL;
}

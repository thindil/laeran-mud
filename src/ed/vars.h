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

typedef struct {
    char *name;		/* long variable name */
    char *sname;	/* short variable name */
    Int val;		/* value */
} vars;

# define IGNORECASE(v)	(v[0].val)
# define SHIFTWIDTH(v)	(v[1].val)
# define WINDOW(v)	(v[2].val)

# define NUMBER_OF_VARS	3

extern vars *va_new  (void);
extern void  va_del  (vars*);
extern void  va_set  (vars*, char*);
extern void  va_show (vars*);

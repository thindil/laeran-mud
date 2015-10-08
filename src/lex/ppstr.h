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
    char *buffer;	/* string buffer */
    int size;		/* size of buffer */
    int len;		/* lenth of string (-1 if illegal) */
} str;

extern void pps_init	(void);
extern void pps_clear	(void);
extern str *pps_new	(char*, int);
extern void pps_del	(str*);
extern int  pps_scat	(str*, char*);
extern int  pps_ccat	(str*, int);

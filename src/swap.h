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

extern bool	sw_init		(char*, unsigned int, unsigned int,
				   unsigned int);
extern void	sw_finish	(void);
extern void	sw_newv		(sector*, unsigned int);
extern void	sw_wipev	(sector*, unsigned int);
extern void	sw_delv		(sector*, unsigned int);
extern void	sw_readv	(char*, sector*, Uint, Uint);
extern void	sw_writev	(char*, sector*, Uint, Uint);
extern void	sw_dreadv	(char*, sector*, Uint, Uint);
extern void	sw_conv		(char*, sector*, Uint, Uint);
extern void	sw_conv2	(char*, sector*, Uint, Uint);
extern sector	sw_mapsize	(unsigned int);
extern sector	sw_count	(void);
extern bool	sw_copy		(Uint);
extern int	sw_dump		(char*, bool);
extern void	sw_dump2	(char*, int, bool);
extern void	sw_restore	(int, unsigned int);
extern void	sw_restore2	(int);

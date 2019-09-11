
/* 
    Copyright (C) 2009  Benjamin Vernoux, titanmkd@gmail.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA */

#ifndef MD5_UTILS_H
#define MD5_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "global_types.h"

#define MD5_SIZE 32
#define MAX_SIZE_MD5 MD5_SIZE+1

	union md5hash
	{
		uint ui[4];
		char ch[16];
	};

	extern bool is_MD5_valid(char *MD5_string);

	extern void ascii_hash_to_md5hash(char *src_md5_str, md5hash *dest_hash);

	extern void md5hash_to_ascii_hash(md5hash *src_hash, char *dest_md5_str);

#ifdef  __cplusplus
}
#endif

#endif /* MD5_UTILS_H */


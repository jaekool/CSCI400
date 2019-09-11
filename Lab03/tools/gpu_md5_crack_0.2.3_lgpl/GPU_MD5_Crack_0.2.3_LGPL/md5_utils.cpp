
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "md5_utils.h"

/* We check MD5 is 32bytes and contain only Hex */
bool is_MD5_valid(char *MD5_string)
{
	if(strlen(MD5_string)!= MD5_SIZE)
		return false;

	for(int i=0; i<MD5_SIZE; i++)
	{
		if(!isxdigit(MD5_string[i]))
		{
			return false;
		}
	}

	return true;
}

/*
Convert ASCII Hash to MD5hash
*/
void ascii_hash_to_md5hash(char *src_md5_str, md5hash *dest_hash)
{
	int tmp, i, j=0;
	for(i=0; i<16; i++)
	{
		sscanf(&src_md5_str[j], "%02x", &tmp);
		dest_hash->ch[i] = (unsigned char)tmp;
		j+=2;
	}
}

/*
Convert MD5hash to ASCII Hash
*/
void md5hash_to_ascii_hash(md5hash *src_hash, char *dest_md5_str)
{
	snprintf(dest_md5_str,MAX_SIZE_MD5,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	(uint)(src_hash->ch[0])&0xFF,
	(uint)(src_hash->ch[1])&0xFF,
	(uint)(src_hash->ch[2])&0xFF,
	(uint)(src_hash->ch[3])&0xFF,
	(uint)(src_hash->ch[4])&0xFF,
	(uint)(src_hash->ch[5])&0xFF,
	(uint)(src_hash->ch[6])&0xFF,
	(uint)(src_hash->ch[7])&0xFF,
	(uint)(src_hash->ch[8])&0xFF,
	(uint)(src_hash->ch[9])&0xFF,
	(uint)(src_hash->ch[10])&0xFF,
	(uint)(src_hash->ch[11])&0xFF,
	(uint)(src_hash->ch[12])&0xFF,
	(uint)(src_hash->ch[13])&0xFF,
	(uint)(src_hash->ch[14])&0xFF,
	(uint)(src_hash->ch[15])&0xFF
	);
}



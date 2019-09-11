
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

#ifndef CHARSET_H
#define CHARSET_H

#ifdef  __cplusplus
extern "C" {
#endif

	//
	// Shared used both by GPU and CPU setup code
	//
	typedef enum t_charset
	{
		CHAR_NUM = 0,
		CHAR_LOW_AZ = 1,
		CHAR_LOW_AZ_NUM = 2,
		CHAR_UP_AZ = 3,
		CHAR_UP_AZ_NUM = 4,
		CHAR_UP_LOW_AZ = 5,
		CHAR_UP_LOW_AZ_NUM = 6,
		CHAR_ALL_PRINTABLE = 7
	}e_charset;

	#define CHAR_NUM_SIZE 10
	#define CHAR_LOW_AZ_SIZE 26
	#define CHAR_LOW_AZ_NUM_SIZE 36
	#define CHAR_UP_AZ_SIZE 26
	#define CHAR_UP_AZ_NUM_SIZE 36
	#define CHAR_UP_LOW_AZ_SIZE 52
	#define CHAR_UP_LOW_AZ_NUM_SIZE 62
	#define CHAR_ALL_PRINTABLE_SIZE 94

	extern void get_charsetCPU_charsetLen(e_charset charset, char **out_charset, unsigned char *out_charsetlen);

#ifdef  __cplusplus
}
#endif

#endif /* CHARSET_H */

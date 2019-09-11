
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

#include "charset.h"

static const char char_num_cpu[CHAR_NUM_SIZE+16] = "0123456789";
static const char char_low_az_cpu[CHAR_LOW_AZ_SIZE+16] = "abcdefghijklmnopqrstuvwxyz";
static const char char_low_az_num_cpu[CHAR_LOW_AZ_NUM_SIZE+16] = "abcdefghijklmnopqrstuvwxyz0123456789";
static const char char_up_az_cpu[CHAR_UP_AZ_SIZE+16] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char char_up_az_num_cpu[CHAR_UP_AZ_NUM_SIZE+16] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static const char char_up_low_az_cpu[CHAR_UP_LOW_AZ_SIZE+16] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static const char char_up_low_az_num_cpu[CHAR_UP_LOW_AZ_NUM_SIZE+16] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static const char char_all_printable_cpu[CHAR_ALL_PRINTABLE_SIZE+16] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

void get_charsetCPU_charsetLen(e_charset charset, char **out_charset, unsigned char *out_charsetlen)
{
	switch(charset)
	{
		case CHAR_NUM:
			*out_charset = (char *)char_num_cpu;
			*out_charsetlen = CHAR_NUM_SIZE;
			break;

		case CHAR_LOW_AZ:
			*out_charset = (char *)char_low_az_cpu;
			*out_charsetlen = CHAR_LOW_AZ_SIZE;
			break;

		case CHAR_LOW_AZ_NUM:
			*out_charset = (char *)char_low_az_num_cpu;
			*out_charsetlen = CHAR_LOW_AZ_NUM_SIZE;
			break;

		case CHAR_UP_AZ:
			*out_charset = (char *)char_up_az_cpu;
			*out_charsetlen = CHAR_UP_AZ_SIZE;
			break;

		case CHAR_UP_AZ_NUM:
			*out_charset = (char *)char_up_az_num_cpu;
			*out_charsetlen = CHAR_UP_AZ_NUM_SIZE;
			break;

		case CHAR_UP_LOW_AZ:
			*out_charset = (char *)char_up_low_az_cpu;
			*out_charsetlen = CHAR_UP_LOW_AZ_SIZE;
			break;

		case CHAR_UP_LOW_AZ_NUM:
			*out_charset = (char *)char_up_low_az_num_cpu;
			*out_charsetlen = CHAR_UP_LOW_AZ_NUM_SIZE;
			break;

		case CHAR_ALL_PRINTABLE:
			*out_charset = (char *)char_all_printable_cpu;
			*out_charsetlen = CHAR_ALL_PRINTABLE_SIZE;
			break;

		default:
		      /* Default Charset */
		      *out_charset = (char *)char_low_az_num_cpu;
		      *out_charsetlen = CHAR_LOW_AZ_NUM_SIZE;
		      break;
	}

}

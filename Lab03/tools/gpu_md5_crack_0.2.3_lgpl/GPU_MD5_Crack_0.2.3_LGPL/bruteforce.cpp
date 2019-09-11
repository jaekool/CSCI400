
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
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <cuda.h>

// includes, project
#include "bruteforce.h"
#include "cuda_bruteforce_gpu.h"
#include "cuda_md5_gpu.h"
#include "md5_utils.h"
#include "timer.h"
#include "gpu_dev.h"

// Timer
unsigned int timer = 0;
float elapsedTimeInMs = 0.0f;

int64 m_Freq = 0;   // does not change while system is running
int64 m_Adjust; // Adjustment time it takes to Start and Stop
int niters = 1;

// GPU hash returned
uint *gpuHash = NULL;
// GPU words
char *gpuWords = NULL;
char *gpu_cpuWords_min10char = NULL;

char *cpuWords = NULL;
uint gpu_cpuWords_Size = 0;
uint gpu_cpuWords_min10char_Size = 0;

char *out_buf_cpu = NULL;
unsigned int out_buf_cpu_size = 0;

inline void update_curr_pwd_pos(unsigned char *pwd,
						unsigned char c0, 
						unsigned char c1, 
						unsigned char c2, 
						unsigned char c3,
						unsigned char c4,
						unsigned char c5,
						unsigned char c6,
						unsigned char c7,
						unsigned char c8,
						unsigned char c9,
						unsigned char c10)
{
	// Update current password position
	pwd[0] = c0; pwd[1] = c1; pwd[2] = c2; pwd[3] = c3; pwd[4] = c4; pwd[5] = c5;
	pwd[6] = c6; pwd[7] = c7; pwd[8] = c8; pwd[9] = c9; pwd[10] = c10;
}

/*
	return 0 if reach pwd_limit_end or other
	return >0 is current index
*/
int bf_depth_max01(unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0;
	uint index16b;

	index16b = 0;
	/* brute force max 12 chars */
	for(c0=pwd[0];c0<1; c0++)
	{
		out_buf[index16b+0] = (unsigned char)c0; //charset[c0];
		out_buf[index16b+1] = 0x80; // bit 1 after the message
		out_buf[index16b+2] = 0;
		out_buf[index16b+3] = 0;
		*((uint *)&out_buf[index16b+4]) = 0;
		*((uint *)&out_buf[index16b+8]) = 0;
		*((uint *)&out_buf[index16b+12]) = 1*8; // message length in bits
		index16b += 16;

		*total_pwd_limit_end = *total_pwd_limit_end - 1;
		if(*total_pwd_limit_end == 0)
		{
			// Exit for loop
			update_curr_pwd_pos(pwd,(unsigned char)c0,0,0,0,0,0,0,0,0,0,0);
			return 0;
		}
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}

/*
	return 0 if reach pwd_limit_end or other
	return >0 is current index
*/
int bf_depth_max02(const int charset_len, unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0, c1;
	uint index16b;

	index16b = 0;
	/* brute force max 12 chars */
	for(c0=pwd[0];c0<charset_len; c0++)
	{
		for(c1=pwd[1];c1<1; c1++)
		{
			out_buf[index16b+0] = (unsigned char)c0;//charset[c0];
			out_buf[index16b+1] = (unsigned char)c1;//charset[c1];
			out_buf[index16b+2] = 0x80; // bit 1 after the message
			out_buf[index16b+3] = 0;
			*((uint *)&out_buf[index16b+4]) = 0;
			*((uint *)&out_buf[index16b+8]) = 0;
			*((uint *)&out_buf[index16b+12]) = 2*8; // message length in bits
			index16b += 16;

			*total_pwd_limit_end = *total_pwd_limit_end - 1;
			if(*total_pwd_limit_end == 0)
			{
				// Exit for loop
				update_curr_pwd_pos(pwd,(unsigned char)c0,(unsigned char)c1,0,0,0,0,0,0,0,0,0);
				return 0;
			}
		}
		pwd[1] = 0;
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}

/*
	return 0 if reach pwd_limit_end or other
	return >0 is current index
*/
int bf_depth_max03(const int charset_len, unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0, c1, c2;
	uint index16b;
	uint val1;

	index16b = 0;
	/* brute force max 12 chars */
	for(c0=pwd[0];c0<charset_len; c0++)
	{
	for(c1=pwd[1];c1<charset_len; c1++)
	{
		for(c2=pwd[2];c2<1; c2++)
		{
			val1 = ( 0x00000080 << 24 | c2 << 16 | c1 << 8 | c0);
			*((uint *)&out_buf[index16b+0]) = val1;
			*((uint *)&out_buf[index16b+4]) = 0;
			*((uint *)&out_buf[index16b+8]) = 0;
			*((uint *)&out_buf[index16b+12]) = 3*8; // message length in bits
			index16b += 16;

			*total_pwd_limit_end = *total_pwd_limit_end - 1;
			if(*total_pwd_limit_end == 0)
			{
				// Exit for loop
				update_curr_pwd_pos(pwd,(unsigned char)c0,(unsigned char)c1,(unsigned char)c2,0,0,0,0,0,0,0,0);
				return 0;
			}
		}
		pwd[2] = 0;
	}
	pwd[1] = 0;
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}

/*
	return 0 if reach pwd_limit_end or other
	return >0 is current index
*/
int bf_depth_max04(const int charset_len, unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0, c1, c2, c3;
	uint index16b;
	uint val1, val2;

	index16b = 0;
	/* brute force max 12 chars */
	for(c0=pwd[0];c0<charset_len; c0++)
	{
	for(c1=pwd[1];c1<charset_len; c1++)
	{
	for(c2=pwd[2];c2<charset_len; c2++)
	{
		for(c3=pwd[3];c3<1; c3++)
		{
			val1 = ( c3 << 24 | c2 << 16 | c1 << 8 | c0);
			val2 = 0x00000080;
			*((uint *)&out_buf[index16b+0]) = val1;
			*((uint *)&out_buf[index16b+4]) = val2;
			*((uint *)&out_buf[index16b+8]) = 0;
			*((uint *)&out_buf[index16b+12]) = 4*8; // message length in bits
			index16b += 16;

			*total_pwd_limit_end = *total_pwd_limit_end - 1;
			if(*total_pwd_limit_end == 0)
			{
				// Exit for loop
				update_curr_pwd_pos(pwd,(unsigned char)c0,(unsigned char)c1,(unsigned char)c2,(unsigned char)c3,
					0,0,0,0,0,0,0);
				return 0;
			}
		}
		pwd[3] = 0;
	}
	pwd[2] = 0;
	}
	pwd[1] = 0;
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}

/*
	return 0 if reach pwd_limit_end or other
	return >0 is current index
*/
int bf_depth_max05(const int charset_len, unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0, c1, c2, c3, c4;
	uint val1, val2;
	uint index16b;

	index16b = 0;

	/* brute force max 12 chars */
	for(c0=pwd[0];c0<charset_len; c0++)
	{
	for(c1=pwd[1];c1<charset_len; c1++)
	{
	for(c2=pwd[2];c2<charset_len; c2++)
	{
	for(c3=pwd[3];c3<charset_len; c3++)
	{
		val1 = ( c3 << 24 | c2 << 16 | c1 << 8 | c0);
		for(c4=pwd[4];c4<1; c4++)
		{	
			val2 =	( 0x00000000 << 24 | 0x00000000 << 16 | 0x00000080 << 8 | c4 );
			*((uint *)&out_buf[index16b+0]) = val1;
			*((uint *)&out_buf[index16b+4]) = val2;
			*((uint *)&out_buf[index16b+8]) = 0;
			*((uint *)&out_buf[index16b+12]) = 5*8; // message length in bits
			index16b += 16;

			*total_pwd_limit_end = *total_pwd_limit_end - 1;
			if(*total_pwd_limit_end == 0)
			{
				// Exit for loop
				update_curr_pwd_pos(pwd,(unsigned char)c0,(unsigned char)c1,(unsigned char)c2,(unsigned char)c3,
					(unsigned char)c4,0,0,0,0,0,0);
				return 0;
			}
		}
		pwd[4] = 0;
	}
	pwd[3] = 0;
	}
	pwd[2] = 0;
	}
	pwd[1] = 0;
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}

int bf_depth_max06(const int charset_len, unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0, c1, c2, c3, c4, c5;
	uint index16b;
	uint val1, val2;

	index16b = 0;

	/* brute force max 12 chars */
	for(c0=pwd[0];c0<charset_len; c0++)
	{
	for(c1=pwd[1];c1<charset_len; c1++)
	{
	for(c2=pwd[2];c2<charset_len; c2++)
	{
	for(c3=pwd[3];c3<charset_len; c3++)
	{
		val1 = ( c3 << 24 | c2 << 16 | c1 << 8 | c0);
	for(c4=pwd[4];c4<charset_len; c4++)
	{
		for(c5=pwd[5];c5<1; c5++)
		{
			val2 =	(0x00000080 << 16 | c5 << 8 | c4 );
			*((uint *)&out_buf[index16b+0]) = val1;
			*((uint *)&out_buf[index16b+4]) = val2;
			*((uint *)&out_buf[index16b+8]) = 0;
			*((uint *)&out_buf[index16b+12]) = 6*8; // message length in bits
			index16b += 16;

			*total_pwd_limit_end-=1;
			if(*total_pwd_limit_end == 0)
			{
				// Exit for loop
				update_curr_pwd_pos(pwd,(unsigned char)c0,(unsigned char)c1,(unsigned char)c2,(unsigned char)c3,
					(unsigned char)c4,(unsigned char)c5,0,0,0,0,0);
				return 0;
			}
		}
		pwd[5] = 0;
	}
	pwd[4] = 0;
	}
	pwd[3] = 0;
	}
	pwd[2] = 0;
	}
	pwd[1] = 0;
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}

/*
	return 0 if reach pwd_limit_end or other
	return >0 is current index
*/
int bf_depth_max07(const int charset_len, unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0, c1, c2, c3, c4, c5, c6;
	uint index16b;
	uint val1, val2;

	index16b = 0;

	/* brute force max 12 chars */
	for(c0=pwd[0];c0<charset_len; c0++)
	{
	for(c1=pwd[1];c1<charset_len; c1++)
	{
	for(c2=pwd[2];c2<charset_len; c2++)
	{
	for(c3=pwd[3];c3<charset_len; c3++)
	{
		val1 = ( c3 << 24 | c2 << 16 | c1 << 8 | c0);
	for(c4=pwd[4];c4<charset_len; c4++)
	{
	for(c5=pwd[5];c5<charset_len; c5++)
	{
		for(c6=pwd[6];c6<1; c6++)
		{
			val2 =	( 0x00000080 << 24 | c6 << 16 | c5 << 8 | c4);
			*((uint *)&out_buf[index16b+0]) = val1;
			*((uint *)&out_buf[index16b+4]) = val2;
			*((uint *)&out_buf[index16b+8]) = 0;
			*((uint *)&out_buf[index16b+12]) = 7*8; // message length in bits
			index16b += 16;

			*total_pwd_limit_end-=1;
			if(*total_pwd_limit_end == 0)
			{
				// Exit for loop
				update_curr_pwd_pos(pwd,(unsigned char)c0,(unsigned char)c1,(unsigned char)c2,(unsigned char)c3,
					(unsigned char)c4,(unsigned char)c5,(unsigned char)c6,0,0,0,0);
				return 0;
			}
		}
		pwd[6] = 0;
	}
	pwd[5] = 0;
	}
	pwd[4] = 0;
	}
	pwd[3] = 0;
	}
	pwd[2] = 0;
	}
	pwd[1] = 0;
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}


/*
	return 0 if reach pwd_limit_end or other
	return >0 is current index
*/
int bf_depth_max08(const int charset_len, unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0, c1, c2, c3, c4, c5, c6, c7;
	uint index16b;
	uint val1, val2, val3;

	index16b = 0;

	/* brute force max 12 chars */
	for(c0=pwd[0];c0<charset_len; c0++)
	{
	for(c1=pwd[1];c1<charset_len; c1++)
	{
	for(c2=pwd[2];c2<charset_len; c2++)
	{
	for(c3=pwd[3];c3<charset_len; c3++)
	{
		val1 = ( c3 << 24 | c2 << 16 | c1 << 8 | c0);
	for(c4=pwd[4];c4<charset_len; c4++)
	{
	for(c5=pwd[5];c5<charset_len; c5++)
	{
	for(c6=pwd[6];c6<charset_len; c6++)
	{
		for(c7=pwd[7];c7<1; c7++)
		{
			val2 = ( c7 << 24 | c6 << 16 | c5 << 8 | c4 );
			val3 = 0x00000080;
			*((uint *)&out_buf[index16b+0]) = val1;
			*((uint *)&out_buf[index16b+4]) = val2;
			*((uint *)&out_buf[index16b+8]) = val3;
			*((uint *)&out_buf[index16b+12]) = 8*8; // message length in bits
			index16b += 16;

			*total_pwd_limit_end = *total_pwd_limit_end - 1;
			if(*total_pwd_limit_end == 0)
			{
				// Exit for loop
				update_curr_pwd_pos(pwd,(unsigned char)c0,(unsigned char)c1,(unsigned char)c2,(unsigned char)c3,
					(unsigned char)c4,(unsigned char)c5,(unsigned char)c6,(unsigned char)c7,0,0,0);
				return 0;
			}
		}
		pwd[7] = 0;
	}
	pwd[6] = 0;
	}
	pwd[5] = 0;
	}
	pwd[4] = 0;	
	}
	pwd[3] = 0;
	}
	pwd[2] = 0;
	}
	pwd[1] = 0;
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}

/*
	return 0 if reach pwd_limit_end or other
	return >0 is current index
*/
int bf_depth_max09(const int charset_len, unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0, c1, c2, c3, c4, c5, c6, c7, c8;
	uint index16b;
	uint val1, val2, val3;

	index16b = 0;

	/* brute force max 12 chars */
	for(c0=pwd[0];c0<charset_len; c0++)
	{
	for(c1=pwd[1];c1<charset_len; c1++)
	{
	for(c2=pwd[2];c2<charset_len; c2++)
	{
	for(c3=pwd[3];c3<charset_len; c3++)
	{
		val1 = ( c3 << 24 | c2 << 16 | c1 << 8 | c0);
	for(c4=pwd[4];c4<charset_len; c4++)
	{
	for(c5=pwd[5];c5<charset_len; c5++)
	{
	for(c6=pwd[6];c6<charset_len; c6++)
	{
	for(c7=pwd[7];c7<charset_len; c7++)
	{
		val2 =	( c7 << 24 | c6 << 16 | c5 << 8 | c4 );
		for(c8=pwd[8];c8<1; c8++)
		{
			val3 = (0x00000080 << 8 | c8);
			*((uint *)&out_buf[index16b+0]) = val1;
			*((uint *)&out_buf[index16b+4]) = val2;
			*((uint *)&out_buf[index16b+8]) = val3;
			*((uint *)&out_buf[index16b+12]) = 9*8; // message length in bits
			index16b += 16;

			*total_pwd_limit_end = *total_pwd_limit_end - 1;
			if(*total_pwd_limit_end == 0)
			{
				// Exit for loop
				update_curr_pwd_pos(pwd,(unsigned char)c0,(unsigned char)c1,(unsigned char)c2,(unsigned char)c3,
					(unsigned char)c4,(unsigned char)c5,(unsigned char)c6,(unsigned char)c7,(unsigned char)c8,0,0);
				return 0;
			}
		}
		pwd[8] = 0;
	}
	pwd[7] = 0;
	}
	pwd[6] = 0;
	}
	pwd[5] = 0;
	}
	pwd[4] = 0;
	}
	pwd[3] = 0;
	}
	pwd[2] = 0;
	}
	pwd[1] = 0;
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}

/*
	return 0 if reach pwd_limit_end or other
	return >0 is current index
*/
int bf_depth_max10(const int charset_len, unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0, c1, c2, c3, c4, c5, c6, c7, c8, c9;
	uint index16b;
	uint val1, val2, val3;

	index16b = 0;

	/* brute force max 12 chars */
	for(c0=pwd[0];c0<charset_len; c0++)
	{
	for(c1=pwd[1];c1<charset_len; c1++)
	{
	for(c2=pwd[2];c2<charset_len; c2++)
	{
	for(c3=pwd[3];c3<charset_len; c3++)
	{
		val1 = ( c3 << 24 | c2 << 16 | c1 << 8 | c0);
	for(c4=pwd[4];c4<charset_len; c4++)
	{
	for(c5=pwd[5];c5<charset_len; c5++)
	{
	for(c6=pwd[6];c6<charset_len; c6++)
	{
	for(c7=pwd[7];c7<charset_len; c7++)
	{
		val2 =	( c7 << 24 | c6 << 16 | c5 << 8 | c4 );
	for(c8=pwd[8];c8<charset_len; c8++)
	{
		for(c9=pwd[9];c9<1; c9++)
		{
			val3 = (0x00000080 << 16 | c9 << 8 | c8);
			*((uint *)&out_buf[index16b+0]) = val1;
			*((uint *)&out_buf[index16b+4]) = val2;
			*((uint *)&out_buf[index16b+8]) = val3;
			*((uint *)&out_buf[index16b+12]) = 10*8; // message length in bits
			index16b += 16;

			*total_pwd_limit_end = *total_pwd_limit_end - 1;
			if(*total_pwd_limit_end == 0)
			{
				// Exit for loop
				update_curr_pwd_pos(pwd,(unsigned char)c0,(unsigned char)c1,(unsigned char)c2,(unsigned char)c3,
					(unsigned char)c4,(unsigned char)c5,(unsigned char)c6,(unsigned char)c7,(unsigned char)c8,
					(unsigned char)c9,0);
				return 0;
			}
		}
		pwd[9] = 0;
	}
	pwd[8] = 0;
	}
	pwd[7] = 0;
	}
	pwd[6] = 0;
	}
	pwd[5] = 0;
	}
	pwd[4] = 0;
	}
	pwd[3] = 0;
	}
	pwd[2] = 0;
	}
	pwd[1] = 0;
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}

/*
	return 0 if reach pwd_limit_end or other
	return >0 is current index
*/
int bf_depth_max11(const int charset_len, unsigned char *pwd, uint *total_pwd_limit_end, unsigned char *out_buf)
{
	int c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10;
	uint val1, val2, val3;	
	uint index16b;

	index16b = 0;

	/* brute force max 12 chars */
	for(c0=pwd[0];c0<charset_len; c0++)
	{
	for(c1=pwd[1];c1<charset_len; c1++)
	{
	for(c2=pwd[2];c2<charset_len; c2++)
	{
	for(c3=pwd[3];c3<charset_len; c3++)
	{
		val1 = ( c3 << 24 | c2 << 16 | c1 << 8 | c0);
	for(c4=pwd[4];c4<charset_len; c4++)
	{
	for(c5=pwd[5];c5<charset_len; c5++)
	{
	for(c6=pwd[6];c6<charset_len; c6++)
	{
	for(c7=pwd[7];c7<charset_len; c7++)
	{
		val2 =	( c7 << 24 | c6 << 16 | c5 << 8 | c4 );
	for(c8=pwd[8];c8<charset_len; c8++)
	{
	for(c9=pwd[9];c9<charset_len; c9++)
	{
		for(c10=pwd[10];c10<1; c10++)
		{
			val3 = (0x00000080 << 24 | c10 << 16 | c9 << 8 | c8);
			*((uint *)&out_buf[index16b+0]) = val1;
			*((uint *)&out_buf[index16b+4]) = val2;
			*((uint *)&out_buf[index16b+8]) = val3;
			*((uint *)&out_buf[index16b+12]) = 11*8; // message length in bits
			index16b += 16;

			*total_pwd_limit_end = *total_pwd_limit_end - 1;
			if(*total_pwd_limit_end == 0)
			{
				// Exit for loop
				update_curr_pwd_pos(pwd,(unsigned char)c0,(unsigned char)c1,(unsigned char)c2,(unsigned char)c3,
					(unsigned char)c4,(unsigned char)c5,(unsigned char)c6,(unsigned char)c7,(unsigned char)c8,
					(unsigned char)c9,(unsigned char)c10);
				return 0;
			}
		}
		pwd[10] = 0;
	}
	pwd[9] = 0;
	}
	pwd[8] = 0;
	}
	pwd[7] = 0;
	}
	pwd[6] = 0;
	}
	pwd[5] = 0;
	}
	pwd[4] = 0;
	}
	pwd[3] = 0;
	}
	pwd[2] = 0;
	}
	pwd[1] = 0;
	}
	update_curr_pwd_pos(pwd,0,0,0,0,0,0,0,0,0,0,0);
	return index16b;
}

/* 
In e_charset charset
In unsigned char *pwdstr_cur
Out unsigned char *pwd_table
If < 0 Return Error
If > 0 Return PWD Len
*/
int ConvPwdStr_To_PwdTable(e_charset charset, unsigned char *pwdstr_cur, unsigned char *pwd_table)
{
	#define PWD_MAX 16
	char *cCharset = NULL;
	unsigned char CHAR_LEN;
	unsigned int i, index;
	unsigned int pwd_len;
	bool found_char;

	get_charsetCPU_charsetLen(charset, &cCharset, &CHAR_LEN);

	memset(pwd_table,0,PWD_MAX);

	/* Convert from password string current pos to pwd_table */
	for(i=0; (pwdstr_cur[i] != 0) && (i<PWD_MAX) ; i++)
	{
		found_char = false;
		for(index=0; index<CHAR_LEN; index++)
		{
			if(cCharset[index] == pwdstr_cur[i])
			{
				pwd_table[i] = (unsigned char)index;
				found_char = true;
				break;
			}
		}
		if(found_char == false)
		{
			// Error not found in charset
			// Wrong charset ??
			return -2;
		}
	}

	if(i == 0)
	{
		pwd_len = 1;
	}else
	{
		pwd_len = i;
	}

	return pwd_len;
}

/*
	return >=0 time elapsed in ms
	return < 0
			-1 : Error nb_pwd > out_buf
			-2 : Error not found in charset, wrong charset (not corresponding with pwdstr_cur chars) ??
*/
double inline bruteforce_cpu_md5prep(e_charset charset, unsigned char *pwdstr_cur, uint nb_pwd, unsigned char *out_buf, uint out_buf_size)
{
#define STR_MAX 12
#define PWD_MAX 16

	int64   start, stop;
	double   elapsed_time;

	int len_tmp;
	char *cCharset = NULL;
	unsigned char CHAR_LEN;
	unsigned char pwd_table[PWD_MAX];
	unsigned int i;
	unsigned int index16b, index16_curr, pwd_len;
	uint nbpwd;
	Timer timing;

	get_charsetCPU_charsetLen(charset, &cCharset, &CHAR_LEN);

	if(nb_pwd%CHAR_LEN != 0)
	{
		nbpwd = (nb_pwd / CHAR_LEN) + 1;
	}else
		nbpwd = nb_pwd / CHAR_LEN;

	if(nbpwd > (out_buf_size/16))
		return -1.0;

	len_tmp = ConvPwdStr_To_PwdTable(charset, pwdstr_cur, pwd_table);
	if(len_tmp < 0)
		return len_tmp;
	pwd_len = len_tmp;
	
	index16b = 0;
	index16_curr = 0;
	start = timing.StartTiming();

	if(pwd_len == 1)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max01(pwd_table, &nbpwd, &out_buf[index16b]);
	}
	
	if(pwd_len == 2 || index16_curr)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max02(CHAR_LEN, pwd_table, &nbpwd, &out_buf[index16b]);
		pwd_len = 2;
	}

	if(pwd_len == 3 || index16_curr)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max03(CHAR_LEN, pwd_table, &nbpwd, &out_buf[index16b]);
		pwd_len = 3;
	}

	if(pwd_len == 4 || index16_curr)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max04(CHAR_LEN, pwd_table, &nbpwd, &out_buf[index16b]);
		pwd_len = 4;
	}

	if(pwd_len == 5 || index16_curr)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max05(CHAR_LEN, pwd_table, &nbpwd, &out_buf[index16b]);
		pwd_len = 5;
	}

	if(pwd_len == 6 || index16_curr)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max06(CHAR_LEN, pwd_table, &nbpwd, &out_buf[index16b]);
		pwd_len = 6;
	}

	if(pwd_len == 7 || index16_curr)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max07(CHAR_LEN, pwd_table, &nbpwd, &out_buf[index16b]);
		pwd_len = 7;
	}

	if(pwd_len == 8 || index16_curr)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max08(CHAR_LEN, pwd_table, &nbpwd, &out_buf[index16b]);
		pwd_len = 8;
	}

	if(pwd_len == 9 || index16_curr)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max09(CHAR_LEN, pwd_table, &nbpwd, &out_buf[index16b]);
		pwd_len = 9;
	}

	if(pwd_len == 10 || index16_curr)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max10(CHAR_LEN, pwd_table, &nbpwd, &out_buf[index16b]);
		pwd_len = 10;
	}

	if(pwd_len == 11 || index16_curr)
	{
		index16b += index16_curr;
		index16_curr = bf_depth_max11(CHAR_LEN, pwd_table, &nbpwd, &out_buf[index16b]);
		pwd_len = 11;
	}

	/* Convert from pwd_table current pos to password string current pos */
	for(i=0; i<pwd_len ; i++)
	{
		pwdstr_cur[i] = cCharset[pwd_table[i]];
	}
	pwdstr_cur[i] = 0;

	stop = timing.StopTiming();
	elapsed_time = timing.ElapsedTiming(start, stop);
	//printf( "\nGenerated %01.06f millions pwd, %01.06f millions pwd/sec\nProgram takes %01.04f seconds.\n", ((double)(nbpwd))/1000000.0, (((double)(nbpwd))/elapsed_time)/1000000.0, elapsed_time );

    return elapsed_time;
}

int print_devices(void)
{
	#define MAX_INFO_DEV_SIZE 512
	struct devInfo gpuDevInfo;
	char infodev[MAX_INFO_DEV_SIZE+2];
	GPU_Dev gpuDev = GPU::getInstance();

	infodev[MAX_INFO_DEV_SIZE] = infodev[MAX_INFO_DEV_SIZE+1] = 0;
	if( gpuDev.getDevs(&gpuDevInfo) < 0)
	{
		printf("Error no device found!\n");
		return -1;
	}
	printf("Devices List:\n");
	for(int i=0; i<gpuDevInfo.deviceCount; i++)
	{
		if( gpuDev.getDevInfoStr(gpuDevInfo.Devs[0].CUDA_device_ID, infodev, MAX_INFO_DEV_SIZE) >= 0)
			printf("%s",infodev);
	}
	return 0;
}

/*
Initialize memory and other stuff for CPU and GPU
*/
int init_bruteforce_gpu(uint max_pwd, int gpu_device)
{
	#define MAX_INFO_DEV_SIZE 512
	struct devInfo gpuDevInfo;
	char infodev[MAX_INFO_DEV_SIZE+2];
	GPU_Dev gpuDev = GPU::getInstance();
	
	infodev[MAX_INFO_DEV_SIZE] = infodev[MAX_INFO_DEV_SIZE+1] = 0;
	
	if(gpuHash  == NULL)
	{
		if( gpuDev.getDevs(&gpuDevInfo) < 0)
		{
			printf("Error no CUDA GPU device found !!\n");
			return -1;
		}

		/* Set the GPU Device to be used for GPU computation */
		if( gpuDev.setDevice(gpu_device) > 0 )
		{
			printf("Using CUDA GPU device\n");
			if( gpuDev.getDevInfoStr(gpu_device, infodev, MAX_INFO_DEV_SIZE) < 0)
				return -1;
			printf("%s", infodev);
		}else
		{
			/* Set the default GPU Device to be used for GPU computation */
			if( gpuDev.setDevice(gpuDevInfo.Devs[0].CUDA_device_ID) <0 )
				return -1;
			printf("Using default CUDA GPU device:%d\n",gpuDevInfo.Devs[0].CUDA_device_ID);
			if( gpuDev.getDevInfoStr(gpuDevInfo.Devs[0].CUDA_device_ID, infodev, MAX_INFO_DEV_SIZE) < 0)
				return -1;
			printf("%s", infodev);
		}

		gpu_cpuWords_Size = 16*(max_pwd+4096); /* Worst case round+1 = 32Threads*94char=3008=>*16Bytes=48128Bytes */
		gpu_cpuWords_min10char_Size = gpu_cpuWords_Size / 8;
		// Allocate host memory
		if( gpuDev.mallocHost((void**)&cpuWords, gpu_cpuWords_Size) < 0)
			return -2;
		if(cpuWords == NULL)
			return -2;

		// allocate device memory
		// Prepare buffer to load the CPU password onto the GPU device
		if( gpuDev.mallocGPU((void **)&gpu_cpuWords_min10char, gpu_cpuWords_min10char_Size) < 0 )
			return -2;

		// Prepare buffer to load the MD5 prep bruteforce password onto the GPU device
		if( gpuDev.mallocGPU((void **)&gpuWords, gpu_cpuWords_Size) < 0 )
			return -2;

		// allocate GPU memory for match signal, instead of the actual hashes
		if( gpuDev.mallocGPU((void **)&gpuHash, 4*sizeof(uint)) < 0 )
			return -2;

		return 0;
	}
	else
	{
		/* Already initialized */
		return -1;
	}
	
}

int set_md5_gpu_parameters(e_charset charset, uint *target)
{
	/* Load the Charset in GPU constant memory */
	if( init_constants_bruteforce_gpu(charset) != 0)
	{
		return -2;
	}

	// load the MD5 constant target to search into GPU constant memory and Init all
	init_md5_target_constants(target);

	return 0;
}

/*
Return 0 if no error
Return <>0 if error
*/

int last_error_string(char *str, uint str_len)
{
	return GPU::getInstance().last_error_string(str,str_len);
}

int exit_bruteforce(void)
{
	GPU_Dev gpuDev = GPU::getInstance();

	// Shutdown
	if(cpuWords != NULL)
	{
		gpuDev.freeHost(cpuWords);
		cpuWords = NULL;
	}
	
	if(gpu_cpuWords_min10char  != NULL)
	{
		gpuDev.freeGPU(gpu_cpuWords_min10char);
		gpu_cpuWords_min10char = NULL;
	}

	if(gpuWords  != NULL)
	{
		gpuDev.freeGPU(gpuWords);
		gpuWords = NULL;
	}

	if(gpuHash  != NULL)
	{
		gpuDev.freeGPU(gpuHash);
		gpuHash = NULL;
	}

	return 0;
}

/*
if < 0 error impossible to found exact ReqGridSize or error on parameters exceeding limits
*/
#if 0
int compute_GridSize(uint64 ReqTotalThreads, int ThreadPerBlocks, Nb, int *gridDim[3])
{
	/* Max Thread per Block is 512 */
	if(ThreadPerBlocks > 512)
		return -1;

	/* The maximum sizes of the x-, y-, and z-dimension of a thread block are 512, 512, 
	and 64, respectively so 16777216 */


	uint64 TotalGrid = ReqTotalThreads / ThreadPerBlocks;
	*gridDim[0] = 
}
#endif
//
// GPU MD5 calculation & check
// hash = passwd found if found hashes[0].ui[3] == 1 then hashes[0].ui[0] equal passwd position
//
void cuda_compute_md5s(pwdfound *pwd_found, uint *target, uint *gpuHash, char *gpuWords)
{
	int gridDim[3];
	//
	// compute the optimal number of threads per block,
	// and the number of blocks
	//
	int dynShmemPerThread = 64;	// built in the algorithm
	/*
	int staticShmemPerBlock = 32;	// read from .cubin file
	*/

	/* Max Thread per Block is 512 */
	//int tpb = benchmark ? 10 : 32;	// tpb is number of threads per block
	int tpb = 32; 
	// 32 is the experimentally determined best case scenario on my 8800 GT		
	// 63 is the experimentally determined best case scenario on my 8800 GTX Ultra

	/* Per each threads we compute x MD5 */


	gridDim[0] = 62505; /* Max size 65535 */
	gridDim[1] = 4; /* Max size 65535 */
	gridDim[2] = 1; /* Max size 65535 */

	/* Total Threads = tpb * gridDim[0] * gridDim[1] = 8000640 */

	execute_kernel_md5_search(gridDim[0], gridDim[1], tpb, tpb*dynShmemPerThread, gpuHash, gpuWords);

	// Download the results
	if(target != NULL)
	{
	  cudaMemcpy(pwd_found, gpuHash, sizeof(uint)*4, cudaMemcpyDeviceToHost);
	}	
}

/*
return <0 if error
if >= 0 elapsed time in ms
In buffer = 100MB max
*/
double md5_crack_brute_force(e_charset charset, uint *target, unsigned char *pwdstr_cur, uint nb_pwd, pwdfound *pwd_found)
{
	double elapsed_time_ms_global;
	double elapsed_time_ms_bf, elapsed_time_ms_memcpy, elapsed_time_ms_gpu;

	uint64 gpuWords_num;
	uint64 t_start, t_end; 
	uint64 t_start_mem, t_end_mem; 
	uint gpu_cpuWords_Size_tmp;
	Timer timing;

	t_start = timing.StartTiming();

	gpu_cpuWords_Size_tmp = nb_pwd*16;
	if(gpu_cpuWords_Size_tmp == 0 || gpu_cpuWords_Size_tmp > gpu_cpuWords_Size)
		return 0.0;

	elapsed_time_ms_bf = bruteforce_cpu_gpu(charset, pwdstr_cur, nb_pwd);
	t_start_mem = timing.StartTiming();
	cudaMemcpy(gpuHash, pwd_found, sizeof(uint)*4, cudaMemcpyHostToDevice);
	t_end_mem = timing.StopTiming();

	gpuWords_num = gpu_cpuWords_Size_tmp/16;
	cuda_compute_md5s(pwd_found, target, gpuHash, gpuWords);

	t_end = timing.StopTiming();
	elapsed_time_ms_memcpy = timing.ElapsedTiming(t_start_mem, t_end_mem);
	elapsed_time_ms_global = timing.ElapsedTiming(t_start, t_end);

	elapsed_time_ms_gpu = elapsed_time_ms_global - (elapsed_time_ms_bf + elapsed_time_ms_memcpy);

	return elapsed_time_ms_global;
}

//
// CPU/GPU brute force
// return elapsed time in ms
//
double bruteforce_cpu_gpu(e_charset charset, unsigned char *pwdstr_cur, uint nb_pwd)
{
	double elapsed_time_ms_global, elapsed_time_ms_bf, elapsed_time_ms_mem;
	uint64 t_start, t_end, t_start_mem, t_end_mem; 
	uint gpu_cpuWords_Size_tmp;
	char *cCharset = NULL;
	unsigned char num_char;

	int dynShmemPerThread = 64;	// built in the algorithm
	/*
	int staticShmemPerBlock = 32;	// read from .cubin file
	*/

	int nthreads;
	// tpb is number of threads per block
	int tpb = 32;
	// 32 is the experimentally determined best case scenario on my 8800 GT		
	int gridDim[3];
	Timer timing;
	cudaError_t err;

	t_start = timing.StartTiming();

	get_charsetCPU_charsetLen(charset, &cCharset, &num_char);

	if(nb_pwd%num_char != 0)
	{
		nthreads = (nb_pwd/num_char)+1; /* Round to calculate a bit more */
	}else
		nthreads = nb_pwd/num_char;

	gpu_cpuWords_Size_tmp = nb_pwd*16;
	gpu_cpuWords_min10char_Size = (gpu_cpuWords_Size_tmp / num_char)+1;
	if(gpu_cpuWords_Size_tmp == 0 || gpu_cpuWords_Size_tmp > gpu_cpuWords_Size)
		return 0.0;

	elapsed_time_ms_bf = bruteforce_cpu_md5prep(charset, pwdstr_cur, nb_pwd, (unsigned char *)cpuWords, gpu_cpuWords_Size_tmp);
	
	t_start_mem = timing.StartTiming();

	err = cudaMemcpy(gpu_cpuWords_min10char, cpuWords, gpu_cpuWords_min10char_Size, cudaMemcpyHostToDevice);
	t_end_mem = timing.StopTiming();
	elapsed_time_ms_mem = timing.ElapsedTiming(t_start_mem, t_end_mem);

	if(nthreads%tpb != 0)
	{
		gridDim[0] = (nthreads/tpb)+1; /* Round to calculate a bit more */
	}else
		gridDim[0] = nthreads/tpb;

	gridDim[1] = 1;

	execute_kernel_bruteforce(gridDim[0], gridDim[1], tpb, tpb*dynShmemPerThread, gpu_cpuWords_min10char, gpuWords);

	t_end = timing.StopTiming();
	elapsed_time_ms_global = timing.ElapsedTiming(t_start, t_end);

	// Download the results for debug purpose
	// err = cudaMemcpy(cpuWords, gpuWords, gpu_cpuWords_Size, cudaMemcpyDeviceToHost);

	return (elapsed_time_ms_global);
}

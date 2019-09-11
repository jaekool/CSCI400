
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

#ifndef GLOBAL_TYPES_H
#define GLOBAL_TYPES_H

/*
template <class T> const T& max ( const T& a, const T& b ) {
return (b<a)?a:b;     // or: return comp(b,a)?a:b; for the comp version
}
*/
typedef unsigned int uint;

#if (defined(WIN32) || defined(__WIN32__) || defined(__WIN32))
// Win32
typedef __int64 int64;
typedef unsigned __int64 uint64;
#define snprintf _snprintf
#define atoll _atoi64
#else
// Linux
typedef long long int64;
typedef unsigned long long uint64;
#endif


#endif /* GLOBAL_TYPES_H */

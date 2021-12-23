/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H
#endif /* HAVE_STDINT_H */

#define Y4M_UNKNOWN -1
#define Y4M_ILACE_NONE          0   /* non-interlaced, progressive frame */
#define Y4M_ILACE_TOP_FIRST     1   /* interlaced, top-field first       */
#define Y4M_ILACE_BOTTOM_FIRST  2   /* interlaced, bottom-field first    */
#define Y4M_ILACE_MIXED         3   /* mixed, "refer to frame header"    */

#include "interlacemodes.h"

void ilacemode_to_text(char *string, int ilacemode)
{
	const char *cp = 0;
	switch(ilacemode) {
	case ILACE_MODE_UNDETECTED:     cp = ILACE_MODE_UNDETECTED_T;      break;
	case ILACE_MODE_TOP_FIRST:      cp = ILACE_MODE_TOP_FIRST_T;       break;
	case ILACE_MODE_BOTTOM_FIRST:   cp = ILACE_MODE_BOTTOM_FIRST_T;    break;
	case ILACE_MODE_NOTINTERLACED:  cp = ILACE_MODE_NOTINTERLACED_T;   break;
 	default: cp = ILACE_UNKNOWN_T;  break;
	}
	strcpy(string, _(cp));
}

int ilacemode_from_text(const char *text, int thedefault)
{
	if(!strcasecmp(text, _(ILACE_MODE_UNDETECTED_T)))     return ILACE_MODE_UNDETECTED;
	if(!strcasecmp(text, _(ILACE_MODE_TOP_FIRST_T)))      return ILACE_MODE_TOP_FIRST;
	if(!strcasecmp(text, _(ILACE_MODE_BOTTOM_FIRST_T)))   return ILACE_MODE_BOTTOM_FIRST;
	if(!strcasecmp(text, _(ILACE_MODE_NOTINTERLACED_T)))  return ILACE_MODE_NOTINTERLACED;
	return thedefault;
}

void ilacemode_to_xmltext(char *string, int ilacemode)
{
	switch(ilacemode) {
	case ILACE_MODE_UNDETECTED:     strcpy(string, ILACE_MODE_UNDETECTED_XMLT);      return;
	case ILACE_MODE_TOP_FIRST:      strcpy(string, ILACE_MODE_TOP_FIRST_XMLT);       return;
	case ILACE_MODE_BOTTOM_FIRST:   strcpy(string, ILACE_MODE_BOTTOM_FIRST_XMLT);    return;
	case ILACE_MODE_NOTINTERLACED:  strcpy(string, ILACE_MODE_NOTINTERLACED_XMLT);   return;
	}
	strcpy(string, ILACE_UNKNOWN_T);
}

int ilacemode_from_xmltext(const char *text, int thedefault)
{
	if( text ) {
		if(!strcasecmp(text, ILACE_MODE_UNDETECTED_XMLT))     return ILACE_MODE_UNDETECTED;
		if(!strcasecmp(text, ILACE_MODE_TOP_FIRST_XMLT))      return ILACE_MODE_TOP_FIRST;
		if(!strcasecmp(text, ILACE_MODE_BOTTOM_FIRST_XMLT))   return ILACE_MODE_BOTTOM_FIRST;
		if(!strcasecmp(text, ILACE_MODE_NOTINTERLACED_XMLT))  return ILACE_MODE_NOTINTERLACED;
	}
	return thedefault;
}


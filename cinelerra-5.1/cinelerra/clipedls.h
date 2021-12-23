#ifndef __CLIP_EDLS_H__
#define __CLIP_EDLS_H__

#include "arraylist.h"
#include "edl.inc"

class ClipEDLs : public ArrayList<EDL*>
{
public:
	ClipEDLs();
	~ClipEDLs();

	void add_clip(EDL *clip);
	void remove_clip(EDL *clip);
// Return copy of the src EDL which belongs to the current object.
	EDL* get_nested(EDL *src);
	void copy_nested(ClipEDLs &nested);
	EDL* load(char *path);
	void clear();
	void update_index(EDL *clip);
};

#endif



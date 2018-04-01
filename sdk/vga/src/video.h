/*
 * video.h
 *
 *  Created on: Mar 7, 2018
 *      Author: slippman
 */

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include "global.h"

struct image {
	u16 width;
	u16 height;
	u16 data[0];
};

void CopyImage(struct image * dest, struct image * src);
void DrawImage(u16 * frameptr, struct image * sprite, short x, short y);
void EraseImage(u16 * frameptr, struct image * background, struct image * img, short x, short y);

#endif /* _VIDEO_H_ */

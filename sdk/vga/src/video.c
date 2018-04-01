#include "video.h"

/*****************************************************************************/
/**
*
* This function copies image data from one section of memory on top of an
* existing frame
*
* param	frame 	- pointer to the existing frame
* param	img 	- pointer to the image to copy
* param	x		- horizontal position on existing frame to start
* 					  drawing pixels [top left corner of screen is (0, 0)]
* param	y		- vertical position on existing frame to start
* 					  drawing pixels [top left corner of screen is (0, 0)]
*
* return	None
*
* note		None.
*
******************************************************************************/
void DrawImage(u16 * frameptr, struct image * img, short x, short y)
{
	u32 frameOffset = 0;
	u32 imgOffset = 0;
	int xOffset = 0;
	int yOffset = 0;
	u16 width = img->width;
	u16 height = img->height;
	u16 adjustedWidth = width;
    u16 adjustedHeight = height;

	// handle x overflow
    if(x < 0)
    {
    	xOffset = -x;
    }
    else if(x + width > FRAME_WIDTH)
	{
		adjustedWidth = (FRAME_WIDTH - x) < 0 ? 0 : (FRAME_WIDTH - x);
	}

	// handle y overflow
	if (x < 0)
	{
		yOffset = -y;
	}
	if(y + height > FRAME_HEIGHT)
	{
		adjustedHeight = (FRAME_HEIGHT - y) < 0 ? 0 : (FRAME_HEIGHT - y);
	}

	for(int yi = 0; yi < adjustedHeight; yi++)
	{
		if(yi < yOffset)
			continue;

		imgOffset = yi * width;

		frameOffset = ((y + yi) * FRAME_WIDTH) + x;

		for(int xi = 0; xi < adjustedWidth; xi++)
		{
			if(xi < xOffset)
				continue;

			//check for alpha channel
			if (img->data[imgOffset + xi] & 0xF000)
			{
				frameptr[frameOffset + xi] = img->data[imgOffset + xi];
			}
		}
	}
}

/*****************************************************************************/
/**
*
* This function copies image data from one section of memory to another
*
* param	dest is a pointer to where the image should be copied
* param	src is a pointer to the image to copy
*
* return	None
*
* note		None.
*
******************************************************************************/
void CopyImage(struct image * dest, struct image * src)
{
	u16 width = src->width;
	u16 height = src->height;
	u32 dataLength = (width * height * 2) + IMG_HEADER_LEN;

	memcpy((void *)dest->data, (const void *)src->data, dataLength);
}

/*****************************************************************************/
/**
*
* This function removes image data from one section of memory on top of an
* existing frame by replacing the pixels with background pixels
*
* param	frame 		- pointer to the existing frame
* param	background 	- pointer to the background image
* param	img			- image to erase (used for dimensions)
* param	x			- horizontal position on existing frame to start
* 					  erasing pixels [top left corner of screen is (0, 0)]
* param	y			- vertical position on existing frame to start
*					  erasing pixels [top left corner of screen is (0, 0)]
*
* return 	None
*
* note		None
*
******************************************************************************/
void EraseImage(u16 * frameptr, struct image * background, struct image * img, short x, short y)
{
	u32 frameOffset = 0;
	int xOffset = 0;
	int yOffset = 0;
	u16 width = img->width;
	u16 height = img->height;
	u16 adjustedWidth = width;
    u16 adjustedHeight = height;

    if(background->height != FRAME_HEIGHT || background->width != FRAME_WIDTH)
    	return;

	// handle x overflow
    if(x < 0)
    {
    	xOffset = -x;
    }
    else if(x + width > FRAME_WIDTH)
	{
		adjustedWidth = (FRAME_WIDTH - x) < 0 ? 0 : (FRAME_WIDTH - x);
	}

	// handle y overflow
	if (x < 0)
	{
		yOffset = -y;
	}
	if(y + height > FRAME_HEIGHT)
	{
		adjustedHeight = (FRAME_HEIGHT - y) < 0 ? 0 : (FRAME_HEIGHT - y);
	}

	for(int yi = 0; yi < adjustedHeight; yi++)
	{
		if(yi < yOffset)
			continue;

		frameOffset = ((y + yi) * FRAME_WIDTH) + x;

		for(int xi = 0; xi < adjustedWidth; xi++)
		{
			if(xi < xOffset)
				continue;

			frameptr[frameOffset + xi] = background->data[frameOffset + xi];
		}
	}
}



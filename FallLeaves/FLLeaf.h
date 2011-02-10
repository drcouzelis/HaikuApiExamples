// Copyright (c) 2011 David Couzelis. All Rights Reserved.
// This file may be used under the terms of the MIT License.
#ifndef _FLLEAF_H_
#define _FLLEAF_H_


#include <Bitmap.h>
#include <View.h>


class Leaf
{
public:
					Leaf(BBitmap* bitmap);
					~Leaf() { delete fBitmap; };
	
	void			Update(int32 ticksPerSecond);
	void			Draw(BView* view);
	
	void			SetPos(BPoint pos) { fPos = pos; };
	BPoint			Pos() { return fPos; };
	
	// The Z axis controls how far "in" to the screen
	// the leaf is
	void			SetZ(int32 z) { fZ = z; };
	int32			Z() const { return fZ; };
	
	void			SetSpeed(int32 speed) { fSpeed = speed; };
	
	int32			Width() { return (int32)fBitmap->Bounds().right; };
	int32			Height() { return (int32)fBitmap->Bounds().bottom; };
	
	// A leaf is dead if it moves outside the bounds
	bool			IsDead() { return fDead; };
	void			SetBounds(BRect bounds) { fBounds = bounds; };
	
private:
	BBitmap			*fBitmap;
	
	BPoint			fPos; // The position on the screen
	int32			fZ;
	
	int32			fSpeed; // In pixels per second
	int32			fFudge;
	
	BRect			fBounds;
	bool			fDead;
};


#endif

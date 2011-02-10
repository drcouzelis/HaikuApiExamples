// Copyright (c) 2011 David Couzelis. All Rights Reserved.
// This file may be used under the terms of the MIT License.
#include "FLLeaf.h"


Leaf::Leaf(BBitmap* bitmap)
	:
	fBitmap(bitmap),
	fPos(BPoint()),
	fZ(0),
	fSpeed(0),
	fFudge(0),
	fBounds(BRect()),
	fDead(false)
{
	// Empty
}


void
Leaf::Update(int32 ticksPerSecond)
{
	if (fDead)
		return;
	
	fFudge += fSpeed;
	
	while (fFudge >= ticksPerSecond) {
		
		fPos.y++;
		
		fFudge -= ticksPerSecond;
	}
	
	// If the leaf is out of bounds...
	if (fPos.x < fBounds.left || fPos.x > fBounds.right
			|| fPos.y < fBounds.top || fPos.y > fBounds.bottom) {
		fDead = true; // ...then it's dead
	}
}


void
Leaf::Draw(BView* view)
{
	if (fDead)
		return;
	
	view->DrawBitmap(fBitmap, fPos);
}

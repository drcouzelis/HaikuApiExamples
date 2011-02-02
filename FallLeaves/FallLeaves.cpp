/**
 * A lovely screensaver with falling Haiku leaves.
 *
 * Copyright (c) 2011 David Couzelis. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Inspired by and partially copied from the "Icons" screensaver,
 * by Vincent Duvert.
 *
 * Leaf images by Stephan Aßmus.
 */
#include <Bitmap.h>
#include "BuildScreenSaverDefaultSettingsView.h" // TEMP local
#include "IconUtils.h" // TEMP local
#include <List.h>
#include <ScreenSaver.h>
#include <StringView.h>
#include <stdlib.h>

#include "Leaf.h"


#define RAND_NUM(low, high) ((rand() % ((high) - (low) + 1) + (low)))

#define TICKS_PER_SECOND 100
#define MICROSECS_IN_SEC 1000000


// The number of leaves on the screen
const int32 kMaxLeaves = 35;


class FallLeaves : public BScreenSaver
{
public:
					FallLeaves(BMessage *archive, image_id thisImage);
	void			StartConfig(BView *configView);
	status_t		StartSaver(BView *view, bool preview);
	void			StopSaver();
	void			Draw(BView *view, int32 frame);
private:
	Leaf			*_CreateLeaf(BView *view, bool above);
	BBitmap			*_RandomBitmap(int32 size);
	
	BList			*fLeaves; // TODO: Convert to BObjectList
	int32			fMaxSize; // The max size of a leaf
	int32			fMaxSpeed; // The max speed of a leaf
	BBitmap			*fBackBitmap; // Used to reduce flicker
	BView			*fBackView;
	
	bool			fZUsed[101]; // Used to prevent a flicker bug
};


extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id id)
{
	return new FallLeaves(msg, id);
}


FallLeaves::FallLeaves(BMessage *archive, image_id thisImage)
	:
	BScreenSaver(archive, thisImage),
	fLeaves(NULL),
	fMaxSize(0),
	fMaxSpeed(0),
	fBackBitmap(NULL),
	fBackView(NULL)
{
	for (int32 i = 0; i < 101; i++)
		fZUsed[i] = false;
}


/**
 * A small helper function to sort leaves.
 * Sort the leaves in order of their Z axis,
 * from large to small.
 */
int cmpz(const void *item1, const void *item2)
{
	const Leaf *a = *(const Leaf **)item1;
	const Leaf *b = *(const Leaf **)item2;
	
	if (a->Z() < b->Z())
		return 1;
	
	if (a->Z() > b->Z())
		return -1;
	
	return 0;
}


void FallLeaves::StartConfig(BView *configView)
{
	BPrivate::BuildScreenSaverDefaultSettingsView(configView, "Fall Leaves",
			"by David Couzelis");
}


status_t FallLeaves::StartSaver(BView *view, bool preview)
{
	BRect screenRect(0, 0, view->Frame().Width(), view->Frame().Height());
	fBackBitmap = new BBitmap(screenRect, B_RGBA32, true);
	if (!fBackBitmap->IsValid())
		return B_NO_MEMORY;

	fBackView = new BView(screenRect, NULL, 0, 0);
	if (fBackView == NULL)
		return B_NO_MEMORY;

	fBackBitmap->AddChild(fBackView);
	
	if (fBackBitmap->Lock()) {
		fBackView->FillRect(fBackView->Frame());
		fBackView->SetDrawingMode(B_OP_OVER); // Leaf transparency
		fBackView->Sync();
		fBackBitmap->Unlock();
	}

	// Set the rate the screensaver to be updated 100 times per second
	// The argument here is in microseconds
	SetTickSize(MICROSECS_IN_SEC / TICKS_PER_SECOND);
	
	// Initialize the random number generator
	srand(system_time() % INT_MAX);
	
	// The max size of a leaf will be about 20% the
	// height of the screen
	fMaxSize = (int32)(view->Frame().bottom * 2) / 10;
	
	// The max speed will be about 50% the height of the screen
	fMaxSpeed = (int32)(view->Frame().bottom * 5) / 10;
	
	fLeaves = new BList();
	
	// Create some leaves
	for (int32 i = 0; i < kMaxLeaves; i++)
		fLeaves->AddItem(_CreateLeaf(view, true));
	
	// Sort the leaves by Z axis
	fLeaves->SortItems(cmpz);
	
	return B_OK;
}


void
FallLeaves::StopSaver()
{
	for (int32 i = 0; ; i++) {
		Leaf *leaf = (Leaf *)fLeaves->ItemAt(i);
		if (leaf == NULL)
			break;
		delete leaf;
	}
	
	delete fLeaves;
	
	fBackBitmap->RemoveChild(fBackView);
	
	delete fBackView;
	delete fBackBitmap;
}


void FallLeaves::Draw(BView *view, int32 frame)
{
	fBackBitmap->Lock();
	
	// Clear the offscreen buffer
	fBackView->FillRect(fBackView->Frame());
	
	// Update and draw the leaves
	for (int32 i = fLeaves->CountItems() - 1; ; i--) {
		Leaf *leaf = (Leaf *)fLeaves->ItemAt(i);
		if (leaf == NULL)
			break;
		leaf->Update(TICKS_PER_SECOND);
		leaf->Draw(fBackView);
	}
	
	bool sort = false;
	
	// Remove any dead leaves
	// Do this in a separate loop to prevent flicker when drawing
	for (int32 i = fLeaves->CountItems() - 1; ; i--) {
		Leaf *leaf = (Leaf *)fLeaves->ItemAt(i);
		if (leaf == NULL)
			break;
		if (leaf->IsDead()) {
			// Remove the dead leaf
			fZUsed[leaf->Z()] = false;
			fLeaves->RemoveItem(leaf);
			delete leaf;
			// Add a new leaf to replace the dead one
			fLeaves->AddItem(_CreateLeaf(view, false), i);
			sort = true;
		}
	}
	
	// Keep the leaves sorted by Z axis
	if (sort)
		fLeaves->SortItems(cmpz);
	
	fBackView->Sync();
	fBackBitmap->Unlock();
	view->DrawBitmap(fBackBitmap);
}


Leaf *
FallLeaves::_CreateLeaf(BView *view, bool above)
{
	// The Z axis (how far away the leaf is)
	// determines the size and speed
	int32 z = RAND_NUM(40, 100);
	
	// This is a hack. :'(
	// There was a peculiar flicker when resorting
	// and drawing the leaves. It happens when two
	// leaves have the same Z value. Use this array
	// to ensure unique Z values.
	while (fZUsed[z]) {
		z++;
		if (z > 100)
			z = 40;
	}
	fZUsed[z] = true;
	
	// The lower the Z axis number, the smaller the leaf
	int32 size = (fMaxSize * z) / 100;
	
	// Create the leaf
	Leaf *leaf = new Leaf(_RandomBitmap(size));
	
	leaf->SetZ(z);
	
	// The lower the Z axis number, the slower the leaf
	int32 speed = (fMaxSpeed * z) / 100;
	leaf->SetSpeed(speed);
	
	BRect bounds(-(size / 2), -view->Frame().Height(),
			view->Frame().Width() - (size / 2), view->Frame().Height());
	leaf->SetBounds(bounds);
	
	// Set it to a random position
	BPoint pos;
	pos.x = RAND_NUM((int32)bounds.left, (int32)bounds.right);
	pos.y = -size;
	
	if (above)
		pos.y = -(RAND_NUM(size, (int32)bounds.bottom));
	
	leaf->SetPos(pos);
	
	return leaf;
}


// Leaf 1 - Orange 1
const unsigned char kLeaf1[] = {
	0x6e, 0x63, 0x69, 0x66, 0x08, 0x05, 0x00, 0x02, 0x00, 0x12, 0x02, 0x00,
	0x00, 0x00, 0x3d, 0x40, 0x00, 0xbd, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x4b,
	0x20, 0x00, 0x4a, 0x10, 0x00, 0x00, 0x01, 0x66, 0xff, 0x01, 0x85, 0x02,
	0x00, 0x06, 0x02, 0x3b, 0x9f, 0xd5, 0xbb, 0x5f, 0x70, 0x3e, 0xb9, 0x98,
	0x3e, 0xed, 0x9f, 0x45, 0xbf, 0xbf, 0x47, 0x19, 0x8c, 0x00, 0xc5, 0x84,
	0x30, 0xff, 0xdd, 0x7a, 0x29, 0x02, 0x00, 0x06, 0x02, 0x3b, 0x9f, 0xd5,
	0xbb, 0x5f, 0x70, 0x3e, 0xb9, 0x98, 0x3e, 0xed, 0x9f, 0x45, 0x8f, 0xbf,
	0x46, 0xf9, 0x8c, 0x00, 0xff, 0xd7, 0x60, 0xff, 0xed, 0x9d, 0x46, 0x02,
	0x01, 0x06, 0x02, 0x3a, 0x92, 0xa8, 0x3c, 0xb2, 0x8f, 0xbe, 0x9e, 0xad,
	0x3c, 0x7f, 0xb1, 0x4a, 0xd6, 0x23, 0x45, 0xf4, 0x80, 0x00, 0xda, 0x4c,
	0x05, 0xfe, 0xff, 0xb6, 0x43, 0x02, 0x01, 0x06, 0x02, 0x38, 0xa0, 0x81,
	0x3c, 0xe4, 0x69, 0xc0, 0x05, 0xf5, 0x3b, 0xac, 0xe9, 0x4a, 0x36, 0x3c,
	0x4a, 0x20, 0x00, 0x00, 0xfd, 0xb3, 0x3d, 0xff, 0xda, 0x4c, 0x05, 0x02,
	0x01, 0x06, 0x02, 0xbd, 0x28, 0x86, 0x3d, 0x5c, 0x6c, 0xbc, 0x93, 0x89,
	0xbc, 0x6b, 0xc1, 0x4a, 0x2c, 0xa1, 0x4a, 0x72, 0x01, 0x00, 0xfd, 0xd1,
	0x5b, 0xff, 0xda, 0x4c, 0x05, 0x02, 0x00, 0x06, 0x02, 0x34, 0xb4, 0xd1,
	0x37, 0x3c, 0x1e, 0xbc, 0x49, 0x56, 0x39, 0xd3, 0x68, 0x48, 0xda, 0x03,
	0x4a, 0xe2, 0x5a, 0x00, 0xff, 0xd7, 0x5f, 0xff, 0xa4, 0x37, 0x0b, 0x07,
	0x06, 0x0d, 0xae, 0xff, 0xcf, 0x02, 0x24, 0x5a, 0x29, 0x5a, 0x29, 0x5a,
	0x2f, 0x56, 0x34, 0x51, 0x3e, 0x5c, 0x41, 0x54, 0x40, 0x5b, 0x49, 0x58,
	0x5a, 0x54, 0x57, 0x57, 0x5a, 0x4c, 0x4d, 0x4b, 0x4d, 0x4b, 0x55, 0x4a,
	0x60, 0x3f, 0x5f, 0x43, 0x60, 0x3a, 0x52, 0x3a, 0x52, 0x3a, 0x58, 0x36,
	0x5a, 0x2b, 0x5c, 0x2f, 0xc9, 0x69, 0xb6, 0xe2, 0x54, 0x25, 0x4e, 0x25,
	0x4e, 0x28, 0x50, 0x2e, 0x50, 0x06, 0x0a, 0xee, 0xbe, 0x0b, 0x33, 0x4c,
	0x3e, 0x43, 0x39, 0x48, 0xc1, 0xf5, 0xbd, 0xf0, 0x4d, 0x31, 0xbf, 0xe8,
	0xc0, 0x75, 0x47, 0x3b, 0x48, 0x43, 0x52, 0x40, 0xbf, 0x63, 0xc1, 0x06,
	0x47, 0x44, 0xbe, 0x4d, 0xc2, 0x30, 0xbd, 0x68, 0xc3, 0x0a, 0xbe, 0x25,
	0xc2, 0x56, 0x40, 0x4c, 0x4a, 0x4f, 0xbc, 0xf9, 0xc3, 0x8b, 0xc0, 0x32,
	0xc4, 0xfa, 0xbc, 0x3d, 0xc4, 0x39, 0x35, 0x4d, 0x06, 0x0f, 0xee, 0xbb,
	0xfb, 0x2e, 0xba, 0xb8, 0xc4, 0xbb, 0xbc, 0x44, 0x49, 0xbb, 0x84, 0xc4,
	0x01, 0x33, 0x46, 0x2e, 0x40, 0xbc, 0xca, 0xc2, 0xde, 0x35, 0x46, 0xbd,
	0x85, 0xc2, 0x35, 0xbe, 0xd6, 0xc0, 0xfd, 0xbd, 0xae, 0xc2, 0x1d, 0x3d,
	0x3b, 0x3a, 0x33, 0xbf, 0x80, 0xc0, 0x59, 0x3f, 0x3c, 0xc1, 0xe4, 0xbd,
	0xf5, 0xc5, 0x14, 0xb9, 0xf9, 0xc0, 0x0e, 0xc0, 0xa8, 0x47, 0x3c, 0xc3,
	0x16, 0xc1, 0x25, 0x52, 0x40, 0xbf, 0x89, 0xc1, 0x39, 0xc2, 0xb0, 0xc1,
	0x8b, 0xbe, 0x73, 0xc2, 0x63, 0xbd, 0x8e, 0xc3, 0x3d, 0xbe, 0x4b, 0xc2,
	0x89, 0xbf, 0xe6, 0xc4, 0xbb, 0x4a, 0x4f, 0x3a, 0x4a, 0xc0, 0x6f, 0xc5,
	0x5d, 0xbc, 0xa0, 0xc4, 0x6a, 0x35, 0x4f, 0x06, 0x04, 0xae, 0x4e, 0x3a,
	0x52, 0x26, 0x58, 0x2e, 0xc3, 0x89, 0xb5, 0xfd, 0x43, 0x35, 0x3f, 0x42,
	0x06, 0x08, 0xef, 0xba, 0x26, 0x36, 0x30, 0x35, 0x25, 0x3f, 0x2c, 0x48,
	0x2c, 0x48, 0x28, 0x48, 0x25, 0x4b, 0x33, 0x4c, 0x29, 0x4d, 0x33, 0x4c,
	0x3f, 0x42, 0x43, 0x35, 0x38, 0x2a, 0x41, 0x2d, 0x31, 0x32, 0x35, 0x3e,
	0x06, 0x08, 0xfb, 0xae, 0x35, 0x4d, 0x35, 0x4d, 0x35, 0x56, 0x3b, 0x59,
	0x3e, 0x51, 0x3f, 0x54, 0x3e, 0x51, 0x55, 0x52, 0x4b, 0x57, 0x56, 0x4c,
	0x48, 0x49, 0x5c, 0x3b, 0x59, 0x46, 0x5b, 0x37, 0x4e, 0x3a, 0x3f, 0x42,
	0x06, 0x04, 0xeb, 0x33, 0x4c, 0x33, 0x4c, 0x2d, 0x51, 0x23, 0x55, 0x27,
	0x56, 0x35, 0x4d, 0x30, 0x51, 0x35, 0x4d, 0x07, 0x0a, 0x00, 0x04, 0x03,
	0x04, 0x05, 0x06, 0x10, 0x01, 0x17, 0x84, 0x00, 0x04, 0x0a, 0x05, 0x01,
	0x04, 0x00, 0x0a, 0x06, 0x01, 0x05, 0x00, 0x0a, 0x04, 0x01, 0x03, 0x00,
	0x0a, 0x02, 0x01, 0x02, 0x00, 0x0a, 0x07, 0x01, 0x06, 0x00, 0x0a, 0x03,
	0x01, 0x01, 0x08, 0x15, 0xff
};

// Leaf 2 - Orange 2
const unsigned char kLeaf2[] = {
	0x6e, 0x63, 0x69, 0x66, 0x09, 0x05, 0x00, 0x02, 0x00, 0x12, 0x02, 0x00,
	0x00, 0x00, 0x3d, 0x40, 0x00, 0xbd, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x4a,
	0xe0, 0x00, 0x4a, 0x10, 0x00, 0x00, 0x01, 0x66, 0xff, 0x01, 0x85, 0x02,
	0x00, 0x06, 0x02, 0xbb, 0x54, 0x89, 0xbb, 0xa9, 0xdc, 0x3d, 0x61, 0x95,
	0xbd, 0x12, 0xd5, 0x47, 0x21, 0xcd, 0x4b, 0x2e, 0xe5, 0x00, 0xff, 0xab,
	0x3e, 0xff, 0xdd, 0x7a, 0x29, 0x02, 0x00, 0x06, 0x02, 0xba, 0x3f, 0xb3,
	0x3b, 0x68, 0x5f, 0xbc, 0xea, 0x1e, 0xbb, 0xd8, 0xc4, 0x4a, 0x9e, 0xae,
	0x4a, 0x75, 0x8e, 0x00, 0xff, 0xc1, 0x4b, 0xff, 0xff, 0xd7, 0x5f, 0x02,
	0x01, 0x06, 0x02, 0x3a, 0x92, 0xa8, 0x3c, 0xb2, 0x8f, 0xbe, 0x9e, 0xad,
	0x3c, 0x7f, 0xb1, 0x4a, 0x96, 0x23, 0x45, 0xf4, 0x80, 0x00, 0xda, 0x4c,
	0x05, 0xfe, 0xff, 0xb6, 0x43, 0x02, 0x01, 0x06, 0x02, 0x3a, 0x92, 0xa8,
	0x3c, 0xb2, 0x8f, 0xbe, 0x9e, 0xad, 0x3c, 0x7f, 0xb1, 0x4a, 0x96, 0x23,
	0x45, 0xf4, 0x80, 0x00, 0xf7, 0x70, 0x2e, 0xff, 0xff, 0xd6, 0x59, 0x02,
	0x01, 0x06, 0x02, 0x38, 0xa0, 0x81, 0x3c, 0xe4, 0x69, 0xc0, 0x05, 0xf5,
	0x3b, 0xac, 0xe9, 0x49, 0xec, 0x78, 0x4a, 0x20, 0x00, 0x00, 0xfd, 0xb3,
	0x3d, 0xff, 0xda, 0x4c, 0x05, 0x02, 0x01, 0x06, 0x02, 0xbd, 0x28, 0x86,
	0x3d, 0x5c, 0x6c, 0xbc, 0x93, 0x89, 0xbc, 0x6b, 0xc1, 0x4a, 0x0c, 0xa1,
	0x4a, 0x82, 0x01, 0x00, 0xff, 0xd6, 0x59, 0xff, 0xf7, 0x70, 0x2e, 0x02,
	0x00, 0x06, 0x02, 0x35, 0x9e, 0x73, 0x37, 0x00, 0xe5, 0xbc, 0x1f, 0x7a,
	0x3a, 0x8e, 0xd7, 0x48, 0x5d, 0xc8, 0x4a, 0xdf, 0x90, 0x00, 0xff, 0xd7,
	0x5f, 0xff, 0xa4, 0x37, 0x0b, 0x08, 0x06, 0x0e, 0xee, 0xeb, 0xbb, 0x0b,
	0x22, 0x5a, 0x28, 0x5a, 0x28, 0x5a, 0x2e, 0x55, 0x31, 0x51, 0x3d, 0x58,
	0x39, 0x58, 0x3d, 0x58, 0x3d, 0x53, 0x3d, 0x53, 0x48, 0x58, 0x54, 0x52,
	0x53, 0x4f, 0x60, 0x48, 0x5b, 0x4e, 0x60, 0x41, 0x4f, 0x3f, 0x4f, 0x3f,
	0x57, 0x3a, 0x58, 0x30, 0x54, 0x2a, 0x53, 0x2e, 0x51, 0x2b, 0x4e, 0x2b,
	0x23, 0x4d, 0x23, 0x4d, 0x26, 0x4f, 0x2d, 0x4f, 0x06, 0x0a, 0xee, 0xbe,
	0x0b, 0x2f, 0x4c, 0x38, 0x44, 0xba, 0x74, 0xc3, 0x37, 0x41, 0x3c, 0x4c,
	0x29, 0xbd, 0x1e, 0xc1, 0xa7, 0x42, 0x3c, 0x42, 0x45, 0x4d, 0x45, 0xbc,
	0x99, 0xc2, 0x38, 0x43, 0x46, 0xbc, 0x99, 0xc2, 0x38, 0xbb, 0xd0, 0xc3,
	0x0a, 0xbb, 0xd0, 0xc3, 0x0a, 0x3c, 0x4c, 0x46, 0x4f, 0xbb, 0x61, 0xc3,
	0x8b, 0xbe, 0x9a, 0xc4, 0xfa, 0xba, 0xa5, 0xc4, 0x39, 0x31, 0x4d, 0x06,
	0x0f, 0xee, 0xbb, 0xfb, 0x2e, 0xb9, 0x20, 0xc4, 0xbb, 0xba, 0xac, 0x49,
	0xb9, 0xec, 0xc4, 0x01, 0x2f, 0x46, 0x2c, 0x42, 0xbb, 0x32, 0xc2, 0xde,
	0x31, 0x46, 0xbb, 0x32, 0xc2, 0xde, 0xbc, 0x0c, 0xc2, 0x2f, 0xbc, 0x0c,
	0xc2, 0x2f, 0x35, 0x40, 0x2d, 0x34, 0x38, 0x45, 0xbb, 0x9d, 0xbf, 0x8d,
	0x3e, 0x3e, 0x48, 0x31, 0xbd, 0x44, 0xc1, 0xda, 0x40, 0x40, 0xc0, 0x4c,
	0xc2, 0x57, 0x53, 0x44, 0xbc, 0xbf, 0xc2, 0x6b, 0x40, 0x49, 0xbc, 0xbf,
	0xc2, 0x6b, 0x37, 0x49, 0x37, 0x49, 0xbe, 0x81, 0xc4, 0xd3, 0x3f, 0x4e,
	0x36, 0x4a, 0x3a, 0x4d, 0xbb, 0x08, 0xc4, 0x6a, 0x31, 0x4f, 0x06, 0x05,
	0xae, 0x02, 0x4e, 0x26, 0x42, 0x2a, 0x46, 0x2b, 0x3b, 0x2e, 0x38, 0x36,
	0x3b, 0x42, 0x46, 0x34, 0x06, 0x05, 0xae, 0x02, 0x49, 0x3e, 0x52, 0x2f,
	0xc6, 0xab, 0xbb, 0x90, 0xc5, 0xc8, 0xb8, 0x7c, 0x4e, 0x26, 0x46, 0x34,
	0x3b, 0x42, 0x06, 0x0a, 0xeb, 0xaa, 0x0b, 0x22, 0x36, 0x22, 0x36, 0x21,
	0x3f, 0x28, 0x48, 0x24, 0x49, 0x2f, 0x4c, 0x27, 0x4d, 0x2f, 0x4c, 0x3b,
	0x42, 0x38, 0x36, 0x30, 0x2a, 0x2c, 0x2c, 0x28, 0x29, 0x28, 0x29, 0x24,
	0x2d, 0x26, 0x36, 0x06, 0x0a, 0xeb, 0x6e, 0x0a, 0x31, 0x4d, 0x31, 0x4d,
	0x31, 0x54, 0x39, 0x56, 0x3a, 0x51, 0x50, 0x4f, 0x49, 0x55, 0x50, 0x4f,
	0x4e, 0x4c, 0x5a, 0x44, 0x55, 0x4b, 0x5a, 0x44, 0x56, 0x42, 0x40, 0x49,
	0x3e, 0x3b, 0x42, 0x06, 0x04, 0xeb, 0x2f, 0x4c, 0x2f, 0x4c, 0x29, 0x52,
	0x22, 0x57, 0x26, 0x58, 0x31, 0x4d, 0x2c, 0x53, 0x31, 0x4d, 0x08, 0x0a,
	0x00, 0x05, 0x03, 0x05, 0x06, 0x07, 0x04, 0x10, 0x01, 0x17, 0x84, 0x02,
	0x04, 0x0a, 0x06, 0x01, 0x05, 0x00, 0x0a, 0x07, 0x01, 0x06, 0x00, 0x0a,
	0x04, 0x01, 0x03, 0x00, 0x0a, 0x05, 0x01, 0x04, 0x00, 0x0a, 0x02, 0x01,
	0x02, 0x00, 0x0a, 0x08, 0x01, 0x07, 0x00, 0x0a, 0x03, 0x01, 0x01, 0x08,
	0x15, 0xff
};

// Leaf 3 - Green 1
const unsigned char kLeaf3[] = {
	0x6e, 0x63, 0x69, 0x66, 0x08, 0x05, 0x00, 0x02, 0x00, 0x12, 0x02, 0x00,
	0x00, 0x00, 0x3d, 0x40, 0x00, 0xbd, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x4b,
	0x20, 0x00, 0x4a, 0x10, 0x00, 0x00, 0x01, 0x66, 0xff, 0x01, 0x85, 0x02,
	0x00, 0x06, 0x02, 0x3b, 0x9f, 0xd5, 0xbb, 0x5f, 0x70, 0x3e, 0xb9, 0x98,
	0x3e, 0xed, 0x9f, 0x45, 0xbf, 0xbf, 0x47, 0x19, 0x8c, 0x00, 0xc5, 0xc3,
	0x30, 0xff, 0xdd, 0xc5, 0x27, 0x02, 0x00, 0x06, 0x02, 0x3b, 0x9f, 0xd5,
	0xbb, 0x5f, 0x70, 0x3e, 0xb9, 0x98, 0x3e, 0xed, 0x9f, 0x45, 0x8f, 0xbf,
	0x46, 0xf9, 0x8c, 0x00, 0xe3, 0xff, 0x5f, 0xff, 0xed, 0xe2, 0x46, 0x02,
	0x01, 0x06, 0x02, 0x3a, 0x92, 0xa8, 0x3c, 0xb2, 0x8f, 0xbe, 0x9e, 0xad,
	0x3c, 0x7f, 0xb1, 0x4a, 0xd6, 0x23, 0x45, 0xf4, 0x80, 0x00, 0xda, 0xa5,
	0x05, 0xfe, 0xf8, 0xff, 0x43, 0x02, 0x01, 0x06, 0x02, 0x38, 0xa0, 0x81,
	0x3c, 0xe4, 0x69, 0xc0, 0x05, 0xf5, 0x3b, 0xac, 0xe9, 0x4a, 0x36, 0x3c,
	0x4a, 0x20, 0x00, 0x00, 0xf7, 0xfd, 0x3d, 0xff, 0xda, 0xa5, 0x05, 0x02,
	0x01, 0x06, 0x02, 0xbd, 0x28, 0x86, 0x3d, 0x5c, 0x6c, 0xbc, 0x93, 0x89,
	0xbc, 0x6b, 0xc1, 0x4a, 0x2c, 0xa1, 0x4a, 0x72, 0x01, 0x00, 0xe5, 0xfd,
	0x5b, 0xff, 0xda, 0xa5, 0x05, 0x02, 0x00, 0x06, 0x02, 0x34, 0xb4, 0xd1,
	0x37, 0x3c, 0x1e, 0xbc, 0x49, 0x56, 0x39, 0xd3, 0x68, 0x48, 0xda, 0x03,
	0x4a, 0xe2, 0x5a, 0x00, 0xe3, 0xff, 0x5f, 0xff, 0xa4, 0x78, 0x0a, 0x07,
	0x06, 0x0d, 0xae, 0xff, 0xcf, 0x02, 0x24, 0x5a, 0x29, 0x5a, 0x29, 0x5a,
	0x2f, 0x56, 0x34, 0x51, 0x3e, 0x5c, 0x41, 0x54, 0x40, 0x5b, 0x49, 0x58,
	0x5a, 0x54, 0x57, 0x57, 0x5a, 0x4c, 0x4d, 0x4b, 0x4d, 0x4b, 0x55, 0x4a,
	0x60, 0x3f, 0x5f, 0x43, 0x60, 0x3a, 0x52, 0x3a, 0x52, 0x3a, 0x58, 0x36,
	0x5a, 0x2b, 0x5c, 0x2f, 0xc9, 0x69, 0xb6, 0xe2, 0x54, 0x25, 0x4e, 0x25,
	0x4e, 0x28, 0x50, 0x2e, 0x50, 0x06, 0x0a, 0xee, 0xbe, 0x0b, 0x33, 0x4c,
	0x3e, 0x43, 0x39, 0x48, 0xc1, 0xf5, 0xbd, 0xf0, 0x4d, 0x31, 0xbf, 0xe8,
	0xc0, 0x75, 0x47, 0x3b, 0x48, 0x43, 0x52, 0x40, 0xbf, 0x63, 0xc1, 0x06,
	0x47, 0x44, 0xbe, 0x4d, 0xc2, 0x30, 0xbd, 0x68, 0xc3, 0x0a, 0xbe, 0x25,
	0xc2, 0x56, 0x40, 0x4c, 0x4a, 0x4f, 0xbc, 0xf9, 0xc3, 0x8b, 0xc0, 0x32,
	0xc4, 0xfa, 0xbc, 0x3d, 0xc4, 0x39, 0x35, 0x4d, 0x06, 0x0f, 0xee, 0xbb,
	0xfb, 0x2e, 0xba, 0xb8, 0xc4, 0xbb, 0xbc, 0x44, 0x49, 0xbb, 0x84, 0xc4,
	0x01, 0x33, 0x46, 0x2e, 0x40, 0xbc, 0xca, 0xc2, 0xde, 0x35, 0x46, 0xbd,
	0x85, 0xc2, 0x35, 0xbe, 0xd6, 0xc0, 0xfd, 0xbd, 0xae, 0xc2, 0x1d, 0x3d,
	0x3b, 0x3a, 0x33, 0xbf, 0x80, 0xc0, 0x59, 0x3f, 0x3c, 0xc1, 0xe4, 0xbd,
	0xf5, 0xc5, 0x14, 0xb9, 0xf9, 0xc0, 0x0e, 0xc0, 0xa8, 0x47, 0x3c, 0xc3,
	0x16, 0xc1, 0x25, 0x52, 0x40, 0xbf, 0x89, 0xc1, 0x39, 0xc2, 0xb0, 0xc1,
	0x8b, 0xbe, 0x73, 0xc2, 0x63, 0xbd, 0x8e, 0xc3, 0x3d, 0xbe, 0x4b, 0xc2,
	0x89, 0xbf, 0xe6, 0xc4, 0xbb, 0x4a, 0x4f, 0x3a, 0x4a, 0xc0, 0x6f, 0xc5,
	0x5d, 0xbc, 0xa0, 0xc4, 0x6a, 0x35, 0x4f, 0x06, 0x04, 0xae, 0x4e, 0x3a,
	0x52, 0x26, 0x58, 0x2e, 0xc3, 0x89, 0xb5, 0xfd, 0x43, 0x35, 0x3f, 0x42,
	0x06, 0x08, 0xef, 0xba, 0x26, 0x36, 0x30, 0x35, 0x25, 0x3f, 0x2c, 0x48,
	0x2c, 0x48, 0x28, 0x48, 0x25, 0x4b, 0x33, 0x4c, 0x29, 0x4d, 0x33, 0x4c,
	0x3f, 0x42, 0x43, 0x35, 0x38, 0x2a, 0x41, 0x2d, 0x31, 0x32, 0x35, 0x3e,
	0x06, 0x08, 0xfb, 0xae, 0x35, 0x4d, 0x35, 0x4d, 0x35, 0x56, 0x3b, 0x59,
	0x3e, 0x51, 0x3f, 0x54, 0x3e, 0x51, 0x55, 0x52, 0x4b, 0x57, 0x56, 0x4c,
	0x48, 0x49, 0x5c, 0x3b, 0x59, 0x46, 0x5b, 0x37, 0x4e, 0x3a, 0x3f, 0x42,
	0x06, 0x04, 0xeb, 0x33, 0x4c, 0x33, 0x4c, 0x2d, 0x51, 0x23, 0x55, 0x27,
	0x56, 0x35, 0x4d, 0x30, 0x51, 0x35, 0x4d, 0x07, 0x0a, 0x00, 0x04, 0x03,
	0x04, 0x05, 0x06, 0x10, 0x01, 0x17, 0x84, 0x00, 0x04, 0x0a, 0x05, 0x01,
	0x04, 0x00, 0x0a, 0x06, 0x01, 0x05, 0x00, 0x0a, 0x04, 0x01, 0x03, 0x00,
	0x0a, 0x02, 0x01, 0x02, 0x00, 0x0a, 0x07, 0x01, 0x06, 0x00, 0x0a, 0x03,
	0x01, 0x01, 0x08, 0x15, 0xff
};

// Leaf 4 - Green 2
const unsigned char kLeaf4[] = {
	0x6e, 0x63, 0x69, 0x66, 0x09, 0x05, 0x00, 0x02, 0x00, 0x12, 0x02, 0x00,
	0x00, 0x00, 0x3d, 0x40, 0x00, 0xbd, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x4a,
	0xe0, 0x00, 0x4a, 0x10, 0x00, 0x00, 0x01, 0x66, 0xff, 0x01, 0x85, 0x02,
	0x00, 0x06, 0x02, 0xbb, 0x54, 0x89, 0xbb, 0xa9, 0xdc, 0x3d, 0x61, 0x95,
	0xbd, 0x12, 0xd5, 0x47, 0x21, 0xcd, 0x4b, 0x2e, 0xe5, 0x00, 0xff, 0xfc,
	0x3c, 0xff, 0xdd, 0xc5, 0x27, 0x02, 0x00, 0x06, 0x02, 0xba, 0x3f, 0xb3,
	0x3b, 0x68, 0x5f, 0xbc, 0xea, 0x1e, 0xbb, 0xd8, 0xc4, 0x4a, 0x9e, 0xae,
	0x4a, 0x75, 0x8e, 0x00, 0xf2, 0xff, 0x4b, 0xff, 0xe7, 0xff, 0x5f, 0x02,
	0x01, 0x06, 0x02, 0x3a, 0x92, 0xa8, 0x3c, 0xb2, 0x8f, 0xbe, 0x9e, 0xad,
	0x3c, 0x7f, 0xb1, 0x4a, 0x96, 0x23, 0x45, 0xf4, 0x80, 0x00, 0xda, 0xa5,
	0x05, 0xfe, 0xf8, 0xff, 0x43, 0x02, 0x01, 0x06, 0x02, 0x3a, 0x92, 0xa8,
	0x3c, 0xb2, 0x8f, 0xbe, 0x9e, 0xad, 0x3c, 0x7f, 0xb1, 0x4a, 0x96, 0x23,
	0x45, 0xf4, 0x80, 0x00, 0xf7, 0xc5, 0x2e, 0xff, 0xe3, 0xff, 0x59, 0x02,
	0x01, 0x06, 0x02, 0x38, 0xa0, 0x81, 0x3c, 0xe4, 0x69, 0xc0, 0x05, 0xf5,
	0x3b, 0xac, 0xe9, 0x49, 0xec, 0x78, 0x4a, 0x20, 0x00, 0x00, 0xf7, 0xfd,
	0x3d, 0xff, 0xda, 0xa5, 0x05, 0x02, 0x01, 0x06, 0x02, 0xbd, 0x28, 0x86,
	0x3d, 0x5c, 0x6c, 0xbc, 0x93, 0x89, 0xbc, 0x6b, 0xc1, 0x4a, 0x0c, 0xa1,
	0x4a, 0x82, 0x01, 0x00, 0xe3, 0xff, 0x59, 0xff, 0xf7, 0xc5, 0x2e, 0x02,
	0x00, 0x06, 0x02, 0x35, 0x9e, 0x73, 0x37, 0x00, 0xe5, 0xbc, 0x1f, 0x7a,
	0x3a, 0x8e, 0xd7, 0x48, 0x5d, 0xc8, 0x4a, 0xdf, 0x90, 0x00, 0xe3, 0xff,
	0x5f, 0xff, 0xa4, 0x78, 0x0a, 0x08, 0x06, 0x0e, 0xee, 0xeb, 0xbb, 0x0b,
	0x22, 0x5a, 0x28, 0x5a, 0x28, 0x5a, 0x2e, 0x55, 0x31, 0x51, 0x3d, 0x58,
	0x39, 0x58, 0x3d, 0x58, 0x3d, 0x53, 0x3d, 0x53, 0x48, 0x58, 0x54, 0x52,
	0x53, 0x4f, 0x60, 0x48, 0x5b, 0x4e, 0x60, 0x41, 0x4f, 0x3f, 0x4f, 0x3f,
	0x57, 0x3a, 0x58, 0x30, 0x54, 0x2a, 0x53, 0x2e, 0x51, 0x2b, 0x4e, 0x2b,
	0x23, 0x4d, 0x23, 0x4d, 0x26, 0x4f, 0x2d, 0x4f, 0x06, 0x0a, 0xee, 0xbe,
	0x0b, 0x2f, 0x4c, 0x38, 0x44, 0xba, 0x74, 0xc3, 0x37, 0x41, 0x3c, 0x4c,
	0x29, 0xbd, 0x1e, 0xc1, 0xa7, 0x42, 0x3c, 0x42, 0x45, 0x4d, 0x45, 0xbc,
	0x99, 0xc2, 0x38, 0x43, 0x46, 0xbc, 0x99, 0xc2, 0x38, 0xbb, 0xd0, 0xc3,
	0x0a, 0xbb, 0xd0, 0xc3, 0x0a, 0x3c, 0x4c, 0x46, 0x4f, 0xbb, 0x61, 0xc3,
	0x8b, 0xbe, 0x9a, 0xc4, 0xfa, 0xba, 0xa5, 0xc4, 0x39, 0x31, 0x4d, 0x06,
	0x0f, 0xee, 0xbb, 0xfb, 0x2e, 0xb9, 0x20, 0xc4, 0xbb, 0xba, 0xac, 0x49,
	0xb9, 0xec, 0xc4, 0x01, 0x2f, 0x46, 0x2c, 0x42, 0xbb, 0x32, 0xc2, 0xde,
	0x31, 0x46, 0xbb, 0x32, 0xc2, 0xde, 0xbc, 0x0c, 0xc2, 0x2f, 0xbc, 0x0c,
	0xc2, 0x2f, 0x35, 0x40, 0x2d, 0x34, 0x38, 0x45, 0xbb, 0x9d, 0xbf, 0x8d,
	0x3e, 0x3e, 0x48, 0x31, 0xbd, 0x44, 0xc1, 0xda, 0x40, 0x40, 0xc0, 0x4c,
	0xc2, 0x57, 0x53, 0x44, 0xbc, 0xbf, 0xc2, 0x6b, 0x40, 0x49, 0xbc, 0xbf,
	0xc2, 0x6b, 0x37, 0x49, 0x37, 0x49, 0xbe, 0x81, 0xc4, 0xd3, 0x3f, 0x4e,
	0x36, 0x4a, 0x3a, 0x4d, 0xbb, 0x08, 0xc4, 0x6a, 0x31, 0x4f, 0x06, 0x05,
	0xae, 0x02, 0x4e, 0x26, 0x42, 0x2a, 0x46, 0x2b, 0x3b, 0x2e, 0x38, 0x36,
	0x3b, 0x42, 0x46, 0x34, 0x06, 0x05, 0xae, 0x02, 0x49, 0x3e, 0x52, 0x2f,
	0xc6, 0xab, 0xbb, 0x90, 0xc5, 0xc8, 0xb8, 0x7c, 0x4e, 0x26, 0x46, 0x34,
	0x3b, 0x42, 0x06, 0x0a, 0xeb, 0xaa, 0x0b, 0x22, 0x36, 0x22, 0x36, 0x21,
	0x3f, 0x28, 0x48, 0x24, 0x49, 0x2f, 0x4c, 0x27, 0x4d, 0x2f, 0x4c, 0x3b,
	0x42, 0x38, 0x36, 0x30, 0x2a, 0x2c, 0x2c, 0x28, 0x29, 0x28, 0x29, 0x24,
	0x2d, 0x26, 0x36, 0x06, 0x0a, 0xeb, 0x6e, 0x0a, 0x31, 0x4d, 0x31, 0x4d,
	0x31, 0x54, 0x39, 0x56, 0x3a, 0x51, 0x50, 0x4f, 0x49, 0x55, 0x50, 0x4f,
	0x4e, 0x4c, 0x5a, 0x44, 0x55, 0x4b, 0x5a, 0x44, 0x56, 0x42, 0x40, 0x49,
	0x3e, 0x3b, 0x42, 0x06, 0x04, 0xeb, 0x2f, 0x4c, 0x2f, 0x4c, 0x29, 0x52,
	0x22, 0x57, 0x26, 0x58, 0x31, 0x4d, 0x2c, 0x53, 0x31, 0x4d, 0x08, 0x0a,
	0x00, 0x05, 0x03, 0x05, 0x06, 0x07, 0x04, 0x10, 0x01, 0x17, 0x84, 0x02,
	0x04, 0x0a, 0x06, 0x01, 0x05, 0x00, 0x0a, 0x07, 0x01, 0x06, 0x00, 0x0a,
	0x04, 0x01, 0x03, 0x00, 0x0a, 0x05, 0x01, 0x04, 0x00, 0x0a, 0x02, 0x01,
	0x02, 0x00, 0x0a, 0x08, 0x01, 0x07, 0x00, 0x0a, 0x03, 0x01, 0x01, 0x08,
	0x15, 0xff
};

// Leaf 5 - Red 1
const unsigned char kLeaf5[] = {
	0x6e, 0x63, 0x69, 0x66, 0x08, 0x05, 0x00, 0x02, 0x00, 0x12, 0x02, 0x00,
	0x00, 0x00, 0x3d, 0x40, 0x00, 0xbd, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x4b,
	0x20, 0x00, 0x4a, 0x10, 0x00, 0x00, 0x01, 0x66, 0xff, 0x01, 0x85, 0x02,
	0x00, 0x06, 0x02, 0x3b, 0x9f, 0xd5, 0xbb, 0x5f, 0x70, 0x3e, 0xb9, 0x98,
	0x3e, 0xed, 0x9f, 0x45, 0xbf, 0xbf, 0x47, 0x19, 0x8c, 0x00, 0xc5, 0x53,
	0x30, 0xff, 0xdd, 0x5b, 0x27, 0x02, 0x00, 0x06, 0x02, 0x3b, 0x9f, 0xd5,
	0xbb, 0x5f, 0x70, 0x3e, 0xb9, 0x98, 0x3e, 0xed, 0x9f, 0x45, 0x8f, 0xbf,
	0x46, 0xf9, 0x8c, 0x00, 0xff, 0xa2, 0x5f, 0xff, 0xed, 0x65, 0x46, 0x02,
	0x01, 0x06, 0x02, 0x3a, 0x92, 0xa8, 0x3c, 0xb2, 0x8f, 0xbe, 0x9e, 0xad,
	0x3c, 0x7f, 0xb1, 0x4a, 0xd6, 0x23, 0x45, 0xf4, 0x80, 0x00, 0xda, 0x05,
	0x05, 0xfe, 0xff, 0x78, 0x43, 0x02, 0x01, 0x06, 0x02, 0x38, 0xa0, 0x81,
	0x3c, 0xe4, 0x69, 0xc0, 0x05, 0xf5, 0x3b, 0xac, 0xe9, 0x4a, 0x36, 0x3c,
	0x4a, 0x20, 0x00, 0x00, 0xfd, 0x73, 0x3d, 0xff, 0xda, 0x05, 0x05, 0x02,
	0x01, 0x06, 0x02, 0xbd, 0x28, 0x86, 0x3d, 0x5c, 0x6c, 0xbc, 0x93, 0x89,
	0xbc, 0x6b, 0xc1, 0x4a, 0x2c, 0xa1, 0x4a, 0x72, 0x01, 0x00, 0xfd, 0x9c,
	0x5b, 0xff, 0xda, 0x05, 0x05, 0x02, 0x00, 0x06, 0x02, 0x34, 0xb4, 0xd1,
	0x37, 0x3c, 0x1e, 0xbc, 0x49, 0x56, 0x39, 0xd3, 0x68, 0x48, 0xda, 0x03,
	0x4a, 0xe2, 0x5a, 0x00, 0xff, 0xa2, 0x5f, 0xff, 0xa4, 0x0a, 0x12, 0x07,
	0x06, 0x0d, 0xae, 0xff, 0xcf, 0x02, 0x24, 0x5a, 0x29, 0x5a, 0x29, 0x5a,
	0x2f, 0x56, 0x34, 0x51, 0x3e, 0x5c, 0x41, 0x54, 0x40, 0x5b, 0x49, 0x58,
	0x5a, 0x54, 0x57, 0x57, 0x5a, 0x4c, 0x4d, 0x4b, 0x4d, 0x4b, 0x55, 0x4a,
	0x60, 0x3f, 0x5f, 0x43, 0x60, 0x3a, 0x52, 0x3a, 0x52, 0x3a, 0x58, 0x36,
	0x5a, 0x2b, 0x5c, 0x2f, 0xc9, 0x69, 0xb6, 0xe2, 0x54, 0x25, 0x4e, 0x25,
	0x4e, 0x28, 0x50, 0x2e, 0x50, 0x06, 0x0a, 0xee, 0xbe, 0x0b, 0x33, 0x4c,
	0x3e, 0x43, 0x39, 0x48, 0xc1, 0xf5, 0xbd, 0xf0, 0x4d, 0x31, 0xbf, 0xe8,
	0xc0, 0x75, 0x47, 0x3b, 0x48, 0x43, 0x52, 0x40, 0xbf, 0x63, 0xc1, 0x06,
	0x47, 0x44, 0xbe, 0x4d, 0xc2, 0x30, 0xbd, 0x68, 0xc3, 0x0a, 0xbe, 0x25,
	0xc2, 0x56, 0x40, 0x4c, 0x4a, 0x4f, 0xbc, 0xf9, 0xc3, 0x8b, 0xc0, 0x32,
	0xc4, 0xfa, 0xbc, 0x3d, 0xc4, 0x39, 0x35, 0x4d, 0x06, 0x0f, 0xee, 0xbb,
	0xfb, 0x2e, 0xba, 0xb8, 0xc4, 0xbb, 0xbc, 0x44, 0x49, 0xbb, 0x84, 0xc4,
	0x01, 0x33, 0x46, 0x2e, 0x40, 0xbc, 0xca, 0xc2, 0xde, 0x35, 0x46, 0xbd,
	0x85, 0xc2, 0x35, 0xbe, 0xd6, 0xc0, 0xfd, 0xbd, 0xae, 0xc2, 0x1d, 0x3d,
	0x3b, 0x3a, 0x33, 0xbf, 0x80, 0xc0, 0x59, 0x3f, 0x3c, 0xc1, 0xe4, 0xbd,
	0xf5, 0xc5, 0x14, 0xb9, 0xf9, 0xc0, 0x0e, 0xc0, 0xa8, 0x47, 0x3c, 0xc3,
	0x16, 0xc1, 0x25, 0x52, 0x40, 0xbf, 0x89, 0xc1, 0x39, 0xc2, 0xb0, 0xc1,
	0x8b, 0xbe, 0x73, 0xc2, 0x63, 0xbd, 0x8e, 0xc3, 0x3d, 0xbe, 0x4b, 0xc2,
	0x89, 0xbf, 0xe6, 0xc4, 0xbb, 0x4a, 0x4f, 0x3a, 0x4a, 0xc0, 0x6f, 0xc5,
	0x5d, 0xbc, 0xa0, 0xc4, 0x6a, 0x35, 0x4f, 0x06, 0x04, 0xae, 0x4e, 0x3a,
	0x52, 0x26, 0x58, 0x2e, 0xc3, 0x89, 0xb5, 0xfd, 0x43, 0x35, 0x3f, 0x42,
	0x06, 0x08, 0xef, 0xba, 0x26, 0x36, 0x30, 0x35, 0x25, 0x3f, 0x2c, 0x48,
	0x2c, 0x48, 0x28, 0x48, 0x25, 0x4b, 0x33, 0x4c, 0x29, 0x4d, 0x33, 0x4c,
	0x3f, 0x42, 0x43, 0x35, 0x38, 0x2a, 0x41, 0x2d, 0x31, 0x32, 0x35, 0x3e,
	0x06, 0x08, 0xfb, 0xae, 0x35, 0x4d, 0x35, 0x4d, 0x35, 0x56, 0x3b, 0x59,
	0x3e, 0x51, 0x3f, 0x54, 0x3e, 0x51, 0x55, 0x52, 0x4b, 0x57, 0x56, 0x4c,
	0x48, 0x49, 0x5c, 0x3b, 0x59, 0x46, 0x5b, 0x37, 0x4e, 0x3a, 0x3f, 0x42,
	0x06, 0x04, 0xeb, 0x33, 0x4c, 0x33, 0x4c, 0x2d, 0x51, 0x23, 0x55, 0x27,
	0x56, 0x35, 0x4d, 0x30, 0x51, 0x35, 0x4d, 0x07, 0x0a, 0x00, 0x04, 0x03,
	0x04, 0x05, 0x06, 0x10, 0x01, 0x17, 0x84, 0x00, 0x04, 0x0a, 0x05, 0x01,
	0x04, 0x00, 0x0a, 0x06, 0x01, 0x05, 0x00, 0x0a, 0x04, 0x01, 0x03, 0x00,
	0x0a, 0x02, 0x01, 0x02, 0x00, 0x0a, 0x07, 0x01, 0x06, 0x00, 0x0a, 0x03,
	0x01, 0x01, 0x08, 0x15, 0xff
};

// Leaf 6 - Red 2
const unsigned char kLeaf6[] = {
	0x6e, 0x63, 0x69, 0x66, 0x09, 0x05, 0x00, 0x02, 0x00, 0x12, 0x02, 0x00,
	0x00, 0x00, 0x3d, 0x40, 0x00, 0xbd, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x4a,
	0xe0, 0x00, 0x4a, 0x10, 0x00, 0x00, 0x01, 0x66, 0xff, 0x01, 0x85, 0x02,
	0x00, 0x06, 0x02, 0xbb, 0x54, 0x89, 0xbb, 0xa9, 0xdc, 0x3d, 0x61, 0x95,
	0xbd, 0x12, 0xd5, 0x47, 0x21, 0xcd, 0x4b, 0x2e, 0xe5, 0x00, 0xff, 0x69,
	0x3c, 0xff, 0xdd, 0x5b, 0x27, 0x02, 0x00, 0x06, 0x02, 0xba, 0x3f, 0xb3,
	0x3b, 0x68, 0x5f, 0xbc, 0xea, 0x1e, 0xbb, 0xd8, 0xc4, 0x4a, 0x9e, 0xae,
	0x4a, 0x75, 0x8e, 0x00, 0xff, 0x84, 0x4b, 0xff, 0xff, 0xa2, 0x5f, 0x02,
	0x01, 0x06, 0x02, 0x3a, 0x92, 0xa8, 0x3c, 0xb2, 0x8f, 0xbe, 0x9e, 0xad,
	0x3c, 0x7f, 0xb1, 0x4a, 0x96, 0x23, 0x45, 0xf4, 0x80, 0x00, 0xda, 0x17,
	0x05, 0xfe, 0xff, 0x78, 0x43, 0x02, 0x01, 0x06, 0x02, 0x3a, 0x92, 0xa8,
	0x3c, 0xb2, 0x8f, 0xbe, 0x9e, 0xad, 0x3c, 0x7f, 0xb1, 0x4a, 0x96, 0x23,
	0x45, 0xf4, 0x80, 0x00, 0xf7, 0x2e, 0x2e, 0xff, 0xff, 0x9e, 0x59, 0x02,
	0x01, 0x06, 0x02, 0x38, 0xa0, 0x81, 0x3c, 0xe4, 0x69, 0xc0, 0x05, 0xf5,
	0x3b, 0xac, 0xe9, 0x49, 0xec, 0x78, 0x4a, 0x20, 0x00, 0x00, 0xfd, 0x73,
	0x3d, 0xff, 0xda, 0x05, 0x05, 0x02, 0x01, 0x06, 0x02, 0xbd, 0x28, 0x86,
	0x3d, 0x5c, 0x6c, 0xbc, 0x93, 0x89, 0xbc, 0x6b, 0xc1, 0x4a, 0x0c, 0xa1,
	0x4a, 0x82, 0x01, 0x00, 0xff, 0x9e, 0x59, 0xff, 0xf7, 0x2e, 0x2e, 0x02,
	0x00, 0x06, 0x02, 0x35, 0x9e, 0x73, 0x37, 0x00, 0xe5, 0xbc, 0x1f, 0x7a,
	0x3a, 0x8e, 0xd7, 0x48, 0x5d, 0xc8, 0x4a, 0xdf, 0x90, 0x00, 0xff, 0xa2,
	0x5f, 0xff, 0xa4, 0x0a, 0x10, 0x08, 0x06, 0x0e, 0xee, 0xeb, 0xbb, 0x0b,
	0x22, 0x5a, 0x28, 0x5a, 0x28, 0x5a, 0x2e, 0x55, 0x31, 0x51, 0x3d, 0x58,
	0x39, 0x58, 0x3d, 0x58, 0x3d, 0x53, 0x3d, 0x53, 0x48, 0x58, 0x54, 0x52,
	0x53, 0x4f, 0x60, 0x48, 0x5b, 0x4e, 0x60, 0x41, 0x4f, 0x3f, 0x4f, 0x3f,
	0x57, 0x3a, 0x58, 0x30, 0x54, 0x2a, 0x53, 0x2e, 0x51, 0x2b, 0x4e, 0x2b,
	0x23, 0x4d, 0x23, 0x4d, 0x26, 0x4f, 0x2d, 0x4f, 0x06, 0x0a, 0xee, 0xbe,
	0x0b, 0x2f, 0x4c, 0x38, 0x44, 0xba, 0x74, 0xc3, 0x37, 0x41, 0x3c, 0x4c,
	0x29, 0xbd, 0x1e, 0xc1, 0xa7, 0x42, 0x3c, 0x42, 0x45, 0x4d, 0x45, 0xbc,
	0x99, 0xc2, 0x38, 0x43, 0x46, 0xbc, 0x99, 0xc2, 0x38, 0xbb, 0xd0, 0xc3,
	0x0a, 0xbb, 0xd0, 0xc3, 0x0a, 0x3c, 0x4c, 0x46, 0x4f, 0xbb, 0x61, 0xc3,
	0x8b, 0xbe, 0x9a, 0xc4, 0xfa, 0xba, 0xa5, 0xc4, 0x39, 0x31, 0x4d, 0x06,
	0x0f, 0xee, 0xbb, 0xfb, 0x2e, 0xb9, 0x20, 0xc4, 0xbb, 0xba, 0xac, 0x49,
	0xb9, 0xec, 0xc4, 0x01, 0x2f, 0x46, 0x2c, 0x42, 0xbb, 0x32, 0xc2, 0xde,
	0x31, 0x46, 0xbb, 0x32, 0xc2, 0xde, 0xbc, 0x0c, 0xc2, 0x2f, 0xbc, 0x0c,
	0xc2, 0x2f, 0x35, 0x40, 0x2d, 0x34, 0x38, 0x45, 0xbb, 0x9d, 0xbf, 0x8d,
	0x3e, 0x3e, 0x48, 0x31, 0xbd, 0x44, 0xc1, 0xda, 0x40, 0x40, 0xc0, 0x4c,
	0xc2, 0x57, 0x53, 0x44, 0xbc, 0xbf, 0xc2, 0x6b, 0x40, 0x49, 0xbc, 0xbf,
	0xc2, 0x6b, 0x37, 0x49, 0x37, 0x49, 0xbe, 0x81, 0xc4, 0xd3, 0x3f, 0x4e,
	0x36, 0x4a, 0x3a, 0x4d, 0xbb, 0x08, 0xc4, 0x6a, 0x31, 0x4f, 0x06, 0x05,
	0xae, 0x02, 0x4e, 0x26, 0x42, 0x2a, 0x46, 0x2b, 0x3b, 0x2e, 0x38, 0x36,
	0x3b, 0x42, 0x46, 0x34, 0x06, 0x05, 0xae, 0x02, 0x49, 0x3e, 0x52, 0x2f,
	0xc6, 0xab, 0xbb, 0x90, 0xc5, 0xc8, 0xb8, 0x7c, 0x4e, 0x26, 0x46, 0x34,
	0x3b, 0x42, 0x06, 0x0a, 0xeb, 0xaa, 0x0b, 0x22, 0x36, 0x22, 0x36, 0x21,
	0x3f, 0x28, 0x48, 0x24, 0x49, 0x2f, 0x4c, 0x27, 0x4d, 0x2f, 0x4c, 0x3b,
	0x42, 0x38, 0x36, 0x30, 0x2a, 0x2c, 0x2c, 0x28, 0x29, 0x28, 0x29, 0x24,
	0x2d, 0x26, 0x36, 0x06, 0x0a, 0xeb, 0x6e, 0x0a, 0x31, 0x4d, 0x31, 0x4d,
	0x31, 0x54, 0x39, 0x56, 0x3a, 0x51, 0x50, 0x4f, 0x49, 0x55, 0x50, 0x4f,
	0x4e, 0x4c, 0x5a, 0x44, 0x55, 0x4b, 0x5a, 0x44, 0x56, 0x42, 0x40, 0x49,
	0x3e, 0x3b, 0x42, 0x06, 0x04, 0xeb, 0x2f, 0x4c, 0x2f, 0x4c, 0x29, 0x52,
	0x22, 0x57, 0x26, 0x58, 0x31, 0x4d, 0x2c, 0x53, 0x31, 0x4d, 0x08, 0x0a,
	0x00, 0x05, 0x03, 0x05, 0x06, 0x07, 0x04, 0x10, 0x01, 0x17, 0x84, 0x02,
	0x04, 0x0a, 0x06, 0x01, 0x05, 0x00, 0x0a, 0x07, 0x01, 0x06, 0x00, 0x0a,
	0x04, 0x01, 0x03, 0x00, 0x0a, 0x05, 0x01, 0x04, 0x00, 0x0a, 0x02, 0x01,
	0x02, 0x00, 0x0a, 0x08, 0x01, 0x07, 0x00, 0x0a, 0x03, 0x01, 0x01, 0x08,
	0x15, 0xff
};


const int32 kNumLeafTypes = 6;


BBitmap *
FallLeaves::_RandomBitmap(int32 size)
{
	// Load an image of a leaf
	BBitmap *bitmap = new BBitmap(BRect(0, 0, size, size), B_RGBA32);
	
	// Select one randomly
	switch (RAND_NUM(1, kNumLeafTypes)) {
		
		case 1:
			BIconUtils::GetVectorIcon(kLeaf1, sizeof(kLeaf1), bitmap);
			break;
		case 2:
			BIconUtils::GetVectorIcon(kLeaf2, sizeof(kLeaf2), bitmap);
			break;
		case 3:
			BIconUtils::GetVectorIcon(kLeaf3, sizeof(kLeaf3), bitmap);
			break;
		case 4:
			BIconUtils::GetVectorIcon(kLeaf4, sizeof(kLeaf4), bitmap);
			break;
		case 5:
			BIconUtils::GetVectorIcon(kLeaf5, sizeof(kLeaf5), bitmap);
			break;
		case 6:
			BIconUtils::GetVectorIcon(kLeaf6, sizeof(kLeaf6), bitmap);
			break;
	}
	
	return bitmap;
}

/**
 * A lovely screensaver with falling Haiku leaves.
 *
 * Copyright (c) 2011 David Couzelis. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Inspired by and partially copied from the "Icons" screensaver,
 * by Vincent Duvert.
 *
 * Leaf images by Stephan Asmus.
 */

// libbe.so
#include <Bitmap.h>
#include <List.h>
#include <StringView.h>

// libscreensaver.so
#include <ScreenSaver.h>

#include <stdlib.h>

#include "Images.h"
#include "Leaf.h"

// Create a random number between low and high (inclusive)
#define RAND_NUM(low, high) ((rand() % ((high) - (low) + 1) + (low)))

// How fast the screensaver runs
#define TICKS_PER_SECOND 100

#define MICROSECS_IN_SEC 1000000

const char *kModuleName = "Fall Leaves";
const char *kModuleInfo = "by David Couzelis";

// The number of leaves on the screen
const int32 kMaxLeaves = 35;

// See BScreenSaver to learn about screensaver hooks
//   src/kits/screensaver/ScreenSaver.cpp
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
	
	BList			*fLeaves;
	int32			fMaxSize; // The max size of a leaf
	int32			fMaxSpeed; // The max speed of a leaf
	BBitmap			*fBackBitmap;
	BView			*fBackView;
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
	// Empty
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
	
	return -1;
}

/**
 * This is called when the user selects the screensaver
 * from the list of screensavers in the Screensavers
 * Preferences dialog.
 *
 * Whatever you add to the BView parameter will be
 * drawn to the screen.
 */
void FallLeaves::StartConfig(BView *configView)
{
	// Add the module name to the screensaver dialog
	BRect rect(15, 15, 20, 20);
	BStringView* stringView = new BStringView(rect, "module", kModuleName);
	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	configView->AddChild(stringView);

	// Add the module info to the screensaver dialog
	rect.OffsetBy(0, stringView->Bounds().Height() + 4);
	stringView = new BStringView(rect, "info", kModuleInfo);
	stringView->ResizeToPreferred();
	configView->AddChild(stringView);

	// You can add whatever else you want to the BView
}

/**
 * This is called when the screensaver starts.
 * Put any initialization stuff here.
 * If the preview parameter is true,
 * then this screensaver is being run in the tiny
 * demo window in the Screensaver Preferences dialog.
 */
status_t FallLeaves::StartSaver(BView *view, bool preview)
{
	// This would look flickery without a screen buffer...
	// So, we use a BView and a BBitmap as an offscreen buffer.
	// When you finish drawing to it, draw the buffer onto the screen
	BRect screenRect(0, 0, view->Frame().Width(), view->Frame().Height());
	fBackBitmap = new BBitmap(screenRect, B_RGBA32, true);
	if (!fBackBitmap->IsValid())
		return B_NO_MEMORY;

	fBackView = new BView(screenRect, NULL, 0, 0);
	if (fBackView == NULL)
		return B_NO_MEMORY;

	//fBackView->SetViewColor(32, 32, 32); //ui_color(B_DESKTOP_COLOR));
	//fBackView->SetHighColor(ui_color(B_DESKTOP_COLOR)); // TEMP

	fBackBitmap->AddChild(fBackView);
	
	if (fBackBitmap->Lock()) {
		fBackView->FillRect(fBackView->Frame());
		// Draw the leaves with transparency
		fBackView->SetDrawingMode(B_OP_OVER);
		//fBackView->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY); // Used with B_OP_ALPHA
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
	Leaf *leaf;
	
	for (int32 i = 0; (leaf = (Leaf *)fLeaves->ItemAt(i)); i++)
		delete leaf;
	
	delete fLeaves;
	
	fBackBitmap->RemoveChild(fBackView);
	
	delete fBackView;
	delete fBackBitmap;
}

/**
 * This is called to draw the screensaver.
 * Add anything to the BView parameter that you'd
 * like drawn to the screen.
 * The frame parameter holds the current frame number.
 */
void FallLeaves::Draw(BView *view, int32 frame)
{
	fBackBitmap->Lock();
	
	// Clear the offscreen buffer
	fBackView->FillRect(fBackView->Frame());
	
	Leaf *leaf;
	
	// Update and draw the leaves
	for (int32 i = fLeaves->CountItems() - 1; (leaf = (Leaf *)fLeaves->ItemAt(i)); i--) {
		leaf->Update(TICKS_PER_SECOND);
		leaf->Draw(fBackView);
	}
	
	bool sort = false;
	
	// Remove any dead leaves
	// Do this in a separate loop to prevent flicker when drawing
	for (int32 i = fLeaves->CountItems() - 1; (leaf = (Leaf *)fLeaves->ItemAt(i)); i--) {
		if (leaf->IsDead()) {
			
			// Remove the dead leaf
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

#include "IconUtils.h" // libbe.so // Temporarily local

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

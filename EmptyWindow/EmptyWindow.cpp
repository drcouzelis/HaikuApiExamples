#include <Application.h> // To use the variable "be_app"

#include "EmptyWindow.h"


EmptyWindow::EmptyWindow()
	:
	BWindow(BRect(), "Empty Window", B_TITLED_WINDOW,
		B_QUIT_ON_WINDOW_CLOSE)
{
	MoveTo(100, 100);
	ResizeTo(200, 200);
	Show();
}


// This method is called when the "close" button is pressed
// on the application title bar.
bool EmptyWindow::QuitRequested()
{
	// Tell the application to quit
	be_app->PostMessage(B_QUIT_REQUESTED);

	return true;
}

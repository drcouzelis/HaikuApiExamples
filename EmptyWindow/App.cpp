#include <Application.h>

#include "App.h"
#include "MainWindow.h"


App::App()
	:
	BApplication("application/x-vnd.demo-app")
{
	// The only thing that happens when you start this application is
	// a window is created.
	BWindow *mainwin = new MainWindow();
	
	// Now that it has been created, show it!
	mainwin->Show();
}


// Start the application.
int main()
{
	// Setup the application.
	// This includes setting up the user interface.
	new App();

	// Run the application.
	// This includes all of the application logic.
	be_app->Run();

	delete be_app;

	return 0;
}

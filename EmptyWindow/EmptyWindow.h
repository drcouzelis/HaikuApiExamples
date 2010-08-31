#include <Window.h> // To use "BWindow"


// Represents the window on the screen.
// You can't see it until you use the "Show" method.
class EmptyWindow : public BWindow
{
public:

	EmptyWindow();

	virtual bool QuitRequested(); // Override the BWindow method
};

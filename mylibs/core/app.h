#include <string>
#include <vector>
#include <cstdint>
#include <core/timer.h>
#define SDL_USAGE 0

#if SDL_USAGE
	struct SDL_Window;
	typedef void *SDL_GLContext;
#else
	struct GLFWwindow;
#endif


namespace core
{
struct MouseState
{
	int x;
	int y;
	bool leftButtonDown;
	bool rightButtonDown;
	bool middleButtonDown;
};

class App
{
public:
	bool init(const char *windowStr, int screenWidth, int screenHeight);
	virtual ~App();
	void resizeWindow(int w, int h);
	void setVsyncEnabled(bool enable);
	void setClearColor(float r, float g, float b, float a);
	void present();
	void setTitle(const char *str);

	double getDeltaTime();
	MouseState getMouseState();

	public: 
	#if SDL_USAGE
		SDL_Window *window = nullptr;
		SDL_GLContext mainContext = nullptr;		
	#else
		GLFWwindow *window = nullptr;
	#endif
		int windowWidth = 0;
		int windowHeight = 0;
		bool vSync = true;
		bool inited = false;

	private:
	Timer timer;
	double dt = 0.0;

};

};
#include <string>
#include <vector>
#include <cstdint>

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
	#if SDL_USAGE
		uint64_t nowStamp = 0;
		uint64_t lastStamp = 0;
		double freq = 0.0;
	#else
		double nowStamp = 0.0;
		double lastStamp = 0.0; 
	#endif
	double dt = 0.0;

};

bool loadFontData(const std::string &fileName, std::vector<char> &dataOut);

// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a);

};
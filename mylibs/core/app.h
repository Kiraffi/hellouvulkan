#include <string>
#include <vector>

struct SDL_Window;
typedef void *SDL_GLContext;

namespace core
{


class App
{
public:
	bool init(const char *windowStr, int screenWidth, int screenHeight);
	virtual ~App();
	void resizeWindow(int w, int h);
	void setVsyncEnabled(bool enable);
	void setClearColor(float r, float g, float b, float a);

	public: 
		SDL_Window *window = nullptr;
		SDL_GLContext mainContext = nullptr;		
		int windowWidth = 0;
		int windowHeight = 0;
		bool vSync = true;
};

bool loadFontData(const std::string &fileName, std::vector<char> &dataOut);

// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a);

};
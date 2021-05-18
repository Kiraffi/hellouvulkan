#include "app.h"
#include "glad/glad.h"
#if SDL_USAGE
#include <SDL2/SDL.h>
#else
#include <GLFW/glfw3.h>
#endif


#include <stdio.h>
#include <filesystem>
#include <fstream>
#include <cmath>

static void APIENTRY openglCallbackFunction(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	(void)source; (void)type; (void)id; 
	(void)severity; (void)length; (void)userParam;
	
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
	{
		fprintf(stderr, "%s\n", message);
	}

	if (severity==GL_DEBUG_SEVERITY_HIGH)
	{
		fprintf(stderr, "Aborting...\n");
		abort();
	}
}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}


namespace core {

bool App::init(const char *windowStr, int screenWidth, int screenHeight)
{
	#if SDL_USAGE
	// Initialize SDL 
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Couldn't initialize SDL\n");
		return false;
	}
	atexit (SDL_Quit);
	SDL_GL_LoadLibrary(NULL); // Default OpenGL is fine.

	// Request an OpenGL 4.5 context (should be core)
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// Also request a depth buffer
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

	// Debug!
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);


	window = SDL_CreateWindow(windowStr, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screenWidth, screenHeight, SDL_WINDOW_OPENGL);


	if (window == NULL)
	{
		printf("Couldn't set video mode\n");
		return false;
	}
	
	mainContext = SDL_GL_CreateContext(window);
	if (mainContext == nullptr)
	{
		printf("Failed to create OpenGL context\n");
		return false;
	}

	int w,h;
	SDL_GetWindowSize(window, &w, &h);
	

	SDL_SetWindowResizable(window, SDL_TRUE);
	gladLoadGLLoader(SDL_GL_GetProcAddress);
	resizeWindow(w, h);

	#else

	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
	{
		printf("Couldn't initialize GLFW\n");
		return false;
	}

	inited = true;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	window = glfwCreateWindow(screenWidth, screenHeight, windowStr, NULL, NULL);
	if (!window)
	{
		printf("Couldn't create glfw window\n");
		return false;
	}
	glfwMakeContextCurrent(window);
	//gladLoadGL(glfwGetProcAddress);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);


	int w,h;
	glfwGetFramebufferSize(window, &w, &h);
	resizeWindow(w, h);

	#endif


	// Check OpenGL properties
	printf("OpenGL loaded\n");
	printf("Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version:  %s\n", glGetString(GL_VERSION));


	// Enable the debug callback
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);


	// Disable depth test and face culling.
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glClearColor(0.0f, 0.5f, 1.0f, 0.0f);

	#if SDL_USAGE
	nowStamp = SDL_GetPerformanceCounter();
	lastStamp = nowStamp;
	freq = (double)SDL_GetPerformanceFrequency();
	#else
	nowStamp = glfwGetTime();
	lastStamp = nowStamp; 
	#endif

	// rdoc....
	//glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	//glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);

	return true;

}

App::~App()
{
	#if SDL_USAGE
	if(mainContext)
		SDL_GL_DeleteContext(mainContext);
	mainContext = nullptr;
	SDL_DestroyWindow(window);
	#else
	if(window)
		glfwDestroyWindow(window);
	if(inited)
		glfwTerminate();
	#endif
	window = nullptr;
}

void App::resizeWindow(int w, int h)
{
	windowWidth = w;
	windowHeight = h;
	printf("Window size: %i: %i\n", w, h);
	glViewport(0, 0, w, h);
}

void App::setVsyncEnabled(bool enable)
{
	vSync = enable;
	// Use v-sync
	#if SDL_USAGE
		SDL_GL_SetSwapInterval(vSync);
	#else
		glfwSwapInterval(vSync ? 1 : 0);
	#endif
}



void App::setClearColor(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
}

void App::present()
{
	#if SDL_USAGE
		SDL_GL_SwapWindow(window);
		lastStamp = nowStamp;
		nowStamp = SDL_GetPerformanceCounter();
		dt = double((nowStamp - lastStamp) * 1000.0 / freq );

	#else
		glfwSwapBuffers(window);
		lastStamp = nowStamp; 
		nowStamp = glfwGetTime();
		dt = (nowStamp - lastStamp) * 1000.0;
	#endif

}
void App::setTitle(const char *str)
{
	#if SDL_USAGE
	SDL_SetWindowTitle(window, str);
	#else
	glfwSetWindowTitle(window, str);
	#endif
}

double App::getDeltaTime()
{
	return dt;
}

bool loadFontData(const std::string &fileName, std::vector<char> &dataOut)
{
	//
	if (std::filesystem::exists(fileName))
	{
		std::filesystem::path p(fileName);
		uint32_t s = uint32_t(std::filesystem::file_size(p));

		dataOut.resize(s);

		std::ifstream f(p, std::ios::in | std::ios::binary);


		f.read(dataOut.data(), s);

		printf("filesize: %u\n", s);
		return true;
	}
	return false;
}

MouseState App::getMouseState()
{
	MouseState mouseState;
	#if SDL_USAGE
		uint32_t mousePress = SDL_GetMouseState(&mouseState.x, &mouseState.y);
		mouseState.y = windowHeight - mouseState.y;

		mouseState.leftButtonDown = (mousePress & SDL_BUTTON(SDL_BUTTON_LEFT)) == SDL_BUTTON(SDL_BUTTON_LEFT);
		mouseState.rightButtonDown = (mousePress & SDL_BUTTON(SDL_BUTTON_RIGHT)) == SDL_BUTTON(SDL_BUTTON_RIGHT);
		mouseState.middleButtonDown = (mousePress & SDL_BUTTON(SDL_BUTTON_MIDDLE)) == SDL_BUTTON(SDL_BUTTON_MIDDLE);
	#else
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		mouseState.x = xpos;
		mouseState.y = ypos;
		mouseState.y = windowHeight - mouseState.y;

		mouseState.leftButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		mouseState.rightButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
		mouseState.middleButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
	#endif
	return mouseState;
}


// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a)
{
	r = fmaxf(0.0f, fminf(r, 1.0f));
	g = fmaxf(0.0f, fminf(g, 1.0f));
	b = fmaxf(0.0f, fminf(b, 1.0f));
	a = fmaxf(0.0f, fminf(a, 1.0f));

	uint32_t c = 0u;
	c += (uint32_t(r * 255.0f) & 255u);
	c += (uint32_t(g * 255.0f) & 255u) << 8u;
	c += (uint32_t(b * 255.0f) & 255u) << 16u;
	c += (uint32_t(a * 255.0f) & 255u) << 24u;
	
	return c;
}

}; // end of core namespace.

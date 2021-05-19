#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include <core/app.h>
#include <glad/glad.h>
#if SDL_USAGE
#include <SDL2/SDL.h>
#else
#include <GLFW/glfw3.h>
#endif

#include <ogl/shader.h>
#include <ogl/shaderbuffer.h>

#include <chrono>
#include <vector>
#include <filesystem>
#include <fstream>
#include <thread>

static constexpr int SCREEN_WIDTH  = 640;
static constexpr int SCREEN_HEIGHT = 540;

struct GPUVertexData
{
	float posX;
	float posY;
	uint16_t pixelSizeX;
	uint16_t pixelSizeY;
	uint32_t color;
};



bool saveFontData(const std::string &filename, const std::vector<char> &data)
{
	//
	if(std::filesystem::exists(filename))
	{
		std::filesystem::path p(filename);


		std::ofstream f(p, std::ios::out | std::ios::binary);


		f.write(data.data(), data.size());

		printf("filesize: %u\n", uint32_t(data.size()));
		return true;
	}
	return false;
}



struct MyKeyStates
{
	char letter = '\0';
	int newWindowWidth = 0;
	int newWindowHeight = 0;

	bool save = false;
	bool load = false;
	bool copy = false;
	bool paste = false;

	bool quit = false;
	bool resize = false;

	bool upPress = false;
	bool downPress = false;
	bool leftPress = false;
	bool rightPress = false;
};

static MyKeyStates keyStates;

#if SDL_USAGE
static void handleSDLEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{

		switch(event.type)
		{
			case SDL_QUIT:
				keyStates.quit = true;
				break;
			
			case SDL_KEYDOWN:
			{
				SDL_Keycode sym = event.key.keysym.sym; 
				printf("key: %i\n", sym);
				bool ctrlDown = ((event.key.keysym.mod) & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL)) != 0;
				if(ctrlDown &&	sym == SDLK_s)
					keyStates.save = true;
				else if(ctrlDown &&	sym == SDLK_l)
					keyStates.load = true;
				else if(ctrlDown && sym == SDLK_c)
					keyStates.copy = true;
				else if(ctrlDown && sym == SDLK_v)
					keyStates.paste = true;
				else if(sym >= 32 && sym < 128)
					keyStates.letter = sym;

				switch(sym)
				{
					case SDLK_ESCAPE:
						keyStates.quit = true;
						break;
					case SDLK_UP:
						keyStates.upPress = true;
						break;

					case SDLK_DOWN:
						keyStates.downPress = true;
						break;

					case SDLK_LEFT:
						keyStates.leftPress = true;
						break;

					case SDLK_RIGHT:
						keyStates.rightPress = true;
						break;

					default:
						break;
				}
				break;
			}
			case SDL_KEYUP:
			{
				switch(event.key.keysym.sym)
				{
					case SDLK_UP:
						keyStates.upPress = false;
						break;

					case SDLK_DOWN:
						keyStates.downPress = false;
						break;

					case SDLK_LEFT:
						keyStates.leftPress = false;
						break;


					case SDLK_RIGHT:
						keyStates.rightPress = false;
						break;

					default:
						break;
				}
			}

			case SDL_WINDOWEVENT:
			{
				if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					keyStates.newWindowWidth = event.window.data1;
					keyStates.newWindowHeight = event.window.data2;
					keyStates.resize = true;
				}
				break;
			}
		}
	}
}
#else
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(action == GLFW_PRESS)
	{
		bool ctrlDown = mods & GLFW_MOD_CONTROL;
		switch(key)
		{
			case GLFW_KEY_ESCAPE: 
				keyStates.quit = true;
				glfwSetWindowShouldClose(window, GLFW_TRUE);
				break;
			case GLFW_KEY_LEFT:
				keyStates.leftPress = true;
				break;
			case GLFW_KEY_RIGHT:
				keyStates.rightPress = true;
				break;
			case GLFW_KEY_UP:
				keyStates.upPress = true;
				break;
			case GLFW_KEY_DOWN:
				keyStates.downPress = true;
				break;

			default:
				if(ctrlDown && key == GLFW_KEY_S)
				{
					keyStates.save = true;
				}
				else if(ctrlDown && key == GLFW_KEY_L)
				{
					keyStates.save = true;
				}
				if(ctrlDown && key == GLFW_KEY_C)
				{
					keyStates.copy = true;
				}
				if(ctrlDown && key == GLFW_KEY_V)
				{
					keyStates.paste = true;
				}
				else if(key >= 32 && key < 128)
				{
					char letter = (char)key;
					keyStates.letter = letter;
				}
				break;
		}

	}
	if(action == GLFW_RELEASE)
	{
		switch(key)
		{
			case GLFW_KEY_LEFT:
				keyStates.leftPress = false;
				break;
			case GLFW_KEY_RIGHT:
				keyStates.rightPress = false;
				break;
			case GLFW_KEY_UP:
				keyStates.upPress = false;
				break;
			case GLFW_KEY_DOWN:
				keyStates.downPress = false;
				break;

			default:
				break;
		}

	}
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	keyStates.resize = true;
	keyStates.newWindowWidth = width;
	keyStates.newWindowHeight = height;
}

#endif



static void mainProgramLoop(core::App &app, std::vector<char> &data, std::string &filename)
{
	Shader shader;
	if (!shader.initShader("assets/shaders/colorquad.vert", "assets/shaders/colorquad.frag"))
	{
		printf("Failed to init shader\n");
		return;
	}
	#if !SDL_USAGE
	glfwSetKeyCallback(app.window, key_callback);
	glfwSetFramebufferSizeCallback(app.window, framebuffer_size_callback);
	#endif

	ShaderBuffer ssbo(GL_SHADER_STORAGE_BUFFER, 10240u * 16u, GL_DYNAMIC_COPY, nullptr);
	
	int32_t chosenLetter = 'a';
	//uint32_t lastTicks = SDL_GetTicks();

	std::vector<uint32_t> indices;
	indices.resize(6 * 10240);
	for(int i = 0; i < 10240; ++i)
	{
		indices[size_t(i) * 6 + 0] = i * 4 + 0;
		indices[size_t(i) * 6 + 1] = i * 4 + 1;
		indices[size_t(i) * 6 + 2] = i * 4 + 2;

		indices[size_t(i) * 6 + 3] = i * 4 + 0;
		indices[size_t(i) * 6 + 4] = i * 4 + 2;
		indices[size_t(i) * 6 + 5] = i * 4 + 3;
	}


	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);

	ShaderBuffer indexBuffer(
		GL_ELEMENT_ARRAY_BUFFER, 
		uint32_t(indices.size() * sizeof(uint32_t)),
		GL_STATIC_DRAW,
		indices.data()
		);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.handle)
;	//glEnableVertexAttribArray(0);  

	std::vector<GPUVertexData> vertData;
	vertData.resize(12*8* (128-32 + 1) + 1);


	static constexpr float buttonSize = 20.0f;
	static constexpr float smallButtonSize = 2.0f;
	static constexpr float borderSizes = 2.0f;

	{
		float offX = (borderSizes + buttonSize) + app.windowWidth * 0.5f;
		float offY = (borderSizes + buttonSize) + app.windowHeight * 0.5f;

		GPUVertexData &vdata = vertData[0];
		vdata.color = core::getColor(1.0f, 0.0f, 0.0f, 1.0f);
		vdata.pixelSizeX = uint16_t(smallButtonSize) * 8 + 4;
		vdata.pixelSizeY = uint16_t(smallButtonSize) * 12 + 4;
		vdata.posX = offX;
		vdata.posY = offY;
	}

	for(int j = 0; j < 12; ++j)
	{
		for(int i = 0; i < 8; ++i)
		{
			float offX = float((i - 4) * (borderSizes + buttonSize)) + app.windowWidth * 0.5f;
			float offY = float((j - 6) * (borderSizes + buttonSize)) + app.windowHeight * 0.5f;

			GPUVertexData &vdata = vertData[i + size_t(j) * 8 + 1];
			vdata.color = 0;
			vdata.pixelSizeX = vdata.pixelSizeY = buttonSize;
			vdata.posX = offX;
			vdata.posY = offY;
		}
	}

	for(int k = 0; k < 128 - 32; ++k)
	{
		int x = k % 8;
		int y = k / 8;
		for(int j = 0; j < 12; ++j)
		{
			for(int i = 0; i < 8; ++i)
			{
				GPUVertexData &vdata = vertData[i + size_t(j) * 8 + (size_t(k) + 1) * 8 * 12 + 1];

				float smallOffX = float(i * (smallButtonSize)) + 10.0f + float(x * 8) * smallButtonSize + x * 2;
				float smallOffY = float(j * (smallButtonSize)) + 10.0f + float(y * 12) * smallButtonSize + y * 2;

				uint32_t indx = k * 12 + j;
				bool isVisible = ((data[indx] >> i) & 1) == 1;

				vdata.color = isVisible ? ~0u : 0u;
				vdata.pixelSizeX = vdata.pixelSizeY = smallButtonSize;
				vdata.posX = smallOffX;
				vdata.posY = smallOffY;

			}
		}
	}

	char buffData[12] = {};
	bool quit = false;



	while (!quit)
	{
		keyStates = MyKeyStates{};
		double dt = app.getDeltaTime();
		#if SDL_USAGE
			handleSDLEvents();
		#else
			glfwPollEvents();
			quit = glfwWindowShouldClose(app.window);
		#endif
		core::MouseState mouseState = app.getMouseState();

		if(keyStates.quit)
			quit = true;

		if(quit)
			break;

		if(keyStates.resize)
		{
			app.resizeWindow(keyStates.newWindowWidth, keyStates.newWindowHeight);
			glUniform2f(0, GLfloat(app.windowWidth), GLfloat(app.windowHeight));

		}


		if(keyStates.leftPress)
			--chosenLetter;

		if(keyStates.rightPress)
			++chosenLetter;

		if(keyStates.downPress)
			chosenLetter -= 8;

		if(keyStates.upPress)
			chosenLetter += 8;

		if(keyStates.letter != '\0')
			chosenLetter = (int)keyStates.letter;

		if(keyStates.save)
			saveFontData(filename, data);

		if(keyStates.load)
			core::loadFontData(filename, data);

		if(keyStates.copy)
		{
			for(int i = 0; i < 12; ++i)
			{
				uint32_t ind = (chosenLetter - 32) * 12 + i;
				buffData[i] = data[ind]; 
			}
		}

		if(keyStates.paste)
		{
			for(int i = 0; i < 12; ++i)
			{
				uint32_t ind = (chosenLetter - 32) * 12 + i;
				data[ind] = char(buffData[i]); 
			}
		}

		if(chosenLetter < 32)
			chosenLetter = 32;
		if(chosenLetter > 127)
			chosenLetter = 127;


		 //Clear color buffer
		glClear( GL_COLOR_BUFFER_BIT );
		
		shader.useProgram();
		glUniform2f(0, GLfloat(app.windowWidth), GLfloat(app.windowHeight));


		
		for(int j = 0; j < 12; ++j)
		{
			
			for(int i = 0; i < 8; ++i)
			{
				float offX = float((i - 4) * (borderSizes + buttonSize)) + app.windowWidth * 0.5f;
				float offY = float((j - 6) * (borderSizes + buttonSize)) + app.windowHeight * 0.5f;

				bool insideRect = mouseState.x > offX - (borderSizes + buttonSize) * 0.5f &&
					mouseState.x < offX + (borderSizes + buttonSize) * 0.5f &&
					mouseState.y > offY - (borderSizes + buttonSize) * 0.5f &&
					mouseState.y < offY + (borderSizes + buttonSize) * 0.5f;

				uint32_t indx = (chosenLetter - 32) * 12 + j;

				if(mouseState.leftButtonDown && insideRect)
					data[indx] |= (1 << i);
				else if(mouseState.rightButtonDown && insideRect)
					data[indx] &= ~(char(1 << i));
				
				bool isVisible = ((data[indx] >> i) & 1) == 1;

				vertData[i + size_t(j) * 8 + 1].color = isVisible ? ~0u : 0u;
				vertData[i + size_t(j) * 8 + 1].posX = uint16_t(offX);
				vertData[i + size_t(j) * 8 + 1].posY = uint16_t(offY);
				vertData[(size_t(indx) + 12) * 8 + i + 1].color = isVisible ? ~0u : 0u;

			}

		}
		uint32_t xOff = (chosenLetter - 32) % 8;
		uint32_t yOff = (chosenLetter - 32) / 8;
		
		vertData[0].posX = 10.0f + (4 + xOff * 8) * smallButtonSize + xOff * 2 - 1;
		vertData[0].posY = 10.0f + (6 + yOff * 12) * smallButtonSize + yOff * 2 - 1;


		ssbo.updateBuffer(0, uint32_t(vertData.size() * sizeof(GPUVertexData)), vertData.data());
		ssbo.bind(0);
//		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(VAO);
		
		glDrawElements(GL_TRIANGLES, GLsizei(vertData.size() * 6), GL_UNSIGNED_INT, 0);

		app.present();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));


		char str[100];
		char renderLetter = chosenLetter != 127 ? char(chosenLetter) : ' ';
		float fps = dt > 0.0 ? float(1.0 / dt) : 0.0f;
		sprintf(str, "%2.2fms, fps: %4.2f, mx: %i, my: %i, ml: %i, mr: %i, mb: %i, Letter: %c", 
			float(dt * 1000.0), fps, 
			mouseState.x, mouseState.y, mouseState.leftButtonDown, mouseState.rightButtonDown, mouseState.middleButtonDown, 
			renderLetter);
		app.setTitle(str);

		//printf("Frame duration: %f fps: %f\n", dt, 1000.0f / dt);
	}
}

int main(int argCount, char **argv) 
{
	std::vector<char> data;
	std::string filename;
	if(argCount < 2)
	{
		filename = "assets/font/new_font.dat";
	}
	else
	{
		filename = argv[1];
	}
	
	if (!core::loadFontData(filename, data))
	{

	}
	else
	{
		core::App app;
		if (app.init("OpenGL 4.5", SCREEN_WIDTH, SCREEN_HEIGHT))
		{
			mainProgramLoop(app, data, filename);
		}
	}
	return 0;
}
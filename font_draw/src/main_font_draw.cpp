#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include "glad/glad.h"
#include <SDL2/SDL.h>

#include "core/app.h"

#include "ogl/shader.h"
#include "ogl/shaderbuffer.h"

#include <vector>
#include <filesystem>
#include <fstream>

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


static void mainProgramLoop(core::App &app, std::vector<char> &data, std::string &filename)
{
	Shader shader;
	if (!shader.initShader("assets/shaders/colorquad.vert", "assets/shaders/colorquad.frag"))
	{
		printf("Failed to init shader\n");
		return;
	}

	ShaderBuffer ssbo(GL_SHADER_STORAGE_BUFFER, 10240u * 16u, GL_DYNAMIC_COPY, nullptr);
	
	uint32_t chosenLetter = 'a';
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
	SDL_Event event;
	bool quit = false;
	float dt = 0.0f;

	Uint64 nowStamp = SDL_GetPerformanceCounter();
	Uint64 lastStamp = 0;
	double freq = (double)SDL_GetPerformanceFrequency();


	while (!quit)
	{
		lastStamp = nowStamp;
		nowStamp = SDL_GetPerformanceCounter();
		dt = float((nowStamp - lastStamp)*1000 / freq );


		int mouseX, mouseY;
		uint32_t mousePress = SDL_GetMouseState(&mouseX, &mouseY);

		mouseY = app.windowHeight - mouseY;

		bool mouseLeftDown = (mousePress & SDL_BUTTON(SDL_BUTTON_LEFT)) == SDL_BUTTON(SDL_BUTTON_LEFT);
		bool mouseRightDown = (mousePress & SDL_BUTTON(SDL_BUTTON_RIGHT)) == SDL_BUTTON(SDL_BUTTON_RIGHT);


		while (SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_QUIT:
					quit = true;
					break;
				
				case SDL_KEYDOWN:
				{
					if(((event.key.keysym.mod) & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL)) != 0 &&
						event.key.keysym.sym == SDLK_s)
					{
						// save;
						saveFontData(filename, data);
					}
					else if((event.key.keysym.mod & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL)) != 0 &&
						event.key.keysym.sym == SDLK_l)
					{
						// load;
						core::loadFontData(filename, data);
					}

					else if(((event.key.keysym.mod) & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL)) != 0 &&
						event.key.keysym.sym == SDLK_c)
					{
						// copy
						for(int i = 0; i < 12; ++i)
						{
							uint32_t ind = (chosenLetter - 32) * 12 + i;
							buffData[i] = data[ind]; 
						}
					}
					else if((event.key.keysym.mod & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL)) != 0 &&
						event.key.keysym.sym == SDLK_v)
					{
						// paste
						for(int i = 0; i < 12; ++i)
						{
							uint32_t ind = (chosenLetter - 32) * 12 + i;
							data[ind] = char(buffData[i]); 
						}
					}

					else if(event.key.keysym.sym >= SDLK_SPACE && 
						event.key.keysym.sym < 128)
					{
						chosenLetter = event.key.keysym.sym;
						//updateArray(arr, data, chosenLetter);
					}
					else switch(event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							quit = true;
							break;

						case SDLK_RIGHT:
							if(chosenLetter < 127)
								++chosenLetter;
							break; 

						case SDLK_LEFT:
							if(chosenLetter > 32)
								--chosenLetter;
							break;

						case SDLK_UP:
							chosenLetter += 8;
							if(chosenLetter > 127)
								chosenLetter = 127;
							break;

						case SDLK_DOWN:
							chosenLetter -= 8;
							if(chosenLetter < 32)
								chosenLetter = 32;
							break;

						default:
							break;
					}
				}
				break;

				case SDL_WINDOWEVENT:
				{
					if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						app.resizeWindow(event.window.data1, event.window.data2);
						glUniform2f(0, GLfloat(app.windowWidth), GLfloat(app.windowHeight));
					}
				}
				break;
			}
		}

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

				bool insideRect = mouseX > offX - (borderSizes + buttonSize) * 0.5f &&
					mouseX < offX + (borderSizes + buttonSize) * 0.5f &&
					mouseY > offY - (borderSizes + buttonSize) * 0.5f &&
					mouseY < offY + (borderSizes + buttonSize) * 0.5f;

				uint32_t indx = (chosenLetter - 32) * 12 + j;

				if(mouseLeftDown && insideRect)
					data[indx] |= (1 << i);
				else if(mouseRightDown && insideRect)
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

		SDL_GL_SwapWindow(app.window);
		SDL_Delay(1);

		char str[100];
		char renderLetter = chosenLetter != 127 ? char(chosenLetter) : ' ';
		sprintf(str, "%2.2fms, fps: %4.2f, mx: %i, my: %i, mbs: %i ml: %i, mr: %i, Letter: %c", 
			dt, 1000.0f / dt, mouseX, mouseY, mousePress, mouseLeftDown, mouseRightDown, renderLetter);
		SDL_SetWindowTitle(app.window, str);

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
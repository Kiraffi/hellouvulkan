#include <cstdio>
#include <cstdlib>
#include <stdint.h>


#include "glad/glad.h"
#include <SDL2/SDL.h>

#include "core/app.h"

#include "ogl/shader.h"
#include "ogl/shaderbuffer.h"

#include <string>
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

	float uvX;
	float uvY;

	float padding[2];
};

static const char *vertSrc = R"(
	#version 450 core

	layout (location = 0) uniform vec2 windowSize;

	struct VData
	{
		vec2 vpos;
		uint vSizes;
		uint vColor;

		vec2 vUv;
		vec2 tmp;
	};

	layout (std430, binding=0) buffer shader_data
	{
		VData values[];
	};


	layout (location = 0) out vec4 colOut;
	layout (location = 1) out vec2 uvOut;
	void main()
	{
		int quadId = gl_VertexID / 4;
		int vertId = gl_VertexID % 4;

		vec2 p = vec2(-0.5f, -0.5f);
		p.x = (vertId + 1) % 4 < 2 ? -0.5f : 0.5f;
		p.y = vertId < 2 ? -0.5f : 0.5f;

		uvOut = p + 0.5f;
		
		uvOut.x = (uvOut.x) / (128.0f - 32.0f) + values[quadId].vUv.x;

		vec2 vSize = vec2(float(values[quadId].vSizes & 0xffffu),
			float((values[quadId].vSizes >> 16) & 0xffffu)); 
		p *= vSize;
		p += values[quadId].vpos;
		p /= windowSize * 0.5f;
		p -= 1.0f;
		
		gl_Position = vec4(p.xy, 0.5, 1.0);
		vec4 c = vec4(0, 0, 0, 0);
		c.r = float((values[quadId].vColor >> 0u) & 255u) / 255.0f;
		c.g = float((values[quadId].vColor >> 8u) & 255u) / 255.0f;
		c.b = float((values[quadId].vColor >> 16u) & 255u) / 255.0f;
		c.a = float((values[quadId].vColor >> 24u) & 255u) / 255.0f;
		colOut = c;
	})";

static const char *fragSrc = R"(
	#version 450 core
	layout(origin_upper_left) in vec4 gl_FragCoord;
	layout (location = 0) out vec4 outColor;

	layout (location = 0) in vec4 colIn;
	layout (location = 1) in vec2 uvIn;
	layout(depth_unchanged) out float gl_FragDepth;
	
	layout (binding = 0) uniform sampler2D ourTexture;

	void main()
	{
		outColor = texture(ourTexture, uvIn);
		outColor.a = clamp(outColor.a / 0.5f, 0.0f, 1.0f);
		outColor.rgb = colIn.rgb;

	}
	)";



struct Cursor
{
	float xPos = 0.0f;
	float yPos = 0.0f;

	int charWidth = 8;
	int charHeight = 12;
};

static void addText(std::string &str, std::vector<GPUVertexData> &vertData, Cursor &cursor)
{
	for(int i = 0; i < int(str.length()); ++i)
	{
		GPUVertexData vdata;
		vdata.color = core::getColor(0.0f, 1.0f, 0.0f, 1.0f);
		vdata.pixelSizeX = cursor.charWidth;
		vdata.pixelSizeY = cursor.charHeight;
		vdata.posX = cursor.xPos;
		vdata.posY = cursor.yPos;

		uint32_t letter = str[i] - 32;

		vdata.uvX = float(letter) / float(128-32);
		vdata.uvY = 0.0f;

		vertData.emplace_back(vdata);

		cursor.xPos += cursor.charWidth;
	}

}
static void updateText(std::string &str, std::vector<GPUVertexData> &vertData, Cursor &cursor)
{
	cursor.xPos = 100.0f;
	cursor.yPos = 400.0f;
	std::string tmpStr = "w";
	tmpStr += std::to_string(cursor.charWidth);
	tmpStr += ",h";
	tmpStr += std::to_string(cursor.charHeight);
	vertData.clear();
	addText(tmpStr, vertData, cursor);

	cursor.xPos = 100.0f;
	cursor.yPos = 100.0f;
	addText(str, vertData, cursor);
}




static void mainProgramLoop(core::App &app, std::vector<char> &data, std::string &filename)
{
	Shader shader;
	if(!shader.initShader("assets/shaders/texturedquad.vert", "assets/shaders/texturedquad.frag"))
	{
		printf("Failed to init shader\n");
		return;
	}


	ShaderBuffer ssbo(GL_SHADER_STORAGE_BUFFER, 10240u * 16u, GL_DYNAMIC_COPY, nullptr);
	

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
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.handle);
	//glEnableVertexAttribArray(0);  

	std::vector<GPUVertexData> vertData;



	std::string txt = "Hiiohoi";


	uint32_t texHandle = 0;

	{
		std::vector<uint8_t> fontPic;
		fontPic.resize((128-32) * 8 * 12 * 4);

		// Note save order is a bit messed up!!! Since the file has one char 8x12 then next
		uint32_t index = 0;
		for(int y = 0; y < 12; ++y)
		{
			for(int charIndex = 0; charIndex < 128 - 32; ++charIndex)
			{
				uint8_t p = data[y + size_t(charIndex) * 12];
				for(int x = 0; x < 8; ++x)
				{
					uint8_t bitColor = uint8_t((p >> x) & 1) * 255;
					fontPic[size_t(index) * 4 + 0] = bitColor;
					fontPic[size_t(index) * 4 + 1] = bitColor;
					fontPic[size_t(index) * 4 + 2] = bitColor;
					fontPic[size_t(index) * 4 + 3] = bitColor;

					++index;
				}
			}
		}
		const int textureWidth = 8*(128-32);
		const int textureHeight = 12;

		glGenTextures(1, &texHandle);
		glBindTexture(GL_TEXTURE_2D, texHandle);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, textureWidth, textureHeight);
		
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, GL_BGRA, GL_UNSIGNED_BYTE, fontPic.data());
	}





	Cursor cursor;
	updateText(txt, vertData, cursor);

	SDL_Event event;
	bool quit = false;
	float dt = 0.0f;

	Uint64 nowStamp = SDL_GetPerformanceCounter();
	Uint64 lastStamp = 0;
	double freq = (double)SDL_GetPerformanceFrequency();


	app.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	while (!quit)
	{
		lastStamp = nowStamp;
		nowStamp = SDL_GetPerformanceCounter();
		dt = float((nowStamp - lastStamp)*1000 / freq );


		while (SDL_PollEvent(&event))
		{
			switch(event.type)
			{
				case SDL_QUIT:
					quit = true;
					break;
				
				case SDL_KEYDOWN:
				{
					if(event.key.keysym.sym >= 32 && event.key.keysym.sym < 128)
					{
						if(((event.key.keysym.mod) & (KMOD_SHIFT | KMOD_LSHIFT | KMOD_RSHIFT | KMOD_CAPS)) != 0 &&
							event.key.keysym.sym >= 96 && event.key.keysym.sym <= 122)
						{
							txt += char(event.key.keysym.sym - 32);
						
						} 
						else
						{
							txt += char(event.key.keysym.sym);
						}
						updateText(txt, vertData, cursor);

					}

					switch(event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							quit = true;
							break;
						case SDLK_UP:
							cursor.charHeight++;
							updateText(txt, vertData, cursor);
							break;

						case SDLK_DOWN:
							cursor.charHeight--;
							if(cursor.charHeight < 2)
								++cursor.charHeight;
							updateText(txt, vertData, cursor);
							break;

						case SDLK_LEFT:
							cursor.charWidth--;
							if(cursor.charWidth < 2)
								++cursor.charWidth;
							updateText(txt, vertData, cursor);
							break;


						case SDLK_RIGHT:
							cursor.charWidth++;
							updateText(txt, vertData, cursor);
							break;

						default:
							break;
					}
					break;
				}

				case SDL_WINDOWEVENT:
				{
					if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						app.resizeWindow(event.window.data1, event.window.data2);
						glUniform2f(0, GLfloat(app.windowWidth), GLfloat(app.windowHeight));
					}
					break;
				}
			}
		}

		 //Clear color buffer
		glClear( GL_COLOR_BUFFER_BIT );
		
		shader.useProgram();
		glUniform2f(0, GLfloat(app.windowWidth), GLfloat(app.windowHeight));


		

		ssbo.updateBuffer(0, uint32_t(vertData.size() * sizeof(GPUVertexData)), vertData.data());
		ssbo.bind(0);
//		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(VAO);

		glBindTexture(GL_TEXTURE_2D, texHandle);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


		glDrawElements(GL_TRIANGLES, GLsizei(vertData.size() * 6), GL_UNSIGNED_INT, 0);

		SDL_GL_SwapWindow(app.window);
		SDL_Delay(1);

		char str[100];
		sprintf(str, "%2.2fms, fps: %4.2f", 
			dt, 1000.0f / dt);
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
	
	if(core::loadFontData(filename, data))
	{
		core::App app;
		if(app.init("OpenGL 4.5, render font", SCREEN_WIDTH, SCREEN_HEIGHT))
		{
			mainProgramLoop(app, data, filename);
		}
	}
	else
	{
		printf("Failed to load file: %s\n", filename.c_str());
	}
	
	return 0;
}
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include <core/app.h>
#include <core/timer.h>

#include <glad/glad.h>
#if SDL_USAGE
	#include <SDL2/SDL.h>
#else
	#include <GLFW/glfw3.h>
#endif

#include <ogl/shader.h>
#include <ogl/shaderbuffer.h>

#include <core/mytypes.h>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#include <cmath>

static constexpr int SCREEN_WIDTH  = 640;
static constexpr int SCREEN_HEIGHT = 540;


struct Entity
{
	float posX;
	float posY;
	float posZ;
	float rotation;

	float speedX;
	float speedY;
	float size;
	float padding;
};

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

/*

struct GpuModelInstance
{
	float posX;
	float posY;
	//float posZ;
	float sinRotation;
	float cosRotation;
	//float rotation;

	uint32_t color;
	float size;
	uint32_t modelVertexStartIndex;
	uint32_t modelIndiceCount;
};
*/

struct GpuModelInstance
{
	uint32_t pos;
	uint32_t sinCosRotSize;
	uint32_t color;
	uint32_t modelVertexStartIndex;
};

struct GpuModelVertex
{
	float posX;
	float posY;
	//float posZ;

	//float padding;
};


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
		vdata.color = core::getColor(0.0f, 1.0f, 0, 1.0f);
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

struct MyKeyStates
{
	int newWindowWidth = 0;
	int newWindowHeight = 0;

	bool quit = false;
	bool resize = false;
};

static MyKeyStates keyStates;
static bool keysDown[ 255 ] = {};


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
				switch(event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						keyStates.quit = true;
						break;
					case SDLK_UP:
					case SDLK_w:
						keysDown[ 0 ] = true;
						break;

					case SDLK_DOWN:
					case SDLK_s:
						break;

					case SDLK_LEFT:
					case SDLK_a:
						keysDown[ 1 ] = true;
						break;


					case SDLK_RIGHT:
					case SDLK_d:
						keysDown[ 2 ] = true;
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
					case SDLK_w:
						keysDown[ 0 ] = false;
						break;

					case SDLK_DOWN:
					case SDLK_s:
						break;

					case SDLK_LEFT:
					case SDLK_a:
						keysDown[ 1 ] = false;
						break;


					case SDLK_RIGHT:
					case SDLK_d:
						keysDown[ 2 ] = false;
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
		switch(key)
		{
			case GLFW_KEY_ESCAPE: 
				keyStates.quit = true;
				glfwSetWindowShouldClose(window, GLFW_TRUE);
				break;
			case GLFW_KEY_LEFT:
			case GLFW_KEY_A:
				keysDown[ 1 ] = true;
				break;
			case GLFW_KEY_RIGHT:
			case GLFW_KEY_D:
				keysDown[ 2 ] = true;
				break;
			case GLFW_KEY_UP:
			case GLFW_KEY_W:
				keysDown[ 0 ] = true;
				break;
			case GLFW_KEY_DOWN:
			case GLFW_KEY_S:
				break;

			default:
				break;
		}

	}
	if(action == GLFW_RELEASE)
	{
		switch(key)
		{
			case GLFW_KEY_LEFT:
			case GLFW_KEY_A:
				keysDown[ 1 ] = false;
				break;
			case GLFW_KEY_RIGHT:
			case GLFW_KEY_D:
				keysDown[ 2 ] = false;
				break;
			case GLFW_KEY_UP:
			case GLFW_KEY_W:
				keysDown[ 0 ] = false;
				break;
			case GLFW_KEY_DOWN:
			case GLFW_KEY_S:
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
	srand(100);

	Shader modelShader;
	if(!modelShader.initShader("assets/shaders/model.vert", "assets/shaders/model.frag"))
	{
		printf("Failed to init model shader\n");
		return;
	}

	#if !SDL_USAGE
	glfwSetKeyCallback(app.window, key_callback);
	glfwSetFramebufferSizeCallback(app.window, framebuffer_size_callback);
	#endif


	Shader shaderTexture;
	if (!shaderTexture.initShader("assets/shaders/texturedquad.vert", "assets/shaders/texturedquad.frag"))
	{
		printf("Failed to init texture shader\n");
		return;
	}

	std::vector< GpuModelInstance > modelInstances;
	std::vector< uint32_t > freeModelInstanceIndices;

	std::vector< uint32_t > modelIndices;

	std::vector < GpuModelVertex > vertices;

	modelInstances.reserve(100);
	
	std::vector < Entity > entities;

	constexpr uint32_t AsteroidMaxTypes = 10'000u;
	for(uint32_t asteroidTypes = 0u; asteroidTypes < AsteroidMaxTypes; ++asteroidTypes)
	{
		static constexpr uint32_t AsteroidCorners = 256u;

		float xPos = float(rand()) / float(RAND_MAX) * 2000.0f;
		float yPos = float(rand()) / float(RAND_MAX) * 1200.0f;
		float size = 1.0f + 1.0f * float(rand()) / float(RAND_MAX);
		entities.emplace_back(Entity{ .posX = xPos,  .posY = yPos, .posZ = 0.5f, .rotation = 0.0f, .speedX = 0.0f, .speedY = 0.0f, .size = size, .padding = 0.0f });

/*
		modelInstances.emplace_back(GpuModelInstance{ .posX = xPos, .posY = yPos, 
				//.posZ = 0.0f, .rotation = 0.0f, 
				.sinRotation = 0.0f, .cosRotation = 1.0f,
				.color = core::getColor(0.5, 0.5, 0.5, 1.0f), .size = size,
			.modelVertexStartIndex = uint32_t(vertices.size()), .modelIndiceCount = AsteroidCorners });
*/
		uint32_t pos = uint32_t((xPos / 2048.0f) * 65535.0f);
		pos += uint32_t((yPos / 2048.0f) * 65535.0f) << 16u;
		uint32_t sincossize = uint32_t((size / 64.0f) * 1023.0f);
		modelInstances.emplace_back(GpuModelInstance{ .pos = pos, 
				.sinCosRotSize = sincossize,
				.color = core::getColor(0.5, 0.5, 0.5, 1.0f),
			.modelVertexStartIndex = uint32_t(vertices.size()) });


		uint32_t startIndex = uint32_t(asteroidTypes << 8u);
		vertices.emplace_back(GpuModelVertex{ .posX = 0.0f, .posY = 0.0f}); //, .posZ = 0.5f, .padding = 0.0f });
		for (uint32_t i = 0; i < AsteroidCorners; ++i)
		{
			float angle = float(i) * float(2.0f * PI) / float(AsteroidCorners);
			float x = cos(angle);
			float y = sin(angle);
			float r = 0.8f + 0.2f * (float(rand()) / float(RAND_MAX));
			vertices.emplace_back(GpuModelVertex{ .posX = x * r, .posY = y *r}); //, .posZ = 0.5f, .padding = 0.0f });
			modelIndices.emplace_back(startIndex);
			modelIndices.emplace_back((i + 1) % AsteroidCorners + startIndex);
			modelIndices.emplace_back((i + 2) % AsteroidCorners + startIndex);
		}

		modelIndices.emplace_back(startIndex);
		modelIndices.emplace_back(AsteroidCorners - 1u + startIndex);
		modelIndices.emplace_back(1u + startIndex);
	}

	{
		float xPos = 200.0f;
		float yPos = 200.0f;
		float size = 10.0f;
		entities.emplace_back(Entity{ .posX = xPos,  .posY = yPos, .posZ = 0.5f, .rotation = 0.0f, .speedX = 0.0f, .speedY = 0.0f, .size = size, .padding = 0.0f });

		uint32_t pos = uint32_t((xPos / 2048.0f) * 65535.0f);
		pos += uint32_t((yPos / 2048.0f) * 65535.0f) << 16u;
		uint32_t sincossize = uint32_t((size / 64.0f) * 1023.0f);

/*
		modelInstances.emplace_back(GpuModelInstance{ .posX = xPos, .posY = yPos, 
			//.posZ = 0.0f, .rotation = 0.0f, 
			.sinRotation = 0.0f, .cosRotation = 0.0f,
			.color = core::getColor(1.0f, 1.0f, 0.0f, 1.0f), .size = size,
			.modelVertexStartIndex = uint32_t(vertices.size()), .modelIndiceCount = 3 });
*/
		modelInstances.emplace_back(GpuModelInstance{ .pos = pos, 
				.sinCosRotSize = sincossize,
				.color = core::getColor(1.0, 1.0, 0.0, 1.0f),
			.modelVertexStartIndex = uint32_t(vertices.size()) });

		vertices.emplace_back(GpuModelVertex{ .posX = -1.0f, .posY = -1.0f}); //, .posZ = 0.5f, .padding = 0.0f });
		vertices.emplace_back(GpuModelVertex{ .posX = 0.0f, .posY = 1.5f});//, .posZ = 0.5f, .padding = 0.0f });
		vertices.emplace_back(GpuModelVertex{ .posX = 1.0f, .posY = -1.0f});//, .posZ = 0.5f, .padding = 0.0f });
		uint32_t startIndex = AsteroidMaxTypes << 8u;
		modelIndices.emplace_back(0 + startIndex);
		modelIndices.emplace_back(1 + startIndex);
		modelIndices.emplace_back(2 + startIndex);
	}

	//GL_TEXTURE_BUFFER
	//ShaderBuffer verticesBuffer(GL_SHADER_STORAGE_BUFFER, uint32_t(vertices.size() * sizeof(GpuModelVertex)), GL_STATIC_DRAW, vertices.data());
	//ShaderBuffer indicesModels(GL_ELEMENT_ARRAY_BUFFER, uint32_t(modelIndices.size() * sizeof(uint32_t)), GL_STATIC_DRAW, modelIndices.data());
	ShaderBuffer instanceDataBuffer(GL_SHADER_STORAGE_BUFFER, uint32_t(modelInstances.size() * sizeof(GpuModelInstance)), GL_DYNAMIC_DRAW, modelInstances.data());


	ShaderBuffer verticesBuffer(GL_SHADER_STORAGE_BUFFER, uint32_t(vertices.size() * sizeof(GpuModelVertex)), 0, vertices.data(), true);
	ShaderBuffer indicesModels(GL_ELEMENT_ARRAY_BUFFER, uint32_t(modelIndices.size() * sizeof(uint32_t)), 0, modelIndices.data(), true);
	//ShaderBuffer instanceDataBuffer(GL_SHADER_STORAGE_BUFFER, uint32_t(modelInstances.size() * sizeof(GpuModelInstance)), 0, modelInstances.data(), true);

	ShaderBuffer ssbo(GL_SHADER_STORAGE_BUFFER, 1024u * 16u, GL_DYNAMIC_COPY, nullptr);
	

	std::vector<uint32_t> quadIndices;
	quadIndices.resize(6 * 1024);
	for(int i = 0; i < 1024; ++i)
	{
		quadIndices[size_t(i) * 6 + 0] = i * 4 + 0;
		quadIndices[size_t(i) * 6 + 1] = i * 4 + 1;
		quadIndices[size_t(i) * 6 + 2] = i * 4 + 2;

		quadIndices[size_t(i) * 6 + 3] = i * 4 + 0;
		quadIndices[size_t(i) * 6 + 4] = i * 4 + 2;
		quadIndices[size_t(i) * 6 + 5] = i * 4 + 3;
	}


	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);

	ShaderBuffer indexBufferQuads(GL_ELEMENT_ARRAY_BUFFER, uint32_t(quadIndices.size() * sizeof(uint32_t)), GL_STATIC_DRAW,	quadIndices.data());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferQuads.handle);


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesModels.handle);
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

	bool quit = false;


	app.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);



	uint32_t queries[2 * 4] = {0, 0};
	glGenQueries(2 * 4, queries);
	for(uint32_t i = 2; i < 8; ++i)
		glQueryCounter(queries[i], GL_TIMESTAMP);
	
	uint32_t queryIndex = 0;
	while (!quit)
	{
		double dt = app.getDeltaTime();
		keyStates = MyKeyStates{};
		#if SDL_USAGE
			handleSDLEvents();
		#else
		#endif

		#if SDL_USAGE
			handleSDLEvents();
		#else
			glfwPollEvents();
			quit = glfwWindowShouldClose(app.window);
		#endif

		if(keyStates.quit)
			quit = true;

		if(quit)
			break;

		if(keyStates.resize)
		{
			app.resizeWindow(keyStates.newWindowWidth, keyStates.newWindowHeight);
			glUniform2f(0, GLfloat(app.windowWidth), GLfloat(app.windowHeight));

		}

		Entity &playerEntity = entities[ AsteroidMaxTypes ];

		Timer updateDurTimer;
		float dtSplit = dt;
		{
			// Update position, definitely not accurate physics, if dt is big this doesn't work properly, trying to split it into several updates.
			while (dtSplit > 0.0f)
			{
				float dddt = fminf(dtSplit, 0.005f);
				float origSpeed = 1.0f * sqrtf(playerEntity.speedX * playerEntity.speedX + playerEntity.speedY * playerEntity.speedY);

				if (keysDown[ 1 ])
				{
					float rotSpeed = 1.0f; // fminf(origSpeed, 1.00f);
					rotSpeed = rotSpeed * 2.0f - ( 1.0f - rotSpeed ) * 0.005;
					playerEntity.rotation += rotSpeed * dddt;
				}
				if (keysDown[ 2 ])
				{
					float rotSpeed = 1.0f; //fminf(origSpeed, 1.0f);
					rotSpeed = rotSpeed * 2.0f - ( 1.0f - rotSpeed ) * 0.005;
					playerEntity.rotation -= rotSpeed * dddt;
				}
				playerEntity.rotation = std::fmod(playerEntity.rotation, PI * 2.0);
				if (keysDown[ 0 ])
				{
					playerEntity.speedX += cosf(playerEntity.rotation + float(PI) * 0.5f) * 5000.0f * dddt;
					playerEntity.speedY += sinf(playerEntity.rotation + float(PI) * 0.5f) * 5000.0f * dddt;
				}




				{
					float origSpeed = sqrtf(playerEntity.speedX * playerEntity.speedX + playerEntity.speedY * playerEntity.speedY);
					float dec = dddt * 0.001f * origSpeed;
					float speed = fmax(origSpeed - dec, 0.0f);
					float slowDown = 0.95f; //origSpeed > 0.01f ? speed / std::max(origSpeed, 1.0f) : 0.0f;
					playerEntity.speedX *= slowDown;
					playerEntity.speedY *= slowDown;

					playerEntity.posX += playerEntity.speedX * dddt;
					playerEntity.posY += playerEntity.speedY * dddt;
				}


				dtSplit -= dddt;
			}

			while(playerEntity.posX > app.windowWidth)
			{
				playerEntity.posX -= app.windowWidth;
			}
			while(playerEntity.posX < 0.0f)
			{
				playerEntity.posX += app.windowWidth;
			}
			while(playerEntity.posY > app.windowHeight)
			{
				playerEntity.posY -= app.windowHeight;
			}
			while(playerEntity.posY < 0.0f)
			{
				playerEntity.posY += app.windowHeight;
			}

			for (uint32_t i = 0; i < modelInstances.size(); ++i)
			{
				uint32_t pos = uint32_t((entities[ i ].posX / 2048.0f) * 65535.0f);
				pos += uint32_t((entities[ i ].posY / 2048.0f) * 65535.0f) << 16u;
				uint32_t sincossize = uint32_t((entities[ i ].size / 64.0f) * 1023.0f);
				float sinv = sinf(entities[ i ].rotation);
				float cosv = cosf(entities[ i ].rotation);
				sincossize += uint32_t((sinv * 0.5f + 0.5f) * 1023.0f) << 10u;
				sincossize += uint32_t((cosv * 0.5f + 0.5f) * 1023.0f) << 20u;


				modelInstances[ i ].pos = pos;
				modelInstances[ i ].sinCosRotSize = sincossize;
				/*
				modelInstances[ i ].posX = entities[ i ].posX;
				modelInstances[ i ].posY = entities[ i ].posY;
				modelInstances[ i ].sinRotation = sinf(entities[ i ].rotation);
				modelInstances[ i ].cosRotation = cosf(entities[ i ].rotation);
				//modelInstances[ i ].posZ = entities[ i ].posZ;
				//modelInstances[ i ].rotation = entities[ i ].rotation;
				*/
			}
		}
		float updateDur = float(updateDurTimer.getDuration());
		//Clear color buffer
		glClear(GL_COLOR_BUFFER_BIT);

		glQueryCounter(queries[queryIndex], GL_TIMESTAMP);
			
		modelShader.useProgram();
		instanceDataBuffer.updateBuffer(0, uint32_t(modelInstances.size() * sizeof(GpuModelInstance)), modelInstances.data());

		
		// "Model rendering"
		{
			modelShader.useProgram();
			glUniform2f(0, GLfloat(app.windowWidth), GLfloat(app.windowHeight));

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesModels.handle);
			verticesBuffer.bind(1);
			instanceDataBuffer.bind(2);
			glDrawElements(GL_TRIANGLES, GLsizei(modelIndices.size()), GL_UNSIGNED_INT, 0);
			verticesBuffer.unbind();
			instanceDataBuffer.unbind();
		}
		// UI

		{
			shaderTexture.useProgram();
			glUniform2f(0, GLfloat(app.windowWidth), GLfloat(app.windowHeight));

			ssbo.updateBuffer(0, uint32_t(vertData.size() * sizeof(GPUVertexData)), vertData.data());
			ssbo.bind(0);
			//		glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferQuads.handle);

			glBindTexture(GL_TEXTURE_2D, texHandle);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


			glDrawElements(GL_TRIANGLES, GLsizei(vertData.size() * 6), GL_UNSIGNED_INT, 0);

			glDisable(GL_BLEND);
			ssbo.unbind();
		}
		glQueryCounter(queries[queryIndex + 1], GL_TIMESTAMP);

		app.present();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		float gpuDuration = 0.0f;
		
		{
			int done = 0;
			uint32_t qlast = (queryIndex + 6) % 8;
			while(!done)
			{
				glGetQueryObjectiv(queries[qlast + 0], GL_QUERY_RESULT_AVAILABLE, &done);
			}
			done = 0;
			while(!done)
			{
				glGetQueryObjectiv(queries[qlast + 1], GL_QUERY_RESULT_AVAILABLE, &done);
			}

			GLuint64 startTime = 0;
			GLuint64 endTime = 0;
			glGetQueryObjectui64v(queries[qlast + 0], GL_QUERY_RESULT, &startTime);
			glGetQueryObjectui64v(queries[qlast + 1], GL_QUERY_RESULT, &endTime);
			
			gpuDuration = float(double(endTime - startTime) / 1000000.0);
	
		}
		
		char str[100];
		static float avg = gpuDuration;
		avg += gpuDuration;
		static int amount = 0;
		static float avgShow = avg;
		if(++amount >= 20)
		{
			avgShow = avg / float(amount);
			amount = 0;
			avg = 0.0f;
		}
		float fps = dt > 0.0 ? float(1.0 / dt) : 0.0f;

		sprintf(str, "%2.2fms, fps: %4.2f, update: %2.3fms, gpu: %2.3fms, gpuavg: %2.3fms", 
			float(dt * 1000.0f), fps, updateDur, gpuDuration, avgShow);
		app.setTitle(str);

		queryIndex = (queryIndex + 2) % 8;

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
			app.setVsyncEnabled(false);
			mainProgramLoop(app, data, filename);
		}
	}
	else
	{
		printf("Failed to load file: %s\n", filename.c_str());
	}
	
	return 0;
}
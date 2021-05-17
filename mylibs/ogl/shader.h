#pragma once

class Shader
{
public:
	~Shader();
	bool initShader(const char *vertShaderFilename, const char *fragShaderFilename);
	void useProgram();
private:
	unsigned int programId = 0u;
};
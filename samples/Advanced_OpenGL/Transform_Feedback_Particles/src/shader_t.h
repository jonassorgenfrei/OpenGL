#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "modules/shader_m.h"

class TransformFeedbackShader : public Shader {
public:
	TransformFeedbackShader(std::vector<const GLchar*> Varyings, const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr, const char* tessellationControlPath = nullptr, const char* tessellatioEvaluationPath = nullptr)
	{
		// 1. retrieve the vertex/fragment/geometry/tessellation source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::string geometryCode;
		std::string tessContCode;
		std::string tessEvalCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		std::ifstream gShaderFile;
		std::ifstream tcShaderFile;
		std::ifstream teShaderFile;
		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		tcShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		teShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			fShaderFile.close();
			// convert stream into string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
			// if geometry shader path is present, also load a geometry shader
			if (geometryPath != nullptr)
			{
				gShaderFile.open(geometryPath);
				std::stringstream gShaderStream;
				gShaderStream << gShaderFile.rdbuf();
				gShaderFile.close();
				geometryCode = gShaderStream.str();
			}
			// if tessellations Control shader path is present also load a tessellation control shader 
			if (tessellationControlPath != nullptr)
			{
				tcShaderFile.open(tessellationControlPath);
				std::stringstream tcShaderStream;
				tcShaderStream << tcShaderFile.rdbuf();
				tcShaderFile.close();
				tessContCode = tcShaderStream.str();
			}
			// if tessellations Control shader path is present also load a tessellation control shader 
			if (tessellatioEvaluationPath != nullptr)
			{
				teShaderFile.open(tessellatioEvaluationPath);
				std::stringstream teShaderStream;
				teShaderStream << teShaderFile.rdbuf();
				teShaderFile.close();
				tessEvalCode = teShaderStream.str();
			}
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();
		// 2. compile shaders
		unsigned int vertex, fragment;
		int success;
		char infoLog[512];
		// vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX");
		// fragment Shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT");
		// tessellations Shader
		unsigned int tessCont, tessEval;
		//Tessellation Control Shader (TCS)
		if (tessellationControlPath != nullptr)
		{
			const char* tcShaderCode = tessContCode.c_str();
			tessCont = glCreateShader(GL_TESS_CONTROL_SHADER);
			glShaderSource(tessCont, 1, &tcShaderCode, NULL);
			glCompileShader(tessCont);
			checkCompileErrors(tessCont, "TESSELLATION CONTROL");
		}
		//Tessellation Evaluation Shader (TES)
		if (tessellatioEvaluationPath != nullptr)
		{
			const char* teShaderCode = tessEvalCode.c_str();
			tessEval = glCreateShader(GL_TESS_EVALUATION_SHADER);
			glShaderSource(tessEval, 1, &teShaderCode, NULL);
			glCompileShader(tessEval);
			checkCompileErrors(tessEval, "TESSELLATION EVALUATION");
		}
		// if geometry shader is given, compile geometry shader
		unsigned int geometry;
		if (geometryPath != nullptr)
		{
			const char* gShaderCode = geometryCode.c_str();
			geometry = glCreateShader(GL_GEOMETRY_SHADER);
			glShaderSource(geometry, 1, &gShaderCode, NULL);
			glCompileShader(geometry);
			checkCompileErrors(geometry, "GEOMETRY");
		}
		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		if (tessellationControlPath != nullptr)
			glAttachShader(ID, tessCont);
		if (tessellatioEvaluationPath != nullptr)
			glAttachShader(ID, tessEval);
		if (geometryPath != nullptr)
			glAttachShader(ID, geometry);

		// specify attributes that go into the buffer, before the programm is linked!
		/* TODO CHECK !!! */
		/**
		 * The last parameter to glTransformFeedbackVaryings() tells
		 * OpenGL either to write all the attributes as a single structure
		 * into a single buffer (GL_INTERLEAVED_ATTRIBS). Or to dedicate
		 * a single buffer for each attribute (GL_SEPARATE_ATTRIBS).
		 * If you use GL_INTERLEAVED_ATTRIBS you can only have a single
		 * transform feedback buffer bound. If you use GL_SEPARATE_ATTRIBS
		 * you will need to bind a different buffer to each slot
		 * (according to the number of attributes)
		 */
		glTransformFeedbackVaryings(ID, 4, Varyings.data(), GL_INTERLEAVED_ATTRIBS);

		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");
		// delete the shaders as they're linked into our program now and no longer necessary
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		if (tessellationControlPath != nullptr)
			glDeleteShader(tessCont);
		if (tessellatioEvaluationPath != nullptr)
			glDeleteShader(tessEval);
		if (geometryPath != nullptr)
			glDeleteShader(geometry);
	}
};

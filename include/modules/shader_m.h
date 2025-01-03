#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;

	Shader() {};

    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------    
	Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr, const char* tessellationControlPath = nullptr, const char* tessellatioEvaluationPath = nullptr)
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

	// constructor for Comput Shader
	// -----------------------------
	Shader(const char* computePath) {
		// 1. retrieve the compute shader source code from filePath
		std::string computeCode;
		std::ifstream cShaderFile;
		// ensure ifstream objects can throw exceptions:
		cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			cShaderFile.open(computePath);
			std::stringstream cShaderStream;
			// read file's buffer contents into streams
			cShaderStream << cShaderFile.rdbuf();
			// close file handlers
			cShaderFile.close();
			// convert stream into string
			computeCode = cShaderStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		const char* cShaderCode = computeCode.c_str();
		// 2. compile shaders
		unsigned int compute;
		//int success;
		//char infoLog[512];

		// compuute shader
		compute = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(compute, 1, &cShaderCode, NULL);
		glCompileShader(compute);
		checkCompileErrors(compute, "COMPUTE");

		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, compute);

		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");

		// delete the shaders as they're linked into our program now and no longer necessary
		glDeleteShader(compute);
	}

	void link() {
		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");
		// delete the shaders as they're linked into our program now and no longer necessary
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		if (tessCont != -1)
			glDeleteShader(tessCont);
		if (tessEval != -1)
			glDeleteShader(tessEval);
		if (geometry != -1)
			glDeleteShader(geometry);
	}

	// activate the shader
    // ------------------------------------------------------------------------
    void use() 
    { 
        glUseProgram(ID); 
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const
    {         
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
    }
	void setBool(const int id, bool value) const
	{
		glUniform1i(id, (int)value);
	}
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const
    { 
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
    }
	void setInt(const int id, int value) const
	{
		glUniform1i(id, value);
	}
	// ------------------------------------------------------------------------
	void setInt2(const std::string& name, int value1, int value2) const
	{
		glUniform2i(glGetUniformLocation(ID, name.c_str()), value1, value2);
	}
	void setInt2(const int id, int value1, int value2) const
	{
		glUniform2i(id, value1, value2);
	}
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const
    { 
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
    }
	void setFloat(const int id, float value) const
	{
		glUniform1f(id, value);
	}
	// ------------------------------------------------------------------------
	void setVec2(const std::string &name, const glm::vec2 &value) const
	{
		glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec2(const int id, const glm::vec2& value) const
	{
		glUniform2fv(id, 1, &value[0]);
	}
	void setVec2(const std::string &name, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
	}
	void setVec2(const int id, float x, float y) const
	{
		glUniform2f(id, x, y);
	}
	// ------------------------------------------------------------------------
	void setVec3(const std::string &name, const glm::vec3 &value) const
	{
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec3(const int id, const glm::vec3& value) const
	{
		glUniform3fv(id, 1, &value[0]);
	}
	void setVec3(const std::string &name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
	}
	void setVec3(const int id, float x, float y, float z) const
	{
		glUniform3f(id, x, y, z);
	}
	// ------------------------------------------------------------------------
	void setVec4(const std::string &name, const glm::vec4 &value) const
	{
		glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec4(const int id, const glm::vec4& value) const
	{
		glUniform4fv(id, 1, &value[0]);
	}
	void setVec4(const std::string &name, float x, float y, float z, float w) const
	{
		glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
	}
	void setVec4(const int id, float x, float y, float z, float w) const
	{
		glUniform4f(id, x, y, z, w);
	}
	// ------------------------------------------------------------------------
	void setMat2(const std::string &name, const glm::mat2 &mat) const
	{
		glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	void setMat2(const int id, const glm::mat2& mat) const
	{
		glUniformMatrix2fv(id, 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat3(const std::string &name, const glm::mat3 &mat) const
	{
		glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	void setMat3(const int id, const glm::mat3 &mat) const
	{
		glUniformMatrix3fv(id, 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat4(const std::string &name, const glm::mat4 &mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	void setMat4(const int id, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(id, 1, GL_FALSE, &mat[0][0]);
	}

	int getLocation(const std::string& name) {
		return glGetUniformLocation(ID, name.c_str());
	}
protected:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }

	/*std::string getexepath()
	{
		char result[MAX_PATH];
		return std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
	} */
	private:
		unsigned int vertex = -1, fragment = -1;
		unsigned int tessCont = -1, tessEval = -1;
		unsigned int geometry = -1;
};
#endif
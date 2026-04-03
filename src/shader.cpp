#include "shader.h"

#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "glad/gl.h"

static int createShader(const GLenum shaderType, const std::stringstream& file) {
    const GLint shaderId = glCreateShader(shaderType);

    const auto str = file.str();

    const auto length = str.length();

    const GLchar* shaders[] = {str.c_str()};

    glShaderSource(shaderId, 1, shaders, reinterpret_cast<const int*>(&length));

    glCompileShader(shaderId);

    GLint completionStatus;

    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &completionStatus);

    if (completionStatus == GL_FALSE) {
        return -1;
    }

    return shaderId;
}

static bool loadFile(const std::string* fileName, std::stringstream& out) {
    const auto c_str = fileName->c_str();
    std::ifstream fileStream(c_str);

    if (fileStream.is_open()) {
        out.clear();

        out.str("");

        out << fileStream.rdbuf();

        fileStream.close();

        return true;
    }

    std::cout << "Could not open file '" << *fileName << "'." << std::endl;
    return false;
}

Shader::Shader(std::string&& vertPath, std::string&& fragPath) : vertPath(std::move(vertPath)), fragPath(std::move(fragPath)) {}

void Shader::init() {
    std::stringstream file;

    if (!loadFile(&vertPath, file)) {
        throw std::runtime_error("");
    }

    vertexID = createShader(GL_VERTEX_SHADER, file);

    if (vertexID < 0) {
        throw std::runtime_error("Failed to compile the vertex shader.");
    }

    if (!loadFile(&fragPath, file)) {
        throw std::runtime_error("Couldn't open the background_frag.glsl file");
    }

    fragmentID = createShader(GL_FRAGMENT_SHADER, file);

    if (fragmentID < 0) {
        throw std::runtime_error("Failed to compile the fragment shader.");
    }

    shaderProgramID = glCreateProgram();

    glAttachShader(shaderProgramID, vertexID);
    glAttachShader(shaderProgramID, fragmentID);
    glLinkProgram(shaderProgramID);

    GLint linkStatus;

    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &linkStatus);

    int infoLogLength;

    GLchar infoLog[1024];

    if (linkStatus == GL_FALSE) {
        glGetShaderiv(shaderProgramID, GL_INFO_LOG_LENGTH, &infoLogLength);

        if (infoLogLength > 0 && infoLogLength < 1024) {
            glGetShaderInfoLog(shaderProgramID, infoLogLength, &infoLogLength, infoLog);

            std::cout << "Shader Info Log:" << std::endl;
            std::cout << infoLog << std::endl;
        }

        throw std::runtime_error("Failed to link the shaders to the program id.");
    }

    glGetProgramiv(shaderProgramID, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (infoLogLength > 0 && infoLogLength < 1024) {
        glGetProgramInfoLog(shaderProgramID, infoLogLength, &infoLogLength, infoLog);

        std::cout << "Shader PROGRAM Info Log:" << std::endl;
        std::cout << infoLog << std::endl;
    }
}

void Shader::use() {
    if (shaderProgramID != -1) {
        glUseProgram(shaderProgramID);
    } else {
        printf("Attempted to use an uninitialized Shader...\n");
    }
}

void Shader::detach() {
    glUseProgram(0);
}

void Shader::destroy() {
    glDeleteShader(vertexID);
    glDeleteShader(fragmentID);
    glDeleteProgram(shaderProgramID);

    vertexID = -1;
    fragmentID = -1;
    shaderProgramID = -1;
}

GLint Shader::getAttribLocation(const GLchar* name) const {
    return glGetAttribLocation(shaderProgramID, name);
}

GLint Shader::getUniformLocation(const GLchar* name) const {
    return glGetUniformLocation(shaderProgramID, name);
}

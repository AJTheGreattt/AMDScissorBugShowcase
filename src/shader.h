#pragma once

#ifndef SHADER_H
#define SHADER_H

#include <string>

#ifdef __cplusplus
    extern "C" {
#endif

#include "gl_executor.h"
#include "glad/gl.h"

class Shader final : GLExecutor {
    int shaderProgramID = -1, vertexID = -1, fragmentID = -1;
    std::string vertPath, fragPath;

public:
    Shader(std::string&&, std::string&&);

    ~Shader() override = default;

    void init() override;

    void use() override;

    void detach() override;

    void destroy() override;

    GLint getAttribLocation(const GLchar*) const;

    GLint getUniformLocation(const GLchar *) const;
};

#ifdef __cplusplus
        }
#endif

#endif //SHADER_H
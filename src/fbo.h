//
// Created by AJ The Greattt on 3/6/2026.
//
#ifndef AMDSCISSORBUGDEMO_FBO_H
#define AMDSCISSORBUGDEMO_FBO_H
#include "glad/gl.h"

#ifdef __cplusplus
    extern "C" {
#endif

enum FBOTarget : GLenum {
    FRAMEBUFFER = GL_FRAMEBUFFER,
    READ_FRAMEBUFFER = GL_READ_FRAMEBUFFER,
    DRAW_FRAMEBUFFER = GL_DRAW_FRAMEBUFFER
};

class FBOHandler {
    const GLuint textureUnit, unitOrdinal;
    GLuint name = -1, textureName = -1;

public:
    explicit FBOHandler(GLuint textureUnit, GLuint unitOrdinal);

    void use() const;

    void use(FBOTarget target) const;

    void detach() const;

    void detach(FBOTarget target) const;

    void destroy();

    void adjustToSize(GLsizei width, GLsizei height);

    [[nodiscard]]
    GLuint getUnitOrdinal() const {
        return unitOrdinal;
    }
};


#ifdef __cplusplus
    }
#endif

#endif //AMDSCISSORBUGDEMO_FBO_H
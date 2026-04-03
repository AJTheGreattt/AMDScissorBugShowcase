//
// Created by AJ The Greattt on 3/6/2026.
//
#include "fbo.h"

#include <stdexcept>
#include <format>

FBOHandler::FBOHandler(const GLuint textureUnit, GLuint unitOrdinal) : textureUnit(textureUnit), unitOrdinal(unitOrdinal) {
}

void FBOHandler::use() const {
    use(FRAMEBUFFER);
}

void FBOHandler::use(const FBOTarget target) const {
    if (name != -1) {
        glBindFramebuffer(target, name);
    }
}

void FBOHandler::detach() const {
    detach(FRAMEBUFFER);
}

void FBOHandler::detach(const FBOTarget target) const {
    glBindFramebuffer(target, 0);
}

void FBOHandler::destroy() {
    if (name != -1) {
        glDeleteFramebuffers(1, &name);
        name = -1;
    }
    if (textureName != -1) {
        glDeleteTextures(1, &textureName);
        textureName = -1;
    }
}

void FBOHandler::adjustToSize(const GLsizei width, const GLsizei height) {
    destroy();

    glGenFramebuffers(1, &name);

    use();

    glGenTextures(1, &textureName);

    glActiveTexture(textureUnit);

    glBindTexture(GL_TEXTURE_2D, textureName);

    glTexImage2D(GL_TEXTURE_2D,
                0,
                GL_RGBA8,
                width,
                height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                textureName,
                0);

    if (int status; (status = glCheckFramebufferStatus(FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error(std::format("This FBO could not be completed!\nSTATUS CODE 0x{}", status));
    }

    detach();
}
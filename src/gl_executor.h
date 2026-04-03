#pragma once

#ifndef AJ_SCREENSAVER_GL_EXECUTOR_H
#define AJ_SCREENSAVER_GL_EXECUTOR_H

#ifdef __cplusplus
    extern "C" {
#endif

class GLExecutor {
public:
    GLExecutor() = default;

    virtual ~GLExecutor() = default;

    virtual void init() = 0;

    virtual void use() = 0;

    virtual void detach() = 0;

    virtual void destroy() = 0;
};

#ifdef __cplusplus
        }
#endif

#endif //AJ_SCREENSAVER_GL_EXECUTOR_H
#include <cstdio>

#include "fbo.h"
#include "shader.h"
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"
#include "cstring"

static void errorCallback(const int error_code, const char* description) {
    printf("[GLFW_ERROR]: %d, %s", error_code, description);
}

static void keyCallback(GLFWwindow* const window, const int key, int, int, int) {
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

struct WindowDimensions {
    int width;
    int height;
};

static WindowDimensions windowDimensions = { 0 , 0 };

#define IS_AMD_GPU strstr(reinterpret_cast<const char*>(glGetString(GL_RENDERER)), "AMD") ||\
    strcmp(reinterpret_cast<const char*>(glGetString(GL_VENDOR)), "ATI Technologies Inc.") == 0

/*
 This value determines if the GL_SCISSOR_TEST area is readjusted when blitting back to the default FBO,
 which is required for the desired effect to show on an AMD GPU. When set to true on an NVIDIA GPU,
 this causes the desired effect to not be shown.
 */
#define USE_AMD_FIX true

/*
 This value is passed to a glfwWindowHint(GLFW_SAMPLES, ...) call.
 You can replace 4 with any other value for testing.

 When using a SAMPLE_COUNT (strictly) > 0, if USE_AMD_FIX is set to false,
 the desired effect will not be shown (hence this submission of a bug report).
 */
#define SAMPLE_COUNT 4

static FBOHandler* FX_FBO_A = nullptr;
static FBOHandler* FX_FBO_B = nullptr;

static void framebufferCallback(GLFWwindow*, const int width, const int height) {
    glViewport(0, 0, width, height);

    windowDimensions.width = width;
    windowDimensions.height = height;

    if (FX_FBO_A && FX_FBO_B) {
        FX_FBO_A->adjustToSize(width, height);
        FX_FBO_B->adjustToSize(width, height);
    }
}

static int setupGLFW(GLFWmonitor **outPrimaryMonitor,
                     GLFWwindow **outWindow,
                     const GLFWvidmode** outVideoMode) {
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    GLFWmonitor * const primaryMonitor = glfwGetPrimaryMonitor();

    if (!primaryMonitor) {
        printf("Failed to retrieve the primary monitor.");
        glfwTerminate();
        return -1;
    }

    *outPrimaryMonitor = primaryMonitor;

    glfwSetErrorCallback(errorCallback);

    glfwDefaultWindowHints();

    glfwWindowHint(GLFW_SAMPLES,  SAMPLE_COUNT);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

    *outVideoMode = videoMode;

    const int width = videoMode->width / 4;
    const int height = videoMode->height / 4;

    GLFWwindow* window = glfwCreateWindow(width,
                                          height,
                                          "AMDScissorBugDemo",
                                          nullptr,
                                          nullptr);

    windowDimensions.width = width;
    windowDimensions.height = height;

    if (!window) {
        printf("Failed to create the GLFW window.");
        glfwTerminate();
        return -1;
    }

    *outWindow = window;

    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebufferCallback);

    glfwMakeContextCurrent(window);

    return 0;
}

static int setupGL(const GLFWvidmode* const videoMode, GLuint* vaoName,  GLuint* vboName) {
    int glVersion = 0;

    if (!((glVersion = gladLoadGL(glfwGetProcAddress)))) {
        printf("Failed to initialize the GL context through GLAD.");
        return -1;
    }

    printf("Working with GL Version: %d.%d\n", GLAD_VERSION_MAJOR(glVersion), GLAD_VERSION_MINOR(glVersion));

    glViewport(0, 0, videoMode->width / 4, videoMode->height / 4);

    GLuint vaoId;

    glGenVertexArrays(1, &vaoId);

    glBindVertexArray(vaoId);

    GLuint vboId;

    glGenBuffers(1, &vboId);

    glBindBuffer(GL_ARRAY_BUFFER, vboId);

    // Define the viewport dimensions

    constexpr float data[] = {
        //TL, BL, BR
        //NDC         UV Coords
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        //BR, TR, TL
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

    glVertexAttribPointer(0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        nullptr);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        reinterpret_cast<GLvoid*>(sizeof(float) * 2));

    glEnableVertexAttribArray(1);

    glClearColor(0., 0., 0., 0.);

    *vaoName = vaoId;
    *vboName = vboId;

    return 0;
}

static int getMainImage(GLuint* textureName) {
    int imageWidth, imageHeight, channels;

    const auto imageFile  = fopen("../an_image.png", "rb");

    if (!imageFile) {
        printf("Failed to load the test image.");
        return -1;
    }

    stbi_set_flip_vertically_on_load(true);

    auto* const imagePixels = stbi_load_from_file(imageFile, &imageWidth, &imageHeight, &channels, 4);

    glGenTextures(1, textureName);

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, *textureName);

    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        imageWidth,
        imageHeight,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        imagePixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    fclose(imageFile);

    stbi_image_free(imagePixels);

    return 0;
}

//Simulates a ping pong FBO system,
//but since this is a simple chromatic aberration effect...
static void pingPongFX(Shader& effectShader) {
    //We will draw the affected image to FX FBO A,
    //which means we need the unaffected image on FX FBO B.
    FX_FBO_B->use(DRAW_FRAMEBUFFER);

    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(READ_FRAMEBUFFER, 0);

    const int halfHeight = windowDimensions.height / 2;
    const int halfWidth = windowDimensions.width / 2;

    //Enable scissoring for the blit operation to FX FBO A.

    glEnable(GL_SCISSOR_TEST);

    glScissor(0, halfHeight, halfWidth, halfHeight);

    glBlitFramebuffer(0, halfHeight, halfWidth, windowDimensions.height,
                      0, halfHeight, halfWidth, windowDimensions.height,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    FX_FBO_A->use(DRAW_FRAMEBUFFER);

    glClear(GL_COLOR_BUFFER_BIT);

    effectShader.use();

    glUniform1i(
        effectShader.getUniformLocation("uTexture"),
        static_cast<GLint>(FX_FBO_B->getUnitOrdinal()));

    glDrawArrays(GL_TRIANGLES, 0, 6);

    FX_FBO_A->use(READ_FRAMEBUFFER);

    FX_FBO_A->detach(DRAW_FRAMEBUFFER);

    if (USE_AMD_FIX) {
        glScissor(0,
                0,
                halfWidth,
                halfHeight);
    }

    glBlitFramebuffer(0, halfHeight, halfWidth, windowDimensions.height,
                      0, halfHeight, halfWidth, windowDimensions.height,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glDisable(GL_SCISSOR_TEST);

    FX_FBO_A->detach(READ_FRAMEBUFFER);
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);

    GLFWmonitor* primaryMonitor;
    GLFWwindow* window;
    const GLFWvidmode* videoMode;

    if (setupGLFW(&primaryMonitor, &window, &videoMode) < 0) {
        return -1;
    }

    GLuint vaoName, vboName;

    if (setupGL(videoMode, &vaoName, &vboName) < 0) {
        glfwTerminate();
        return -1;
    }

    GLuint mainImageTextureName;

    if (getMainImage(&mainImageTextureName)) {
        glfwTerminate();
        return -1;
    }

    //This shader draws an_image.png to the screen.
    Shader mainDrawShader("..\\shaders\\generic_texture_vert.glsl", "..\\shaders\\generic_texture_frag.glsl");

    mainDrawShader.init();

    glUniform1i(mainDrawShader.getUniformLocation("uTexture"), 0);

    Shader chromaticAbShader("..\\shaders\\generic_texture_vert.glsl", "..\\shaders\\invert_color.glsl");

    chromaticAbShader.init();

    FX_FBO_A = new FBOHandler(GL_TEXTURE1, 1);

    FX_FBO_A->adjustToSize(videoMode->width / 4, videoMode->height / 4);

    FX_FBO_B = new FBOHandler(GL_TEXTURE2, 2);

    FX_FBO_B->adjustToSize(videoMode->width / 4, videoMode->height / 4);

    glfwSetWindowAspectRatio(window, 16, 9);
    glfwShowWindow(window);
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        //Draws the blue background.

        glBindFramebuffer(FRAMEBUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT);

        mainDrawShader.use();

        glDrawArrays(GL_TRIANGLES, 0, 6);

        pingPongFX(chromaticAbShader);

        glfwSwapBuffers(window);
    }

    mainDrawShader.destroy();

    glDeleteBuffers(1, &vboName);
    glDeleteVertexArrays(1, &vaoName);

    FX_FBO_A->destroy();
    delete FX_FBO_A;
    FX_FBO_A = nullptr;

    FX_FBO_B->destroy();
    delete FX_FBO_B;
    FX_FBO_B = nullptr;

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

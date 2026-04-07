import com.ajthegreattt.renderdoc4j.backbone.RenderDocAPI;
import org.lwjgl.BufferUtils;
import org.lwjgl.glfw.*;
import org.lwjgl.opengl.*;
import org.lwjgl.system.*;

import java.io.IOException;
import java.nio.*;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Objects;
import java.util.function.Supplier;

import static org.lwjgl.glfw.GLFW.*;
import static org.lwjgl.opengl.GL11.*;
import static org.lwjgl.opengl.GL13.*;
import static org.lwjgl.opengl.GL15.*;
import static org.lwjgl.opengl.GL20.*;
import static org.lwjgl.opengl.GL30.*;
import static org.lwjgl.stb.STBImage.*;
import static org.lwjgl.system.MemoryUtil.*;

public class Main {
    private static class WindowDimensions {
        public int width, height;
    }

    private static final WindowDimensions windowDimensions = new WindowDimensions();

    @SuppressWarnings("unused")
    private static final Supplier<Boolean> IS_AMD_GPU = new Supplier<>() {

        Boolean cachedValue = null;

        @Override
        public Boolean get() {
            return Objects.requireNonNullElseGet(this.cachedValue,
                    () -> this.cachedValue = Objects.requireNonNullElse(glGetString(GL_RENDERER), "")
                            .toLowerCase().contains("amd") ||
                            Objects.requireNonNullElse(glGetString(GL_VENDOR), "")
                                    .equals("ATI Technologies Inc."));
        }
    };

    /**
     * This value determines if the {@link GL11#GL_SCISSOR_TEST} area is readjusted when blitting back to the default FBO,
     * which is required for the desired effect to show on an AMD GPU. When set to true on an NVIDIA GPU,
     * this causes the desired effect to not be shown.
     */
    private static final Supplier<Boolean> USE_AMD_FIX = () -> Boolean.TRUE /*IS_AMD_GPU.get()*/;

    /**
     * This value is passed to a {@code glfwWindowHint(GLFW_SAMPLES, ...)} call.
     * You can replace {@code 4} with any other value for testing.
     *
     * <p>When using a {@code SAMPLE_COUNT} (strictly) > {@code 0}, if {@link #USE_AMD_FIX} is set to {@code false},
     * the desired effect will not be shown (hence this submission of a bug report).</p>
     */
    private static final int SAMPLE_COUNT = 4;

    private static FBOHandler FX_FBO_A;
    private static FBOHandler FX_FBO_B;

    private static boolean setupGLFW(long[] outPrimaryMonitor, long[] outWindow, GLFWVidMode[] outVideoMode) {
        if (!glfwInit()) {
            System.err.println("Failed to initialize GLFW");
            return false;
        }

        long primaryMonitor = glfwGetPrimaryMonitor();

        if (primaryMonitor == NULL) {
            System.err.println("Failed to retrieve the primary monitor.");
            glfwTerminate();
            return false;
        }

        outPrimaryMonitor[0] = primaryMonitor;

        glfwSetErrorCallback(GLFWErrorCallback.createPrint(System.err));

        glfwDefaultWindowHints();

        glfwWindowHint(GLFW_SAMPLES,  SAMPLE_COUNT);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

        GLFWVidMode videoMode = glfwGetVideoMode(primaryMonitor);

        if (videoMode == null) {
            System.err.println("Failed to get video mode.");
            glfwTerminate();
            return false;
        }

        outVideoMode[0] = videoMode;

        int windowWidth = videoMode.width() / 2;
        int windowHeight = videoMode.height() / 2;

        long window = glfwCreateWindow(windowWidth,
                windowHeight,
                "AMDScissorBugDemo",
                NULL,
                NULL);

        windowDimensions.width = windowWidth;
        windowDimensions.height = windowHeight;

        if (window == NULL) {
            System.err.println("Failed to create the GLFW window.");
            glfwTerminate();
            return false;
        }

        outWindow[0] = window;

        glfwSetKeyCallback(window, (_, key, _, _, _) -> {
            if (key == GLFW_KEY_ESCAPE) {
                glfwSetWindowShouldClose(window, true);
            }
        });

        glfwSetFramebufferSizeCallback(window, (_, w, h) -> {
            glViewport(0, 0, w, h);

            windowDimensions.width = w;
            windowDimensions.height = h;

            if (FX_FBO_A != null && FX_FBO_B != null) {
                FX_FBO_A.adjustToSize(w, h);
                FX_FBO_B.adjustToSize(w, h);
            }
        });

        glfwMakeContextCurrent(window);

        return true;
    }

    static boolean setupGL(GLFWVidMode videoMode, int[] vaoName, int[] vboName) {
        GL.createCapabilities();

        System.out.printf("Working with GL Version: %s\n", glGetString(GL_VERSION));

        glViewport(0, 0, videoMode.width() / 2, videoMode.height() / 2);

        int vaoId = glGenVertexArrays();
        glBindVertexArray(vaoId);

        int vboId = glGenBuffers();
        glBindBuffer(GL_ARRAY_BUFFER, vboId);

        float[] data = {
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

        try (MemoryStack stack = MemoryStack.stackPush()) {
            FloatBuffer dataBuffer = stack.mallocFloat(data.length);
            dataBuffer.put(data).flip();
            glBufferData(GL_ARRAY_BUFFER, dataBuffer, GL_STATIC_DRAW);
        }

        glVertexAttribPointer(0, 2, GL_FLOAT, false, Float.BYTES * 4, 0L);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, false, Float.BYTES * 4, Float.BYTES * 2);
        glEnableVertexAttribArray(1);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        vaoName[0] = vaoId;
        vboName[0] = vboId;

        return true;
    }

    static boolean getMainImage(int[] textureName) {
        String imagePath = "an_image.png";

        try (MemoryStack stack = MemoryStack.stackPush()) {
            IntBuffer w = stack.mallocInt(1);
            IntBuffer h = stack.mallocInt(1);
            IntBuffer channels = stack.mallocInt(1);

            stbi_set_flip_vertically_on_load(true);

            final byte[] imageBytes = Files.readAllBytes(Paths.get(imagePath));

            final ByteBuffer imageDirectBuffer = BufferUtils.createByteBuffer(imageBytes.length).put(imageBytes).flip();

            ByteBuffer imagePixels = stbi_load_from_memory(imageDirectBuffer, w, h, channels, 4);

            if (imagePixels == null) {
                System.err.println("Failed to load the test image: " + stbi_failure_reason());
                return false;
            }

            int texName = glGenTextures();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texName);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w.get(0), h.get(0), 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePixels);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            stbi_image_free(imagePixels);
            textureName[0] = texName;
        } catch (IOException e) {
            throw new RuntimeException("Failed to the load the test image file.", e);
        }

        return true;
    }

    private static void pingPongFX(Shader effectShader) {
        //We will draw the affected image to FX FBO A,
        //which means we need the unaffected image on FX FBO B.
        FX_FBO_B.use(FBOHandler.Target.DRAW_FRAMEBUFFER);

        glClear(GL_COLOR_BUFFER_BIT);

        glBindFramebuffer(GL30.GL_READ_FRAMEBUFFER, 0);

        final int halfHeight = windowDimensions.height / 2;
        final int halfWidth = windowDimensions.width / 2;

        //Enable scissoring for the blit operation to FX FBO A.

        glEnable(GL_SCISSOR_TEST);

        glScissor(0, halfHeight, halfWidth, halfHeight);

        glBlitFramebuffer(0, halfHeight, halfWidth, windowDimensions.height,
                0, halfHeight, halfWidth, windowDimensions.height,
                GL_COLOR_BUFFER_BIT, GL_LINEAR);

        FX_FBO_A.use(FBOHandler.Target.DRAW_FRAMEBUFFER);

        glClear(GL_COLOR_BUFFER_BIT);

        effectShader.use();

        glUniform1i(
                effectShader.getUniformLocation("uTexture"),
                FX_FBO_B.getUnitOrdinal());

        glDrawArrays(GL_TRIANGLES, 0, 6);

        FX_FBO_A.use(FBOHandler.Target.READ_FRAMEBUFFER);

        FX_FBO_A.detach(FBOHandler.Target.DRAW_FRAMEBUFFER);

        if (USE_AMD_FIX.get()) {
            glScissor(0,
                    0,
                    halfWidth,
                    halfHeight);
        }

        glBlitFramebuffer(0, halfHeight, halfWidth, windowDimensions.height,
                0, halfHeight, halfWidth, windowDimensions.height,
                GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glDisable(GL_SCISSOR_TEST);

        FX_FBO_A.detach(FBOHandler.Target.READ_FRAMEBUFFER);
    }

    public static void main(String[] args) {
        try {
            RenderDocAPI.getInstance();
        } catch (Throwable ignored) {}

        long[] primaryMonitor = new long[1];
        long[] window = new long[1];
        GLFWVidMode[] videoModeArr = new GLFWVidMode[1];

        if (!setupGLFW(primaryMonitor, window, videoModeArr)) {
            return;
        }

        GLFWVidMode videoMode = videoModeArr[0];

        int[] vaoName = new int[1];
        int[] vboName = new int[1];

        if (!setupGL(videoMode, vaoName, vboName)) {
            glfwTerminate();
            return;
        }

        int[] mainImageTextureName = new int[1];

        if (!getMainImage(mainImageTextureName)) {
            glfwTerminate();
            return;
        }

        // This shader draws an_image.png to the screen.
        Shader mainDrawShader = new Shader("shaders\\generic_texture_vert.glsl", "shaders\\generic_texture_frag.glsl");
        mainDrawShader.init();
        glUniform1i(mainDrawShader.getUniformLocation("uTexture"), 0);

        Shader chromaticAbShader = new Shader("shaders\\generic_texture_vert.glsl", "shaders\\invert_color.glsl");
        chromaticAbShader.init();

        FX_FBO_A = new FBOHandler(GL13.GL_TEXTURE1, 1);
        final int fboWidth = videoMode.width() / 2;

        final int fboHeight = videoMode.height() / 2;

        FX_FBO_A.adjustToSize(fboWidth, fboHeight);

        FX_FBO_B = new FBOHandler(GL13.GL_TEXTURE2, 2);
        FX_FBO_B.adjustToSize(fboWidth, fboHeight);

        glfwSetWindowAspectRatio(window[0], 16, 9);
        glfwShowWindow(window[0]);
        glfwSwapInterval(1);

        while (!glfwWindowShouldClose(window[0])) {
            glfwPollEvents();

            // Draws the blue background.
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            mainDrawShader.use();
            glDrawArrays(GL_TRIANGLES, 0, 6);

            pingPongFX(chromaticAbShader);

            glfwSwapBuffers(window[0]);
        }

        mainDrawShader.destroy();
        chromaticAbShader.destroy();

        glDeleteBuffers(vboName[0]);
        glDeleteVertexArrays(vaoName[0]);

        FX_FBO_A.destroy();
        FX_FBO_A = null;

        FX_FBO_B.destroy();
        FX_FBO_B = null;

        glfwDestroyWindow(window[0]);
        glfwTerminate();
    }
}

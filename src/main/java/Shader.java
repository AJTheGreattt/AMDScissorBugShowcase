import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

import static org.lwjgl.opengl.GL20.*;

public class Shader {
    public int shaderProgramID = -1, vertexID = -1, fragmentID = -1;

    public String vertPath, fragPath;

    public Shader(String vertexPath, String fragPath) {
        this.vertPath = vertexPath;
        this.fragPath = fragPath;
    }

    private static String loadFile(String path) {
        try {
            return new String(Files.readAllBytes(Paths.get(path)));
        } catch (IOException e) {
            System.err.println("Failed to load file: " + path);
            return null;
        }
    }

    private int createShader(int type, String source) {
        int shaderId = glCreateShader(type);

        glShaderSource(shaderId, source);
        glCompileShader(shaderId);

        int status = glGetShaderi(shaderId, GL_COMPILE_STATUS);
        if (status == GL_FALSE) {
            String infoLog = glGetShaderInfoLog(shaderId);
            System.err.println("Shader compilation failed (" + (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") + "): " + infoLog);
            glDeleteShader(shaderId);
            return -1;
        }

        return shaderId;
    }

    public void init() {
        String vertSource = loadFile(vertPath);

        if (vertSource == null) {
            throw new RuntimeException("Failed to load vertex shader source.");
        }

        vertexID = createShader(GL_VERTEX_SHADER, vertSource);
        if (vertexID < 0) {
            throw new RuntimeException("Failed to compile the vertex shader.");
        }

        String fragSource = loadFile(fragPath);
        if (fragSource == null) {
            throw new RuntimeException("Failed to load fragment shader source.");
        }

        fragmentID = createShader(GL_FRAGMENT_SHADER, fragSource);
        if (fragmentID < 0) {
            throw new RuntimeException("Failed to compile the fragment shader.");
        }

        shaderProgramID = glCreateProgram();

        glAttachShader(shaderProgramID, vertexID);
        glAttachShader(shaderProgramID, fragmentID);
        glLinkProgram(shaderProgramID);

        int linkStatus = glGetProgrami(shaderProgramID, GL_LINK_STATUS);

        if (linkStatus == GL_FALSE) {
            String infoLog = glGetProgramInfoLog(shaderProgramID);
            System.err.println("Shader Info Log:\n" + infoLog);
            throw new RuntimeException("Failed to link the shaders to the program id.");
        }

        String programInfoLog = glGetProgramInfoLog(shaderProgramID);
        if (!programInfoLog.isEmpty()) {
            System.out.println("Shader PROGRAM Info Log:\n" + programInfoLog);
        }
    }

    public int getUniformLocation(String uniformName) {
        return glGetUniformLocation(shaderProgramID, uniformName);
    }

    public void use() {
        if (shaderProgramID != -1) {
            glUseProgram(shaderProgramID);
        } else {
            System.out.println("Attempted to use an uninitialized Shader...");
        }
    }

    public void destroy() {
        glDeleteShader(vertexID);
        glDeleteShader(fragmentID);
        glDeleteProgram(shaderProgramID);

        vertexID = -1;
        fragmentID = -1;
        shaderProgramID = -1;
    }
}

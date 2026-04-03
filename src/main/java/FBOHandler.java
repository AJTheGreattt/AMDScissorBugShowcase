import org.lwjgl.opengl.GL30;

import java.nio.ByteBuffer;

import static org.lwjgl.opengl.GL11.glDeleteTextures;
import static org.lwjgl.opengl.GL30.*;

public class FBOHandler {
    private final int textureUnit;
    private final int unitOrdinal;

    public FBOHandler(int textureUnit, int unitOrdinal) {
        this.textureUnit = textureUnit;
        this.unitOrdinal = unitOrdinal;
    }

    public int getUnitOrdinal() {
        return this.unitOrdinal;
    }

    public void adjustToSize(int width, int height) {
        destroy();

        name = glGenFramebuffers();

        use(Target.FRAMEBUFFER);

        textureName = glGenTextures();

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
                (ByteBuffer) null);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(Target.FRAMEBUFFER.target,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                textureName,
                0);

        int status;
        if ((status = glCheckFramebufferStatus(Target.FRAMEBUFFER.target)) != GL_FRAMEBUFFER_COMPLETE) {
            throw new RuntimeException(String.format("This FBO could not be completed!\nSTATUS CODE 0x%d", status));
        }

        detach(Target.FRAMEBUFFER);
    }

    public int name = -1, textureName = -1;

    public void use(Target target) {
        if (name != -1) {
            glBindFramebuffer(target.target, name);
        }
    }

    public void detach(Target target) {
        glBindFramebuffer(target.target, 0);
    }

    public void destroy() {
        if (name != -1) {
            glDeleteFramebuffers(name);
            name = -1;
        }
        if (textureName != -1) {
            glDeleteTextures(textureName);
            textureName = -1;
        }
    }

    public enum Target {
        FRAMEBUFFER(GL30.GL_FRAMEBUFFER),
        READ_FRAMEBUFFER(GL30.GL_READ_FRAMEBUFFER),
        DRAW_FRAMEBUFFER(GL30.GL_DRAW_FRAMEBUFFER);

        public final int target;

        Target(int target) {
            this.target = target;
        }
    }
}

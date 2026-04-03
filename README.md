# Overview
A repo with 2 different language replications of an AMD `glScissor` interpretation bug.

This bug has been submitted to AMD via the AMD Developer Community Discord Server,
and it has been granted the tag `SWDEV-580334` for reference to the internal ticket for this report.

The Java implementation can be found on the [`java`](https://github.com/AJTheGreattt/AMDScissorBugShowcase/tree/java) branch. It includes a Gradle build script with the necessary dependencies,
as well as an additional dependency on `RenderDoc4J` for quality of life. The Main class is already configured to inject
RenderDoc into the program at load time. No other configuration is necessary. 

The C++ implementation can be found on the [`cpp`](https://github.com/AJTheGreattt/AMDScissorBugShowcase/tree/cpp) branch. It includes a [CMakeLists.txt](/cpp/CMakeLists.txt) file for linking
the necessary dependencies, as well as the necessary binaries and source files. It does not contain any RenderDoc linking code.

## What is the bug?

Before explaining the bug, let's go over the prerequisites:

- You must be using [`GLFW`](https://github.com/glfw/glfw) on an AMD GPU.
- You must create a window with the hint `glfwWindowHint(GLFW_SAMPLES, SAMPLE_COUNT)`, where `SAMPLE_COUNT` is any number greater
  than 0 (as `GLFW_DONT_CARE` is -1, and it looks like any number less than 0 has no effect).
- You must be blitting to the default FBO of the window, and must have `GL_SCISSOR_TEST` enabled.

This bug **DOES NOT** occur when simply `glDraw*`ing with `GL_SCISSOR_TEST` enabled, and only happens with blitting. 

> PC Details\
> GPU: 7900XTX\
> Other PC's GPU (Explained Later): 3060-Ti\
> CPU (Although it's likely irrelevant): 9950X3D\
> Other PC's CPU: 3900X\
> GPU Driver Version: 25.30.17.01-260108a-197640C-AMD-Software-Adrenalin-Edition\
> AMD Windows Driver Version when found: 32.0.23017.1001\
> OpenGL® Driver Version when found: 32.0.23017.1001

Below are two screenshots showing what's happening. The screenshot with a window titled `AMD_REPORT` is showing what's being drawn by a 7900XTX.
The screenshot with a window titled `NVIDIA_REPORT` is showing what's being rendered by a 3060-Ti. Both are rendering to the default FBO.

To showcase the issue, I am using an example that applies a blur to what should be the top-right quadrant of the window. The operations are in the following order,
where `FBO 0` is the default FBO/backbuffer of the window, `FX FBO` is the FBO(s) used for applying the effects, and AoE is Area of Effect, all for conciseness. 

1. Scissor `FX FBO` AoE (this `glScissor` call interprets the scissor area properly, further explained at the bottom)
2. Blit AoE from `FBO 0` to `FX FBO`
3. Apply Two-Pass Blur Shader
4. Scissor `FBO 0` AoE (this `glScissor` call is interpreted improperly)
5. Blit AoE `FX FBO` to `FBO 0`

<img width="1800" height="534" alt="image" src="https://github.com/user-attachments/assets/1f0bfd93-39bd-4374-a691-47325c51e269" />

It's important to note that in order for this scissor to work properly on the AMD rendered window, I am inverting the Y-coordinate by adding the height of the desired
area to what would be the proper OpenGL Y-coordinate. Without this inversion, the NVIDIA-rendered window would show the effect properly in the top-right quadrant,
and the AMD-rendered window would be missing the effect (the opposite of what's happening now). Again, both PCs are running the same program, with the only code 
changes being the window's title.

Also, please note that I'm specifying the issue only occurs when scissoring `FBO 0`, and does not happen when blitting (with `GL_SCISSOR_TEST` enabled) to surfaces created by the GPU,
e.g., both of the ping-pong `FX FBO`'s textures and FBO objects.

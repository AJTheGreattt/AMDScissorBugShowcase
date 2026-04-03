plugins {
    java
}

group = ""
version = "1.0.0"

repositories {
    mavenCentral()
}

val lwjglNatives = "natives-windows"
val lwjglVersion = "3.4.1"

project.java.sourceCompatibility = JavaVersion.VERSION_24

dependencies {
    implementation("com.ajthegreattt:renderdoc4j:2.0.1")

    implementation(platform("org.lwjgl:lwjgl-bom:$lwjglVersion"))

    implementation("org.lwjgl:lwjgl")
    implementation("org.lwjgl:lwjgl-glfw")
    implementation("org.lwjgl:lwjgl-opengl")
    implementation("org.lwjgl:lwjgl-stb")

    runtimeOnly("org.lwjgl:lwjgl::$lwjglNatives")
    runtimeOnly("org.lwjgl:lwjgl-glfw::$lwjglNatives")
    runtimeOnly("org.lwjgl:lwjgl-opengl::$lwjglNatives")
    runtimeOnly("org.lwjgl:lwjgl-stb::$lwjglNatives")
}

tasks.test {
    useJUnitPlatform()
}
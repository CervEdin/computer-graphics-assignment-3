// Assignment 3, Part 1 and 2
//
// Modify this file according to the lab instructions.
//

#include "utils.h"
#include "utils2.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cstdlib>
#include <algorithm>

// The attribute locations we will use in the vertex shader
enum AttributeLocation {
    POSITION = 0,
    NORMAL = 1
};

// Struct for representing an indexed triangle mesh
struct Mesh {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
};

// Struct for representing a vertex array object (VAO) created from a
// mesh. Used for rendering.
struct MeshVAO {
    GLuint vao;
    GLuint vertexVBO;
    GLuint normalVBO;
    GLuint indexVBO;
    int numVertices;
    int numIndices;
};

// Struct for resources and state
struct Context {
    int width;
    int height;
    float aspect;
    GLFWwindow *window;
    GLuint program;
    Trackball trackball;
    Mesh mesh;
    MeshVAO meshVAO;
    GLuint defaultVAO;
    float elapsed_time;
    
    bool ambient_on;
    bool diffuse_on;
    bool specular_on;
    bool gamma_on;
    bool surface_normal_rgb_on;
    
    float zoomFactor;
    int active_texture;
    GLuint cubemap[8];
};

// Returns the value of an environment variable
std::string getEnvVar(const std::string &name)
{
    char const* value = std::getenv(name.c_str());
    if (value == nullptr) {
        return std::string();
    }
    else {
        return std::string(value);
    }
}

// Returns the absolute path to the shader directory
std::string shaderDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/model_viewer/src/shaders/";
}

// Returns the absolute path to the 3D model directory
std::string modelDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/model_viewer/3d_models/";
}

// Returns the absolute path to the cubemap texture directory
std::string cubemapDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT3_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT3_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/model_viewer/cubemaps/";
}

void loadMesh(const std::string &filename, Mesh *mesh)
{
    OBJMesh obj_mesh;
    objMeshLoad(obj_mesh, filename);
    mesh->vertices = obj_mesh.vertices;
    mesh->normals = obj_mesh.normals;
    mesh->indices = obj_mesh.indices;
}

void createMeshVAO(Context &ctx, const Mesh &mesh, MeshVAO *meshVAO)
{
    // Generates and populates a VBO for the vertices
    glGenBuffers(1, &(meshVAO->vertexVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    auto verticesNBytes = mesh.vertices.size() * sizeof(mesh.vertices[0]);
    glBufferData(GL_ARRAY_BUFFER, verticesNBytes, mesh.vertices.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the vertex normals
    glGenBuffers(1, &(meshVAO->normalVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->normalVBO);
    auto normalsNBytes = mesh.normals.size() * sizeof(mesh.normals[0]);
    glBufferData(GL_ARRAY_BUFFER, normalsNBytes, mesh.normals.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the element indices
    glGenBuffers(1, &(meshVAO->indexVBO));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVAO->indexVBO);
    auto indicesNBytes = mesh.indices.size() * sizeof(mesh.indices[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesNBytes, mesh.indices.data(), GL_STATIC_DRAW);

    // Creates a vertex array object (VAO) for drawing the mesh
    glGenVertexArrays(1, &(meshVAO->vao));
    glBindVertexArray(meshVAO->vao);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->normalVBO);
    glEnableVertexAttribArray(NORMAL);
    glVertexAttribPointer(NORMAL, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVAO->indexVBO);
    glBindVertexArray(ctx.defaultVAO); // unbinds the VAO

    // Additional information required by draw calls
    meshVAO->numVertices = mesh.vertices.size();
    meshVAO->numIndices = mesh.indices.size();
}

void initializeTrackball(Context &ctx)
{
    double radius = double(std::min(ctx.width, ctx.height)) / 2.0;
    ctx.trackball.radius = radius;
    glm::vec2 center = glm::vec2(ctx.width, ctx.height) / 2.0f;
    ctx.trackball.center = center;
}

void init(Context &ctx)
{
    ctx.program = loadShaderProgram(shaderDir() + "mesh.vert",
                                    shaderDir() + "mesh.frag");

    loadMesh((modelDir() + "gargo.obj"), &ctx.mesh);
    createMeshVAO(ctx, ctx.mesh, &ctx.meshVAO);

    // Load cubemap texture(s)
    std::string prefiltered_levels[8] = { "0.5", "0.125", "2", "8", "32", "128", "512", "2048" };
    std::string texture_name = "Forrest";
    for (int i = 0; i < 8; i++) {
        std::string path = cubemapDir() + "/" + texture_name + "/prefiltered/" + prefiltered_levels[i];
        ctx.cubemap[i] = loadCubemap(path);
    }
    ctx.active_texture = 0;
    
    initializeTrackball(ctx);
}

// MODIFY THIS FUNCTION
void drawMesh(Context &ctx, GLuint program, const MeshVAO &meshVAO)
{   
    // Define uniforms
    glm::mat4 model = trackballGetRotationMatrix(ctx.trackball);
	glm::mat4 view = glm::lookAt(
		glm::vec3(.0f, .0f, -5.0f), // Camera position
		glm::vec3(.0f, .0f, .0f), // Target position
		glm::vec3(0, 1, 0)
	);
    glm::mat4 projection = glm::perspective(glm::radians(30.0f * ctx.zoomFactor), 1.0f, 0.01f, 100.0f);
    glm::mat4 mv = view * model;
    glm::mat4 mvp = projection * mv;
    // Light source
	glm::vec3 pos_light_pos = glm::vec3(10.f, .0f, .0f); // Positional light position, straight above
	glm::vec3 pos_light_col = glm::vec3(1.f, 1.f, 1.f); // Positional light color, white
	glm::vec3 diffuse_color = glm::vec3(0.f, 0.f, 0.5f);
	glm::vec3 ambient_color = diffuse_color * 0.01f;
	glm::vec3 specular_color = glm::vec3(0.04f);
	float specular_power = 40.0f;

    // Activate program
    glUseProgram(program);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.cubemap[ctx.active_texture]);
    glUniform1i(glGetUniformLocation(program, "u_cubemap"), 0);

    // Pass uniforms
    glUniform1i(glGetUniformLocation(program, "u_ambient_on"), ctx.ambient_on);
    glUniform1i(glGetUniformLocation(program, "u_diffuse_on"), ctx.diffuse_on);
    glUniform1i(glGetUniformLocation(program, "u_specular_on"), ctx.specular_on);
    glUniform1i(glGetUniformLocation(program, "u_gamma_on"), ctx.gamma_on);
    glUniform1i(glGetUniformLocation(program, "u_surface_normal_rgb_on"), ctx.surface_normal_rgb_on);
    
    glUniformMatrix4fv(glGetUniformLocation(program, "u_mv"), 1, GL_FALSE, &mv[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, &mvp[0][0]);
	glUniform3fv(glGetUniformLocation(program, "u_pos_light_pos"), 1, &pos_light_pos[0]);
	glUniform3fv(glGetUniformLocation(program, "u_pos_light_col"), 1, &pos_light_col[0]);
	glUniform3fv(glGetUniformLocation(program, "u_ambient_color"), 1, &ambient_color[0]);
	glUniform3fv(glGetUniformLocation(program, "u_diffuse_color"), 1, &diffuse_color[0]);
	glUniform3fv(glGetUniformLocation(program, "u_specular_color"), 1, &specular_color[0]);
	glUniform1f(glGetUniformLocation(program, "u_specular_power"), specular_power);
	
	glUniform1f(glGetUniformLocation(program, "u_time"), ctx.elapsed_time);
    // ...

    // Draw!
    glBindVertexArray(meshVAO.vao);
    glDrawElements(GL_TRIANGLES, meshVAO.numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(ctx.defaultVAO);
}

void display(Context &ctx)
{
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST); // ensures that polygons overlap correctly
    drawMesh(ctx, ctx.program, ctx.meshVAO);
}

void reloadShaders(Context *ctx)
{
    glDeleteProgram(ctx->program);
    ctx->program = loadShaderProgram(shaderDir() + "mesh.vert",
                                     shaderDir() + "mesh.frag");
}

void mouseButtonPressed(Context *ctx, int button, int x, int y)
{
	//sstd::cout << button << std::endl;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        ctx->trackball.center = glm::vec2(x, y);
        trackballStartTracking(ctx->trackball, glm::vec2(x, y));
    }
}

void mouseButtonReleased(Context *ctx, int button, int x, int y)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        trackballStopTracking(ctx->trackball);
    }
}

void moveTrackball(Context *ctx, int x, int y)
{
    if (ctx->trackball.tracking) {
        trackballMove(ctx->trackball, glm::vec2(x, y));
    }
}

void errorCallback(int /*error*/, const char* description)
{
    std::cerr << description << std::endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // std::cout << key <<std::endl;
    // Forward event to GUI
    ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard) { return; }  // Skip other handling

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        reloadShaders(ctx);
    } else if (key == 49 && action == GLFW_PRESS) { //1
        ctx->ambient_on = !ctx->ambient_on;
    } else if (key == 50 && action == GLFW_PRESS) { //2
        ctx->diffuse_on = !ctx->diffuse_on;
    } else if (key == 51 && action == GLFW_PRESS) { //3
        ctx->specular_on = !ctx->specular_on;
    } else if (key == 52 && action == GLFW_PRESS) { //4
        ctx->gamma_on = !ctx->gamma_on;
    } else if (key == 53 && action == GLFW_PRESS) { //5
        ctx->surface_normal_rgb_on = !ctx->surface_normal_rgb_on;
    } else if (key == 54 && action == GLFW_PRESS) { //6
        ctx->active_texture = 0;
    } else if (key == 55 && action == GLFW_PRESS) { //7
        ctx->active_texture = 1;
    } else if (key == 56 && action == GLFW_PRESS) { //8
        ctx->active_texture = 3;
    } else if (key == 57 && action == GLFW_PRESS) { //9
        ctx->active_texture = 6;
    } else if (key == 48 && action == GLFW_PRESS) { //0
        ctx->active_texture = 7;
    }
}

void charCallback(GLFWwindow* window, unsigned int codepoint)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_CharCallback(window, codepoint);
    if (ImGui::GetIO().WantTextInput) { return; }  // Skip other handling
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) { return; }  // Skip other handling

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        mouseButtonPressed(ctx, button, x, y);
    }
    else {
        mouseButtonReleased(ctx, button, x, y);
    }
}

void cursorPosCallback(GLFWwindow* window, double x, double y)
{
    if (ImGui::GetIO().WantCaptureMouse) { return; }  // Skip other handling   

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    moveTrackball(ctx, x, y);
}

void resizeCallback(GLFWwindow* window, int width, int height)
{
    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->width = width;
    ctx->height = height;
    ctx->aspect = float(width) / float(height);
    ctx->trackball.radius = double(std::min(width, height)) / 2.0;
    ctx->trackball.center = glm::vec2(width, height) / 2.0f;
    glViewport(0, 0, width, height);
}

void scrollCallback(GLFWwindow* window, double x, double y) {
	if (ImGui::GetIO().WantCaptureMouse) { return; }  // Skip other handling   

	Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
	ctx->zoomFactor = ctx->zoomFactor + 0.1f*(float)y;
}

int main(void)
{
    Context ctx;

    // Create a GLFW window
    glfwSetErrorCallback(errorCallback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    ctx.width = 1200;
    ctx.height = 1200;
    ctx.aspect = float(ctx.width) / float(ctx.height);
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Model viewer", nullptr, nullptr);
    ctx.zoomFactor = 1.0;

    // turn on switches
    ctx.ambient_on = true;
    ctx.diffuse_on = true;
    ctx.specular_on = true;
    ctx.gamma_on = true;
    ctx.surface_normal_rgb_on = false;
    
    glfwMakeContextCurrent(ctx.window);
    glfwSetWindowUserPointer(ctx.window, &ctx);
    glfwSetKeyCallback(ctx.window, keyCallback);
    glfwSetCharCallback(ctx.window, charCallback);
    glfwSetMouseButtonCallback(ctx.window, mouseButtonCallback);
    glfwSetCursorPosCallback(ctx.window, cursorPosCallback);
    glfwSetFramebufferSizeCallback(ctx.window, resizeCallback);
	glfwSetScrollCallback(ctx.window, scrollCallback);

    // Load OpenGL functions
    glewExperimental = true;
    GLenum status = glewInit();
    if (status != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(status) << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Initialize GUI
    ImGui_ImplGlfwGL3_Init(ctx.window, false /*do not install callbacks*/);

    // Initialize rendering
    glGenVertexArrays(1, &ctx.defaultVAO);
    glBindVertexArray(ctx.defaultVAO);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    init(ctx);

    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        ctx.elapsed_time = glfwGetTime();
        ImGui_ImplGlfwGL3_NewFrame();
        display(ctx);
        ImGui::Render();
        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

// For Blending
unsigned int loadTexture(char const * path);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 900;
const unsigned int SCR_HEIGHT = 700;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Blinn-Phong
int blinn_flag = 0;
bool blinnKeyPressed = false;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

// Program state init
//----------------------------------------------------------------------------------

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;

    // Cart object
    glm::vec3 cartPosition = glm::vec3(0.0f);
    float cartScale = 3.0f;
    float cartXRotationDeg = -90.0f; float cartZRotationDeg = -45.0f;
//    diameters infered from scene.gltf -> accessors
    float cartXDiameter = (1.096f + 0.045f) * cartScale; float cartYDiameter = 2 * 0.311f * cartScale; float cartZDiameter = (0.085f + 1.036) * cartScale;

    // Lays chips object
    glm::vec3 laysStartPosition = glm::vec3(cartPosition.x + cartXDiameter/2, cartPosition.y - cartYDiameter/2, cartPosition.z + cartZDiameter/2);
    float laysScale = 0.025f;
    //instancing
    int laysAmount = 4;
    // ovo ce morati da bude niz za instancing
    float laysRotationDeg = 0.0f;
    // instancing
    float laysInstancingOffset = 0.1f;

    // Aisle object
    glm::vec3 aislePosition = glm::vec3(0.0f, -3.0f, -10.0f);
    float aisleScale = 5.0f; float aisleRotationDeg = -90.0f;

    // Floor
    glm::vec3 floorPosition = glm::vec3(0.0f, -cartYDiameter-1.5f, 0.0f); // for some reason cart front wheels are missing
    float floorXRotationDeg = -90.0f;

    // Plastic Bottle
    glm::vec3 bottlePosition = glm::vec3(-3.0f, -3.0f, 0.0f);
    float bottleXRotationDeg = -90.0f;

    // Glass Bottle
    glm::vec3 bottle2Position = glm::vec3(5.0f, -3.0f, -3.0f);
    float bottle2Scale = 0.5f;

    // Transparent windows positions
    // --------------------------------
    vector<glm::vec3> windows
            {
                    glm::vec3( 3.0f, 0.0f, 3.0f),
                    glm::vec3( -7.0f, 0.0f, 3.0f)
            };
    float windowScale = 7.0f;


    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    // Multiple light sources
    glm::vec3 pointLightPositions[4] = {
            glm::vec3(cartPosition.x, 4.0f, cartPosition.z),
            glm::vec3(3.0f, 5.0f, 4.0f),
            glm::vec3(-7.0f, 4.0f, 3.8f),
            glm::vec3(-7.0f, 3.0f, camera.Position.z - 0.5f)
    };

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}
// ----------------------------------------------------------------------------


ProgramState *programState;

void DrawImGui(ProgramState *programState);
void renderModels(Shader &shader, vector<Model> &models);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");


    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
//    Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// Code copied from https://learnopengl.com/code_viewer_gh.php?code=src/4.advanced_opengl/3.2.blending_sort/blending_sorted.cpp
//----------------------------------------------------------------------------------------------------
    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

//    loading texture
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/blending_transparent_window.png").c_str());

//    Face-culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW); // front face is counter clock wise
    glCullFace(GL_BACK); // don't render back (important for rendering the floor)

    // build and compile shaders
    // -------------------------
    Shader ourShader(FileSystem::getPath("resources/shaders/2.model_lighting.vs").c_str(), FileSystem::getPath("resources/shaders/2.model_lighting.fs").c_str());
    Shader instanceShader(FileSystem::getPath("resources/shaders/instancing.vs").c_str(), FileSystem::getPath("resources/shaders/instancing.fs").c_str());
    Shader blendingShader(FileSystem::getPath("resources/shaders/blending.vs").c_str(), FileSystem::getPath("resources/shaders/blending.fs").c_str());
    // load models
    // -----------
    vector<Model> models;
    Model cartModel(FileSystem::getPath("resources/objects/simple_shopping_cart/scene.gltf"));
    Model laysModel(FileSystem::getPath("resources/objects/lays_classic__hd_textures__free_download/scene.gltf"));
    Model aisleModel(FileSystem::getPath("resources/objects/supermarket_potato_chips_shelf_asset/scene.gltf"));
    Model floorModel(FileSystem::getPath("resources/objects/checkered_tile_floor/scene.gltf"));
    Model bottleModel(FileSystem::getPath("resources/objects/water_bottle/scene.gltf"));
    Model bottle2Model(FileSystem::getPath("resources/objects/low_poly_bottle/scene.gltf"));
    models.push_back(cartModel);
    models.push_back(laysModel);
    models.push_back(aisleModel);
    models.push_back(floorModel);
    models.push_back(bottleModel);
    models.push_back(bottle2Model);

    cartModel.SetShaderTextureNamePrefix("material.");
    laysModel.SetShaderTextureNamePrefix("material.");
    aisleModel.SetShaderTextureNamePrefix("material.");
//    -----------------------------------------------------------------------------------
// Instancing
// Code copied from: https://learnopengl.com/Advanced-OpenGL/Instancing -----------------

    unsigned int amount = programState->laysAmount - 1;
    glm::mat4 *modelMatrices = new glm::mat4[amount];
    float radius = 1.0f;
    float offset = 0.5f;

    for (unsigned int i = 0; i < amount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / (float)amount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = programState->laysStartPosition.x * radius + displacement - 0.2f;
//        int sign = (i % 2) ? -1 : 1;
//        float x = programState->laysStartPosition.x + sign*i*offset;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.6f - 0.2f; // keep height of asteroid field smaller compared to width of x and z
//        float y = programState->laysStartPosition.y;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
//        float z = programState->laysStartPosition.z;
        float z = programState->laysStartPosition.z * radius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        // scale
        model = glm::scale(model, glm::vec3(programState->laysScale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = static_cast<float>((rand() % 360));
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        // 4. now add to list of matrices
        modelMatrices[i] = model;
    }

    // configure instanced array
    // -------------------------
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, programState->laysAmount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

    for(unsigned int i = 0; i < laysModel.meshes.size(); i++)
    {
        unsigned int VAO = laysModel.meshes[i].VAO;
        glBindVertexArray(VAO);
        // vertex attributes
        std::size_t vec4Size = sizeof(glm::vec4);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }
// ----------------------------------------------------------

//    Lights; TODO -> vise pointLights
// ----------------------------------------------------------

    PointLight& pointLight = programState->pointLight;
    pointLight.ambient = glm::vec3(1.0, 1.0, 1.0);
    pointLight.diffuse = glm::vec3(1.0, 1.0, 1.0);
    pointLight.specular = glm::vec3(5.0, 3.0, 2.0);

    pointLight.constant = 1.3f;
    pointLight.linear = 0.0f;
    pointLight.quadratic = 0.0f;
//--------------------------------------------------------------

// Blending
// buffer objects for window
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // texCoords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0); // ? zasto ovo ?


    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // sort the transparent windows before rendering
        // ---------------------------------------------
        std::map<float, glm::vec3> sorted;
        for (unsigned int i = 0; i < programState->windows.size(); i++)
        {
            float distance = glm::length(programState->camera.Position - programState->windows[i]);
            sorted[distance] = programState->windows[i];
        }


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        // Lights
        ourShader.setVec3("pointLight[0].position", programState->pointLightPositions[0]);
        ourShader.setVec3("pointLight[1].position", programState->pointLightPositions[1]);
        ourShader.setVec3("pointLight[2].position", programState->pointLightPositions[2]);
        ourShader.setVec3("pointLight[3].position", programState->pointLightPositions[3]);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        ourShader.setInt("blinn_flag", blinn_flag);
        std::cout << (blinn_flag ? "Blinn-Phong" : "Phong") << std::endl;

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // first render the opaque models
        renderModels(ourShader, models);

        // render aisle without face-cull
        glDisable(GL_CULL_FACE);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->aislePosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(
                programState->aisleScale));    // it's a bit too big for our scene, so scale it dow
        model = glm::rotate(model, glm::radians(programState->aisleRotationDeg), glm::vec3(1.0f, 0.0f, 0.0f));
        ourShader.setMat4("model", model);
        models[2].Draw(ourShader);

        // Instancing
        instanceShader.use();
        instanceShader.setMat4("projection", projection);
        instanceShader.setMat4("view", view);
        instanceShader.setInt("texture_diffuse1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, laysModel.textures_loaded[0].id);
        for(unsigned int i = 0; i < laysModel.meshes.size(); i++)
        {
            glBindVertexArray(laysModel.meshes[i].VAO);
            glDrawElementsInstanced(
                    GL_TRIANGLES, laysModel.meshes[i].indices.size(), GL_UNSIGNED_INT, 0, programState->laysAmount
            );
            glBindVertexArray(0);
        }

        // Blending
        blendingShader.use();
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);

        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
                                       // render from furthest to nearest
        for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, it->second);
            model = glm::scale(model, glm::vec3(programState->windowScale));
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

    // -----------------------------------------

        if (programState->ImGuiEnabled)
            DrawImGui(programState);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
//    glDeleteVertexArrays(1, &transparentVAO);
//    glDeleteBuffers(1, &transparentVBO);
    // nmg da oslobodim za instancing (nije ni na learnOpenGl)
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// renderModels implementation
void renderModels(Shader &shader, vector<Model> &models) {

    glm::mat4 model = glm::mat4(1.0f);

    // Shopping cart model
    model = glm::mat4(1.0f);
//    model = glm::translate(model,
//                           programState->cartPosition); // translate it down so it's at the center of the scene
    model = glm::scale(model, glm::vec3(
            programState->cartScale));    // it's a bit too big for our scene, so scale it dow
    model = glm::translate(model,
                           programState->cartPosition);
    model = glm::rotate(model, glm::radians(programState->cartXRotationDeg), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(programState->cartZRotationDeg), glm::vec3(0.0f, 0.0f, 1.0f));
    shader.setMat4("model", model);
    models[0].Draw(shader);

    // Lays chips model
    model = glm::mat4(1.0f);
    model = glm::translate(model,
                           programState->laysStartPosition);
    model = glm::scale(model, glm::vec3(
            programState->laysScale));
//    model = glm::translate(model,
//                           programState->laysStartPosition); nisam pratila dobar redosled transformacija jer sam samo gledala kako scena izgleda
    model = glm::rotate(model, glm::radians(programState->laysRotationDeg), glm::vec3(1.0f, 0.0f, 0.0f));
    shader.setMat4("model", model);
    models[1].Draw(shader);


    // Floor model
    model = glm::mat4(1.0f);
    model = glm::translate(model,
                           programState->floorPosition); // translate it down so it's at the center of the scene
    model = glm::rotate(model, glm::radians(programState->floorXRotationDeg), glm::vec3(1.0f, 0.0f, 0.0f));
    shader.setMat4("model", model);
    models[3].Draw(shader);

    // Plastic bottle model
    model = glm::mat4(1.0f);
    model = glm::translate(model,
                           programState->bottlePosition);
    model = glm::rotate(model, glm::radians(programState->bottleXRotationDeg), glm::vec3(1.0f, 0.0f, 0.0f));
    shader.setMat4("model", model);
    models[4].Draw(shader);

    // Glass bottle model
    model = glm::mat4(1.0f);
    model = glm::translate(model,
                           programState->bottle2Position);
    model = glm::scale(model, glm::vec3(
            programState->bottle2Scale));
    model = glm::rotate(model, glm::radians(programState->bottleXRotationDeg), glm::vec3(1.0f, 0.0f, 0.0f));
    shader.setMat4("model", model);
    models[5].Draw(shader);
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    //Blinn-Phong
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !blinnKeyPressed){
        blinn_flag = !blinn_flag;
        blinnKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE){
        blinnKeyPressed = false;
    }

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Settings");
        ImGui::Text("Choose your chips");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);

        ImGui::DragFloat("constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);

        ImGui::End();
    }

    {
        ImGui::Begin("Instancing");
        ImGui::Text("Stay healthy");
        ImGui::SliderInt("Chips amount", &programState->laysAmount, 0, 4);
        ImGui::End();
    }


//    {
//        ImGui::Begin("Camera info");
//        const Camera& c = programState->camera;
//        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
//        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
//        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
//        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
//        ImGui::End();
//    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
// ----------------------------------------------------------------------


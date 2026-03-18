#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>

// Function Prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
bool checkCollision(glm::vec3 pos1, glm::vec3 size1, glm::vec3 pos2, glm::vec3 size2);


// Dash Settings
float dashSpeed = 20.0f;      // How fast the dash is
float dashDuration = 0.2f;   // How long the "burst" lasts
float dashCooldown = 3.0f;   // 3 second wait
float dashTimer = 0.0f;      // Current cooldown progress
float dashEffectTimer = 0.0f; // Current active dash time
bool isDashing = false;
glm::vec3 dashDir(0.0f);

// Settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// Game Object Structure
struct GameObject {
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 color;
    bool active;
};

// Global Game State
glm::vec3 playerPos = glm::vec3(0.0f, 0.1f, 0.0f);
glm::vec3 playerSize = glm::vec3(0.8f);
float playerRotation = 0.0f;
float playerSpeed = 5.0f;

GameObject enemy = { glm::vec3(-7.0f, 0.5f, -7.0f), glm::vec3(0.8f), glm::vec3(1.0f, 0.1f, 0.1f), true };
float enemySpeed = 2.5f;
float enemyRotation = 0.0f;

std::vector<GameObject> walls;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Play Tag. (Press Q for Dash)", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // --- LOAD SHADERS & MODELS ---
    // Ensure these files exist in your project folder!
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");
    Model playerModel(FileSystem::getPath("resources/objects/animal-cat.obj"));
    Model enemyModel(FileSystem::getPath("resources/objects/animal-dog.obj"));
    Model sceneModel(FileSystem::getPath("resources/objects/floor.obj"));
    Model itemModel(FileSystem::getPath("resources/objects/cube.obj"));


    // --- INITIALIZE WORLD ---
    // Create a boundary of walls
    walls.push_back({ glm::vec3(0.0f, 1.0f, 10.0f),  glm::vec3(20.0f, 2.0f, 0.5f), glm::vec3(0.4f), true }); // Back
    walls.push_back({ glm::vec3(0.0f, 1.0f, -10.0f), glm::vec3(20.0f, 2.0f, 0.5f), glm::vec3(0.4f), true }); // Front
    walls.push_back({ glm::vec3(10.0f, 1.0f, 0.0f),  glm::vec3(0.5f, 2.0f, 20.0f), glm::vec3(0.4f), true }); // Right
    walls.push_back({ glm::vec3(-10.0f, 1.0f, 0.0f), glm::vec3(0.5f, 2.0f, 20.0f), glm::vec3(0.4f), true }); // Left

    // --- RENDER LOOP ---
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        processInput(window);

        // --- Enemy AI: Chase Player ---
        glm::vec3 enemyDir = playerPos - enemy.position;
        if (glm::length(enemyDir) > 0.1f) {
            // 1. Normalize the direction for movement
            glm::vec3 normDir = glm::normalize(enemyDir);
            enemy.position += normDir * enemySpeed * deltaTime;

            float targetAngle = glm::degrees(atan2(normDir.x, normDir.z));
            // Smoothly move the current rotation toward the target angle
            float rotationSpeed = 5.0f;
            enemyRotation = glm::mix(enemyRotation, targetAngle, rotationSpeed * deltaTime);
        }

        // Game Over Collision
        if (checkCollision(playerPos, playerSize, enemy.position, enemy.size)) {
            std::cout << "Game Over! Restarting..." << std::endl;
            playerPos = glm::vec3(0.0f, 0.5f, 0.0f);
            playerRotation = 0.0f;

            // 2. Reset Enemy to its starting corner
            enemy.position = glm::vec3(-7.0f, 0.5f, -7.0f);
        }

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // CAMERA SETUP (Follow Camera)
        glm::vec3 cameraOffset = glm::vec3(0.0f, 10.0f, 12.0f);
        glm::mat4 view = glm::lookAt(playerPos + cameraOffset, playerPos, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // --- DRAW FLOOR ---
        ourShader.setBool("useTexture", false); // FLOOR IS SOLID COLOR
        ourShader.setVec3("objectColor", glm::vec3(0.1f, 0.3f, 0.1f));
        ourShader.setMat4("model", glm::mat4(1.0f));
        sceneModel.Draw(ourShader);

        // --- DRAW WALLS ---
        ourShader.setBool("useTexture", false); // WALLS ARE SOLID COLOR
        for (auto& wall : walls) {
            ourShader.setVec3("objectColor", wall.color);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), wall.position);
            model = glm::scale(model, wall.size);
            ourShader.setMat4("model", model);
            itemModel.Draw(ourShader);
        }

        // --- DRAW ENEMY ---
        ourShader.setBool("useTexture", true);
        ourShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f)); // Light red tint

        glm::mat4 enemyM = glm::mat4(1.0f);
        enemyM = glm::translate(enemyM, enemy.position);

        // Apply the rotation around the Y-axis
        enemyM = glm::rotate(enemyM, glm::radians(enemyRotation), glm::vec3(0.0f, 1.0f, 0.0f));

        enemyM = glm::scale(enemyM, enemy.size);
        ourShader.setMat4("model", enemyM);
        enemyModel.Draw(ourShader);

        // --- DRAW PLAYER ---
        ourShader.setBool("useTexture", true); // PLAYER HAS TEXTURE
        ourShader.setVec3("objectColor", glm::vec3(1.0f));
        glm::mat4 playerM = glm::translate(glm::mat4(1.0f), playerPos);
        playerM = glm::rotate(playerM, glm::radians(playerRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        playerM = glm::scale(playerM, playerSize);
        ourShader.setMat4("model", playerM);
        playerModel.Draw(ourShader);
        // Change color to bright cyan during dash
        if (isDashing) {
            ourShader.setVec3("objectColor", glm::vec3(0.0f, 1.0f, 1.0f));
        }
        else {
            ourShader.setVec3("objectColor", glm::vec3(0.1f, 0.5f, 1.0f)); // Normal Blue
        }

        // --- RENDER FAKE SHADOWS ---
        ourShader.use();
        ourShader.setBool("useTexture", false); // Shadows are solid color
        // Set color to Black with 0.5 Alpha (50% transparency)
        ourShader.setVec3("objectColor", glm::vec3(0.0f, 0.0f, 0.0f));
        // Note: You might need to update your FS to support alpha, or just use a dark grey:
        // ourShader.setVec3("objectColor", glm::vec3(0.1f, 0.1f, 0.1f)); 

        // Player Shadow
        glm::mat4 shadowM = glm::mat4(1.0f);
        // Position it slightly above the floor (y=0.01) to avoid "Z-fighting" (flickering)
        shadowM = glm::translate(shadowM, glm::vec3(playerPos.x, 0.01f, playerPos.z));
        // Scale it to be a flat circle/square
        shadowM = glm::scale(shadowM, glm::vec3(playerSize.x * 1.2f, 0.001f, playerSize.z * 1.2f));
        ourShader.setMat4("model", shadowM);
        playerModel.Draw(ourShader); // Or playerModel.Draw(ourShader)

        // Enemy Shadow
        shadowM = glm::mat4(1.0f);
        shadowM = glm::translate(shadowM, glm::vec3(enemy.position.x, 0.01f, enemy.position.z));
        shadowM = glm::scale(shadowM, glm::vec3(playerSize.x * 1.2f, 0.001f, playerSize.z * 1.2f));
        ourShader.setMat4("model", shadowM);
		enemyModel.Draw(ourShader); // Or enemyModel.Draw(ourShader)

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 1. Cooldown Management
    if (dashTimer > 0) dashTimer -= deltaTime;

    // 2. Dash Activation (Press Q)
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && dashTimer <= 0 && !isDashing) {
        // Get the current movement direction to dash toward
        glm::vec3 currentDir(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) currentDir.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) currentDir.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) currentDir.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) currentDir.x += 1.0f;

        // Only dash if we are actually holding a direction
        if (glm::length(currentDir) > 0.0f) {
            dashDir = glm::normalize(currentDir);
            isDashing = true;
            dashEffectTimer = dashDuration;
            dashTimer = dashCooldown; // Start the 3s cooldown
            std::cout << "DASHED! Cooldown started..." << std::endl;
        }
    }

    // 3. Movement Logic
    float currentSpeed = playerSpeed;
    glm::vec3 moveDir(0.0f);

    if (isDashing) {
        currentSpeed = dashSpeed;
        moveDir = dashDir;
        dashEffectTimer -= deltaTime;
        if (dashEffectTimer <= 0) isDashing = false;
    }
    else {
        // Regular WASD movement
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir.x += 1.0f;
        if (glm::length(moveDir) > 0.0f) moveDir = glm::normalize(moveDir);
    }

    // Apply movement with collision
    if (glm::length(moveDir) > 0.0f) {
        glm::vec3 nextPos = playerPos + (moveDir * currentSpeed * deltaTime);

        // Update rotation to face movement direction
        playerRotation = glm::degrees(atan2(moveDir.x, moveDir.z));

        bool collided = false;
        for (const auto& wall : walls) {
            if (checkCollision(nextPos, playerSize, wall.position, wall.size)) {
                collided = true;
                isDashing = false; // Stop dash if we hit a wall
                break;
            }
        }
        if (!collided) playerPos = nextPos;
    }
}
bool checkCollision(glm::vec3 pos1, glm::vec3 size1, glm::vec3 pos2, glm::vec3 size2) {
    bool colX = pos1.x + size1.x / 2 >= pos2.x - size2.x / 2 && pos2.x + size2.x / 2 >= pos1.x - size1.x / 2;
    bool colY = pos1.y + size1.y / 2 >= pos2.y - size2.y / 2 && pos2.y + size2.y / 2 >= pos1.y - size1.y / 2;
    bool colZ = pos1.z + size1.z / 2 >= pos2.z - size2.z / 2 && pos2.z + size2.z / 2 >= pos1.z - size1.z / 2;
    return colX && colY && colZ;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
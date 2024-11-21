#define GLM_ENABLE_EXPERIMENTAL
// Include standard headers
#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

class Camera {
public:
    Camera(GLFWwindow* window, glm::vec3 position, glm::vec3 target, glm::vec3 up)
        : window(window), position(position), target(target), up(up),
        speed(0.05f), rotationSpeed(3.0f), deltaTime(0.0f), lastFrame(0.0f) {}

    Camera() = default; // Default constructor

    void setWindow(GLFWwindow* window) {
        this->window = window;
    }

    void update() {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Move forward
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            glm::vec3 direction = glm::normalize(target - position);
            position += direction * speed * deltaTime;
        }
        // Move backward
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            glm::vec3 direction = glm::normalize(target - position);
            position -= direction * speed * deltaTime;
        }
        // Rotate counter-clockwise
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            glm::vec3 right = glm::normalize(glm::cross(target - position, up));
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotationSpeed * deltaTime), up);
            target = position + glm::vec3(rotation * glm::vec4(target - position, 0.0f));
        }
        // Rotate clockwise
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            glm::vec3 right = glm::normalize(glm::cross(target - position, up));
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-rotationSpeed * deltaTime), up);
            target = position + glm::vec3(rotation * glm::vec4(target - position, 0.0f));
        }
    }


    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, target, up);
    }

private:
    GLFWwindow* window;
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    float speed;
    float rotationSpeed;
    float deltaTime;
    float lastFrame;

    void rotateCamera(float angle) {
        glm::vec3 direction = target - position;
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, up);
        direction = glm::vec3(rotation * glm::vec4(direction, 1.0f));
        target = position + direction;
    }
};

struct VertexData {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
    float u, v;
};

struct TriData {
    int v1, v2, v3;
};

void loadARGB_BMP(const char* imagepath, unsigned char** data, unsigned int* width, unsigned int* height) {
    printf("Reading image %s\n", imagepath);
    // Data read from the header of the BMP file
    unsigned char header[54];
    unsigned int dataPos;
    unsigned int imageSize;
    // Actual RGBA data
    // Open the file
    FILE* file = fopen(imagepath, "rb");
    if (!file) {
        printf("%s could not be opened. Are you in the right directory?\n", imagepath);
        getchar();
        return;
    }
    // Read the header, i.e. the 54 first bytes
    // If less than 54 bytes are read, problem
    if (fread(header, 1, 54, file) != 54) {
        printf("Not a correct BMP file1\n");
        fclose(file);
        return;
    }
    // Read the information about the image
    dataPos = *(int*)&(header[0x0A]);
    imageSize = *(int*)&(header[0x22]);
    *width = *(int*)&(header[0x12]);
    *height = *(int*)&(header[0x16]);
    // A BMP files always begins with "BM"
    if (header[0] != 'B' || header[1] != 'M') {
        printf("Not a correct BMP file2\n");
        fclose(file);
        return;
    }
    // Make sure this is a 32bpp file
    if (*(int*)&(header[0x1E]) != 3) {
        printf("Not a correct BMP file3\n");
        fclose(file);
        return;
    }
    // Some BMP files are misformatted, guess missing information
    if (imageSize == 0) imageSize = (*width) * (*height) * 4; // 4 : one byte for each Red, Green, Blue, Alpha component
    if (dataPos == 0) dataPos = 54; // The BMP header is done that way
    // Create a buffer
    *data = new unsigned char[imageSize];
    if (dataPos != 54) {
        fread(header, 1, dataPos - 54, file);
    }
    // Read the actual data from the file into the buffer
    fread(*data, 1, imageSize, file);
    fprintf(stderr, "Done reading!\n");
    // Everything is in memory now, the file can be closed.
    fclose(file);
}

class TexturedMesh {
public:
    TexturedMesh(const std::string &plyFilePath, const std::string &textureFilePath) {
        readPLYFile(plyFilePath, vertices, faces);
        loadTexture(textureFilePath);
        setupBuffers();
        setupShaders();
    }

    void draw(glm::mat4 MVP) {

        glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Bind shader program
        glUseProgram(ProgramID);
        glUniformMatrix4fv(MVPUniform, 1, GL_FALSE, &MVP[0][0]);

        // Bind VAO
        glBindVertexArray(vaoID);

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Draw
        glDrawElements(GL_TRIANGLES, faces.size() * 3, GL_UNSIGNED_INT, 0);

        // Cleanup
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);

		glPopMatrix();
		glDisable(GL_BLEND);
    }

private:
    std::vector<VertexData> vertices;
    std::vector<TriData> faces;
    GLuint vboID, texCoordID, VBO_indices, textureID, vaoID, ProgramID, MVPUniform;

    void readPLYFile(const std::string& fname, std::vector<VertexData>& vertices, std::vector<TriData>& faces) {
        std::ifstream file(fname);
        std::string line;

        // Read header
        int numVertices = 0, numFaces = 0;
        while (std::getline(file, line)) {
            if (line == "end_header")
                break;

            std::istringstream iss(line);
            std::string token;
            iss >> token;

            if (token == "element") {
                iss >> token;
                if (token == "vertex") {
                    iss >> numVertices;
                } else if (token == "face") {
                    iss >> numFaces;
                }
            }
        }

        // Read vertices
        vertices.resize(numVertices);
        for (int i = 0; i < numVertices; ++i) {
            std::getline(file, line);
            std::istringstream iss(line);
            iss >> vertices[i].x >> vertices[i].y >> vertices[i].z;
            if (iss >> vertices[i].nx >> vertices[i].ny >> vertices[i].nz) {
                // Normal data is available
            }
            if (iss >> vertices[i].u >> vertices[i].v) {
                // Texture coordinates data is available
            }
            if (iss >> vertices[i].r >> vertices[i].g >> vertices[i].b) {
                // Color data is available
            }
        }

        // Read faces
        faces.resize(numFaces);
        for (int i = 0; i < numFaces; ++i) {
            std::getline(file, line);
            std::istringstream iss(line);
            std::string token;
            iss >> token; // "3"
            iss >> faces[i].v1 >> faces[i].v2 >> faces[i].v3;
        }
    }


    void loadTexture(const std::string &textureFilePath) {
        unsigned char *data;
        GLuint width, height;
        loadARGB_BMP(textureFilePath.c_str(), &data, &width, &height);
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        delete[] data; // Free the image data
    }

    void setupBuffers() {
        glGenVertexArrays(1, &vaoID);
        glBindVertexArray(vaoID);

        glGenBuffers(1, &vboID);
        glBindBuffer(GL_ARRAY_BUFFER, vboID);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexData), &vertices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) offsetof(VertexData, x));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void *) offsetof(VertexData, u));

        glGenBuffers(1, &VBO_indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(TriData), &faces[0], GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    void setupShaders() {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        std::string VertexShaderCode = "\
            #version 330 core\n\
            // Input vertex data, different for all executions of this shader.\n\
            layout(location = 0) in vec3 vertexPosition;\n\
            layout(location = 1) in vec2 uv;\n\
            // Output data ; will be interpolated for each fragment.\n\
            out vec2 uv_out;\n\
            // Values that stay constant for the whole mesh.\n\
            uniform mat4 MVP;\n\
            void main(){ \n\
                // Output position of the vertex, in clip space : MVP * position\n\
                gl_Position =  MVP * vec4(vertexPosition,1);\n\
                // The color will be interpolated to produce the color of each fragment\n\
                uv_out = uv;\n\
            }\n";
        char const * VertexSourcePointer = VertexShaderCode.c_str();
        glShaderSource(vertexShader, 1, &VertexSourcePointer, NULL);
        glCompileShader(vertexShader);
        checkCompileErrors(vertexShader, "VERTEX");

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        // Read the Fragment Shader code from the file
        std::string FragmentShaderCode = "\
            #version 330 core\n\
            in vec2 uv_out; \n\
            uniform sampler2D tex;\n\
            void main() {\n\
                gl_FragColor = texture(tex, uv_out);\n\
            }\n";
        char const * FragmentSourcePointer = FragmentShaderCode.c_str();
        glShaderSource(fragmentShader, 1, & FragmentSourcePointer, NULL);
        glCompileShader(fragmentShader);
        checkCompileErrors(fragmentShader, "FRAGMENT");

        ProgramID = glCreateProgram();
        glAttachShader(ProgramID, vertexShader);
        glAttachShader(ProgramID, fragmentShader);
        glLinkProgram(ProgramID);
        checkCompileErrors(ProgramID, "PROGRAM");

        glDetachShader(ProgramID, vertexShader);
        glDetachShader(ProgramID, fragmentShader);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        MVPUniform = glGetUniformLocation(ProgramID, "MVP");
        GLuint textureUniform = glGetUniformLocation(ProgramID, "textureSampler");
        glUniform1i(textureUniform, 0); // Texture unit 0 is active
    }

    void checkCompileErrors(GLuint shader, std::string type) {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};


int main( int argc, char* argv[])
{
	float screenW = 1100;
	float screenH = 700;
	float stepsize = 0.1f;

	if (argc > 1 ) {
		screenW = atoi(argv[1]);
	}
	if (argc > 2) {
		screenH = atoi(argv[2]);
	}
	if (argc > 3) {
		stepsize = atof(argv[3]);
	}

	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

    // Set OpenGL version to 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( screenW, screenH, "House", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

    // Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Dark blue background
	glClearColor(0.2f, 0.2f, 0.3f, 0.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // Create TexturedMesh objects
    TexturedMesh bottlesMesh("Bottles.ply", "bottles.bmp");
    TexturedMesh floorMesh("Floor.ply", "floor.bmp");
    TexturedMesh patioMesh("Patio.ply", "patio.bmp");
    TexturedMesh tableMesh("Table.ply", "table.bmp");
    TexturedMesh wallsMesh("Walls.ply", "walls.bmp");
    TexturedMesh windowBGMesh("WindowBG.ply", "windowbg.bmp");
    TexturedMesh woodObjectsMesh("WoodObjects.ply", "woodobjects.bmp");

    // Transparent
    TexturedMesh curtainsMesh("Curtains.ply", "curtains.bmp");
    TexturedMesh doorBGMesh("DoorBG.ply", "doorbg.bmp");
    TexturedMesh metalObjectsMesh("MetalObjects.ply", "metalobjects.bmp");


    glm::vec3 lightpos(5.0f, 5.0f, 5.0f);


	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

    // Set up camera
    Camera camera(window, glm::vec3(0.5f, 0.4f, 0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Render loop
	do{

        // Update camera
        camera.update();

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glm::mat4 Projection = glm::perspective(glm::radians(45.0f), screenW/screenH, 0.001f, 1000.0f);
		// Projection = glm::mat4(1.0f);
		glLoadMatrixf(glm::value_ptr(Projection));
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 M = glm::mat4(1.0f);

        glm::mat4 MVP = Projection * view * M;

        // Render each TexturedMesh object
        bottlesMesh.draw(MVP);
        floorMesh.draw(MVP);
        patioMesh.draw(MVP);
        tableMesh.draw(MVP);
        wallsMesh.draw(MVP);
        windowBGMesh.draw(MVP);
        woodObjectsMesh.draw(MVP);
        //Render transparent objects last
        doorBGMesh.draw(MVP);
        metalObjectsMesh.draw(MVP);
        curtainsMesh.draw(MVP);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
	return 0;
}


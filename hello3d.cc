#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <chipmunk.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern "C" {
#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <gl/glu.h>

#include <ft2build.h>
#include FT_FREETYPE_H
}

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;

char WINDOW_NAME[] = "Hello, World! Now in 3D!";
SDL_Window * gWindow = NULL;
SDL_GLContext gContext;

FT_Library ft;
FT_Face face;

void die(string message) {
    cout << message << endl;
    exit(1);
}

struct Character {
    GLuint TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    GLuint Advance;
};

map<GLchar, Character> Characters;

// initialize libraries
void init()
{
    // init SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) die("SDL");
    if (! SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) die("texture");

    // init SDL GL
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

    gWindow = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH, SCREEN_HEIGHT,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (gWindow == NULL) die("window");

    gContext = SDL_GL_CreateContext(gWindow);
    if (! gContext) die("gl context");

    // init GLEW
    glewExperimental = GL_TRUE; 
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) die("glew");

    // init Freetype and load glyphs
    if (FT_Init_FreeType(& ft)) die("freetype");
    if (FT_New_Face(ft, "arial.ttf", 0, & face)) die("font");
    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (GLubyte c=0 ; c<128 ; c+=1) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) die("glyph");
        GLuint texture;
        glGenTextures(1, & texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(pair<GLchar, Character>(c, character));
    }
}

void close()
{
    //TODO close Freetype and OpenGL

    SDL_DestroyWindow(gWindow);
    gWindow = NULL;

    SDL_Quit();
}

default_random_engine gen;
uniform_real_distribution<double> dist(0.0, 1.0);

double random(double min, double max) {
    return dist(gen) * (max - min) + min;
}

cpSpace * space;

void bouncy() {
    // set up physics
    cpVect gravity = cpv(0, -1);
    space = cpSpaceNew();
    cpSpaceSetGravity(space, gravity);

    // static body
    cpBody * body0 = cpSpaceAddBody(space, cpBodyNewStatic());
    cpBodySetPosition(body0, cpv(0, 1));

    // "Hello,"
    cpFloat mass1 = 1;
    cpFloat moment1 = cpMomentForBox(mass1, 160, 120);
    cpBody * body1 = cpSpaceAddBody(space, cpBodyNew(mass1, moment1/2));
    cpBodySetPosition(body1, cpv(0.5, 0.75));

    // "World!"
    cpFloat mass2 = 1;
    cpFloat moment2 = cpMomentForBox(mass2, 160, 120);
    cpBody * body2 = cpSpaceAddBody(space, cpBodyNew(mass2, moment2/2));
    cpBodySetPosition(body2, cpv(0.5, 0.333));

    // springs
    cpSpaceAddConstraint(space, cpDampedSpringNew(body0, body1, cpv(0,0), cpv(0,0.01), 0.667, 30, 0.0001));
    cpSpaceAddConstraint(space, cpDampedSpringNew(body1, body2, cpv(0,-0.01), cpv(0,0.01), 0.667, 30, 0.0001));

    while (true) {
        for (int n=0 ; n<20 ; n+=1) cpSpaceStep(space, 1/1000.0);

        // Hello,
        cpVect pos1 = cpBodyGetPosition(body1);
        cpFloat rot1 = cpBodyGetAngle(body1);

        // World!
        cpVect pos2 = cpBodyGetPosition(body2);
        cpFloat rot2 = cpBodyGetAngle(body2);
    }
}

unsigned int shaderProgram;
unsigned int VAO;

void inittetrahedron() {
    // vertex shader
    const char * vertex_shader_code =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "out vec3 ourColor;\n"
        "uniform mat4 transform;\n"
        "void main() {\n"
        "  gl_Position = transform * vec4(aPos.x,aPos.y,aPos.z, 1.0);\n"
        "  ourColor = aColor;\n"
        "}";
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, & vertex_shader_code, NULL);
    glCompileShader(vertexShader);
    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, & success);
    if (! success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << infoLog << endl;
        die("vertex shader");
    }

    // fragment shader
    const char * fragment_shader_code = 
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 ourColor;\n"
        "uniform bool outline;\n"
        "void main() {\n"
        "  if (outline) FragColor = vec4(1.0f,1.0f,1.0f,1.0f);\n"
        "  else FragColor = vec4(ourColor, 1.0f);\n"
        "}";
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, & fragment_shader_code, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, & success);
    if (! success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << infoLog << endl;
        die("fragment shader");
    }

    // shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, & success);
    if (! success) die("shader program");

    // delete shaders (unneeded after program link)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // vertices with colors
    float vertices[] = {
        sqrt(8.0/9.0),  0.0,            -1.0/3.0, 1.0,0.0,0.0,
        -sqrt(2.0/9.0), sqrt(2.0/3.0),  -1.0/3.0, 0.0,1.0,0.0,
        -sqrt(2.0/9.0), -sqrt(2.0/3.0), -1.0/3.0, 0.0,0.0,1.0,
        0.0,            0.0,            1.0,      1.0,1.0,0.0,
    };
    // triangles from vertices
    unsigned int indices[] = {
        0, 1, 2,
        0, 1, 3,
        0, 2, 3,
        1, 2, 3,
    };

    // vertex array object
    glGenVertexArrays(1, & VAO);

    // vertex buffer object
    unsigned int VBO;
    glGenBuffers(1, & VBO);

    // element buffer object
    unsigned int EBO;
    glGenBuffers(1, & EBO);

    // bind all the things
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // vertices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    // colors
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void *) (3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // z-buffer
    glEnable(GL_DEPTH_TEST);

    // lines
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(2.0);
}

unsigned int text_shaderProgram;
unsigned int text_VAO;
unsigned int text_VBO;

void inittext() {
    // vertex shader
    const char * vertex_shader_code =
        "#version 330 core\n"
        "layout (location = 0) in vec4 vertex;\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "  gl_Position = projection * vec4(vertex.xy, 0.667, 1.0);\n"
        "  TexCoords = vertex.zw;\n"
        "}";
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, & vertex_shader_code, NULL);
    glCompileShader(vertexShader);
    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, & success);
    if (! success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << infoLog << endl;
        die("text vertex shader");
    }

    // fragment shader
    const char * fragment_shader_code = 
        "#version 330 core\n"
        "in vec2 TexCoords;\n"
        "out vec4 color;\n"
        "uniform sampler2D text;\n"
        "uniform vec3 textColor;\n"
        "void main() {\n"
        "  vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
        "  color = vec4(textColor, 1.0) * sampled;\n"
        "}";
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, & fragment_shader_code, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, & success);
    if (! success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << infoLog << endl;
        die("text fragment shader");
    }

    // shader program
    text_shaderProgram = glCreateProgram();
    glAttachShader(text_shaderProgram, vertexShader);
    glAttachShader(text_shaderProgram, fragmentShader);
    glLinkProgram(text_shaderProgram);
    glGetProgramiv(text_shaderProgram, GL_LINK_STATUS, & success);
    if (! success) die("text shader program");

    // delete shaders (unneeded after program link)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glm::mat4 projection = glm::ortho(0.0f, (float) SCREEN_WIDTH,
                                      0.0f, (float) SCREEN_HEIGHT);

    glUseProgram(text_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(text_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glGenVertexArrays(1, & text_VAO);
    glGenBuffers(1, & text_VBO);
    glBindVertexArray(text_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, text_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*6*4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int frame = 0;

void drawtetrahedron() {
    // transformation (rotate 1 degree per frame)
    glm::mat4 transform = glm::mat4(1.0); // identity matrix
    transform = glm::rotate(transform, (float) (frame*M_PI/360.0), glm::vec3(0,1,0));

    // enable shader with transform
    glUseProgram(shaderProgram);
    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

    // get outline uniform location
    unsigned int outlineLoc = glGetUniformLocation(shaderProgram, "outline");

    // draw faces
    glBindVertexArray(VAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glUniform1ui(outlineLoc, false);
    glDrawElements(GL_TRIANGLES, 4*3, GL_UNSIGNED_INT, 0);

    // draw edges
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUniform1ui(outlineLoc, true);
    glDrawElements(GL_TRIANGLES, 4*3, GL_UNSIGNED_INT, 0);
}

void drawtext(string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(text_shaderProgram);
    glUniform3f(glGetUniformLocation(text_shaderProgram, "textColor"),
                color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(text_VAO);

    // Iterate through all characters
    string::const_iterator c;
    for (c=text.begin() ; c!=text.end() ; c+=1)
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }
        };
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, text_VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

int BLIT_READY;

uint32_t timer_callback(uint32_t interval, void * param) {
    SDL_Event e;
    e.type = BLIT_READY;
    SDL_PushEvent(& e);

    return interval;
}

int main(int nargs, char * args[])
{
    init();

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    inittetrahedron();
    inittext();

    BLIT_READY = SDL_RegisterEvents(1);
    SDL_TimerID draw_timer_id = SDL_AddTimer(20, timer_callback, NULL); // timer tick every 20msec

    bool done = false;
    while (! done)
    {
        SDL_Event e;
        SDL_WaitEvent(& e); //TODO check for error

        if (e.type == SDL_QUIT) done = true;
        else if (e.type == BLIT_READY) {
            // background color
            glClearColor(0.2, 0.3, 0.3, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            drawtetrahedron();
            drawtext("Hello, World!", 275.0, 400.0, 1.0,
                     glm::vec3(0.9,0.9,0.9));
            SDL_GL_SwapWindow(gWindow);
            frame += 1;
        }
    }

    close();

    return 0;
}

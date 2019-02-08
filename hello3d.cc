#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <chipmunk.h>

extern "C" {
#include <SDL.h>
#include <gl\glew.h>
#include <SDL_opengl.h>
#include <gl\glu.h>

#include <cairo.h>
}

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;

char WINDOW_NAME[] = "Hello, World! Now in 3D!";
SDL_Window * gWindow = NULL;
SDL_GLContext gContext;

void die(string message) {
    cout << message << endl;
    exit(1);
}

// initialize SDL and OpenGL
void init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) die("SDL");
    if (! SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) die("texture");

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

    glewExperimental = GL_TRUE; 
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) die("glew");
}

void close()
{
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

int BLIT_READY;

/*
void drawstuff(cairo_t * cr) {
    // 0,0 at center of window and 1,1 at top right
    cairo_scale(cr, SCREEN_WIDTH/2.0, -SCREEN_HEIGHT/2.0);
    cairo_translate(cr, 1, -1);

    // set up physics
    cpVect gravity = cpv(0, -1);
    space = cpSpaceNew();
    cpSpaceSetGravity(space, gravity);

    // static body
    cpBody * body0 = cpSpaceAddBody(space, cpBodyNewStatic());
    cpBodySetPosition(body0, cpv(0, 1));

    // set font
    cairo_select_font_face(cr, "Georgia", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 0.2);
    cpFloat pad = 0.02;

    // "Hello,"
    cairo_text_extents_t te1;
    cairo_text_extents(cr, "Hello,", & te1);
    cpFloat mass1 = 1;
    cpFloat moment1 = cpMomentForBox(mass1, te1.width, te1.height);
    cpBody * body1 = cpSpaceAddBody(space, cpBodyNew(mass1, moment1/2));
    cpBodySetPosition(body1, cpv(0.5, 0.75));

    // "World!"
    cairo_text_extents_t te2;
    cairo_text_extents(cr, "World!", & te2);
    cpFloat mass2 = 1;
    cpFloat moment2 = cpMomentForBox(mass2, te2.width, te2.height);
    cpBody * body2 = cpSpaceAddBody(space, cpBodyNew(mass2, moment2/2));
    cpBodySetPosition(body2, cpv(0.5, 0.333));

    // springs
    cpSpaceAddConstraint(space, cpDampedSpringNew(body0, body1, cpv(0,0), cpv(0,0.01), 0.667, 30, 0.0001));
    cpSpaceAddConstraint(space, cpDampedSpringNew(body1, body2, cpv(0,-0.01), cpv(0,0.01), 0.667, 30, 0.0001));

    while (true) {
        for (int n=0 ; n<20 ; n+=1) cpSpaceStep(space, 1/1000.0);

        // clear screen
        cairo_rectangle(cr, -1, -1, 2, 2);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_fill(cr);

        // Hello,
        cpVect pos1 = cpBodyGetPosition(body1);
        cpFloat rot1 = cpBodyGetAngle(body1);

        cairo_save(cr);

        cairo_move_to(cr, pos1.x, pos1.y);
        cairo_scale(cr, 1, -1);
        cairo_rotate(cr, rot1);
        cairo_rel_move_to(cr, -te1.width/2, te1.height/2);
        cairo_text_path(cr, "Hello,");
        cairo_set_source_rgb(cr, 0,0,1);
        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 0.001);
        cairo_set_source_rgb(cr, 0,0,0);
        cairo_stroke(cr);

        cairo_restore(cr);

        // World!
        cpVect pos2 = cpBodyGetPosition(body2);
        cpFloat rot2 = cpBodyGetAngle(body2);

        cairo_save(cr);

        cairo_move_to(cr, pos2.x, pos2.y);
        cairo_scale(cr, 1, -1);
        cairo_rotate(cr, rot2);
        cairo_rel_move_to(cr, -te2.width/2, te2.height/2);
        cairo_text_path(cr, "World!");
        cairo_set_source_rgb(cr, 0,1,0);
        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 0.001);
        cairo_set_source_rgb(cr, 0,0,0);
        cairo_stroke(cr);

        cairo_restore(cr);

        SDL_Event e;
        e.type = BLIT_READY;
        SDL_PushEvent(& e);

        this_thread::sleep_for(chrono::milliseconds(20));
    }
}
*/

uint32_t timer_callback(uint32_t interval, void * param) {
    SDL_Event e;
    e.type = BLIT_READY;
    SDL_PushEvent(& e);

    return interval;
}

void drawstuff3d() {
    // background color
    glClearColor(0.2, 0.3, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // vertex shader
    const char * vertex_shader_code =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;"
        "layout (location = 1) in vec3 aColor;"
        "out vec3 ourColor;"
        "void main() {"
        "  gl_Position = vec4(aPos, 1.0);"
        "  ourColor = aColor;"
        "}";
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, & vertex_shader_code, NULL);
    glCompileShader(vertexShader);
    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, & success);
    if (! success) die("vertex shader");

    // fragment shader
    const char * fragment_shader_code = 
        "#version 330 core\n"
        "out vec4 FragColor;"
        "in vec3 ourColor;"
        "void main() {"
        "  FragColor = vec4(ourColor, 1.0f);"
        "}";
    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, & fragment_shader_code, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, & success);
    if (! success) die("fragment shader");

    // shader program
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, & success);
    if (! success) die("shader program");

    // delete shaders (unneeded after program link)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // vertex array object
    unsigned int VAO;
    glGenVertexArrays(1, & VAO);
    glBindVertexArray(VAO);

    // triangle (with vertex buffer object)
    float vertices[] = {
        -0.5, -0.5, 0.0, 1.0,0.0,0.0,
         0.5, -0.5, 0.0, 0.0,1.0,0.0,
         0.0,  0.5, 0.0, 0.0,0.0,1.0
    };
    unsigned int VBO;
    glGenBuffers(1, & VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // vertices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    // colors
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void *) (3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

int main(int nargs, char * args[])
{
    init();

    glViewport(0, 0, 800, 800);

    BLIT_READY = SDL_RegisterEvents(1);
    SDL_TimerID draw_timer_id = SDL_AddTimer(20, timer_callback, NULL);

    bool done = false;
    while (! done)
    {
        SDL_Event e;
        SDL_WaitEvent(& e); //TODO check for error

        if (e.type == SDL_QUIT) done = true;
        else if (e.type == BLIT_READY) {
            drawstuff3d();
            SDL_GL_SwapWindow(gWindow);
        }
    }

    close();

    return 0;
}

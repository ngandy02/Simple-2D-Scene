/**
* Author:  Andy Ng
* Assignment: Simple 2D Scene
* Date due: 2024-02-17, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               // We'll talk about these later in the course
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};


// Window dimensions
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

// Scaling factor
const float GROWTH_FACTOR = 1.05f;  // grow by 1.0% / frame
const float SHRINK_FACTOR = 0.95f;  // grow by -1.0% / frame
const int MAX_FRAMES = 60;

// Translation
float charizard_move = 1.0f;
float pikachu_move = 2.0f;

// Rotation angle
float rotation_angle = glm::radians(3.5f);


int g_frame_counter = 0;
bool g_is_growing = true;

// Background color components
const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

// Our viewport—or our "camera"'s—position and dimensions
const int VIEWPORT_X      = 0,
          VIEWPORT_Y      = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

//Shader giles necessary for textured game objects
const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float DEGREES_PER_SECOND = 90.0f;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

//texture identifications for both objects (needs to be global bc used in render and intitialise)
GLuint g_charizard_texture_id;
GLuint g_pikachu_texture_id;

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    //variables wheres stbi_load() function will stroes the dimensions and count of our image
    int width, height, number_of_components;
    //image loading function (stbi_load()): variables are passed as  arguments to itself or stbi_load() to get an image variable
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    //the raw image data is saved in image
    
    //quit if fails
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    // takeing the raw image data saved in image onto our video card memory in a form that it can actually use
    GLuint textureID;  //declaration
    glGenTextures(NUMBER_OF_TEXTURES, &textureID); //assignment
    glBindTexture(GL_TEXTURE_2D, textureID); // tell OpenGL to bind this ID to a 2-dimensional texture
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA,  //sending image to the graphics acrd by binding our texture ID with our raw image data
                 width, height,
                 TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE,
                 image);
    
    // STEP 3: Setting our texture filter parameters
    // the following two lines that set your desired mode in the case of magnification and minification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}

const char PLAYER1_SPRITE[] = "assets/charizard.png";
const char PLAYER2_SPRITE[] = "assets/pikachu.png";

// Our object's fill colour
const float TRIANGLE_RED     = 1.0,
            TRIANGLE_BLUE    = 0.4,
            TRIANGLE_GREEN   = 0.4,
            TRIANGLE_OPACITY = 1.0;

bool g_game_is_running = true;
SDL_Window* g_display_window;

ShaderProgram g_shader_program;
ShaderProgram g_charizard_program;
ShaderProgram g_pikachu_program;
glm::mat4 view_matrix, m_model_matrix, m_projection_matrix, m_trans_matrix;
float m_previous_ticks = 0.0f;



// overall position
glm::vec3 g_player_position = glm::vec3(0.0f, 0.0f, 0.0f);

// movement tracker
glm::vec3 g_player_movement = glm::vec3(0.0f, 0.0f, 0.0f);


void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id){
    g_charizard_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id); //bind texture
    glDrawArrays(GL_TRIANGLES, 0, 6); // now drawing 2 triangles, so use 6 intead of 3 vertices
    
}
float get_screen_to_ortho(float coordinate, Coordinate axis)
{
    switch (axis) {
        case x_coordinate:
            return ((coordinate / WINDOW_WIDTH) * 10.0f ) - (10.0f / 2.0f);
        case y_coordinate:
            return (((WINDOW_HEIGHT - coordinate) / WINDOW_HEIGHT) * 7.5f) - (7.5f / 2.0);
        default:
            return 0.0f;
    }
}

glm::mat4 g_view_matrix,        // Defines the position (location and orientation) of the camera
          g_projection_matrix; // Defines the characteristics of your camera, such as clip panes, field of view, projection method, etc.
glm::mat4 g_charizard_matrix, g_pikachu_matrix = glm::mat4(1.0f);   // Defines every translation, rotation, and/or scaling applied to an object; we'll look at these next week
         
const float INIT_TRIANGLE_ANGLE = glm::radians(45.0);



void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello, Triangle!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
    // Load up our shaders
    g_charizard_program.load(V_SHADER_PATH, F_SHADER_PATH);
    g_pikachu_program.load(V_SHADER_PATH, F_SHADER_PATH);
    

   
    
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // Initialise our camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    // Initialise our view, model, and projection matrices
    g_view_matrix       = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    
    
    g_charizard_program.set_projection_matrix(g_projection_matrix);
    g_pikachu_program.set_projection_matrix(g_projection_matrix);
    
    g_charizard_program.set_view_matrix(g_view_matrix);
    g_pikachu_program.set_view_matrix(g_view_matrix);
    
    
    // Each object has its own unique ID
    glUseProgram(g_charizard_program.get_program_id());
    glUseProgram(g_pikachu_program.get_program_id());
    
 
    // Binds texture id
    g_charizard_texture_id = load_texture(PLAYER1_SPRITE);
    g_pikachu_texture_id = load_texture(PLAYER2_SPRITE);
    
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
}

void process_input()
{
    g_player_movement = glm::vec3(0.0f);
    
    SDL_Event event;
    
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_WINDOWEVENT_CLOSE:
            case SDL_QUIT:
                g_game_is_running = false;
                break;
        }
    }
}
void update()
{
    
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - m_previous_ticks; // the delta time is the difference from the last frame
    m_previous_ticks = ticks;

    // this entire sections moves the shapes
    
    // Add             direction       * elapsed time * units per second
    g_player_position += g_player_movement * delta_time * 500.0f;


    glm::vec3 scale_vector;
    g_frame_counter += 1;
    
    //increases translation over time
    charizard_move += 0.3f * delta_time;
    
    // resets it
    g_charizard_matrix = glm::mat4(1.0f);
    g_pikachu_matrix = glm::mat4(1.0f);
    
    // increases rotation over time
    rotation_angle += 0.25f * delta_time;
    g_charizard_matrix = glm::mat4(1.0f);
    
    
    
    
    // Every frame, rotate my model by an angle of rotation_angle on the z-axis
    g_charizard_matrix = glm::rotate(g_charizard_matrix, rotation_angle, glm::vec3(0.0f, 0.0f, 1.0f));
    g_pikachu_matrix = glm::rotate(g_pikachu_matrix, rotation_angle, glm::vec3(0.0f, 0.0f, 1.0f));

    if (g_frame_counter >= MAX_FRAMES)
    {
        g_is_growing = !g_is_growing;
        g_frame_counter = 0;
    }

    scale_vector = glm::vec3(g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                             g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                             1.0f);

    // translates the shape
    g_charizard_matrix = glm::translate(g_charizard_matrix, glm::vec3(charizard_move, 0.0f, 0.0f));
    g_pikachu_matrix = glm::translate(g_charizard_matrix, glm::vec3(pikachu_move,1.0f, 0.0f));


    // scales the shape. EXTRA CREDIT
    g_charizard_matrix = glm::scale(g_charizard_matrix, scale_vector);
    g_pikachu_matrix = glm::scale(g_pikachu_matrix, scale_vector);
}




void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    
    // Vertices
     float vertices[] = {
         -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
         -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
     };

     // Textures
     float texture_coordinates[] = {
         0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
         0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
     };
    
    // charizard object
    glVertexAttribPointer(g_charizard_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices); // vertices
    glEnableVertexAttribArray(g_charizard_program.get_position_attribute());
    
    glVertexAttribPointer(g_charizard_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates); // texture
    glEnableVertexAttribArray(g_charizard_program.get_tex_coordinate_attribute());
    
    //these three lines actually draw the object on the screen
    draw_object(g_charizard_matrix, g_charizard_texture_id);
   
   
    // disable two attribute arrays now
    glDisableVertexAttribArray(g_charizard_program.get_position_attribute());
    glDisableVertexAttribArray(g_charizard_program.get_tex_coordinate_attribute());
    
    // pikachu object
    glVertexAttribPointer(g_pikachu_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices); //vertices
    glEnableVertexAttribArray(g_pikachu_program.get_position_attribute());
    
    glVertexAttribPointer(g_pikachu_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates); // texture
    glEnableVertexAttribArray(g_pikachu_program.get_tex_coordinate_attribute());
    
    //these three lines actually draw the object on the screen
    draw_object(g_pikachu_matrix, g_pikachu_texture_id);

    
    glDisableVertexAttribArray(g_charizard_program.get_position_attribute());
    glDisableVertexAttribArray(g_charizard_program.get_tex_coordinate_attribute());
    
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }
    shutdown();
    return 0;
    
}

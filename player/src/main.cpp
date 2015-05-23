/**
* Project: VSXu: Realtime modular visual programming language, music/audio visualizer.
*
* This file is part of Vovoid VSXu.
*
* @author Jonatan Wallmander, Robert Wenzel, Vovoid Media Technologies AB Copyright (C) 2003-2013
* @see The GNU Public License (GPL)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <stdio.h>
#include "application.h"
#include <GL/glew.h>

#define GLFW_INCLUDE_GLU
#include "GLFW/glfw3.h"

#include <container/vsx_nw_vector.h>
#include <string/vsx_string.h>
#include <string/vsx_string_helper.h>
#include <vsxfst.h>
#include <vsx_version.h>
#include <stdlib.h>
#include "vsx_platform.h"
#include <vsx_gl_state.h>

// implementation of app externals
bool app_ctrl = false;
bool app_alt = false;
bool app_shift = false;
bool disable_randomizer = false;
bool option_preload_all = false;
bool no_overlay = false;

int app_argc = 0;
char** app_argv;

GLFWwindow* mainWindow = NULL;


void set_modifiers()
{
  app_ctrl = (bool)glfwGetKey(mainWindow, GLFW_KEY_LEFT_CONTROL);
  app_shift = (bool)glfwGetKey(mainWindow, GLFW_KEY_LEFT_SHIFT);
  app_alt = (bool)glfwGetKey(mainWindow, GLFW_KEY_LEFT_ALT);
}


long key_pressed = -1;
int key_character = -1;
float key_time;
float key_repeat_time;
float initial_key_delay = 0.04f;

void key_char_event( GLFWwindow* window, unsigned int character )
{
  VSX_UNUSED(window);
//  if (action == GLFW_PRESS)
//  {
    app_char(character);
    key_character = character;
//  }
}

void app_close_window()
{
  glfwDestroyWindow(mainWindow);
}

void key_event(GLFWwindow* window, int key, int scancode, int action, int modifier)
{
  VSX_UNUSED(window);
  VSX_UNUSED(scancode);
  VSX_UNUSED(modifier);

  set_modifiers();
  if (action == GLFW_PRESS)
  {
    app_key_down((long)key);
    key_character = -1;
    key_time = 0.0f;
    key_repeat_time = 0.0f;
    key_pressed = key;
    initial_key_delay = 0.04f;
  }
  if (action == GLFW_RELEASE)
  {
    app_key_up((long)key);
    key_pressed = -1;
  }
}

double last_x = 0, last_y = 0;
int mouse_state = 0;

void mouse_button_event(GLFWwindow* window, int button, int action, int modifiers)
{
  VSX_UNUSED(window);
  VSX_UNUSED(modifiers);

  glfwGetCursorPos(mainWindow, &last_x, &last_y);
  set_modifiers();
  unsigned long i_but = 0;
  switch (button)
  {
    case GLFW_MOUSE_BUTTON_MIDDLE: i_but = 1; break;
    case GLFW_MOUSE_BUTTON_RIGHT: i_but = 2;
  }
  if (action == GLFW_PRESS)
  {
    mouse_state = 1; // pressed
    app_mouse_down(i_but, last_x, last_y);
  }
  else
  {
    mouse_state = 0; // depression
    app_mouse_up(i_but, last_x, last_y);
  }
}

int mouse_pos_type = 0;

void mouse_pos_event(GLFWwindow* window, double x, double y)
{
  VSX_UNUSED(window);
  VSX_UNUSED(x);
  VSX_UNUSED(y);
  set_modifiers();
  glfwGetCursorPos(mainWindow, &last_x, &last_y);
  if (mouse_state) mouse_pos_type = 1;
  else mouse_pos_type = 2;
}


int mousewheel_prev_pos = 0;

void mouse_wheel(GLFWwindow* window, double xoffset, double yoffset)
{
  VSX_UNUSED(window);
  VSX_UNUSED(xoffset);
  set_modifiers();
  //int pos = yoffset;
  //app_mousewheel((float)(pos-mousewheel_prev_pos),last_x,last_y);
  app_mousewheel(yoffset,last_x,last_y);
  //mousewheel_prev_pos = pos;
}

void window_size(GLFWwindow* window, int width, int height )
{
  VSX_UNUSED(window);
  vsx_gl_state::get_instance()->viewport_change(0,0,width, height);
}

//========================================================================
// main()
//========================================================================

int main(int argc, char* argv[])
{
  app_argc = argc;
  app_argv = argv;
  int     width, height, frames;
  double  x, y;
  double  t, t1;
  char    titlestr[ 200 ];

  // Initialise GLFW
  glfwInit();

  bool start_fullscreen = false;
  int x_res = 1280;
  int y_res = 720;
  bool manual_resolution_set = false;
  for (int i = 1; i < argc; i++)
  {
    vsx_string<>arg1 = argv[i];
    if (arg1 == "--help" || arg1 == "/?" || arg1 == "-help" || arg1 == "-?")
    {
      printf(
             "Usage:\n"
          "  vsxu_player [path_to_vsx_file]\n"
          "\n"
          "Flags: \n"
          "  -pl        Preload all visuals on start \n"
          "  -dr        Disable randomizer     \n"
          "  -p [x,y]   Set window position x,y \n"
          "  -s [x,y]   Set window size x,y \n\n\n"
            );
      exit(0);
    } else
    if (arg1 == "-f") {
      start_fullscreen = true;
    } else
    if (arg1 == "-pl") {
      option_preload_all = true;
    } else
    if (arg1 == "-dr") {
      disable_randomizer = true;
    } else
    if (arg1 == "-no") {
      no_overlay = true;
    } else
    if (arg1 == "-s")
    {
      if (i+1 < argc)
      {
        i++;
        vsx_string<>arg2 = argv[i];
        vsx_nw_vector< vsx_string<> > parts;
        vsx_string<>deli = ",";
        explode(arg2, deli, parts);
        if (parts.size() == 2)
        {
          x_res = vsx_string_helper::s2i(parts[0]);
          y_res = vsx_string_helper::s2i(parts[1]);
          manual_resolution_set = true;
        } else
        {
          deli = "x";
          explode(arg2, deli, parts);
          if ( parts.size() == 2 )
          {
            x_res = vsx_string_helper::s2i(parts[0]);
            y_res = vsx_string_helper::s2i(parts[1]);
            manual_resolution_set = true;
          }
        }
      }
    }
  }
  if (start_fullscreen && !manual_resolution_set)
  {
    // try to get the resolution from the desktop for fullscreen
      const GLFWvidmode* video_mode = glfwGetVideoMode( glfwGetPrimaryMonitor() );
      x_res = video_mode->height;
      y_res = video_mode->width;
  }
  
  // Open OpenGL window
//  glfwOpenWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

  mainWindow = glfwCreateWindow( x_res, y_res, "VSXu Player", start_fullscreen? glfwGetPrimaryMonitor() : NULL, NULL );
  if( !mainWindow ) // GLFW_FULLSCREEN
  {
    vsx_printf(L"Error! Could not create an OpenGL context. Please check your GPU drivers...\n");
    glfwTerminate();
    return 0;
  }
  glfwMakeContextCurrent(mainWindow);


  glewInit();

  if (start_fullscreen)
    glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL );

  printf("INFO: app_init\n");
  app_init(0);
  printf("INFO: app_init done\n");

//  glfwEnable(GLFW_AUTO_POLL_EVENTS);

  for (int i = 1; i < argc; i++) {
    vsx_string<>arg1 = argv[i];
    if (arg1 == "-p") {
      if (i+1 < argc)
      {
        i++;
        vsx_string<>arg2 = argv[i];
        vsx_nw_vector< vsx_string<> > parts;
        vsx_string<>deli = ",";
        explode(arg2, deli, parts);
        glfwSetWindowPos( mainWindow, vsx_string_helper::s2i(parts[0]), vsx_string_helper::s2i(parts[1]) );
      }
    }
  }

  set_modifiers();
  glfwSetCharCallback(mainWindow, &key_char_event);
  glfwSetKeyCallback(mainWindow, &key_event);
  glfwSetMouseButtonCallback(mainWindow, &mouse_button_event);
  glfwSetCursorPosCallback(mainWindow, &mouse_pos_event);
  glfwSetScrollCallback(mainWindow, &mouse_wheel);
  // set window size callback function
  glfwSetWindowSizeCallback(mainWindow, window_size);

  // Enable sticky keys
  glfwSetInputMode(mainWindow, GLFW_STICKY_KEYS, GL_TRUE );

  glfwSwapInterval(1);

  // Main loop
  frames = 0;

  #if PLATFORM_FAMILY == PLATFORM_FAMILY_UNIX
    sprintf( titlestr, "Vovoid VSXu Player %s [GNU/Linux %d-bit]", vsxu_ver, PLATFORM_BITS);
  #endif
  #if PLATFORM_FAMILY == PLATFORM_FAMILY_WINDOWS
    sprintf( titlestr, "Vovoid VSXu Player %s [Windows %d-bit]", vsxu_ver, PLATFORM_BITS);
  #endif
  glfwSetWindowTitle( mainWindow, titlestr );


  while( !glfwWindowShouldClose(mainWindow) )
  {
    if (mouse_pos_type)
    {
      if (mouse_pos_type == 1) app_mouse_move(last_x,last_y);
      else app_mouse_move_passive(last_x,last_y);
      mouse_pos_type = 0;
    }


    app_pre_draw();

    // Get time and mouse position
    t = glfwGetTime();
    glfwGetCursorPos( mainWindow, &x, &y );
    float delta = t-t1;
    t1 = t;
    if (key_pressed != -1)
    {
          //printf("%f\n", delta);
      key_time += delta;
      if (key_time > 0.3f)
      {
        key_repeat_time += delta;
        if (key_repeat_time > initial_key_delay)
        {
          key_repeat_time = 0.0f;
          if (key_character != -1)
            app_char(key_character);
          app_key_down((long)key_pressed);
          initial_key_delay *= 0.99f;
              //printf("repeating key: %d\n", key_character);
        }
      }
    }
    frames ++;

    // TODO: verify if glfwGetFramebufferSize(mainWindow, &width, &height); is more apt here
    glfwGetWindowSize( mainWindow, &width, &height );
    height = height > 0 ? height : 1;

    // Set viewport
    vsx_gl_state::get_instance()->viewport_set( 0, 0, width, height );

    // Clear color buffer
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    app_draw(0);


    glfwSwapBuffers(mainWindow);
    glfwPollEvents();
  }

  // Close OpenGL window and terminate GLFW
  glfwTerminate();

  return 0;
}

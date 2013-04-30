/*
 * Copyright (c) 2013, Jasper Ruoff <jruoff@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "environment_2d.h"

#include <GL/glut.h>
#include <algorithm>

static IDrawable *s_drawable;

static double s_scale;
static int s_translate_x;
static int s_translate_y;

static int s_window_width  = 800;
static int s_window_height = 800;

static void reset()
{
	s_translate_x = -s_drawable->size() / 2;
	s_translate_y = -s_drawable->size() / 2;
	s_scale = std::min(1.,
		std::min(s_window_width, s_window_height)
		/ s_drawable->size() / 1.05
	);
}

static void reshape(int width, int height)
{
	s_window_width  = width;
	s_window_height = height;
	
	glViewport(0, 0, width, height);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(-width/2., width/2., height/2., -height/2.);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void display()
{
	glClear(GL_COLOR_BUFFER_BIT);


	glPushMatrix();
		glScalef(s_scale, s_scale, 0);

		glPushMatrix();
			glTranslatef(s_translate_x, s_translate_y, 0);
			s_drawable->draw(s_scale);
		glPopMatrix();

		if (s_scale > 1/3.) {
			glBegin(GL_LINE_STRIP);
			glColor4f(0, 1, 1, .6 * sqrt(std::max(.0, 3 * s_scale - 1)));
			glVertex2i(-20, -20); glVertex2i( 20, -20);
			glVertex2i( 20,  20); glVertex2i(-20,  20);
			glVertex2i(-20, -20);
			glEnd();
		}
	glPopMatrix();


	glBegin(GL_LINES);

	glColor4f(0, 1, 1, .6);
	glVertex2i(-20, 0); glVertex2i(20, 0);
	glVertex2i(0, -20); glVertex2i(0, 20);

	glColor4f(0, 1, 1, .2);
	glVertex2i(-s_window_width/2., 0); glVertex2i(s_window_width/2., 0);
	glVertex2i(0, -s_window_height/2.); glVertex2i(0, s_window_width/2.);

	glEnd();


	glutSwapBuffers();
}

static void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case  8: reset(); break; // BACKSPACE
	case 27: exit(0); break; // ESCAPE
	}

	glutPostRedisplay();
}

static int s_old_x, s_old_y;

static void mouse(int button, int state, int x, int y)
{
	if (state == GLUT_UP) return;
	
	switch (button) {
		case 3: // scroll up
			s_scale = std::min(1., s_scale * 1.2);
			break;
			
		case 4: // scroll down
			s_scale /= 1.2;
			break;
			
		default:
			s_old_x = s_translate_x*s_scale - x;
			s_old_y = s_translate_y*s_scale - y;
	}
	
	glutPostRedisplay();
}

static void motion(int x, int y)
{
	s_translate_x = (s_old_x + x)/s_scale;
	s_translate_y = (s_old_y + y)/s_scale;
	glutPostRedisplay();
}

void Environment2D::init(int &argc, char **argv)
{
	glutInit(&argc, argv);
}

void Environment2D::set_drawable(IDrawable *drawable)
{
	s_drawable = drawable;
}

void Environment2D::start()
{
	reset();
	
	glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_ALPHA);
	
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(s_window_width, s_window_height);
	glutCreateWindow("CloneGrid (c) 2013, Jasper Ruoff");
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0,0,0,0);

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glutMainLoop();
}

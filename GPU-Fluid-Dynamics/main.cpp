/*
* Copyright (c) 2017 Owen Glofcheski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
*    1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
*
*    2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
*    3. This notice may not be removed or altered from any source
*    distribution.
*/

#include <ctime>
#include <GL\glew.h>
#include <GL\freeglut.h>
#include <vector>

#include "Utility\algebra.hpp"
#include "Utility\colour.hpp"
#include "Utility\flip_buffer.hpp"
#include "Utility\gl.hpp"
#include "Utility\quaternion.hpp"

struct AdvectionShader : public Shader
{
	Uniform velocity_uniform = -1;
	Uniform colour_uniform = -1;
};

struct RenderShader : public Shader
{
	Uniform colour_uniform = -1;
};

struct State
{
	int window = 0;

	Size canvas_size;

	AdvectionShader advection_shader;
	RenderShader render_shader;

	FlipBuffer velocity_texture;
	FlipBuffer colour_texture;

	FlipBuffer colour_buffer;
};

State state;

void update() {
	glUseProgram(state.advection_shader.program);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, state.velocity_texture.front());
	glUniform1i(state.advection_shader.velocity_uniform, 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, state.colour_texture.front());
	glUniform1i(state.advection_shader.colour_uniform, 2);

	// Render to the back buffer.
	glBindFramebuffer(GL_FRAMEBUFFER, state.colour_buffer.back());
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, (GLenum*)buffers);

	glDrawRect(-1, 1, -1, 1, 0);

	// TODO(orglofch): Update velocity based on pressure.

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(state.render_shader.program);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, state.colour_texture.back());
	glUniform1i(state.render_shader.colour_uniform, 1);

	glDrawRect(-1, 1, -1, 1, 0);

	glutSwapBuffers();
	glutPostRedisplay();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void tick() {
	update();
	render();

	state.colour_texture.flip();
	state.colour_buffer.flip();
}

void handleMouseButton(int button, int button_state, int x, int y) {
	switch (button) {
		case GLUT_LEFT_BUTTON:
			break;
		case GLUT_RIGHT_BUTTON:
			break;
	}
}

void handlePressNormalKeys(unsigned char key, int x, int y) {
	switch (key) {
		case 'q':
		case 'Q':
		case 27:
			exit(EXIT_SUCCESS);
	}
}

void handleReleaseNormalKeys(unsigned char key, int x, int y) {
}

void handlePressSpecialKey(int key, int x, int y) {
}

void handleReleaseSpecialKey(int key, int x, int y) {
}


void init() {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_NORMALIZE);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glutDisplayFunc(tick);

	glutIgnoreKeyRepeat(1);
	glutMouseFunc(handleMouseButton);
	glutKeyboardFunc(handlePressNormalKeys);
	glutKeyboardUpFunc(handleReleaseNormalKeys);
	glutSpecialFunc(handlePressSpecialKey);
	glutSpecialUpFunc(handleReleaseSpecialKey);

	// Advection shader.
	state.advection_shader.program = glLoadShader("advection.vert", "advection.frag");
	state.advection_shader.velocity_uniform = glGetUniform(state.advection_shader, "velocity");
	state.advection_shader.colour_uniform = glGetUniform(state.advection_shader, "colour");

	// Render shader.
	state.render_shader.program = glLoadShader("render.vert", "render.frag");
	state.render_shader.colour_uniform = glGetUniform(state.render_shader, "colour");

	// Velocity texture.
	glGenTextures(2, state.velocity_texture.buffers);
	GLfloat *velocity_data = new GLfloat[state.canvas_size.area() * 4]();
	glBindTexture(GL_TEXTURE_2D, state.velocity_texture.back());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4 ? GL_RGBA32F : GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGBA, GL_FLOAT, velocity_data);
	for (int x = 0; x < state.canvas_size.width; ++x) {
		for (int y = 0; y < state.canvas_size.height; ++y) {
			int i = (x + y * state.canvas_size.width) * 4;
			velocity_data[i + 0] = sin(2 * PI * y / 200) / 300;
			velocity_data[i + 1] = sin(2 * PI * x / 200) / 300;
			velocity_data[i + 2] = 0;
			velocity_data[i + 3] = 1;
		}
	}
	glBindTexture(GL_TEXTURE_2D, state.velocity_texture.front());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4 ? GL_RGBA32F : GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGBA, GL_FLOAT, velocity_data);
	delete[] velocity_data;

	// Colour texture.
	glGenTextures(2, state.colour_texture.buffers);
	GLfloat *colour_data = new GLfloat[state.canvas_size.area() * 4]();
	glBindTexture(GL_TEXTURE_2D, state.colour_texture.back());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4 ? GL_RGBA32F : GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGBA, GL_FLOAT, colour_data);
	for (int x = 0; x < state.canvas_size.width; ++x) {
		for (int y = 0; y < state.canvas_size.height; ++y) {
			int i = (x + y * state.canvas_size.width) * 4;
			colour_data[i + 0] = ((x + y) % 100) < 50;
			colour_data[i + 1] = (x % 100) < 50;
			colour_data[i + 2] = (y % 100) < 50;
			colour_data[i + 3] = 1;
		}
	}
	glBindTexture(GL_TEXTURE_2D, state.colour_texture.front());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4 ? GL_RGBA32F : GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGBA, GL_FLOAT, colour_data);
	delete[] colour_data;

	glGenFramebuffers(2, state.colour_buffer.buffers);
	// Front buffer.
	glBindFramebuffer(GL_FRAMEBUFFER, state.colour_buffer.front());
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, state.colour_texture.front(), 0);
	// Back buffer.
	glBindFramebuffer(GL_FRAMEBUFFER, state.colour_buffer.back());
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, state.colour_texture.back(), 0);

	glBindBuffer(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void cleanup() {
	glDeleteTextures(2, state.velocity_texture.buffers);
	glDeleteTextures(2, state.colour_texture.buffers);
	glDeleteFramebuffers(2, state.colour_buffer.buffers);
}

int main(int argc, char **argv) {
	srand((unsigned int)time(NULL));

	state.canvas_size = Size(400, 400);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(state.canvas_size.width, state.canvas_size.height);
	state.window = glutCreateWindow("GPU Fluid Dynamics");
	glewInit();

	init();

	glutMainLoop();

	cleanup();
}
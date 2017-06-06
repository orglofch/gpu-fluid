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

#include <algorithm>
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

	Uniform grid_size_uniform = -1;

	Uniform mouse_position_uniform = -1;
	Uniform mouse_impulse_uniform = -1;
	Uniform impulse_radius_uniform = -1;

	Uniform timestep_uniform = -1;
};

struct RenderShader : public Shader
{
	Uniform velocity_uniform = -1;
	Uniform colour_uniform = -1;

	Uniform grid_size_uniform = -1;
};

struct VectorFieldShader : public Shader
{
	Uniform vector_field_uniform = -1;
	Uniform colour_uniform = -1;
};

struct DivergenceShader : public Shader
{
	Uniform velocity_uniform = -1;

	Uniform grid_size_uniform = -1;

	Uniform timestep_uniform = -1;
};

struct PressureShader : public Shader
{
	Uniform pressure_uniform = -1;
	Uniform divergence_uniform = -1;

	Uniform grid_size_uniform = -1;
};

struct VelocityNormalizationShader : public Shader
{
	Uniform velocity_uniform = -1;
	Uniform pressure_uniform = -1;

	Uniform grid_size_uniform = -1;

	Uniform timestep_uniform = -1;
};

struct State
{
	int window = 0;

	Size canvas_size;

	AdvectionShader advection_shader;
	RenderShader render_shader;
	VectorFieldShader vector_field_shader;
	DivergenceShader divergence_shader;
	PressureShader pressure_shader;
	VelocityNormalizationShader velocity_normalization_shader;

	FlipBuffer velocity_texture;
	FlipBuffer colour_texture;
	GLuint divergence_texture;
	FlipBuffer pressure_texture;

	GLuint advection_buffer;
	GLuint divergence_buffer;
	GLuint pressure_buffer;
	GLuint velocity_normalization_buffer;

	bool render_velocity_indicators = false;
	bool render_pressure_indicators = false;
	int indicator_render_inverval = 10; // pixels.

	float mouse_impulse_radius = 40; // pixels.

	Point2 last_mouse_pos;
	Vector2 mouse_frame_impulse;
};

State state;

void renderIndicators(GLuint vector_field, Colour colour) {
	glUseProgram(state.vector_field_shader.program);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, vector_field);
	glUniform1i(state.vector_field_shader.vector_field_uniform, 1);
	glUniform4f(state.vector_field_shader.colour_uniform, colour.r, colour.g, colour.b, 1);

	Size triangle_grid = state.canvas_size / state.indicator_render_inverval;

	float *triangle_data = new float[triangle_grid.area() * 6];
	for (int r = 0; r < triangle_grid.height; ++r) {
		for (int c = 0; c < triangle_grid.width; ++c) {
			int triangle_index = (c + r * triangle_grid.width) * 6;

			triangle_data[triangle_index + 0]
				= triangle_data[triangle_index + 2]
				= triangle_data[triangle_index + 4]
				= 2.0f * c / (triangle_grid.width - 1) - 1.0f;
			triangle_data[triangle_index + 1]
				= triangle_data[triangle_index + 3]
				= triangle_data[triangle_index + 5]
				= 2.0f * r / (triangle_grid.height - 1) - 1.0f;
		}
	}

	GLuint vertex_buffer;

	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * triangle_grid.area() * 6, triangle_data, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLES, 0, triangle_grid.area() * 4);

	delete[] triangle_data;

	glDeleteBuffers(1, &vertex_buffer);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void advect() {
	glBindFramebuffer(GL_FRAMEBUFFER, state.advection_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, state.colour_texture.back(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
		GL_TEXTURE_2D, state.velocity_texture.back(), 0);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, (GLenum*)buffers);

	glUseProgram(state.advection_shader.program);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, state.velocity_texture.front());
	glUniform1i(state.advection_shader.velocity_uniform, 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, state.colour_texture.front());
	glUniform1i(state.advection_shader.colour_uniform, 2);
	glUniform2f(state.advection_shader.grid_size_uniform,
		state.canvas_size.x, state.canvas_size.y);
	glUniform2f(state.advection_shader.mouse_position_uniform,
		state.last_mouse_pos.x, state.last_mouse_pos.y);
	glUniform2f(state.advection_shader.mouse_impulse_uniform,
		state.mouse_frame_impulse.x, state.mouse_frame_impulse.y);
	glUniform1f(state.advection_shader.impulse_radius_uniform, state.mouse_impulse_radius);
	glUniform1f(state.advection_shader.timestep_uniform, 4);

	glDrawRect(-1, 1, -1, 1, 0);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	state.velocity_texture.flip();
	state.colour_texture.flip();
}

void calculateDivergence() {
	glBindFramebuffer(GL_FRAMEBUFFER, state.divergence_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, state.divergence_texture, 0);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, (GLenum*)buffers);

	glUseProgram(state.divergence_shader.program);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, state.velocity_texture.front());
	glUniform1i(state.divergence_shader.velocity_uniform, 1);
	glUniform2f(state.divergence_shader.grid_size_uniform,
		state.canvas_size.x, state.canvas_size.y);
	glUniform1f(state.divergence_shader.timestep_uniform, 1 / 60.0);

	glDrawRect(-1, 1, -1, 1, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}

void calculatePressure() {
	int jacobi_iterations = 200;

	// Re-zero the pressure texture.
	GLfloat *pressure_data = new GLfloat[state.canvas_size.area() * 3]();
	glBindTexture(GL_TEXTURE_2D, state.pressure_texture.front());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGB, GL_FLOAT, pressure_data);
	delete[] pressure_data;
	
	glUseProgram(state.pressure_shader.program);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, state.divergence_texture);
	glUniform1i(state.pressure_shader.divergence_uniform, 1);
	glUniform2f(state.pressure_shader.grid_size_uniform,
		state.canvas_size.x, state.canvas_size.y);

	for (int i = 0; i < jacobi_iterations; ++i) {
		glBindFramebuffer(GL_FRAMEBUFFER, state.pressure_buffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, state.pressure_texture.back(), 0);
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, (GLenum*)buffers);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, state.pressure_texture.front());
		glUniform1i(state.pressure_shader.pressure_uniform, 2);

		glDrawRect(-1, 1, -1, 1, 0);

		// Flip the input and output textures.
		state.pressure_texture.flip();
	}

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}

void normalizeVelocity() {
	glBindFramebuffer(GL_FRAMEBUFFER, state.velocity_normalization_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, state.velocity_texture.back(), 0);
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, (GLenum*)buffers);

	glUseProgram(state.velocity_normalization_shader.program);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, state.velocity_texture.front());
	glUniform1i(state.velocity_normalization_shader.velocity_uniform, 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, state.pressure_texture.front());
	glUniform1i(state.velocity_normalization_shader.pressure_uniform, 2);
	glUniform2f(state.velocity_normalization_shader.grid_size_uniform,
		state.canvas_size.x, state.canvas_size.y);
	glUniform1f(state.velocity_normalization_shader.timestep_uniform, 1 / 60.0);

	glDrawRect(-1, 1, -1, 1, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	state.velocity_texture.flip();
}

void update() {
	advect();
	calculateDivergence();
	calculatePressure();
	normalizeVelocity();

	// Reset the frame impulse.
	state.mouse_frame_impulse = Vector2(0, 0);
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(state.render_shader.program);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, state.colour_texture.front());
	glUniform1i(state.render_shader.colour_uniform, 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, state.velocity_texture.front());
	glUniform1i(state.render_shader.velocity_uniform, 2);
	glUniform2f(state.render_shader.grid_size_uniform,
		state.canvas_size.x, state.canvas_size.y);

	glDrawRect(-1, 1, -1, 1, 0);

	if (state.render_velocity_indicators) {
		renderIndicators(state.velocity_texture.front(), Colour(1, 0, 0));
	}

	if (state.render_pressure_indicators) {
		renderIndicators(state.pressure_texture.front(), Colour(0, 0, 1));
	}

	glutSwapBuffers();
	glutPostRedisplay();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void tick() {
	update();
	render();
}

void handleMouseMove(int x, int y) {
	Point2 new_mouse_pos(x, state.canvas_size.height - y);

	state.mouse_frame_impulse = (new_mouse_pos - state.last_mouse_pos) / 10;
	state.last_mouse_pos = new_mouse_pos;
}

void handleMouseButton(int button, int button_state, int x, int y) {
	switch (button) {
		case GLUT_LEFT_BUTTON:
			if (button_state == GLUT_DOWN) {
				state.last_mouse_pos = Point2(x, state.canvas_size.height - y);
			}
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
		case 'v':
		case 'V':
			state.render_velocity_indicators =
				!state.render_velocity_indicators;
			break;
		case 'f':
		case 'F':
			state.render_pressure_indicators =
				!state.render_pressure_indicators;
			break;
		case 'c':
		case 'C':
			state.indicator_render_inverval = std::max(1, state.indicator_render_inverval - 1);
			break;
		case 'b':
		case 'B':
			state.indicator_render_inverval++;
			break;
	}
}

void init() {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_NORMALIZE);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glutDisplayFunc(tick);

	glutIgnoreKeyRepeat(1);
	glutMotionFunc(handleMouseMove);
	glutMouseFunc(handleMouseButton);
	glutKeyboardFunc(handlePressNormalKeys);

	// Advection shader.
	state.advection_shader.program = glLoadShader("advection.vert", "advection.frag");
	state.advection_shader.velocity_uniform = glGetUniform(state.advection_shader, "velocity_sampler");
	state.advection_shader.colour_uniform = glGetUniform(state.advection_shader, "colour_sampler");
	state.advection_shader.grid_size_uniform = glGetUniform(state.advection_shader, "grid_size");
	state.advection_shader.mouse_position_uniform = glGetUniform(state.advection_shader, "mouse_position");
	state.advection_shader.mouse_impulse_uniform = glGetUniform(state.advection_shader, "mouse_impulse");
	state.advection_shader.impulse_radius_uniform = glGetUniform(state.advection_shader, "impulse_radius");
	state.advection_shader.timestep_uniform = glGetUniform(state.advection_shader, "timestep");

	// Render shader.
	state.render_shader.program = glLoadShader("render.vert", "render.frag");
	state.render_shader.velocity_uniform = glGetUniform(state.render_shader, "velocity_sampler");
	state.render_shader.colour_uniform = glGetUniform(state.render_shader, "colour_sampler");
	state.render_shader.grid_size_uniform = glGetUniform(state.render_shader, "grid_size");

	// Velocity Indicator shader.
	state.vector_field_shader.program = glLoadShader("vector_field.vert", "vector_field.frag");
	state.vector_field_shader.vector_field_uniform = glGetUniform(state.vector_field_shader, "vector_field");
	state.vector_field_shader.colour_uniform = glGetUniform(state.vector_field_shader, "colour");

	// Divergence shader.
	state.divergence_shader.program = glLoadShader("divergence.vert", "divergence.frag");
	state.divergence_shader.velocity_uniform = glGetUniform(state.divergence_shader, "velocity_sampler");
	state.divergence_shader.grid_size_uniform = glGetUniform(state.divergence_shader, "grid_size");
	state.divergence_shader.timestep_uniform = glGetUniform(state.divergence_shader, "timestep");

	// Pressure shader.
	state.pressure_shader.program = glLoadShader("pressure.vert", "pressure.frag");
	state.pressure_shader.divergence_uniform = glGetUniform(state.pressure_shader, "divergence_sampler");
	state.pressure_shader.grid_size_uniform = glGetUniform(state.pressure_shader, "grid_size");
	state.pressure_shader.pressure_uniform = glGetUniform(state.pressure_shader, "pressure_sampler");

	// Velocity Normalization shader.
	state.velocity_normalization_shader.program = glLoadShader("velocity_normalization.vert", "velocity_normalization.frag");
	state.velocity_normalization_shader.velocity_uniform = glGetUniform(state.velocity_normalization_shader, "velocity_sampler");
	state.velocity_normalization_shader.pressure_uniform = glGetUniform(state.velocity_normalization_shader, "pressure_sampler");
	state.velocity_normalization_shader.grid_size_uniform = glGetUniform(state.velocity_normalization_shader, "grid_size");
	state.velocity_normalization_shader.timestep_uniform = glGetUniform(state.velocity_normalization_shader, "timestep");

	// Velocity texture.
	glGenTextures(2, state.velocity_texture.buffers);
	GLfloat *velocity_data = new GLfloat[state.canvas_size.area() * 3]();
	glBindTexture(GL_TEXTURE_2D, state.velocity_texture.back());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGB, GL_FLOAT, velocity_data);
	for (int x = 0; x < state.canvas_size.width; ++x) {
		for (int y = 0; y < state.canvas_size.height; ++y) {
			int i = (x + y * state.canvas_size.width) * 3;
			velocity_data[i + 0] = 0;
			velocity_data[i + 1] = 0;
			velocity_data[i + 2] = 0;
		}
	}
	glBindTexture(GL_TEXTURE_2D, state.velocity_texture.front());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGB, GL_FLOAT, velocity_data);
	delete[] velocity_data;

	// Colour texture.
	glGenTextures(2, state.colour_texture.buffers);
	GLfloat *colour_data = new GLfloat[state.canvas_size.area() * 3]();
	glBindTexture(GL_TEXTURE_2D, state.colour_texture.back());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGB, GL_FLOAT, colour_data);
	for (int x = 0; x < state.canvas_size.width; ++x) {
		for (int y = 0; y < state.canvas_size.height; ++y) {
			int i = (x + y * state.canvas_size.width) * 3;
			colour_data[i + 0] = ((x + y) % 100) < 50;
			colour_data[i + 1] = (x % 100) < 50;
			colour_data[i + 2] = (y % 100) < 50;
		}
	}
	glBindTexture(GL_TEXTURE_2D, state.colour_texture.front());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGB, GL_FLOAT, colour_data);
	delete[] colour_data;

	// Divergence texture.
	glGenTextures(1, &state.divergence_texture);
	GLfloat *divergence_data = new GLfloat[state.canvas_size.area() * 3]();
	glBindTexture(GL_TEXTURE_2D, state.divergence_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGB, GL_FLOAT, divergence_data);
	delete[] divergence_data;

	// Pressure texture.
	glGenTextures(2, state.pressure_texture.buffers);
	GLfloat *pressure_data = new GLfloat[state.canvas_size.area() * 3]();
	glBindTexture(GL_TEXTURE_2D, state.pressure_texture.back());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGB, GL_FLOAT, pressure_data);
	glBindTexture(GL_TEXTURE_2D, state.pressure_texture.front());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
		state.canvas_size.width, state.canvas_size.height, 0, GL_RGB, GL_FLOAT, pressure_data);
	delete[] pressure_data;

	glGenFramebuffers(1, &state.advection_buffer);
	glGenFramebuffers(1, &state.divergence_buffer);
	glGenFramebuffers(1, &state.pressure_buffer);
	glGenFramebuffers(1, &state.velocity_normalization_buffer);
}

void cleanup() {
	glDeleteTextures(2, state.velocity_texture.buffers);
	glDeleteTextures(2, state.colour_texture.buffers);
	glDeleteTextures(1, &state.divergence_texture);
	glDeleteTextures(2, state.pressure_texture.buffers);
	glDeleteFramebuffers(1, &state.advection_buffer);
	glDeleteFramebuffers(1, &state.divergence_buffer);
	glDeleteFramebuffers(1, &state.pressure_buffer);
	glDeleteFramebuffers(1, &state.velocity_normalization_buffer);
}

int main(int argc, char **argv) {
	srand((unsigned int)time(NULL));

	state.canvas_size = Size(1080, 720);

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
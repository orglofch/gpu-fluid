#ifndef _FLIP_BUFFER_HPP_
#define _FLIP_BUFFER_HPP_

#include "Utility\gl.hpp"

class FlipBuffer
{
public:
	void flip() {
		active_buffer = !active_buffer;
	}

	GLuint &activeBuffer() { return buffers[active_buffer]; }
	GLuint &inactiveBuffer() { return buffers[!active_buffer]; }

private:
	int active_buffer = 0;
	GLuint buffers[2] = { -1, -1 };
};

#endif
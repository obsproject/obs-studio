#include <string>
#include <exception>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

#include "gl_test.hpp"
#ifdef _WIN32
#include "wgl_test.hpp"
#else
#include "glx_test.hpp"
#endif
#include <GL/freeglut.h>

GLuint positionBufferObject;
GLuint program;
GLuint vao;

GLuint BuildShader(GLenum eShaderType, const std::string &shaderText)
{
	GLuint shader = gl::CreateShader(eShaderType);
	const char *strFileData = shaderText.c_str();
	gl::ShaderSource(shader, 1, &strFileData, NULL);

	gl::CompileShader(shader);

	GLint status;
	gl::GetShaderiv(shader, gl::COMPILE_STATUS, &status);
	if (status == gl::FALSE_)
	{
		GLint infoLogLength;
		gl::GetShaderiv(shader, gl::INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		gl::GetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

		const char *strShaderType = NULL;
		switch(eShaderType)
		{
		case gl::VERTEX_SHADER: strShaderType = "vertex"; break;
//		case gl::GEOMETRY_SHADER: strShaderType = "geometry"; break;
		case gl::FRAGMENT_SHADER: strShaderType = "fragment"; break;
		}

		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;

		throw std::runtime_error("Compile failure in shader.");
	}

	return shader;
}


void init()
{
	gl::GenVertexArrays(1, &vao);
	gl::BindVertexArray(vao);

	const float vertexPositions[] = {
		0.75f, 0.75f, 0.0f, 1.0f,
		0.75f, -0.75f, 0.0f, 1.0f,
		-0.75f, -0.75f, 0.0f, 1.0f,
	};

	gl::GenBuffers(1, &positionBufferObject);
	gl::BindBuffer(gl::ARRAY_BUFFER, positionBufferObject);
	gl::BufferData(gl::ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, gl::STATIC_DRAW);
	gl::BindBuffer(gl::ARRAY_BUFFER, 0);

	const std::string vertexShader(
		"#version 330\n"
		"layout(location = 0) in vec4 position;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = position;\n"
		"}\n"
		);

	const std::string fragmentShader(
		"#version 330\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n"
		"}\n"
		);

	GLuint vertShader = BuildShader(gl::VERTEX_SHADER, vertexShader);
	GLuint fragShader = BuildShader(gl::FRAGMENT_SHADER, fragmentShader);

	program = gl::CreateProgram();
	gl::AttachShader(program, vertShader);
	gl::AttachShader(program, fragShader);	
	gl::LinkProgram(program);

	GLint status;
	gl::GetProgramiv (program, gl::LINK_STATUS, &status);
	if (status == gl::FALSE_)
	{
		GLint infoLogLength;
		gl::GetProgramiv(program, gl::INFO_LOG_LENGTH, &infoLogLength);

		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		gl::GetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
		delete[] strInfoLog;

		throw std::runtime_error("Shader could not be linked.");
	}
}

//Called to update the display.
//You should call glutSwapBuffers after all of your rendering to display what you rendered.
//If you need continuous updates of the screen, call glutPostRedisplay() at the end of the function.
void display()
{
	gl::ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	gl::Clear(gl::COLOR_BUFFER_BIT);

	gl::UseProgram(program);

	gl::BindBuffer(gl::ARRAY_BUFFER, positionBufferObject);
	gl::EnableVertexAttribArray(0);
	gl::VertexAttribPointer(0, 4, gl::FLOAT, gl::FALSE_, 0, 0);

	gl::DrawArrays(gl::TRIANGLES, 0, 3);

	gl::DisableVertexAttribArray(0);
	gl::UseProgram(0);

	glutSwapBuffers();
}

//Called whenever the window is resized. The new window size is given, in pixels.
//This is an opportunity to call gl::Viewport or gl::Scissor to keep up with the change in size.
void reshape (int w, int h)
{
	gl::Viewport(0, 0, (GLsizei) w, (GLsizei) h);
}

//Called whenever a key on the keyboard was pressed.
//The key is given by the ''key'' parameter, which is in ASCII.
//It's often a good idea to have the escape key (ASCII value 27) call glutLeaveMainLoop() to 
//exit the program.
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		glutLeaveMainLoop();
		break;
	}
}


int main(int argc, char** argv)
{
	glutInit(&argc, argv);

	int width = 500;
	int height = 500;
	unsigned int displayMode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;

	glutInitDisplayMode(displayMode);
	glutInitContextVersion (3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitWindowSize (width, height); 
	glutInitWindowPosition (300, 200);
	glutCreateWindow (argv[0]);

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

	gl::exts::LoadTest didLoad = gl::sys::LoadFunctions();
	if(!didLoad)
		printf("OpenGL: %i\n", didLoad.GetNumMissing());
	else
		printf("OpenGL Loaded!\n");

	init();

#ifdef _WIN32
	HDC hdc = wglGetCurrentDC();
	wgl::exts::LoadTest load = wgl::sys::LoadFunctions(hdc);
	if(!load)
		printf("WGL: %i\n", load.GetNumMissing());
	else
		printf("WGL Loaded!\n");
#else
#endif

	glutDisplayFunc(display); 
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMainLoop();
	return 0;
}

// Header Files
//=============

#include "../Graphics.h"
#include "../Renderable.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include "../../UserOutput/UserOutput.h"
#include "../../Windows/Functions.h"
#include "../../../External/OpenGlExtensions/OpenGlExtensions.h"
#include "../../Core/Math/cMatrix_transformation.h"
#include "../../Core/Math/Functions.h"

// Static Data Initialization
//===========================

namespace
{
	HWND s_renderingWindow = NULL;
	HDC s_deviceContext = NULL;
	HGLRC s_openGlRenderingContext = NULL;
	eae6320::Graphics::Mesh s_MeshSquare;
	eae6320::Graphics::Mesh s_MeshTriangle;

	eae6320::Graphics::Effect *s_effect = NULL;
	// This struct determines the layout of the data that the CPU will send to the GPU
	//struct sVertex
	//{
	//	// POSITION
	//	// 2 floats == 8 bytes
	//	// Offset = 0
	//	float x, y;
	//	// COLOR0
	//	// 4 uint8_ts == 4 bytes
	//	// Offset = 8
	//	uint8_t r, g, b, a;	// 8 bits [0,255] per RGBA channel (the alpha channel is unused but is present so that color uses a full 4 bytes)
	//};

	// A vertex array encapsulates both the vertex and index data as well as the vertex format
	//GLuint s_vertexArrayId = 0;

	// OpenGL encapsulates a matching vertex shader and fragment shader into what it calls a "program".

	// A vertex shader is a program that operates on vertices.
	// Its input comes from a C/C++ "draw call" and is:
	//	* Position
	//	* Any other data we want
	// Its output is:
	//	* Position
	//		(So that the graphics hardware knows which pixels to fill in for the triangle)
	//	* Any other data we want

	// The fragment shader is a program that operates on fragments
	// (or potential pixels).
	// Its input is:
	//	* The data that was output from the vertex shader,
	//		interpolated based on how close the fragment is
	//		to each vertex in the triangle.
	// Its output is:
	//	* The final color that the pixel should be
}

// Helper Function Declarations
//=============================

namespace
{
	bool CreateRenderingContext();
	bool LoadAndAllocateShaderProgram( const char* i_path, void*& o_shader, size_t& o_size, std::string* o_errorMessage );

	// This helper struct exists to be able to dynamically allocate memory to get "log info"
	// which will automatically be freed when the struct goes out of scope
	struct sLogInfo
	{
		GLchar* memory;
		sLogInfo( const size_t i_size ) { memory = reinterpret_cast<GLchar*>( malloc( i_size ) ); }
		~sLogInfo() { if ( memory ) free( memory ); }
	};
}

// Interface
//==========

bool eae6320::Graphics::Initialize( const HWND i_renderingWindow )
{
	s_renderingWindow = i_renderingWindow;

	// Create an OpenGL rendering context
	if ( !CreateRenderingContext() )
	{
		goto OnError;
	}

	// Load any required OpenGL extensions
	{
		std::string errorMessage;
		if ( !OpenGlExtensions::Load( &errorMessage ) )
		{
			UserOutput::Print( errorMessage );
			goto OnError;
		}
	}

	//Enable Culling
	{
		glEnable(GL_CULL_FACE);
		const GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			goto OnError;
		}
	}
	{
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LEQUAL);
	}

	return true;

OnError:

	ShutDown();
	return false;
}

void eae6320::Graphics::AddRenderable(const char* const i_pathMesh, const char* const i_pathMaterial, eae6320::Graphics::Renderable *i_renderable)
{

	LoadMesh(i_pathMesh, i_renderable->m_mesh);
	LoadMaterial(i_pathMaterial, i_renderable->m_material);
	//LoadEffect(i_pathEffect, i_renderable->m_effect);
	std::vector<eae6320::Graphics::Renderable*>* m_renderableList;
	if (i_renderable->m_material.m_effect.m_renderStates & alpha)
		m_renderableList = GetTransparentRenderableList();
	else
		m_renderableList = GetOpaqueRenderableList();
	m_renderableList->push_back(i_renderable);
}

void eae6320::Graphics::RemoveRenderable(eae6320::Graphics::Renderable *i_renderable)
{
	std::vector<eae6320::Graphics::Renderable*>* m_renderableList;
	if (i_renderable->m_material.m_effect.m_renderStates & alpha)
		m_renderableList = GetTransparentRenderableList();
	else
		m_renderableList = GetOpaqueRenderableList();
	for (std::vector<eae6320::Graphics::Renderable*>::iterator i = m_renderableList->begin(); i < m_renderableList->end(); ++i)
	{
		if (*(i) == i_renderable)
		{
			m_renderableList->erase(i);
			break;
		}
	}
}

bool eae6320::Graphics::CreateBuffers(sVertex*& i_vertexData, int i_numOfVertices, uint32_t*& i_indexData, int i_numOfIndices, Mesh& i_mesh)
{
	bool wereThereErrors = false;
	GLuint vertexBufferId = 0;
	GLuint indexBufferId = 0;

	// Create a vertex array object and make it active
	{
		const GLsizei arrayCount = 1;
		glGenVertexArrays(arrayCount, &(i_mesh.m_vertexArrayID));
		const GLenum errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			glBindVertexArray(i_mesh.m_vertexArrayID);
			const GLenum errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to bind the vertex array: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				eae6320::UserOutput::Print(errorMessage.str());
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to get an unused vertex array ID: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
	}

	// Create a vertex buffer object and make it active
	{
		const GLsizei bufferCount = 1;
		glGenBuffers(bufferCount, &vertexBufferId);
		const GLenum errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
			const GLenum errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to bind the vertex buffer: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				eae6320::UserOutput::Print(errorMessage.str());
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to get an unused vertex buffer ID: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
	}
	// Assign the data to the buffer
	{
		// We are drawing a square
		const unsigned int vertexCount = i_mesh.m_noOfVertices;	// What is the minimum number of vertices a square needs (so that no data is duplicated)?
		sVertex *vertexData = new sVertex[i_mesh.m_noOfVertices];
		// Fill in the data for the triangle
		{
			// You will need to fill in two pieces of information for each vertex:
			//	* 2 floats for the POSITION
			//	* 4 uint8_ts for the COLOR

			// The floats for POSITION are for the X and Y coordinates, like in Assignment 02.
			// The difference this time is that there should be fewer (because we are sharing data).

			// The uint8_ts for COLOR are "RGBA", where "RGB" stands for "Red Green Blue" and "A" for "Alpha".
			// Conceptually each of these values is a [0,1] value, but we store them as an 8-bit value to save space
			// (color doesn't need as much precision as position),
			// which means that the data we send to the GPU will be [0,255].
			// For now the alpha value should _always_ be 255, and so you will choose color by changing the first three RGB values.
			// To make white you should use (255, 255, 255), to make black (0, 0, 0).
			// To make pure red you would use the max for R and nothing for G and B, so (1, 0, 0).
			// Experiment with other values to see what happens!
			std::memcpy(vertexData, i_vertexData, (i_mesh.m_noOfVertices * sizeof(sVertex)));
		}
		glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(sVertex), reinterpret_cast<GLvoid*>(vertexData),
			// Our code will only ever write to the buffer
			GL_STATIC_DRAW);
		const GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to allocate the vertex buffer: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
		delete [] vertexData;
		vertexData = NULL;
	}
	// Initialize the vertex format
	{
		const GLsizei stride = sizeof(sVertex);
		GLvoid* offset = 0;

		// Position (0)
		// 3 floats == 12 bytes
		// Offset = 0
		{
			const GLuint vertexElementLocation = 0;
			const GLint elementCount = 3;
			const GLboolean notNormalized = GL_FALSE;	// The given floats should be used as-is
			glVertexAttribPointer(vertexElementLocation, elementCount, GL_FLOAT, notNormalized, stride, offset);
			const GLenum errorCode = glGetError();
			if (errorCode == GL_NO_ERROR)
			{
				glEnableVertexAttribArray(vertexElementLocation);
				const GLenum errorCode = glGetError();
				if (errorCode == GL_NO_ERROR)
				{
					offset = reinterpret_cast<GLvoid*>(reinterpret_cast<uint8_t*>(offset) + (elementCount * sizeof(float)));
				}
				else
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to enable the POSITION vertex attribute: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
					goto OnExit;
				}
			}
			else
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to set the POSITION vertex attribute: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				eae6320::UserOutput::Print(errorMessage.str());
				goto OnExit;
			}
		}
		// Texture Coordinates (2)
		// 2 floats == 8 bytes
		// Offset = 12
		{
			const GLuint vertexElementLocation = 2;
			const GLint elementCount = 2;
			const GLboolean notNormalized = GL_FALSE;	// The given floats should be used as-is
			glVertexAttribPointer(vertexElementLocation, elementCount, GL_FLOAT, notNormalized, stride, offset);
			const GLenum errorCode = glGetError();
			if (errorCode == GL_NO_ERROR)
			{
				glEnableVertexAttribArray(vertexElementLocation);
				const GLenum errorCode = glGetError();
				if (errorCode == GL_NO_ERROR)
				{
					offset = reinterpret_cast<GLvoid*>(reinterpret_cast<uint8_t*>(offset) + (elementCount * sizeof(float)));
				}
				else
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to enable the POSITION vertex attribute: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
					goto OnExit;
				}
			}
			else
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to set the POSITION vertex attribute: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				eae6320::UserOutput::Print(errorMessage.str());
				goto OnExit;
			}
		}
		// Color (2)
		// 4 uint8_ts == 4 bytes
		// Offset = 20
		{
			const GLuint vertexElementLocation = 1;
			const GLint elementCount = 4;
			// Each element will be sent to the GPU as an unsigned byte in the range [0,255]
			// but these values should be understood as representing [0,1] values
			// and that is what the shader code will interpret them as
			// (in other words, we could change the values provided here in C code
			// to be floats and sent GL_FALSE instead and the shader code wouldn't need to change)
			const GLboolean normalized = GL_TRUE;
			glVertexAttribPointer(vertexElementLocation, elementCount, GL_UNSIGNED_BYTE, normalized, stride, offset);
			const GLenum errorCode = glGetError();
			if (errorCode == GL_NO_ERROR)
			{
				glEnableVertexAttribArray(vertexElementLocation);
				const GLenum errorCode = glGetError();
				if (errorCode == GL_NO_ERROR)
				{
					offset = reinterpret_cast<GLvoid*>(reinterpret_cast<uint8_t*>(offset) + (elementCount * sizeof(uint8_t)));
				}
				else
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to enable the COLOR0 vertex attribute: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
					goto OnExit;
				}
			}
			else
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to set the COLOR0 vertex attribute: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				eae6320::UserOutput::Print(errorMessage.str());
				goto OnExit;
			}
		}
	}

	// Create an index buffer object and make it active
	{
		const GLsizei bufferCount = 1;
		glGenBuffers(bufferCount, &indexBufferId);
		const GLenum errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
			const GLenum errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to bind the index buffer: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				eae6320::UserOutput::Print(errorMessage.str());
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to get an unused index buffer ID: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
	}
	// Allocate space and copy the triangle data into the index buffer
	{
		// We are drawing a square
		const unsigned int triangleCount = (unsigned int)(i_mesh.m_noOfIndices / 3.0f);	// How many triangles does a square have?
		const unsigned int vertexCountPerTriangle = 3;
		uint32_t *indexData = new uint32_t[i_mesh.m_noOfIndices];
		// Fill in the data for the triangle
		{
			// EAE6320_TODO:
			// You will need to fill in 3 indices for each triangle that needs to be drawn.
			// Each index will be a 32-bit unsigned integer,
			// and will index into the vertex buffer array that you have created.
			// The order of indices is important, but the correct order will depend on
			// which vertex you have assigned to which spot in your vertex buffer
			// (also remember to maintain the correct handedness for the triangle winding order).
			std::memcpy(indexData, i_indexData, (i_mesh.m_noOfIndices * sizeof(uint32_t)));
		}

		const GLsizeiptr bufferSize = triangleCount * vertexCountPerTriangle * sizeof(uint32_t);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize, reinterpret_cast<const GLvoid*>(indexData),
			// Our code will only ever write to the buffer
			GL_STATIC_DRAW);
		const GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to allocate the index buffer: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
		delete [] indexData;
		indexData = NULL;
	}

OnExit:

	// Delete the buffer object
	// (the vertex array will hold a reference to it)
	if (i_mesh.m_vertexArrayID != 0)
	{
		// Unbind the vertex array
		// (this must be done before deleting the vertex buffer)
		glBindVertexArray(0);
		const GLenum errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			if (vertexBufferId != 0)
			{
				// NOTE: If you delete the vertex buffer object here, as I recommend,
				// then gDEBugger won't know about it and you won't be able to examine the data.
				// If you find yourself in a situation where you want to see the buffer object data in gDEBugger
				// comment out this block of code temporarily
				// (doing this will cause a memory leak so make sure to restore the deletion code after you're done debugging).
				const GLsizei bufferCount = 1;
				glDeleteBuffers(bufferCount, &vertexBufferId);
				const GLenum errorCode = glGetError();
				if (errorCode != GL_NO_ERROR)
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to delete the vertex buffer: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
				}
				vertexBufferId = 0;
			}
			if (indexBufferId != 0)
			{
				// NOTE: See the same comment above about deleting the vertex buffer object here and gDEBugger
				// holds true for the index buffer
				const GLsizei bufferCount = 1;
				glDeleteBuffers(bufferCount, &indexBufferId);
				const GLenum errorCode = glGetError();
				if (errorCode != GL_NO_ERROR)
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "\nOpenGL failed to delete the index buffer: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
				}
				indexBufferId = 0;
			}
		}
		else
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to unbind the vertex array: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
		}
	}

	return !wereThereErrors;
}

bool eae6320::Graphics::LoadEffect(const char* const i_effectPath, Effect& i_effect)
{
	bool wereThereErrors = false;

	//Getting the Paths for the vertex and fragment shader.
	//Variable declerations.

	char *temporaryBuffer = NULL; //Declaring the temporary buffer to copy file contents into.

								  //File operations.
	FILE *i_file;
	errno_t err;

	err = fopen_s(&i_file, i_effectPath, "rb");
	if (err != 0)
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to load/open one or more mesh File\n";
		eae6320::UserOutput::Print(errorMessage.str());

		goto OnExit;
	}


	//Getting File size to initialize Buffer.
	fseek(i_file, 0, SEEK_END);
	long fSize = ftell(i_file);
	rewind(i_file);

	//Initializing the Buffer
	temporaryBuffer = reinterpret_cast<char*>(malloc(sizeof(char) * fSize));

	if (temporaryBuffer == NULL)
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to allocate memory for buffer.\n";
		eae6320::UserOutput::Print(errorMessage.str());

		goto OnExit;
	}

	fread(temporaryBuffer, sizeof(char), fSize, i_file);

	i_effect.m_renderStates = *reinterpret_cast<uint8_t*>(temporaryBuffer);
	const char* i_vertexPath = reinterpret_cast<char*>(temporaryBuffer + 1);
	size_t offset = strlen(i_vertexPath) + 1;
	const char* i_fragmentPath = reinterpret_cast<char*>(temporaryBuffer + 1 + offset);
	// Create a program
	{
		i_effect.m_programID = glCreateProgram();
		const GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to create a program: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			return false;
		}
		else if (i_effect.m_programID == 0)
		{
			eae6320::UserOutput::Print("OpenGL failed to create a program");
			wereThereErrors = true;
			goto OnExit;
		}
	}

	//Loading the fragment shader.
	// Verify that compiling shaders at run-time is supported
	{
		GLboolean isShaderCompilingSupported;
		glGetBooleanv(GL_SHADER_COMPILER, &isShaderCompilingSupported);
		if (!isShaderCompilingSupported)
		{
			eae6320::UserOutput::Print("Compiling shaders at run-time isn't supported on this implementation (this should never happen)");
			wereThereErrors = true;
			goto OnExit;
		}
	}

	// Load the source code from file and set it into a shader
	GLuint fragmentShaderId = 0;
	void* shaderSource = NULL;
	{
		// Load the shader source code
		size_t fileSize;
		{
			const char* sourceCodeFileName = i_fragmentPath;
			std::string errorMessage;
			if (!LoadAndAllocateShaderProgram(sourceCodeFileName, shaderSource, fileSize, &errorMessage))
			{
				wereThereErrors = true;
				eae6320::UserOutput::Print(errorMessage);
				goto OnExit;
			}
		}
		// Generate a shader
		fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
		{
			const GLenum errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to get an unused fragment shader ID: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				eae6320::UserOutput::Print(errorMessage.str());
				goto OnExit;
			}
			else if (fragmentShaderId == 0)
			{
				wereThereErrors = true;
				eae6320::UserOutput::Print("OpenGL failed to get an unused fragment shader ID");
				goto OnExit;
			}
		}
		// Set the source code into the shader
		{
			const GLsizei shaderSourceCount = 1;
			const GLchar* shaderSources[] =
			{
				reinterpret_cast<GLchar*>(shaderSource)
			};
			const GLint* sourcesAreNullTerminated = NULL;
			glShaderSource(fragmentShaderId, shaderSourceCount, shaderSources, sourcesAreNullTerminated);
			const GLenum errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to set the fragment shader source code: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				eae6320::UserOutput::Print(errorMessage.str());
				goto OnExit;
			}
		}
	}
	// Compile the shader source code
	{
		glCompileShader(fragmentShaderId);
		GLenum errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			// Get compilation info
			// (this won't be used unless compilation fails
			// but it can be useful to look at when debugging)
			std::string compilationInfo;
			{
				GLint infoSize;
				glGetShaderiv(fragmentShaderId, GL_INFO_LOG_LENGTH, &infoSize);
				errorCode = glGetError();
				if (errorCode == GL_NO_ERROR)
				{
					sLogInfo info(static_cast<size_t>(infoSize));
					GLsizei* dontReturnLength = NULL;
					glGetShaderInfoLog(fragmentShaderId, static_cast<GLsizei>(infoSize), dontReturnLength, info.memory);
					errorCode = glGetError();
					if (errorCode == GL_NO_ERROR)
					{
						compilationInfo = info.memory;
					}
					else
					{
						wereThereErrors = true;
						std::stringstream errorMessage;
						errorMessage << "OpenGL failed to get compilation info of the fragment shader source code: " <<
							reinterpret_cast<const char*>(gluErrorString(errorCode));
						eae6320::UserOutput::Print(errorMessage.str());
						goto OnExit;
					}
				}
				else
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to get the length of the fragment shader compilation info: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
					goto OnExit;
				}
			}
			// Check to see if there were compilation errors
			GLint didCompilationSucceed;
			{
				glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &didCompilationSucceed);
				errorCode = glGetError();
				if (errorCode == GL_NO_ERROR)
				{
					if (didCompilationSucceed == GL_FALSE)
					{
						wereThereErrors = true;
						std::stringstream errorMessage;
						errorMessage << "The fragment shader failed to compile:\n" << compilationInfo;
						eae6320::UserOutput::Print(errorMessage.str());
						goto OnExit;
					}
				}
				else
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to find out if compilation of the fragment shader source code succeeded: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
					goto OnExit;
				}
			}
		}
		else
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to compile the fragment shader source code: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
	}
	// Attach the shader to the program
	{
		glAttachShader(i_effect.m_programID, fragmentShaderId);
		const GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to attach the fragment shader to the program: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
	}

	if (shaderSource != NULL)
	{
		free(shaderSource);
		shaderSource = NULL;
	}

	//Loading the vertex shader.
	// Verify that compiling shaders at run-time is supported
	{
		GLboolean isShaderCompilingSupported;
		glGetBooleanv(GL_SHADER_COMPILER, &isShaderCompilingSupported);
		if (!isShaderCompilingSupported)
		{
			eae6320::UserOutput::Print("Compiling shaders at run-time isn't supported on this implementation (this should never happen)");
			wereThereErrors = true;
			goto OnExit;
		}
	}

	// Load the source code from file and set it into a shader
	GLuint vertexShaderId = 0;
	{
		// Load the shader source code
		size_t fileSize;
		{
			const char* sourceCodeFileName = i_vertexPath;
			std::string errorMessage;
			if (!LoadAndAllocateShaderProgram(sourceCodeFileName, shaderSource, fileSize, &errorMessage))
			{
				wereThereErrors = true;
				eae6320::UserOutput::Print(errorMessage);
				goto OnExit;
			}
		}
		// Generate a shader
		vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
		{
			const GLenum errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to get an unused vertex shader ID: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				eae6320::UserOutput::Print(errorMessage.str());
				goto OnExit;
			}
			else if (vertexShaderId == 0)
			{
				wereThereErrors = true;
				eae6320::UserOutput::Print("OpenGL failed to get an unused vertex shader ID");
				goto OnExit;
			}
		}
		// Set the source code into the shader
		{
			const GLsizei shaderSourceCount = 1;
			const GLchar* shaderSources[] =
			{
				reinterpret_cast<GLchar*>(shaderSource)
			};
			const GLint* sourcesAreNullTerminated = NULL;
			glShaderSource(vertexShaderId, shaderSourceCount, shaderSources, sourcesAreNullTerminated);
			const GLenum errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to set the vertex shader source code: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				eae6320::UserOutput::Print(errorMessage.str());
				goto OnExit;
			}
		}
	}
	// Compile the shader source code
	{
		glCompileShader(vertexShaderId);
		GLenum errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			// Get compilation info
			// (this won't be used unless compilation fails
			// but it can be useful to look at when debugging)
			std::string compilationInfo;
			{
				GLint infoSize;
				glGetShaderiv(vertexShaderId, GL_INFO_LOG_LENGTH, &infoSize);
				errorCode = glGetError();
				if (errorCode == GL_NO_ERROR)
				{
					sLogInfo info(static_cast<size_t>(infoSize));
					GLsizei* dontReturnLength = NULL;
					glGetShaderInfoLog(vertexShaderId, static_cast<GLsizei>(infoSize), dontReturnLength, info.memory);
					errorCode = glGetError();
					if (errorCode == GL_NO_ERROR)
					{
						compilationInfo = info.memory;
					}
					else
					{
						wereThereErrors = true;
						std::stringstream errorMessage;
						errorMessage << "OpenGL failed to get compilation info of the vertex shader source code: " <<
							reinterpret_cast<const char*>(gluErrorString(errorCode));
						eae6320::UserOutput::Print(errorMessage.str());
						goto OnExit;
					}
				}
				else
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to get the length of the vertex shader compilation info: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
					goto OnExit;
				}
			}
			// Check to see if there were compilation errors
			GLint didCompilationSucceed;
			{
				glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &didCompilationSucceed);
				errorCode = glGetError();
				if (errorCode == GL_NO_ERROR)
				{
					if (didCompilationSucceed == GL_FALSE)
					{
						wereThereErrors = true;
						std::stringstream errorMessage;
						errorMessage << "The vertex shader failed to compile:\n" << compilationInfo;
						eae6320::UserOutput::Print(errorMessage.str());
						goto OnExit;
					}
				}
				else
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to find out if compilation of the vertex shader source code succeeded: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
					goto OnExit;
				}
			}
		}
		else
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to compile the vertex shader source code: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
	}
	// Attach the shader to the program
	{
		glAttachShader(i_effect.m_programID, vertexShaderId);
		const GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to attach the vertex shader to the program: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
	}

	// Link the program
	{
		glLinkProgram(i_effect.m_programID);
		GLenum errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			// Get link info
			// (this won't be used unless linking fails
			// but it can be useful to look at when debugging)
			std::string linkInfo;
			{
				GLint infoSize;
				glGetProgramiv(i_effect.m_programID, GL_INFO_LOG_LENGTH, &infoSize);
				errorCode = glGetError();
				if (errorCode == GL_NO_ERROR)
				{
					sLogInfo info(static_cast<size_t>(infoSize));
					GLsizei* dontReturnLength = NULL;
					glGetProgramInfoLog(i_effect.m_programID, static_cast<GLsizei>(infoSize), dontReturnLength, info.memory);
					errorCode = glGetError();
					if (errorCode == GL_NO_ERROR)
					{
						linkInfo = info.memory;
					}
					else
					{
						wereThereErrors = true;
						std::stringstream errorMessage;
						errorMessage << "OpenGL failed to get link info of the program: " <<
							reinterpret_cast<const char*>(gluErrorString(errorCode));
						eae6320::UserOutput::Print(errorMessage.str());
						goto OnExit;
					}
				}
				else
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to get the length of the program link info: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
					goto OnExit;
				}
			}
			// Check to see if there were link errors
			GLint didLinkingSucceed;
			{
				glGetProgramiv(i_effect.m_programID, GL_LINK_STATUS, &didLinkingSucceed);
				errorCode = glGetError();
				if (errorCode == GL_NO_ERROR)
				{
					if (didLinkingSucceed == GL_FALSE)
					{
						wereThereErrors = true;
						std::stringstream errorMessage;
						errorMessage << "The program failed to link:\n" << linkInfo;
						eae6320::UserOutput::Print(errorMessage.str());
						goto OnExit;
					}
				}
				else
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to find out if linking of the program succeeded: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					eae6320::UserOutput::Print(errorMessage.str());
					goto OnExit;
				}
			}
		}
		else
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to link the program: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
	}

	//Get uniform location
	{
		//i_effect.location = glGetUniformLocation(i_effect.m_programID, "g_position_offset");
		i_effect.localToWorld = glGetUniformLocation(i_effect.m_programID, "g_transform_localToWorld");
		i_effect.worldToView = glGetUniformLocation(i_effect.m_programID, "g_transform_worldToView");
		i_effect.viewToScreen = glGetUniformLocation(i_effect.m_programID, "g_transform_viewToScreen");
		if (i_effect.localToWorld == -1 || i_effect.worldToView == -1 || i_effect.viewToScreen == -1)
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "Failed to get uniform location ";
			eae6320::UserOutput::Print(errorMessage.str());
			goto OnExit;
		}
	}

OnExit:

	free(temporaryBuffer);
	if (fragmentShaderId != 0)
	{
		// Even if the shader was successfully compiled
		// once it has been attached to the program we can (and should) delete our reference to it
		// (any associated memory that OpenGL has allocated internally will be freed
		// once the program is deleted)
		glDeleteShader(fragmentShaderId);
		const GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to delete the fragment shader ID: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
		}
		fragmentShaderId = 0;
	}

	if (vertexShaderId != 0)
	{
		// Even if the shader was successfully compiled
		// once it has been attached to the program we can (and should) delete our reference to it
		// (any associated memory that OpenGL has allocated internally will be freed
		// once the program is deleted)
		glDeleteShader(vertexShaderId);
		const GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			std::stringstream errorMessage;
			errorMessage << "OpenGL failed to delete the vertex shader ID: " <<
				reinterpret_cast<const char*>(gluErrorString(errorCode));
			eae6320::UserOutput::Print(errorMessage.str());
		}
		vertexShaderId = 0;
	}
	if (shaderSource != NULL)
	{
		free(shaderSource);
		shaderSource = NULL;
	}

	return !wereThereErrors;
}

bool eae6320::Graphics::LoadTexture(const char* const i_path, Material& i_material)
{
	bool wereThereErrors = false;
	HANDLE fileHandle = INVALID_HANDLE_VALUE;
	void* fileContents = NULL;
	i_material.m_3dTexture = 0;
	std::string* o_errorMessage = NULL;

	// Open the texture file
	{
		const DWORD desiredAccess = FILE_GENERIC_READ;
		const DWORD otherProgramsCanStillReadTheFile = FILE_SHARE_READ;
		SECURITY_ATTRIBUTES* useDefaultSecurity = NULL;
		const DWORD onlySucceedIfFileExists = OPEN_EXISTING;
		const DWORD useDefaultAttributes = FILE_ATTRIBUTE_NORMAL;
		const HANDLE dontUseTemplateFile = NULL;
		fileHandle = CreateFile(i_path, desiredAccess, otherProgramsCanStillReadTheFile,
			useDefaultSecurity, onlySucceedIfFileExists, useDefaultAttributes, dontUseTemplateFile);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			wereThereErrors = true;
			if (o_errorMessage)
			{
				std::string windowsErrorMessage(GetLastWindowsError());
				std::stringstream errorMessage;
				errorMessage << "Windows failed to open the texture file: " << windowsErrorMessage;
				*o_errorMessage = errorMessage.str();
			}
			goto OnExit;
		}
	}
	// Get the file's size
	size_t fileSize;
	{
		LARGE_INTEGER fileSize_integer;
		if (GetFileSizeEx(fileHandle, &fileSize_integer) != FALSE)
		{
			assert(fileSize_integer.QuadPart <= SIZE_MAX);
			fileSize = static_cast<size_t>(fileSize_integer.QuadPart);
		}
		else
		{
			wereThereErrors = true;
			if (o_errorMessage)
			{
				std::string windowsErrorMessage(GetLastWindowsError());
				std::stringstream errorMessage;
				errorMessage << "Windows failed to get the size of the texture file: " << windowsErrorMessage;
				*o_errorMessage = errorMessage.str();
			}
			goto OnExit;
		}
	}
	// Read the file's contents into temporary memory
	fileContents = malloc(fileSize);
	if (fileContents)
	{
		DWORD bytesReadCount;
		OVERLAPPED* readSynchronously = NULL;
		assert(fileSize < (uint64_t(1) << (sizeof(DWORD) * 8)));
		if (ReadFile(fileHandle, fileContents, static_cast<DWORD>(fileSize),
			&bytesReadCount, readSynchronously) == FALSE)
		{
			wereThereErrors = true;
			if (o_errorMessage)
			{
				std::string windowsErrorMessage(GetLastWindowsError());
				std::stringstream errorMessage;
				errorMessage << "Windows failed to read the contents of the texture file: " << windowsErrorMessage;
				*o_errorMessage = errorMessage.str();
			}
			goto OnExit;
		}
	}
	else
	{
		wereThereErrors = true;
		if (o_errorMessage)
		{
			std::stringstream errorMessage;
			errorMessage << "Failed to allocate " << fileSize << " bytes to read in the texture " << i_path;
			*o_errorMessage = errorMessage.str();
		}
		goto OnExit;
	}

	// Create a new texture and make it active
	{
		const GLsizei textureCount = 1;
		glGenTextures(textureCount, &(i_material.m_3dTexture));
		const GLenum errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			// This code only supports 2D textures;
			// if you want to support other types you will need to improve this code.
			glBindTexture(GL_TEXTURE_2D, i_material.m_3dTexture);
			const GLenum errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				wereThereErrors = true;
				if (o_errorMessage)
				{
					std::stringstream errorMessage;
					errorMessage << "OpenGL failed to bind a new texture: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					*o_errorMessage = errorMessage.str();
				}
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			if (o_errorMessage)
			{
				std::stringstream errorMessage;
				errorMessage << "OpenGL failed to get an unused texture ID: " <<
					reinterpret_cast<const char*>(gluErrorString(errorCode));
				*o_errorMessage = errorMessage.str();
			}
			goto OnExit;
		}
	}

	// Extract the data
	const uint8_t* currentPosition = reinterpret_cast<uint8_t*>(fileContents);
	// Verify that the file is a valid DDS
	{
		const size_t fourCcCount = 4;
		const uint8_t* const fourCc = currentPosition;
		const uint8_t fourCc_dds[fourCcCount] = { 'D', 'D', 'S', ' ' };
		// Each of the four characters can be compared in a single operation by casting to a uint32_t
		const bool isDds = *reinterpret_cast<const uint32_t*>(fourCc) == *reinterpret_cast<const uint32_t*>(fourCc_dds);
		if (isDds)
		{
			currentPosition += fourCcCount;
		}
		else
		{
			wereThereErrors = true;
			if (o_errorMessage)
			{
				char fourCcString[fourCcCount + 1] = { 0 };	// Add NULL terminator
				memcpy(fourCcString, currentPosition, fourCcCount);
				std::stringstream errorMessage;
				errorMessage << "The texture file \"" << i_path << "\" isn't a valid DDS. The Four CC is \"" << fourCcString << "\""
					" instead of \"DDS \"";
				*o_errorMessage = errorMessage.str();
			}
			goto OnExit;
		}
	}
	// Extract the header
	// (this struct can also be found in Dds.h in the DirectX header files
	// or here as of this comment: https://msdn.microsoft.com/en-us/library/windows/desktop/bb943982(v=vs.85).aspx )
	struct sDdsHeader
	{
		uint32_t structSize;
		uint32_t flags;
		uint32_t height;
		uint32_t width;
		uint32_t pitchOrLinearSize;
		uint32_t depth;
		uint32_t mipMapCount;
		uint32_t reserved1[11];
		struct
		{
			uint32_t structSize;
			uint32_t flags;
			uint8_t fourCc[4];
			uint32_t rgbBitCount;
			uint32_t bitMask_red;
			uint32_t bitMask_green;
			uint32_t bitMask_blue;
			uint32_t bitMask_alpha;
		} pixelFormat;
		uint32_t caps[4];
		uint32_t reserved2;
	};
	const sDdsHeader* ddsHeader = reinterpret_cast<const sDdsHeader*>(currentPosition);
	currentPosition += sizeof(sDdsHeader);
	// Convert the DDS format into an OpenGL format
	GLenum format;
	{
		// This code can only handle the two basic formats that the example TextureBuilder will create.
		// If a DDS in a different format is provided to TextureBuilder it will be passed through unchanged
		// and this code won't work.
		// Similarly, if you improve the example TextureBuilder to support more formats
		// you will also have to update this code to support them.
		const uint8_t fourCc_dxt1[] = { 'D', 'X', 'T', '1' };	// No alpha channel
		const uint8_t fourCc_dxt5[] = { 'D', 'X', 'T', '5' };	// Alpha channel
		const uint32_t fourCc_texture = *reinterpret_cast<const uint32_t*>(ddsHeader->pixelFormat.fourCc);
		if (fourCc_texture == *reinterpret_cast<const uint32_t*>(fourCc_dxt1))
		{
			format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		}
		else if (fourCc_texture == *reinterpret_cast<const uint32_t*>(fourCc_dxt5))
		{
			format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}
		else
		{
			wereThereErrors = true;
			if (o_errorMessage)
			{
				char fourCcString[5] = { 0 };	// Add NULL terminator
				memcpy(fourCcString, ddsHeader->pixelFormat.fourCc, 4);
				std::stringstream errorMessage;
				errorMessage << "The texture file \"" << i_path << "\" has an unsupported format \"" << fourCcString << "\"";
				*o_errorMessage = errorMessage.str();
			}
			goto OnExit;
		}
	}
	// Iterate through each MIP map level and fill in the OpenGL texture
	{
		GLsizei currentWidth = ddsHeader->width;
		GLsizei currentHeight = ddsHeader->height;
		const GLsizei blockSize = format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ? 8 : 16;
		const GLint borderWidth = 0;
		for (uint32_t mipMapLevel = 0; mipMapLevel < ddsHeader->mipMapCount; ++mipMapLevel)
		{
			const GLsizei mipMapSize = ((currentWidth + 3) / 4) * ((currentHeight + 3) / 4) * blockSize;
			glCompressedTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(mipMapLevel), format, currentWidth, currentHeight,
				borderWidth, mipMapSize, currentPosition);
			const GLenum errorCode = glGetError();
			if (errorCode == GL_NO_ERROR)
			{
				currentPosition += static_cast<size_t>(mipMapSize);
				currentWidth = std::max(1, (currentWidth / 2));
				currentHeight = std::max(1, (currentHeight / 2));
			}
			else
			{
				wereThereErrors = true;
				if (o_errorMessage)
				{
					std::stringstream errorMessage;
					errorMessage << "OpenGL rejected compressed texture data: " <<
						reinterpret_cast<const char*>(gluErrorString(errorCode));
					*o_errorMessage = errorMessage.str();
				}
				goto OnExit;
			}
		}
	}

	assert(currentPosition == (reinterpret_cast<uint8_t*>(fileContents) + fileSize));

OnExit:

	if (fileContents != NULL)
	{
		free(fileContents);
		fileContents = NULL;
	}
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		if (CloseHandle(fileHandle) == FALSE)
		{
			wereThereErrors = true;
			if (o_errorMessage)
			{
				std::string windowsErrorMessage(GetLastWindowsError());
				std::stringstream errorMessage;
				errorMessage << "\nWindows failed to close the texture file handle: " << windowsErrorMessage;
				*o_errorMessage += errorMessage.str();
			}
		}
		fileHandle = INVALID_HANDLE_VALUE;
	}
	if (wereThereErrors && (i_material.m_3dTexture != 0))
	{
		const GLsizei textureCount = 1;
		glDeleteTextures(textureCount, &(i_material.m_3dTexture));
		assert(glGetError == GL_NO_ERROR);
		i_material.m_3dTexture = 0;
	}

	return !wereThereErrors;
}

bool eae6320::Graphics::LoadSamplerID(const char* const i_uniformName, Material& i_material)
{
	i_material.m_texHandle = glGetUniformLocation(i_material.m_effect.m_programID, i_uniformName);

	return true;
}

bool eae6320::Graphics::SetTexture(Material& i_material, int i_offset)
{

	glActiveTexture(GL_TEXTURE0 + i_offset);
	glBindTexture(GL_TEXTURE_2D, i_material.m_3dTexture);
	glUniform1i(i_material.m_texHandle, i_offset);

	return true;
}

eae6320::Graphics::tUniformHandle eae6320::Graphics::GetUniform(Effect& i_effect, const char* const i_uniformName, eShaderType i_shaderType)
{
	return glGetUniformLocation(i_effect.m_programID, i_uniformName);
}

void eae6320::Graphics::Clear()
{
	// Black is usually used
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	assert(glGetError() == GL_NO_ERROR);
	// In addition to the color, "depth" and "stencil" can also be cleared,
	// but for now we only care about color
	const GLbitfield clearColor = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
	glDepthMask(GL_TRUE);
	glClear(clearColor);
	assert(glGetError() == GL_NO_ERROR);
}

void eae6320::Graphics::BeginScene()
{

}

void eae6320::Graphics::EndScene()
{

}

void eae6320::Graphics::SwapBuffers()
{
	BOOL result = SwapBuffers(s_deviceContext);
	assert(result != FALSE);
}

bool eae6320::Graphics::BindEffect(Effect& i_effect)
{
	glUseProgram(i_effect.m_programID);
	assert(glGetError() == GL_NO_ERROR);

	//Set Alpha rendering state.
	if (i_effect.m_renderStates & alpha)
	{
		glEnable(GL_BLEND);
		assert(glGetError() == GL_NO_ERROR);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		assert(glGetError() == GL_NO_ERROR);
	}
	else
	{
		glDisable(GL_BLEND);
		assert(glGetError() == GL_NO_ERROR);
	}

	//Set Depth Test rendering state.
	if (i_effect.m_renderStates & depthtest)
	{
		glEnable(GL_DEPTH_TEST);
		assert(glGetError() == GL_NO_ERROR);
		glDepthFunc(GL_LEQUAL);
		assert(glGetError() == GL_NO_ERROR);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
		assert(glGetError() == GL_NO_ERROR);
	}

	//Set Depth Write rendering state.
	if (i_effect.m_renderStates & depthwrite)
	{
		glDepthMask(GL_TRUE);
		assert(glGetError() == GL_NO_ERROR);
	}
	else
	{
		glDepthMask(GL_FALSE);
		assert(glGetError() == GL_NO_ERROR);
	}

	//Set Face Culling rendering state.
	if (i_effect.m_renderStates & faceculling)
	{
		glEnable(GL_CULL_FACE);
		assert(glGetError() == GL_NO_ERROR);
		glFrontFace(GL_CCW);
		assert(glGetError() == GL_NO_ERROR);
	}
	else
	{
		glDisable(GL_CULL_FACE);
		assert(glGetError() == GL_NO_ERROR);
	}

	return true;
}

bool eae6320::Graphics::SetDrawCallUniforms(Effect& i_effect, eae6320::Math::cMatrix_transformation& i_offsetMatrix)
{
	const GLboolean dontTranspose = false; // Matrices are already in the correct format
	const GLsizei uniformCountToSet = 1;

	glUniformMatrix4fv(i_effect.localToWorld, uniformCountToSet, dontTranspose, reinterpret_cast<const GLfloat*>(&i_offsetMatrix));

	eae6320::Math::cMatrix_transformation worldToView = eae6320::Math::cMatrix_transformation::CreateWorldToViewTransform(Camera::getInstance().m_orientation, Camera::getInstance().m_offset);
	glUniformMatrix4fv(i_effect.worldToView, uniformCountToSet, dontTranspose, reinterpret_cast<const GLfloat*>(&worldToView));

	eae6320::Math::cMatrix_transformation viewToScreen = eae6320::Math::cMatrix_transformation::CreateViewToScreenTransform(Camera::getInstance().FOV, getAspectRatio(), 0.1f, 100.0f);
	glUniformMatrix4fv(i_effect.viewToScreen, uniformCountToSet, dontTranspose, reinterpret_cast<const GLfloat*>(&viewToScreen));

	return true;
}

void eae6320::Graphics::SetMaterialUniform(const Effect& i_effect, float i_values[], uint8_t i_valueCountToSet, tUniformHandle i_uniformHandle, eShaderType i_shaderType)
{
	switch (i_valueCountToSet)
	{
	case 1:
		glUniform1fv(i_uniformHandle, 1, i_values);
		break;
	case 2:
		glUniform2fv(i_uniformHandle, 1, i_values);
		break;
	case 3:
		glUniform3fv(i_uniformHandle, 1, i_values);
		assert(glGetError() == GL_NO_ERROR);
		break;
	case 4:
		glUniform4fv(i_uniformHandle, 1, i_values);
		break;
	default:
		break;
	}
	glUniform2fv(i_uniformHandle, i_valueCountToSet, i_values);
}

void eae6320::Graphics::DrawMesh(const eae6320::Graphics::Mesh& i_Mesh)
{
	// The actual function calls that draw geometry
	{
		// Bind a specific vertex buffer to the device as a data source
		{
			glBindVertexArray(i_Mesh.m_vertexArrayID);
			const GLenum errorCode = glGetError();
			assert(glGetError() == GL_NO_ERROR);
		}
		// Render objects from the current streams
		{
			// We are using triangles as the "primitive" type,
			// and we have defined the vertex buffer as a triangle list
			// (meaning that every triangle is defined by three vertices)
			const GLenum mode = GL_TRIANGLES;
			// We'll use 32-bit indices in this class to keep things simple
			// (i.e. every index will be a 32 bit unsigned integer)
			const GLenum indexType = GL_UNSIGNED_INT;
			// It is possible to start rendering in the middle of an index buffer
			const GLvoid* const offset = 0;
			// We are drawing a square
			const GLsizei primitiveCountToRender = (i_Mesh.m_noOfIndices / 3.0f);	// How many triangles will be drawn?
			const GLsizei vertexCountPerTriangle = 3;
			const GLsizei vertexCountToRender = primitiveCountToRender * vertexCountPerTriangle;
			glDrawElements(mode, vertexCountToRender, indexType, offset);
			assert(glGetError() == GL_NO_ERROR);
		}
	}
}

bool eae6320::Graphics::ShutDown()
{
	bool wereThereErrors = false;

	if ( s_openGlRenderingContext != NULL )
	{
		std::vector<eae6320::Graphics::Renderable*>* m_renderableList = GetOpaqueRenderableList();
		size_t size = m_renderableList->size();
		{
			const GLsizei arrayCount = 1;

			for (unsigned int i = 0; i < size; i++)
			{
				{
					glDeleteProgram((*m_renderableList)[i]->m_material.m_effect.m_programID);
					const GLenum errorCode = glGetError();
					if (errorCode != GL_NO_ERROR)
					{
						std::stringstream errorMessage;
						errorMessage << "OpenGL failed to delete the program: " <<
							reinterpret_cast<const char*>(gluErrorString(errorCode));
						UserOutput::Print(errorMessage.str());
					}
					(*m_renderableList)[i]->m_material.m_effect.m_programID = 0;
				}

				{
					glDeleteVertexArrays(arrayCount, &((*m_renderableList)[i]->m_mesh.m_vertexArrayID));
					const GLenum errorCode = glGetError();
					if (errorCode != GL_NO_ERROR)
					{
						std::stringstream errorMessage;
						errorMessage << "OpenGL failed to delete the vertex array: " <<
							reinterpret_cast<const char*>(gluErrorString(errorCode));
						UserOutput::Print(errorMessage.str());
					}
					(*m_renderableList)[i]->m_mesh.m_vertexArrayID = 0;
				}
			}
		}

		m_renderableList = GetTransparentRenderableList();
		size = m_renderableList->size();
		{
			const GLsizei arrayCount = 1;

			for (unsigned int i = 0; i < size; i++)
			{
				{
					glDeleteProgram((*m_renderableList)[i]->m_material.m_effect.m_programID);
					const GLenum errorCode = glGetError();
					if (errorCode != GL_NO_ERROR)
					{
						std::stringstream errorMessage;
						errorMessage << "OpenGL failed to delete the program: " <<
							reinterpret_cast<const char*>(gluErrorString(errorCode));
						UserOutput::Print(errorMessage.str());
					}
					(*m_renderableList)[i]->m_material.m_effect.m_programID = 0;
				}

				{
					glDeleteVertexArrays(arrayCount, &((*m_renderableList)[i]->m_mesh.m_vertexArrayID));
					const GLenum errorCode = glGetError();
					if (errorCode != GL_NO_ERROR)
					{
						std::stringstream errorMessage;
						errorMessage << "OpenGL failed to delete the vertex array: " <<
							reinterpret_cast<const char*>(gluErrorString(errorCode));
						UserOutput::Print(errorMessage.str());
					}
					(*m_renderableList)[i]->m_mesh.m_vertexArrayID = 0;
				}
			}
		}
		
		if ( wglMakeCurrent( s_deviceContext, NULL ) != FALSE )
		{
			if ( wglDeleteContext( s_openGlRenderingContext ) == FALSE )
			{
				std::stringstream errorMessage;
				errorMessage << "Windows failed to delete the OpenGL rendering context: " << GetLastWindowsError();
				UserOutput::Print( errorMessage.str() );
			}
		}
		else
		{
			std::stringstream errorMessage;
			errorMessage << "Windows failed to unset the current OpenGL rendering context: " << GetLastWindowsError();
			UserOutput::Print( errorMessage.str() );
		}
		s_openGlRenderingContext = NULL;
	}

	if ( s_deviceContext != NULL )
	{
		// The documentation says that this call isn't necessary when CS_OWNDC is used
		ReleaseDC( s_renderingWindow, s_deviceContext );
		s_deviceContext = NULL;
	}

	s_renderingWindow = NULL;

	return !wereThereErrors;
}

// Helper Function Declarations
//=============================

float eae6320::Graphics::getAspectRatio()
{
	RECT renderingWindow;
	GetWindowRect(s_renderingWindow, &renderingWindow);

	int screenWidth = renderingWindow.right - renderingWindow.left;
	int screenHeight = renderingWindow.bottom - renderingWindow.top;

	return static_cast<float>((float)screenWidth / (float)screenHeight);
}


namespace
{

	bool CreateRenderingContext()
	{
		// A "device context" can be thought of an abstraction that Windows uses
		// to represent the graphics adaptor used to display a given window
		s_deviceContext = GetDC( s_renderingWindow );
		if ( s_deviceContext == NULL )
		{
			eae6320::UserOutput::Print( "Windows failed to get the device context" );
			return false;
		}
		// Windows requires that an OpenGL "render context" is made for the window we want to use to render into
		{
			// Set the pixel format of the rendering window
			{
				PIXELFORMATDESCRIPTOR desiredPixelFormat = { 0 };
				{
					desiredPixelFormat.nSize = sizeof( PIXELFORMATDESCRIPTOR );
					desiredPixelFormat.nVersion = 1;

					desiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
					desiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
					desiredPixelFormat.cColorBits = 32;
					desiredPixelFormat.iLayerType = PFD_MAIN_PLANE ;
					desiredPixelFormat.cDepthBits = 16;
				}
				// Get the ID of the desired pixel format
				int pixelFormatId;
				{
					pixelFormatId = ChoosePixelFormat( s_deviceContext, &desiredPixelFormat );
					if ( pixelFormatId == 0 )
					{
						std::stringstream errorMessage;
						errorMessage << "Windows couldn't choose the closest pixel format: " << eae6320::GetLastWindowsError();
						eae6320::UserOutput::Print( errorMessage.str() );
						return false;
					}
				}
				// Set it
				if ( SetPixelFormat( s_deviceContext, pixelFormatId, &desiredPixelFormat ) == FALSE )
				{
					std::stringstream errorMessage;
					errorMessage << "Windows couldn't set the desired pixel format: " << eae6320::GetLastWindowsError();
					eae6320::UserOutput::Print( errorMessage.str() );
					return false;
				}
			}
			// Create the OpenGL rendering context
			s_openGlRenderingContext = wglCreateContext( s_deviceContext );
			if ( s_openGlRenderingContext == NULL )
			{
				std::stringstream errorMessage;
				errorMessage << "Windows failed to create an OpenGL rendering context: " << eae6320::GetLastWindowsError();
				eae6320::UserOutput::Print( errorMessage.str() );
				return false;
			}
			// Set it as the rendering context of this thread
			if ( wglMakeCurrent( s_deviceContext, s_openGlRenderingContext ) == FALSE )
			{
				std::stringstream errorMessage;
				errorMessage << "Windows failed to set the current OpenGL rendering context: " << eae6320::GetLastWindowsError();
				eae6320::UserOutput::Print( errorMessage.str() );
				return false;
			}
		}

		return true;
	}

	bool LoadAndAllocateShaderProgram( const char* i_path, void*& o_shader, size_t& o_size, std::string* o_errorMessage )
	{
		bool wereThereErrors = false;

		// Load the shader source from disk
		o_shader = NULL;
		HANDLE fileHandle = INVALID_HANDLE_VALUE;
		{
			// Open the file
			{
				const DWORD desiredAccess = FILE_GENERIC_READ;
				const DWORD otherProgramsCanStillReadTheFile = FILE_SHARE_READ;
				SECURITY_ATTRIBUTES* useDefaultSecurity = NULL;
				const DWORD onlySucceedIfFileExists = OPEN_EXISTING;
				const DWORD useDefaultAttributes = FILE_ATTRIBUTE_NORMAL;
				const HANDLE dontUseTemplateFile = NULL;
				fileHandle = CreateFile( i_path, desiredAccess, otherProgramsCanStillReadTheFile,
					useDefaultSecurity, onlySucceedIfFileExists, useDefaultAttributes, dontUseTemplateFile );
				if ( fileHandle == INVALID_HANDLE_VALUE )
				{
					wereThereErrors = true;
					if ( o_errorMessage )
					{
						std::string windowsErrorMessage = eae6320::GetLastWindowsError();
						std::stringstream errorMessage;
						errorMessage << "Windows failed to open the shader file: " <<
							windowsErrorMessage;
						*o_errorMessage = errorMessage.str();
					}
					goto OnExit;
				}
			}
			// Get the file's size
			{
				LARGE_INTEGER fileSize_integer;
				if ( GetFileSizeEx( fileHandle, &fileSize_integer ) != FALSE )
				{
					assert( fileSize_integer.QuadPart <= SIZE_MAX );
					o_size = static_cast<size_t>( fileSize_integer.QuadPart );
				}
				else
				{
					wereThereErrors = true;
					if ( o_errorMessage )
					{
						std::string windowsErrorMessage = eae6320::GetLastWindowsError();
						std::stringstream errorMessage;
						errorMessage << "Windows failed to get the size of shader: " <<
							windowsErrorMessage;
						*o_errorMessage = errorMessage.str();
					}
					goto OnExit;
				}
				// Add an extra byte for a NULL terminator
				o_size += 1;
			}
			// Read the file's contents into temporary memory
			o_shader = malloc( o_size );
			if ( o_shader )
			{
				DWORD bytesReadCount;
				OVERLAPPED* readSynchronously = NULL;
				if ( ReadFile( fileHandle, o_shader, o_size,
					&bytesReadCount, readSynchronously ) == FALSE )
				{
					wereThereErrors = true;
					if ( o_errorMessage )
					{
						std::string windowsErrorMessage = eae6320::GetLastWindowsError();
						std::stringstream errorMessage;
						errorMessage << "Windows failed to read the contents of shader: " <<
							windowsErrorMessage;
						*o_errorMessage = errorMessage.str();
					}
					goto OnExit;
				}
			}
			else
			{
				wereThereErrors = true;
				if ( o_errorMessage )
				{
					std::stringstream errorMessage;
					errorMessage << "Failed to allocate " << o_size << " bytes to read in the shader program " << i_path;
					*o_errorMessage = errorMessage.str();
				}
				goto OnExit;
			}
			// Add the NULL terminator
			reinterpret_cast<char*>(o_shader)[o_size - 1] = '\0';
		}

	OnExit:

		if ( wereThereErrors && o_shader )
		{
			free( o_shader );
			o_shader = NULL;
		}
		if ( fileHandle != INVALID_HANDLE_VALUE )
		{
			if ( CloseHandle( fileHandle ) == FALSE )
			{
				if ( !wereThereErrors && o_errorMessage )
				{
					std::string windowsError = eae6320::GetLastWindowsError();
					std::stringstream errorMessage;
					errorMessage << "Windows failed to close the shader file handle: " <<
						windowsError;
					*o_errorMessage = errorMessage.str();
				}
				wereThereErrors = true;
			}
			fileHandle = INVALID_HANDLE_VALUE;
		}

		return !wereThereErrors;
	}
}

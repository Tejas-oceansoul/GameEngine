// Header Files
//=============

#include "Graphics.h"
#include "../../External/Lua/Includes.h"
#include "../UserOutput/UserOutput.h"
#include "Renderable.h"
#include "../Core/Math/cMatrix_transformation.h"

#include <cstdint>
#include <cassert>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

//Static Data Initialization
//==========================
namespace
{
	std::vector<eae6320::Graphics::Renderable*> s_opaqueRenderableList;
	std::vector<eae6320::Graphics::Renderable*> s_transparentRenderableList;
}

// Helper Function Declarations
//=============================

void eae6320::Graphics::Render()
{
	// Every frame an entirely new image will be created.
	// Before drawing anything, then, the previous image will be erased
	// by "clearing" the image buffer (filling it with a solid color)
	Clear();

	size_t opaqueSize = s_opaqueRenderableList.size();
	size_t transparentSize = s_transparentRenderableList.size();

	{
		BeginScene();
		{
			//Drawing Opaque objects.
			for (unsigned int i = 0; i < opaqueSize; i++)
			{
				Renderable toRender = *s_opaqueRenderableList[i];
				// Set the shaders
				{
					BindEffect(toRender.m_material.m_effect);
				}

				//Setting the Material uniforms
				{
					for (int i = 0; i < toRender.m_material.m_noOfUniforms; i++)
					{
						SetMaterialUniform(toRender.m_material.m_effect, toRender.m_material.m_uniforms[i].values, toRender.m_material.m_uniforms[i].valueCountToSet, toRender.m_material.m_uniforms[i].uniformHandle, toRender.m_material.m_uniforms[i].shaderType);
					}
				}

				//Setting the Textures
				{
					eae6320::Graphics::SetTexture(toRender.m_material, i);
				}

				//Setting uniform offset
				{
					float floatArray[2] = { toRender.m_offset.x, toRender.m_offset.y };
					//Generating localToWorld Matrix.
					eae6320::Math::cMatrix_transformation localToWorld(toRender.m_orientation, toRender.m_offset);
					SetDrawCallUniforms(toRender.m_material.m_effect, localToWorld);
				}
				// Drawing the Mesh
				{
					eae6320::Graphics::DrawMesh(toRender.m_mesh);
				}
			}

			//Drawing Transparent objects.
			for (unsigned int i = 0; i < transparentSize; i++)
			{
				Renderable toRender = *s_transparentRenderableList[i];
				// Set the shaders
				{
					BindEffect(toRender.m_material.m_effect);
				}

				//Setting the Material uniforms
				{
					for (int i = 0; i < toRender.m_material.m_noOfUniforms; i++)
					{
						SetMaterialUniform(toRender.m_material.m_effect, toRender.m_material.m_uniforms[i].values, toRender.m_material.m_uniforms[i].valueCountToSet, toRender.m_material.m_uniforms[i].uniformHandle, toRender.m_material.m_uniforms[i].shaderType);
					}
				}

				//Setting the Textures
				{
					eae6320::Graphics::SetTexture(toRender.m_material, i + opaqueSize);
				}

				//Setting uniform offset
				{
					float floatArray[2] = { toRender.m_offset.x, toRender.m_offset.y };
					//Generating localToWorld Matrix.
					eae6320::Math::cMatrix_transformation localToWorld(toRender.m_orientation, toRender.m_offset);
					SetDrawCallUniforms(toRender.m_material.m_effect, localToWorld);
				}
				// Drawing the Mesh
				{
					eae6320::Graphics::DrawMesh(toRender.m_mesh);
				}
			}
		}
		EndScene();
	}
	// Everything has been drawn to the "back buffer", which is just an image in memory.
	// In order to display it, the contents of the back buffer must be "presented"
	// (to the front buffer)
	SwapBuffers();
}

bool eae6320::Graphics::LoadMesh(const char* const i_path, Mesh& i_mesh)
{
	bool wereThereErrors = false;

	//Variable declerations.

	uint8_t *temporaryBuffer = NULL; //Declaring the temporary buffer to copy file contents into.

	//File operations.
	FILE *i_file;
	errno_t err;

	err = fopen_s(&i_file, i_path, "rb");
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
	temporaryBuffer = reinterpret_cast<uint8_t*>(malloc(sizeof(uint8_t) * fSize));

	if (temporaryBuffer == NULL)
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to allocate memory for buffer.\n";
		eae6320::UserOutput::Print(errorMessage.str());

		goto OnExit;
	}
	
	eae6320::Graphics::sVertex *o_vertexData = NULL;
	uint32_t *o_indexData = NULL;
	uint32_t o_noOfIndices, o_noOfVertices;

	//Making the read call.
	fread(temporaryBuffer, sizeof(uint8_t), fSize, i_file);

	o_noOfVertices = *reinterpret_cast<uint32_t*>(temporaryBuffer);
	o_noOfIndices = *reinterpret_cast<uint32_t*>(temporaryBuffer + 4);
	o_vertexData = reinterpret_cast<sVertex*>(temporaryBuffer + 8);
	o_indexData = reinterpret_cast<uint32_t*>(temporaryBuffer + (8 + o_noOfVertices*(sizeof(sVertex))));

	i_mesh.m_noOfVertices = o_noOfVertices;
	i_mesh.m_noOfIndices = o_noOfIndices;

	if (o_indexData == NULL || o_vertexData == NULL)
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to correctly create arrays.\n";
		eae6320::UserOutput::Print(errorMessage.str());

		goto OnExit;
	}

	if(!wereThereErrors)
		eae6320::Graphics::CreateBuffers(o_vertexData, o_noOfVertices, o_indexData, o_noOfIndices, i_mesh);

	goto OnExit;

OnExit:

	free(temporaryBuffer);

	if (!wereThereErrors) 
	{
		o_vertexData = NULL;
		o_indexData = NULL;
	}

	return !wereThereErrors;
}

bool eae6320::Graphics::LoadMaterial(const char* const i_path, Material& i_material)
{
	bool wereThereErrors = false;

	//Variable declerations.

	uint8_t *temporaryBuffer = NULL; //Declaring the temporary buffer to copy file contents into.

									 //File operations.
	FILE *i_file;
	errno_t err;

	err = fopen_s(&i_file, i_path, "rb");
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
	temporaryBuffer = reinterpret_cast<uint8_t*>(malloc(sizeof(uint8_t) * fSize));

	if (temporaryBuffer == NULL)
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to allocate memory for buffer.\n";
		eae6320::UserOutput::Print(errorMessage.str());

		goto OnExit;
	}

	//Making the read call.
	fread(temporaryBuffer, sizeof(uint8_t), fSize, i_file);
	
	//Read effect path and load it
	const char *i_effectPath = reinterpret_cast<char*>(temporaryBuffer);
	eae6320::Graphics::LoadEffect(i_effectPath, i_material.m_effect);

	//Updating offset
	size_t offset = strlen(i_effectPath) + 1;

	//Read texture handle
	const char *i_textureHandle = reinterpret_cast<char*>(temporaryBuffer + offset);

	//Updating offset
	offset += strlen(i_textureHandle) + 1;

	//Read texture path
	const char *i_texturePath = reinterpret_cast<char*>(temporaryBuffer + offset);

	//Updating offset
	offset += strlen(i_texturePath) + 1;
	
	//Load Texture
	eae6320::Graphics::LoadTexture(i_texturePath, i_material);

	//Get the handles
	eae6320::Graphics::LoadSamplerID(i_textureHandle, i_material);

	//Read number of uniforms.
	i_material.m_noOfUniforms = *reinterpret_cast<uint8_t*>(temporaryBuffer + offset);

	//Load the sUniformHelpers
	sUniformHelper *uniformsArray = NULL;
	offset += 1;
	uniformsArray = reinterpret_cast<sUniformHelper*>(temporaryBuffer + offset);

	i_material.m_uniforms = new sUniformHelper[i_material.m_noOfUniforms];
	std::memcpy(i_material.m_uniforms, uniformsArray, (sizeof(sUniformHelper) * i_material.m_noOfUniforms));

	//Load the uniform names.
	char** uniformNamesArray = NULL;
	uniformNamesArray = new char*[i_material.m_noOfUniforms];
	offset += (sizeof(sUniformHelper) * i_material.m_noOfUniforms);

	for (int i = 0; i < i_material.m_noOfUniforms; i++)
	{
		uniformNamesArray[i] = reinterpret_cast<char*>(temporaryBuffer + offset);
		offset += strlen(uniformNamesArray[i]) + 1;
	}

	for (int i = 0; i < i_material.m_noOfUniforms; i++)
	{
		i_material.m_uniforms[i].uniformHandle = GetUniform(i_material.m_effect, uniformNamesArray[i], i_material.m_uniforms[i].shaderType);
	}

	goto OnExit;

OnExit:

	free(temporaryBuffer);

	if (!wereThereErrors)
	{
		uniformsArray = NULL;
		uniformNamesArray = NULL;
	}

	return !wereThereErrors;
}

std::vector<eae6320::Graphics::Renderable*>* eae6320::Graphics::GetOpaqueRenderableList()
{
	return &s_opaqueRenderableList;
}

std::vector<eae6320::Graphics::Renderable*>* eae6320::Graphics::GetTransparentRenderableList()
{
	return &s_transparentRenderableList;
}
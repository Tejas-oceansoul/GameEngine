// Header Files
//=============

#include "../Graphics.h"
#include "../Renderable.h"

#include <cassert>
#include <cstdint>
#include <d3d9.h>
#include <d3dx9shader.h>
#include <sstream>
#include <vector>
#include "../../UserOutput/UserOutput.h"
#include "../../Core/Math/cMatrix_transformation.h"
#include "../../Core/Math/Functions.h"

// Static Data Initialization
//===========================

namespace
{
	HWND s_renderingWindow = NULL;
	IDirect3D9* s_direct3dInterface = NULL;
	IDirect3DDevice9* s_direct3dDevice = NULL;

	eae6320::Graphics::Mesh s_MeshSquare;
	eae6320::Graphics::Mesh s_MeshTriangle;
	
	eae6320::Graphics::Effect *s_effect;

	// This struct determines the layout of the data that the CPU will send to the GPU
	//IDirect3DVertexDeclaration9* s_vertexDeclaration = NULL;

	// The vertex buffer holds the data for each vertex
	//IDirect3DVertexBuffer9* s_vertexBuffer = NULL;
	// An index buffer describes how to make triangles with the vertices
	// (i.e. it defines the vertex connectivity)
	//IDirect3DIndexBuffer9* s_indexBuffer = NULL;

	// The vertex shader is a program that operates on vertices.
	// Its input comes from a C/C++ "draw call" and is:
	//	* Position
	//	* Any other data we want
	// Its output is:
	//	* Position
	//		(So that the graphics hardware knows which pixels to fill in for the triangle)
	//	* Any other data we want
	//IDirect3DVertexShader9* s_vertexShader = NULL;
	// The fragment shader is a program that operates on fragments
	// (or potential pixels).
	// Its input is:
	//	* The data that was output from the vertex shader,
	//		interpolated based on how close the fragment is
	//		to each vertex in the triangle.
	// Its output is:
	//	* The final color that the pixel should be
	//IDirect3DPixelShader9* s_fragmentShader = NULL;
}

// Helper Function Declarations
//=============================

namespace
{
	bool CreateDevice();
	bool CreateInterface();
	HRESULT GetVertexProcessingUsage( DWORD& o_usage );
}

// Interface
//==========

bool eae6320::Graphics::Initialize( const HWND i_renderingWindow )
{
	s_renderingWindow = i_renderingWindow;

	// Initialize the interface to the Direct3D9 library
	if ( !CreateInterface() )
	{
		return false;
	}
	// Create an interface to a Direct3D device
	if ( !CreateDevice() )
	{
		goto OnError;
	}
	//Setting Render States.
	{
		HRESULT result;
		result = s_direct3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		assert(!FAILED(result));
		result = s_direct3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		assert(!FAILED(result));
		result = s_direct3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		assert(!FAILED(result));
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
	if(i_renderable->m_material.m_effect.m_renderStates & alpha)
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
	{//Creating Index Buffer
	// The usage tells Direct3D how this vertex buffer will be used
		DWORD usage = 0;
		{
			// The type of vertex processing should match what was specified when the device interface was created with CreateDevice()
			const HRESULT result = GetVertexProcessingUsage(usage);
			if (FAILED(result))
			{
				return false;
			}
			// Our code will only ever write to the buffer
			usage |= D3DUSAGE_WRITEONLY;
		}

		// Create an index buffer
		unsigned int bufferSize;
		{
			bufferSize = i_mesh.m_noOfIndices * sizeof(uint32_t);
			// We'll use 32-bit indices in this class to keep things simple
			// (i.e. every index will be a 32 bit unsigned integer)
			const D3DFORMAT format = D3DFMT_INDEX32;
			// Place the index buffer into memory that Direct3D thinks is the most appropriate
			const D3DPOOL useDefaultPool = D3DPOOL_DEFAULT;
			HANDLE* notUsed = NULL;
			const HRESULT result = s_direct3dDevice->CreateIndexBuffer(bufferSize, usage, format, useDefaultPool,
				&(i_mesh.m_indexBuffer), notUsed);
			if (FAILED(result))
			{
				eae6320::UserOutput::Print("Direct3D failed to create an index buffer");
				return false;
			}
		}
		// Fill the index buffer with the triangles' connectivity data
		{
			// Before the index buffer can be changed it must be "locked"
			uint32_t* indexData;
			{
				const unsigned int lockEntireBuffer = 0;
				const DWORD useDefaultLockingBehavior = 0;
				const HRESULT result = i_mesh.m_indexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
					reinterpret_cast<void**>(&indexData), useDefaultLockingBehavior);
				if (FAILED(result))
				{
					eae6320::UserOutput::Print("Direct3D failed to lock the index buffer");
					return false;
				}
			}
			// Fill the buffer
			{
				// EAE6320_TODO:
				// You will need to fill in 3 indices for each triangle that needs to be drawn.
				// Each index will be a 32-bit unsigned integer,
				// and will index into the vertex buffer array that you have created.
				// The order of indices is important, but the correct order will depend on
				// which vertex you have assigned to which spot in your vertex buffer
				// (also remember to maintain the correct handedness for the triangle winding order).
				//for (int i = 0; i < i_numOfIndices; i++)
				//{
				//	indexData[i] = i_indexData[i];
				//}

				std::memcpy(indexData, i_indexData, (i_mesh.m_noOfIndices * sizeof(uint32_t)));
			}
			// The buffer must be "unlocked" before it can be used
			{
				const HRESULT result = i_mesh.m_indexBuffer->Unlock();
				if (FAILED(result))
				{
					eae6320::UserOutput::Print("Direct3D failed to unlock the index buffer");
					return false;
				}
			}
		}
	}
	
	{
		//Creating Vertex Buffer
		// The usage tells Direct3D how this vertex buffer will be used
		DWORD usage = 0;
		{
			// The type of vertex processing should match what was specified when the device interface was created with CreateDevice()
			const HRESULT result = GetVertexProcessingUsage(usage);
			if (FAILED(result))
			{
				return false;
			}
			// Our code will only ever write to the buffer
			usage |= D3DUSAGE_WRITEONLY;
		}

		// Initialize the vertex format
		{
			// These elements must match the VertexFormat::sVertex layout struct exactly.
			// They instruct Direct3D how to match the binary data in the vertex buffer
			// to the input elements in a vertex shader
			// (by using D3DDECLUSAGE enums here and semantics in the shader,
			// so that, for example, D3DDECLUSAGE_POSITION here matches with POSITION in shader code).
			// Note that OpenGL uses arbitrarily assignable number IDs to do the same thing.
			D3DVERTEXELEMENT9 vertexElements[] =
			{
				// Stream 0

				// POSITION
				// 3 floats == 12 bytes
				// Offset = 0
				{ 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },

				// TEXTURE COORDINATES
				// 2 floats == 8 bytes
				// Offset = 12
				{ 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },

				// COLOR0
				// D3DCOLOR == 4 bytes
				// Offset = 20
				{ 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },

				// The following marker signals the end of the vertex declaration
				D3DDECL_END()
			};
			HRESULT result = s_direct3dDevice->CreateVertexDeclaration(vertexElements, &(i_mesh.m_vertexDeclaration));
			if (SUCCEEDED(result))
			{
				result = s_direct3dDevice->SetVertexDeclaration(i_mesh.m_vertexDeclaration);
				if (FAILED(result))
				{
					eae6320::UserOutput::Print("Direct3D failed to set the vertex declaration");
					return false;
				}
			}
			else
			{
				eae6320::UserOutput::Print("Direct3D failed to create a Direct3D9 vertex declaration");
				return false;
			}
		}

		// Create a vertex buffer
		{
			// We are drawing one square
			//const unsigned int vertexCount = i_numOfVertices;	// What is the minimum number of vertices a square needs (so that no data is duplicated)?
			const unsigned int bufferSize = i_mesh.m_noOfIndices * sizeof(sVertex);
			// We will define our own vertex format
			const DWORD useSeparateVertexDeclaration = 0;
			// Place the vertex buffer into memory that Direct3D thinks is the most appropriate
			const D3DPOOL useDefaultPool = D3DPOOL_DEFAULT;
			HANDLE* const notUsed = NULL;
			const HRESULT result = s_direct3dDevice->CreateVertexBuffer(bufferSize, usage, useSeparateVertexDeclaration, useDefaultPool,
				&(i_mesh.m_vertexBuffer), notUsed);
			if (FAILED(result))
			{
				eae6320::UserOutput::Print("Direct3D failed to create a vertex buffer");
				return false;
			}
		}
		// Fill the vertex buffer with the triangle's vertices
		{
			// Before the vertex buffer can be changed it must be "locked"
			sVertex* vertexData;
			{
				const unsigned int lockEntireBuffer = 0;
				const DWORD useDefaultLockingBehavior = 0;
				const HRESULT result = i_mesh.m_vertexBuffer->Lock(lockEntireBuffer, lockEntireBuffer,
					reinterpret_cast<void**>(&vertexData), useDefaultLockingBehavior);
				if (FAILED(result))
				{
					eae6320::UserOutput::Print("Direct3D failed to lock the vertex buffer");
					return false;
				}
			}
			// Fill the buffer
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
			// The buffer must be "unlocked" before it can be used
			{
				const HRESULT result = i_mesh.m_vertexBuffer->Unlock();
				if (FAILED(result))
				{
					eae6320::UserOutput::Print("Direct3D failed to unlock the vertex buffer");
					return false;
				}
			}
		}
	}

	return true;
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
		errorMessage << "Failed to load/open one or more effect File\n";
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

	fclose(i_file);

	//Loading the fragment shader.
	// Copy Compiled code from shader file
	char *temporaryFragmentBuffer = NULL;

	err = fopen_s(&i_file, i_fragmentPath, "rb");
	if (err != 0)
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to load/open one or more shader File\n";
		eae6320::UserOutput::Print(errorMessage.str());

		goto OnExit;
	}


	//Getting File size to initialize Buffer.
	fseek(i_file, 0, SEEK_END);
	fSize = ftell(i_file);
	rewind(i_file);

	//Initializing the Buffer
	temporaryFragmentBuffer = reinterpret_cast<char*>(malloc(sizeof(uint8_t) * fSize));

	if (temporaryFragmentBuffer == NULL)
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to allocate memory for buffer.\n";
		eae6320::UserOutput::Print(errorMessage.str());

		goto OnExit;
	}

	fread(temporaryFragmentBuffer, sizeof(uint8_t), fSize, i_file);
	ID3DXBuffer* compiledShader = reinterpret_cast<ID3DXBuffer*>(temporaryFragmentBuffer);
	D3DXGetShaderConstantTable(reinterpret_cast<const DWORD*>(temporaryFragmentBuffer), &i_effect.fragmentShaderConstantTable);
	
	// Create the fragment shader object
	{
		HRESULT result = s_direct3dDevice->CreatePixelShader(reinterpret_cast<DWORD*>(compiledShader),
			&i_effect.m_pixelShader);
		if (FAILED(result))
		{
			eae6320::UserOutput::Print("Direct3D failed to create the fragment shader");
			wereThereErrors = true;
		}
	}

	fclose(i_file);
	free(temporaryFragmentBuffer);
	temporaryFragmentBuffer = NULL;

	//Loading the vertex shader.
	// Copy Compiled code from shader file
	char *temporaryVertexBuffer = NULL;

	err = fopen_s(&i_file, i_vertexPath, "rb");
	if (err != 0)
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to load/open one or more shader File\n";
		eae6320::UserOutput::Print(errorMessage.str());

		goto OnExit;
	}

	//Getting File size to initialize Buffer.
	fseek(i_file, 0, SEEK_END);
	fSize = ftell(i_file);
	rewind(i_file);

	//Initializing the Buffer
	temporaryVertexBuffer = reinterpret_cast<char*>(malloc(sizeof(uint8_t) * fSize));

	if (temporaryVertexBuffer == NULL)
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to allocate memory for buffer.\n";
		eae6320::UserOutput::Print(errorMessage.str());

		goto OnExit;
	}

	fread(temporaryVertexBuffer, sizeof(uint8_t), fSize, i_file);
	compiledShader = reinterpret_cast<ID3DXBuffer*>(temporaryVertexBuffer);
	D3DXGetShaderConstantTable(reinterpret_cast<const DWORD*>(temporaryVertexBuffer), &i_effect.vertexShaderConstantTable);
	
	//Getting Handle from ConstantTable
	{
		//i_effect.vhandle = i_effect.vertexShaderConstantTable->GetConstantByName(NULL, "g_position_offset");
		i_effect.localToWorld = i_effect.vertexShaderConstantTable->GetConstantByName(NULL, "g_transform_localToWorld");
		i_effect.worldToView = i_effect.vertexShaderConstantTable->GetConstantByName(NULL, "g_transform_worldToView");
		i_effect.viewToScreen = i_effect.vertexShaderConstantTable->GetConstantByName(NULL, "g_transform_viewToScreen");
		if (i_effect.localToWorld == NULL || i_effect.worldToView == NULL || i_effect.viewToScreen== NULL)
		{
			eae6320::UserOutput::Print("Direct3D failed to get handle from constant table");
			wereThereErrors = true;
		}
	}
	// Create the vertex shader object
	{
		HRESULT result = s_direct3dDevice->CreateVertexShader(reinterpret_cast<DWORD*>(compiledShader),
			&i_effect.m_vertexShader);
		if (FAILED(result))
		{
			eae6320::UserOutput::Print("Direct3D failed to create the vertex shader");
			wereThereErrors = true;
		}
	}

	fclose(i_file);
	free(temporaryVertexBuffer);
	temporaryVertexBuffer = NULL;

	goto OnExit;
OnExit:

	free(temporaryBuffer);
	return !wereThereErrors;
}

bool eae6320::Graphics::LoadTexture(const char* const i_path, Material& i_material)
{
	const unsigned int useDimensionsFromFile = D3DX_DEFAULT_NONPOW2;
	const unsigned int useMipMapsFromFile = D3DX_FROM_FILE;
	const DWORD staticTexture = 0;
	const D3DFORMAT useFormatFromFile = D3DFMT_FROM_FILE;
	const D3DPOOL letD3dManageMemory = D3DPOOL_MANAGED;
	const DWORD useDefaultFiltering = D3DX_DEFAULT;
	const D3DCOLOR noColorKey = 0;
	D3DXIMAGE_INFO* noSourceInfo = NULL;
	PALETTEENTRY* noColorPalette = NULL;
	const HRESULT result = D3DXCreateTextureFromFileEx(s_direct3dDevice, i_path, useDimensionsFromFile, useDimensionsFromFile, useMipMapsFromFile,
		staticTexture, useFormatFromFile, letD3dManageMemory, useDefaultFiltering, useDefaultFiltering, noColorKey, noSourceInfo, noColorPalette,
		&(i_material.m_3dTexture));

	return true;
}

eae6320::Graphics::tUniformHandle eae6320::Graphics::GetUniform(Effect& i_effect, const char* const i_uniformName, eShaderType i_shaderType)
{
	if (i_shaderType == eShaderType::fragment)
		return i_effect.fragmentShaderConstantTable->GetConstantByName(NULL, i_uniformName);
	else
		return i_effect.vertexShaderConstantTable->GetConstantByName(NULL, i_uniformName);
}

void eae6320::Graphics::Clear()
{
	const D3DRECT* subRectanglesToClear = NULL;
	const DWORD subRectangleCount = 0;
	const DWORD clearTheRenderTarget = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
	D3DCOLOR clearColor;
	{
		// Black is usually used:
		clearColor = D3DCOLOR_XRGB(0, 0, 0);
	}
	const float noZBuffer = 1.0f;
	const DWORD noStencilBuffer = 0;
	HRESULT result = s_direct3dDevice->Clear(subRectangleCount, subRectanglesToClear,
		clearTheRenderTarget, clearColor, noZBuffer, noStencilBuffer);
	assert(SUCCEEDED(result));
}

void eae6320::Graphics::BeginScene()
{
	HRESULT result = s_direct3dDevice->BeginScene();
	assert(SUCCEEDED(result));
}

void eae6320::Graphics::EndScene()
{
	HRESULT result = s_direct3dDevice->EndScene();
	assert(SUCCEEDED(result));
}

void eae6320::Graphics::SwapBuffers()
{
	const RECT* noSourceRectangle = NULL;
	const RECT* noDestinationRectangle = NULL;
	const HWND useDefaultWindow = NULL;
	const RGNDATA* noDirtyRegion = NULL;
	HRESULT result = s_direct3dDevice->Present(noSourceRectangle, noDestinationRectangle, useDefaultWindow, noDirtyRegion);
	assert(SUCCEEDED(result));
}

bool eae6320::Graphics::LoadSamplerID(const char* const i_uniformName, Material& i_material)
{
	D3DXHANDLE m_handle = i_material.m_effect.fragmentShaderConstantTable->GetConstantByName(NULL, i_uniformName);
	i_material.m_texHandle = static_cast<DWORD>(i_material.m_effect.fragmentShaderConstantTable->GetSamplerIndex(m_handle));

	return true;
}

bool eae6320::Graphics::SetTexture(Material& i_material, int i_offset)
{
	s_direct3dDevice->SetTexture(i_material.m_texHandle, i_material.m_3dTexture);

	return true;
}

void eae6320::Graphics::DrawMesh(const eae6320::Graphics::Mesh& i_Mesh)
{
	// Bind a specific vertex buffer to the device as a data source
	{
		// There can be multiple streams of data feeding the display adaptor at the same time
		const unsigned int streamIndex = 0;
		// It's possible to start streaming data in the middle of a vertex buffer
		const unsigned int bufferOffset = 0;
		// The "stride" defines how large a single vertex is in the stream of data
		const unsigned int bufferStride = sizeof(sVertex);
		HRESULT result = s_direct3dDevice->SetStreamSource(streamIndex, i_Mesh.m_vertexBuffer, bufferOffset, bufferStride);
		assert(SUCCEEDED(result));

	}
	// Bind a specific index buffer to the device as a data source
	{
		HRESULT result = s_direct3dDevice->SetIndices(i_Mesh.m_indexBuffer);
		assert(SUCCEEDED(result));
	}
	// Render objects from the current streams
	{
		// We are using triangles as the "primitive" type,
		// and we have defined the vertex buffer as a triangle list
		// (meaning that every triangle is defined by three vertices)
		const D3DPRIMITIVETYPE primitiveType = D3DPT_TRIANGLELIST;
		// It's possible to start rendering primitives in the middle of the stream
		const unsigned int indexOfFirstVertexToRender = 0;
		const unsigned int indexOfFirstIndexToUse = 0;
		// We are drawing a square
		const unsigned int vertexCountToRender = i_Mesh.m_noOfVertices;	// How vertices from the vertex buffer will be used?
		const unsigned int primitiveCountToRender = (int)(i_Mesh.m_noOfIndices / 3.0f);	// How many triangles will be drawn?
		HRESULT result = s_direct3dDevice->DrawIndexedPrimitive(primitiveType,
			indexOfFirstVertexToRender, indexOfFirstVertexToRender, vertexCountToRender,
			indexOfFirstIndexToUse, primitiveCountToRender);
		assert(SUCCEEDED(result));
	}
}

bool eae6320::Graphics::SetDrawCallUniforms(Effect& i_effect, eae6320::Math::cMatrix_transformation& i_offsetMatrix)
{
	HRESULT result;
	result = i_effect.vertexShaderConstantTable->SetMatrixTranspose(s_direct3dDevice, i_effect.localToWorld, reinterpret_cast<const D3DXMATRIX*>(&i_offsetMatrix));
	assert(SUCCEEDED(result));

	eae6320::Math::cMatrix_transformation worldToView = eae6320::Math::cMatrix_transformation::CreateWorldToViewTransform(Camera::getInstance().m_orientation, Camera::getInstance().m_offset);
	result = i_effect.vertexShaderConstantTable->SetMatrixTranspose(s_direct3dDevice, i_effect.worldToView, reinterpret_cast<const D3DXMATRIX*>(&worldToView));
	assert(SUCCEEDED(result));

	eae6320::Math::cMatrix_transformation viewToScreen = eae6320::Math::cMatrix_transformation::CreateViewToScreenTransform(Camera::getInstance().FOV, getAspectRatio(), 0.1f, 100.0f);
	result = i_effect.vertexShaderConstantTable->SetMatrixTranspose(s_direct3dDevice, i_effect.viewToScreen, reinterpret_cast<const D3DXMATRIX*>(&viewToScreen));
	assert(SUCCEEDED(result));

	return true;
}

void eae6320::Graphics::SetMaterialUniform(const Effect& i_effect, float i_values[], uint8_t i_valueCountToSet, tUniformHandle i_uniformHandle, eShaderType i_shaderType)
{
	if(i_shaderType == eShaderType::fragment)
		i_effect.fragmentShaderConstantTable->SetFloatArray(s_direct3dDevice, i_uniformHandle, i_values, i_valueCountToSet);
	else
		i_effect.vertexShaderConstantTable->SetFloatArray(s_direct3dDevice, i_uniformHandle, i_values, i_valueCountToSet);
}

bool eae6320::Graphics::BindEffect(Effect& i_effect)
{
	HRESULT result;
	result = s_direct3dDevice->SetVertexShader(i_effect.m_vertexShader);
	assert(SUCCEEDED(result));
	result = s_direct3dDevice->SetPixelShader(i_effect.m_pixelShader);
	assert(SUCCEEDED(result));

	//Set Alpha rendering state.
	if (i_effect.m_renderStates & alpha)
	{
		result = s_direct3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		assert(SUCCEEDED(result));
		result = s_direct3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		assert(SUCCEEDED(result));
		result = s_direct3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		assert(SUCCEEDED(result));
	}
	else
	{
		result = s_direct3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		assert(SUCCEEDED(result));
	}

	//Set Depth Test rendering state.
	if (i_effect.m_renderStates & depthtest)
	{
		result = s_direct3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		assert(SUCCEEDED(result));
		result = s_direct3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		assert(SUCCEEDED(result));
	}
	else
	{
		result = s_direct3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		assert(SUCCEEDED(result));
	}

	//Set Depth Write rendering state.
	if (i_effect.m_renderStates & depthwrite)
	{
		result = s_direct3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		assert(SUCCEEDED(result));
	}
	else
	{
		result = s_direct3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		assert(SUCCEEDED(result));
	}

	//Set Face Culling rendering state.
	if (i_effect.m_renderStates & faceculling)
	{
		result = s_direct3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); //Only Render Front Faces. Cull back faces.
		assert(SUCCEEDED(result));
	}
	else
	{
		result = s_direct3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		assert(SUCCEEDED(result));
	}

	return true;
}

bool eae6320::Graphics::ShutDown()
{
	bool wereThereErrors = false;
	if ( s_direct3dInterface )
	{
		if ( s_direct3dDevice )
		{
			std::vector<eae6320::Graphics::Renderable*>* m_renderableList = GetOpaqueRenderableList();
			size_t size = m_renderableList->size();
			for (unsigned int i = 0; i < size; i++)
			{
				(*m_renderableList)[i]->m_mesh.m_indexBuffer->Release();
				(*m_renderableList)[i]->m_mesh.m_vertexBuffer->Release();
				(*m_renderableList)[i]->m_mesh.m_vertexDeclaration->Release();

				(*m_renderableList)[i]->m_material.m_effect.m_vertexShader->Release();
				(*m_renderableList)[i]->m_material.m_effect.m_pixelShader->Release();
			}
			m_renderableList = GetTransparentRenderableList();
			size = m_renderableList->size();
			for (unsigned int i = 0; i < size; i++)
			{
				(*m_renderableList)[i]->m_mesh.m_indexBuffer->Release();
				(*m_renderableList)[i]->m_mesh.m_vertexBuffer->Release();
				(*m_renderableList)[i]->m_mesh.m_vertexDeclaration->Release();

				(*m_renderableList)[i]->m_material.m_effect.m_vertexShader->Release();
				(*m_renderableList)[i]->m_material.m_effect.m_pixelShader->Release();
			}

			s_direct3dDevice->SetVertexDeclaration(NULL);
			s_direct3dDevice->Release();
			s_direct3dDevice = NULL;
		}

		s_direct3dInterface->Release();
		s_direct3dInterface = NULL;
	}
	s_renderingWindow = NULL;

	return !wereThereErrors;
}

// Helper Function Definitions
//============================

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
	bool CreateDevice()
	{
		const UINT useDefaultDevice = D3DADAPTER_DEFAULT;
		const D3DDEVTYPE useHardwareRendering = D3DDEVTYPE_HAL;
		const DWORD useHardwareVertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
		D3DPRESENT_PARAMETERS presentationParameters = { 0 };
		{
			{
				const unsigned int useRenderingWindowDimensions = 0;
				presentationParameters.BackBufferWidth = useRenderingWindowDimensions;
				presentationParameters.BackBufferHeight = useRenderingWindowDimensions;
			}
			presentationParameters.BackBufferFormat = D3DFMT_X8R8G8B8;
			presentationParameters.BackBufferCount = 1;
			presentationParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
			presentationParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
			presentationParameters.hDeviceWindow = s_renderingWindow;
			presentationParameters.Windowed = TRUE;
			presentationParameters.EnableAutoDepthStencil = TRUE;
			presentationParameters.AutoDepthStencilFormat = D3DFMT_D16;
			presentationParameters.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
		}
		HRESULT result = s_direct3dInterface->CreateDevice( useDefaultDevice, useHardwareRendering,
			s_renderingWindow, useHardwareVertexProcessing, &presentationParameters, &s_direct3dDevice );
		if ( SUCCEEDED( result ) )
		{
			return true;
		}
		else
		{
			eae6320::UserOutput::Print( "Direct3D failed to create a Direct3D9 device" );
			return false;
		}
	}

	bool CreateInterface()
	{
		// D3D_SDK_VERSION is #defined by the Direct3D header files,
		// and is just a way to make sure that everything is up-to-date
		s_direct3dInterface = Direct3DCreate9( D3D_SDK_VERSION );
		if ( s_direct3dInterface )
		{
			return true;
		}
		else
		{
			eae6320::UserOutput::Print( "DirectX failed to create a Direct3D9 interface" );
			return false;
		}
	}

	HRESULT GetVertexProcessingUsage( DWORD& o_usage )
	{
		D3DDEVICE_CREATION_PARAMETERS deviceCreationParameters;
		const HRESULT result = s_direct3dDevice->GetCreationParameters( &deviceCreationParameters );
		if ( SUCCEEDED( result ) )
		{
			DWORD vertexProcessingType = deviceCreationParameters.BehaviorFlags &
				( D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_SOFTWARE_VERTEXPROCESSING );
			o_usage = ( vertexProcessingType != D3DCREATE_SOFTWARE_VERTEXPROCESSING ) ? 0 : D3DUSAGE_SOFTWAREPROCESSING;
		}
		else
		{
			eae6320::UserOutput::Print( "Direct3D failed to get the device's creation parameters" );
		}
		return result;
	}
}

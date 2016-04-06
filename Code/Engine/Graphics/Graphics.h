/*
	This file contains the function declarations for graphics
*/

#ifndef EAE6320_GRAPHICS_H
#define EAE6320_GRAPHICS_H

// Header Files
//=============
#include <cstdint>
#include <vector>

#include "../Windows/Includes.h"
#include "../Core/Math/cMatrix_transformation.h"
#include "../Graphics/Camera.h"
#if defined(EAE6320_PLATFORM_D3D)
#include <d3d9.h>
#include <d3dx9shader.h>
#elif defined(EAE6320_PLATFORM_GL)
#include <gl/GL.h>
#include <gl/GLU.h>
#endif

// Interface
//==========

namespace eae6320
{
	namespace Graphics
	{
		class Renderable;
		struct Mesh 
		{
			uint32_t m_noOfIndices, m_noOfVertices;
			//Mesh structure for Direct X
#if defined(EAE6320_PLATFORM_D3D)
			IDirect3DVertexBuffer9* m_vertexBuffer = NULL;
			IDirect3DIndexBuffer9* m_indexBuffer = NULL;
			IDirect3DVertexDeclaration9* m_vertexDeclaration = NULL;

			~Mesh()
			{
				m_vertexBuffer = NULL;
				m_indexBuffer = NULL;
				m_vertexDeclaration = NULL;
			}

			//Mesh structure for OpenGL
#elif defined(EAE6320_PLATFORM_GL)
			GLuint m_vertexArrayID;
#endif
		};

		//Decleration of Renderstates
		enum RenderStates : uint8_t
		{
			alpha = 1,
			depthtest = 1 << 1,
			depthwrite = 1 << 2,
			faceculling = 1 << 3,
		};

		struct Effect
		{
			uint8_t m_renderStates;
#if defined(EAE6320_PLATFORM_D3D)
			//Effect structure for DirectX.
			IDirect3DVertexShader9 *m_vertexShader				= NULL;
			IDirect3DPixelShader9 *m_pixelShader				= NULL;
			ID3DXConstantTable *vertexShaderConstantTable;
			ID3DXConstantTable *fragmentShaderConstantTable;
			D3DXHANDLE localToWorld								= NULL;
			D3DXHANDLE worldToView								= NULL;
			D3DXHANDLE viewToScreen								= NULL;
			~Effect()
			{
				//m_vertexShader->Release();
				m_vertexShader = NULL;
				//Release->m_pixelShader();
				m_pixelShader = NULL;
			}

			//Effect structure for OpenGL.
#elif defined(EAE6320_PLATFORM_GL)
			GLuint m_programID	= 0;
			GLint localToWorld	= -1;
			GLint worldToView	= -1;
			GLint viewToScreen	= -1;
#endif
		};
		
		struct sVertex
		{
			// POSITION
			// 3 floats == 12 bytes
			// Offset = 0
			float x, y, z;
			// TEXTURE COORDINATES
			// 2 floats == 8 bytes
			// Offset = 12
			float u, v;
			// COLOR0
			// 4 uint8_ts == 4 bytes
			// Offset = 20
#if defined(EAE6320_PLATFORM_D3D)
			uint8_t b, g, r, a;	// 8 bits [0,255] per RGBA channel (the alpha channel is unused but is present so that color uses a full 4 bytes)
#elif defined(EAE6320_PLATFORM_GL)
			uint8_t r, g, b, a;	// 8 bits [0,255] per RGBA channel (the alpha channel is unused but is present so that color uses a full 4 bytes)
#endif
		};

		struct Context
		{
#if defined(EAE6320_PLATFORM_D3D)
			IDirect3DDevice9* m_direct3dDevice;
#elif defined(EAE6320_PLATFORM_GL)
			HGLRC m_openGlRenderingContext;
#endif
		};

		//For the Materials
		typedef
#if defined( EAE6320_PLATFORM_D3D )
			// This is conceptually a D3DXHANDLE but is defined this way
			// so that external files aren't required to specify the #include path to the DirectX SDK
			const char*
#elif defined( EAE6320_PLATFORM_GL )
			GLint
#endif
			tUniformHandle;

		enum eShaderType : uint8_t
		{
			fragment,
			vertex,
		};

		struct sUniformHelper {
			tUniformHandle uniformHandle = 0;
			float values[4];
			uint8_t valueCountToSet;
			eShaderType shaderType;
		};

		//Material struct decleration
		struct Material 
		{
			Effect m_effect;
			sUniformHelper* m_uniforms = NULL;
			uint8_t m_noOfUniforms;
#if defined(EAE6320_PLATFORM_D3D)
			IDirect3DTexture9* m_3dTexture;
			DWORD m_texHandle;
#elif defined(EAE6320_PLATFORM_GL)
			GLuint m_3dTexture;
			GLint m_texHandle;
#endif
		};

		bool Initialize( const HWND i_renderingWindow );
		void Render();
		bool ShutDown();

		//Functions that load and draw meshes and other auxillary functions.
		void DrawMesh( const Mesh& i_mesh );
		bool LoadMesh(const char* const i_path, Mesh& i_mesh );
		bool CreateBuffers(sVertex*& i_vertexData, int i_numOfVertices, uint32_t*& i_indexData, int i_numOfIndices, Mesh& i_mesh );

		//Functions that load and bind effects and other auxillary functions.
		bool LoadEffect(const char* const i_effectPath, Effect& i_effect);
		bool BindEffect(Effect& i_effect);

		//Functions that deal with Materials
		bool LoadMaterial(const char* const i_path, Material& i_material);
		bool LoadTexture(const char* const i_path, Material& i_material);
		bool LoadSamplerID(const char* const i_uniformName, Material& i_material);
		bool SetTexture(Material& i_material, int i_offset);

		//Functions that deal with the uniforms.
		bool SetDrawCallUniforms(Effect& i_effect, eae6320::Math::cMatrix_transformation& i_offsetMatrix);
		tUniformHandle GetUniform(Effect& i_effect, const char* const i_uniformName, eShaderType i_shaderType);
		void SetMaterialUniform(const Effect& i_effect, float i_values[], uint8_t i_valueCountToSet, tUniformHandle i_uniformHandle, eShaderType i_shaderType);

		//Accessor Functions.
		void AddRenderable(const char* const i_pathMesh, const char* const i_pathEffect, eae6320::Graphics::Renderable *i_renderable);
		void RemoveRenderable(eae6320::Graphics::Renderable *i_renderable);
		std::vector<Renderable*>* GetOpaqueRenderableList();
		std::vector<Renderable*>* GetTransparentRenderableList();

		//Misc Functions
		void Clear();
		void BeginScene();
		void EndScene();
		void SwapBuffers();
		float getAspectRatio();
	}
}

#endif	// EAE6320_GRAPHICS_H

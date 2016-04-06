#ifndef EAE6320_RENDERABLE_H
#define EAE6320_RENDERABLE_H


// Header Files
//=============
#include "../Graphics/Graphics.h"
#include "../Core/Math/cVector.h"
#include "../Core/Math/cQuaternion.h"

// Interface
//==========

namespace eae6320
{
	namespace Graphics
	{
		class Renderable
		{
		public:
			Renderable() {}
			Renderable(eae6320::Math::cVector i_cVector) :m_offset(i_cVector) {}

			eae6320::Graphics::Mesh m_mesh;
			//eae6320::Graphics::Effect m_effect;
			eae6320::Graphics::Material m_material;
			eae6320::Math::cVector m_offset;
			eae6320::Math::cQuaternion m_orientation;

			//bool Initialize(const char* const i_pathMesh, const char* const i_pathEffect, eae6320::Math::cVector i_offset);
		};
	}
}
#endif
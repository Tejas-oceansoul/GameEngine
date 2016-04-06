#ifndef EAE6320_CAMERA_H
#define EAE6320_CAMERA_H

#include "../Core/Math/cQuaternion.h"
#include "../Core/Math/cVector.h"
#include "../Core/Math/Functions.h"

namespace eae6320
{
	namespace Graphics
	{
		class Camera
		{
		public:
			static Camera& getInstance()
			{
				static Camera instance;		// Guaranteed to be destroyed.
											// Instantiated on first use.
				return instance;
			}
		private:
			Camera() {};					// Constructor? (the {} brackets) are needed here.

		public:
			//Member Variables.
			eae6320::Math::cQuaternion m_orientation;
			eae6320::Math::cVector m_offset = eae6320::Math::cVector(0.0f, 0.0f, 10.0f);
			float FOV = eae6320::Math::ConvertDegreesToRadians(60.0f);

		private:
			// C++ 11
			// =======
			// We can use the better technique of deleting the methods
			// we don't want.
			Camera(Camera const&) = delete;
			void operator=(Camera const&) = delete;

		};
	}
}

#endif
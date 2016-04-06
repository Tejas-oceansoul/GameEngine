#ifndef EAE6320_ENTITY_H
#define EAE6320_ENTITY_H

// Header Files
//=============
#include "../Math/cVector.h"

// Interface
//==========

namespace eae6320
{
	namespace Entities
	{
		class Component;
		class Entity
		{
			Entity()
			{
				numComponents = 0;
			}

			static const unsigned int NUM_MAX_COMPONENTS = 10;
			Component *m_components[NUM_MAX_COMPONENTS];
			unsigned int numComponents;
			
		public:
			eae6320::Math::cVector m_position;
			bool initialize();
			void update();
			bool shutdown();
			void addComponent(Component *i_component);
		};
	}
}
#endif

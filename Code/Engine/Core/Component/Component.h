#ifndef EAE6320_COMPONENT_H
#define EAE6320_COMPONENT_H

// Header Files
//=============


// Interface
//==========
namespace eae6320 {
	namespace Entities
	{
		class Entity;
		class Component
		{
			friend class Entity;
			Entity *m_owner;
		public:
			virtual bool initialize();
			virtual bool shutdown();
			virtual void update();

			Entity* getOwner() const { return m_owner; }
		};
	}
}

#endif


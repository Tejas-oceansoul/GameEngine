// Header Files
//=============

#include "Entity.h"
#include "../Component/Component.h"
#include <cassert>

// Interface
//==========
bool eae6320::Entities::Entity::initialize()
{
	for (unsigned int i = 0; i < numComponents; i++)
		if (!m_components[i]->initialize())
			return false;
	return true;
}

void eae6320::Entities::Entity::update()
{
	for (unsigned int i = 0; i < numComponents; i++)
		m_components[i]->update();
}

bool eae6320::Entities::Entity::shutdown()
{
	return true;
}

void eae6320::Entities::Entity::addComponent(eae6320::Entities::Component *i_component)
{
	assert(numComponents != NUM_MAX_COMPONENTS);
	m_components[numComponents++] = i_component;
	i_component->m_owner = this;
}
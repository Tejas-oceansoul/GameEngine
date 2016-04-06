// Header Files
//=============

#include "cEffectBuilder.h"
#include "../../External/Lua/Includes.h"
#include "../../Engine/Windows/Functions.h"

#include <sstream>
#include <cassert>
#include <stdio.h>
#include <sys/stat.h>
#include <string>

// Helper Functions
//==========
namespace
{
	enum RenderStates : uint8_t
	{
		alpha = 1,
		depthtest = 1 << 1,
		depthwrite = 1 << 2,
		faceculling = 1 << 3,
	};
}

// Build
//------

bool eae6320::cEffectBuilder::Build( const std::vector<std::string>& )
{
	bool wereThereErrors = false;

	//Read the Lua File.
	// Create a new Lua state
	lua_State* luaState = NULL;
	{
		luaState = luaL_newstate();
		if (!luaState)
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "Failed to create a new Lua state";
			eae6320::OutputErrorMessage(errorMessage.str().c_str());
			goto OnExit;
		}
	}

	// Load the asset file into a table at the top of the stack
	{
		const int luaResult = luaL_dofile(luaState, m_path_source);
		if (luaResult == LUA_OK)
		{
			// A well-behaved asset file will only return a single value
			const int returnedValueCount = lua_gettop(luaState);
			if (returnedValueCount == 1)
			{
				// A correct asset file _must_ return a table
				if (!lua_istable(luaState, -1))
				{
					wereThereErrors = true;
					std::stringstream errorMessage;
					errorMessage << "Asset files must return a table (instead of a " <<
						luaL_typename(luaState, -1) << ")\n";
					eae6320::OutputErrorMessage(errorMessage.str().c_str());
					//std::cerr << "Asset files must return a table (instead of a " <<
					//	luaL_typename(luaState, -1) << ")\n";
					// Pop the returned non-table value
					lua_pop(luaState, 1);
					goto OnExit;
				}
			}
			else
			{
				wereThereErrors = true;
				std::stringstream errorMessage;
				errorMessage << "Asset files must return a single table (instead of " <<
					returnedValueCount << " values)\n";
				eae6320::OutputErrorMessage(errorMessage.str().c_str());
				//std::cerr << "Asset files must return a single table (instead of " <<
				//	returnedValueCount << " values)\n";
				// Pop every value that was returned
				lua_pop(luaState, returnedValueCount);
				goto OnExit;
			}
		}
		else
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << lua_tostring(luaState, -1);
			//std::cerr << lua_tostring(luaState, -1);
			eae6320::OutputErrorMessage(errorMessage.str().c_str());
			// Pop the error message
			lua_pop(luaState, 1);
			goto OnExit;
		}
	}

	// If this code is reached the asset file was loaded successfully,
	// and its table is now at index -1
	
	if (!wereThereErrors)
	{
		std::string vertexPath;
		std::string fragmentPath;
		uint8_t renderStates = 0;

		{
			const char* key = "vertex";
			lua_pushstring(luaState, key);
			lua_gettable(luaState, -2);
			vertexPath = lua_tostring(luaState, -1);
			lua_pop(luaState, 1);
		}
		{
			const char* key = "fragment";
			lua_pushstring(luaState, key);
			lua_gettable(luaState, -2);
			fragmentPath = lua_tostring(luaState, -1);
			lua_pop(luaState, 1);
		}
		{
			// Main table is at -2 after the following operations
			// Renderstates table is at -1
			const char* const key = "renderstates";
			lua_pushstring(luaState, key);
			lua_gettable(luaState, -2);
			if (lua_istable(luaState, -1))
			{
				{
					const char* key = "alpha";
					lua_pushstring(luaState, key);
					lua_gettable(luaState, -2);
					int Alpha;
					Alpha = lua_toboolean(luaState, -1);
					if (Alpha)
						renderStates |= alpha;
					lua_pop(luaState, 1);
				}
				{
					const char* key = "depthtest";
					lua_pushstring(luaState, key);
					lua_gettable(luaState, -2);
					int Depthtest;
					Depthtest = lua_toboolean(luaState, -1);
					if (Depthtest)
						renderStates |= depthtest;
					lua_pop(luaState, 1);
				}
				{
					const char* key = "depthwrite";
					lua_pushstring(luaState, key);
					lua_gettable(luaState, -2);
					int Depthwrite;
					Depthwrite = lua_toboolean(luaState, -1);
					if (Depthwrite)
						renderStates |= depthwrite;
					lua_pop(luaState, 1);
				}
				{
					const char* key = "faceculling";
					lua_pushstring(luaState, key);
					lua_gettable(luaState, -2);
					int Faceculling;
					Faceculling = lua_toboolean(luaState, -1);
					if (Faceculling)
						renderStates |= faceculling;
					lua_pop(luaState, 1);
				}
			}
			lua_pop(luaState, 1);
		}
		lua_pop(luaState, 1);

		FILE *o_file;
		errno_t err;

		err = fopen_s(&o_file, m_path_target, "wb");
		if (err != 0)
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "Failed to create Binary Data output file.\n";
			eae6320::OutputErrorMessage(errorMessage.str().c_str());
		}

		fwrite(&renderStates, sizeof(uint8_t), 1, o_file);
		fwrite(vertexPath.c_str(), sizeof(char), (vertexPath.length()+1), o_file);
		fwrite(fragmentPath.c_str(), sizeof(char), (fragmentPath.length()+1), o_file);
		
		err = fclose(o_file);
		if (err != 0)
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "Failed to close Binary Data output file.\n";
			eae6320::OutputErrorMessage(errorMessage.str().c_str());

			goto OnExit;
		}
	}

OnExit:

	if (luaState)
	{
		// If I haven't made any mistakes
		// there shouldn't be anything on the stack
		// regardless of any errors
		assert(lua_gettop(luaState) == 0);

		lua_close(luaState);
		luaState = NULL;
	}

	return !wereThereErrors;
}

// Header Files
//=============

#include "cMaterialBuilder.h"
#include "../../External/Lua/Includes.h"
#include "../../Engine/Windows/Functions.h"

#if defined(EAE6320_PLATFORM_GL)
#include <gl/GL.h>
#include <gl/GLU.h>
#endif

#include <sstream>
#include <cassert>
#include <stdio.h>
#include <sys/stat.h>
#include <string>

// Helper Functions
//==========
namespace
{
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
}

// Build
//------

bool eae6320::cMaterialBuilder::Build( const std::vector<std::string>& )
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
		std::string effectPath;
		std::string textureHandle;
		std::string texturePath;
		uint8_t uniformCount;
		sUniformHelper *uniforms = NULL;
		std::string *uniformNames = NULL;

		//Load the Effect Path
		{
			const char* key = "effect";
			lua_pushstring(luaState, key);
			lua_gettable(luaState, -2);
			effectPath = lua_tostring(luaState, -1);
			lua_pop(luaState, 1);
		}

		//Get the uniform count
		{
			const char* key = "uniforms";
			lua_pushstring(luaState, key);
			lua_gettable(luaState, -2);
			uniformCount = (uint8_t)luaL_len(luaState, -1);
		}

		//Initialieze the arrays for sUniformHelper
		{
			uniforms = new sUniformHelper[uniformCount];
			uniformNames = new std::string[uniformCount];
		}

		//Load the uniform data. The "uniforms" table is still at -1.
		{
			for (int i = 1; i <= uniformCount; ++i)
			{
				lua_pushinteger(luaState, i);
				lua_gettable(luaState, -2);
				//"uniforms" table is at -2 now
				//We have a dictionary with names, values and shaderType at -1 now

				//Getting the uniform handle.
				{
					lua_pushstring(luaState, "handleName");
					lua_gettable(luaState, -2);
					uniformNames[i-1] = lua_tostring(luaState, -1);
					lua_pop(luaState, 1);
				}


				//Getting the shader type
				{
					lua_pushstring(luaState, "shaderType");
					lua_gettable(luaState, -2);
					std::string i_shaderType;
					i_shaderType = lua_tostring(luaState, -1);
					if (i_shaderType == "fragment")
						uniforms[i-1].shaderType = eShaderType::fragment;
					else
						uniforms[i-1].shaderType = eShaderType::vertex;
					lua_pop(luaState, 1);
				}

				//Getting the values and their count for the uniform
				{
					lua_pushstring(luaState, "values");
					lua_gettable(luaState, -2);
					uniforms[i-1].valueCountToSet = (uint8_t)luaL_len(luaState, -1);
					for (int j = 1; j <= uniforms[i-1].valueCountToSet; ++j)
					{
						lua_pushinteger(luaState, j);
						lua_gettable(luaState, -2);
						uniforms[i-1].values[j-1] = static_cast<float>(lua_tonumber(luaState, -1));
						lua_pop(luaState, 1);
					}
					lua_pop(luaState, 1);
				}
				lua_pop(luaState, 1);
			}
			//Popping "uniforms"
			lua_pop(luaState, 1);
		}

		{
			//Getting the textures
			{
				const char* key = "textures";
				lua_pushstring(luaState, key);
				lua_gettable(luaState, -2);
			}

			//The texture table is at -1 now.
			{
				//Getting the texture handle name.
				lua_pushstring(luaState, "handleName");
				lua_gettable(luaState, -2);
				textureHandle = lua_tostring(luaState, -1);
				lua_pop(luaState, 1);

				//Getting the texture path.
				lua_pushstring(luaState, "path");
				lua_gettable(luaState, -2);
				texturePath = lua_tostring(luaState, -1);
				lua_pop(luaState, 1);
			}
			//Popping the "texture" table.
			lua_pop(luaState, 1);
		}

		//Popping the "return" table.
		lua_pop(luaState, 1);

		//Writing the binary file.
		{
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

			fwrite(effectPath.c_str(), sizeof(char), (effectPath.length() + 1), o_file);

			fwrite(textureHandle.c_str(), sizeof(char), (textureHandle.length() + 1), o_file);

			fwrite(texturePath.c_str(), sizeof(char), (texturePath.length() + 1), o_file);

			fwrite(&uniformCount, sizeof(uint8_t), 1, o_file);
			
			fwrite(uniforms, sizeof(sUniformHelper), uniformCount, o_file);

			for (uint8_t i = 0; i < uniformCount; i++)
			{
				fwrite(uniformNames[i].c_str(), sizeof(char), (uniformNames[i].length() + 1), o_file);
			}
			
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

		delete[] uniforms;
		uniforms = NULL;

		delete[] uniformNames;
		uniformNames = NULL;
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

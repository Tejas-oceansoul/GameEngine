// Header Files
//=============

#include "cMeshBuilder.h"
#include "../../Engine/Windows/Functions.h"
#include "../../External/Lua/Includes.h"

#include <sstream>
#include <cassert>
#include <stdio.h>
#include <sys/stat.h>

// Interface
//==========

// Helper Function Declarations
//=============================

namespace
{
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

	bool LoadVertices(lua_State& io_luaState, sVertex*& i_vertexData, uint32_t& o_noOfVertices);
	bool LoadIndices(lua_State& io_luaState, uint32_t*& i_indexData, uint32_t& o_noOfIndices);
}

// Build
//------

bool eae6320::cMeshBuilder::Build( const std::vector<std::string>& )
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

	sVertex *o_vertexData = NULL;
	uint32_t *o_indexData = NULL;

	uint32_t o_noOfIndices, o_noOfVertices;
	if (!LoadVertices(*luaState, o_vertexData, o_noOfVertices))
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to load Indices from Mesh\n";
		eae6320::OutputErrorMessage(errorMessage.str().c_str());

		goto OnExit;
	}

	if (!LoadIndices(*luaState, o_indexData, o_noOfIndices))
	{
		wereThereErrors = true;
		std::stringstream errorMessage;
		errorMessage << "Failed to load Indices from Mesh\n";
		eae6320::OutputErrorMessage(errorMessage.str().c_str());

		goto OnExit;
	}


	lua_pop(luaState, 1);

	if (!wereThereErrors)
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


		fwrite(&o_noOfVertices, sizeof(uint32_t), 1, o_file);
		fwrite(&o_noOfIndices, sizeof(uint32_t), 1, o_file);
		fwrite(o_vertexData, sizeof(sVertex), o_noOfVertices, o_file);
		fwrite(o_indexData, sizeof(uint32_t), o_noOfIndices, o_file);
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

	goto OnExit;
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

	if (!wereThereErrors)
	{
		free(o_vertexData);
		o_vertexData = NULL;
		delete[] o_indexData;
		o_indexData = NULL;
	}

	return !wereThereErrors;
}


namespace {
	bool LoadVertices(lua_State& io_luaState, sVertex*& i_vertexData, uint32_t& o_noOfVertices)
	{
		bool wereThereErrors = false;

		// Right now the asset table is at -1.
		// After the following table operation it will be at -2
		// and the "vertices" table will be at -1:
		const char* const key = "vertices";
		lua_pushstring(&io_luaState, key);
		lua_gettable(&io_luaState, -2);
		if (lua_istable(&io_luaState, -1))
		{
			const int vertexCount = luaL_len(&io_luaState, -1);
			o_noOfVertices = vertexCount;
			i_vertexData = reinterpret_cast<sVertex*>(malloc(sizeof(sVertex) * vertexCount));
			for (int i = 1; i <= vertexCount; ++i)
			{
				lua_pushinteger(&io_luaState, i);
				lua_gettable(&io_luaState, -2);
				//"vertices" table is at -2 now
				//We have a dictionary with position and color at -1 now
				{
					{
						lua_pushstring(&io_luaState, "position");
						lua_gettable(&io_luaState, -2);
						if (lua_istable(&io_luaState, -1))
						{
							//An array of "position" x,y is at -1 now
							{
								lua_pushinteger(&io_luaState, 1);
								lua_gettable(&io_luaState, -2);
								i_vertexData[i - 1].x = static_cast<float>(lua_tonumber(&io_luaState, -1));
								lua_pop(&io_luaState, 1);
							}
							{
								lua_pushinteger(&io_luaState, 2);
								lua_gettable(&io_luaState, -2);
								i_vertexData[i - 1].y = static_cast<float>(lua_tonumber(&io_luaState, -1));
								lua_pop(&io_luaState, 1);
							}
							{
								lua_pushinteger(&io_luaState, 3);
								lua_gettable(&io_luaState, -2);
								i_vertexData[i - 1].z = static_cast<float>(lua_tonumber(&io_luaState, -1));
								lua_pop(&io_luaState, 1);
							}
							lua_pop(&io_luaState, 1);
						}
						else
						{
							wereThereErrors = true;
							std::stringstream errorMessage;
							errorMessage << "There is no position array in your Indices table.";
							eae6320::OutputErrorMessage(errorMessage.str().c_str());

							lua_pop(&io_luaState, 1);
							goto OnExit;
						}

					}

					{
						lua_pushstring(&io_luaState, "uv");
						lua_gettable(&io_luaState, -2);
						if (lua_istable(&io_luaState, -1))
						{
							//An array of "uv" u,v is at -1 now
							{
								lua_pushinteger(&io_luaState, 1);
								lua_gettable(&io_luaState, -2);
								i_vertexData[i - 1].u = static_cast<float>(lua_tonumber(&io_luaState, -1));
								lua_pop(&io_luaState, 1);
							}
							{
								lua_pushinteger(&io_luaState, 2);
								lua_gettable(&io_luaState, -2);
								float v = static_cast<float>(lua_tonumber(&io_luaState, -1));
								i_vertexData[i - 1].v = (1.0f - v);
								lua_pop(&io_luaState, 1);
							}
							lua_pop(&io_luaState, 1);
						}
						else
						{
							wereThereErrors = true;
							std::stringstream errorMessage;
							errorMessage << "There is no position array in your Indices table.";
							eae6320::OutputErrorMessage(errorMessage.str().c_str());

							lua_pop(&io_luaState, 1);
							goto OnExit;
						}

					}

					{
						lua_pushstring(&io_luaState, "color");
						lua_gettable(&io_luaState, -2);
						//An array of "color" r,g,b,a is at -1 now
						if (lua_istable(&io_luaState, -1))
						{
							{
								lua_pushinteger(&io_luaState, 1);
								lua_gettable(&io_luaState, -2);
								uint8_t value = static_cast<uint8_t>((lua_tonumber(&io_luaState, -1)) * 255);
								i_vertexData[i - 1].r = value;
								lua_pop(&io_luaState, 1);
							}
							{
								lua_pushinteger(&io_luaState, 2);
								lua_gettable(&io_luaState, -2);
								uint8_t value = static_cast<uint8_t>((lua_tonumber(&io_luaState, -1)) * 255);
								i_vertexData[i - 1].g = value;
								lua_pop(&io_luaState, 1);
							}
							{
								lua_pushinteger(&io_luaState, 3);
								lua_gettable(&io_luaState, -2);
								uint8_t value = static_cast<uint8_t>((lua_tonumber(&io_luaState, -1)) * 255);
								i_vertexData[i - 1].b = value;
								lua_pop(&io_luaState, 1);
							}
							{
								lua_pushinteger(&io_luaState, 4);
								lua_gettable(&io_luaState, -2);
								uint8_t value = static_cast<uint8_t>((lua_tonumber(&io_luaState, -1)) * 255);
								i_vertexData[i - 1].a = value;
								lua_pop(&io_luaState, 1);
							}
							lua_pop(&io_luaState, 1);
						}
						else
						{
							wereThereErrors = true;
							std::stringstream errorMessage;
							errorMessage << "There is no color array in your Indices table.";
							eae6320::OutputErrorMessage(errorMessage.str().c_str());

							lua_pop(&io_luaState, 1);
							goto OnExit;
						}

					}
				}
				//Popping the dictionary of position and color.
				lua_pop(&io_luaState, 1);
				//"vertices" is back at -1
			}
			goto OnExit;
		}
		else
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "The value at \"" << key << "\" must be a table "
				"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";
			eae6320::OutputErrorMessage(errorMessage.str().c_str());

			goto OnExit;
		}

	OnExit:

		// Pop the vertices table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}

	bool LoadIndices(lua_State& io_luaState, uint32_t*& i_indexData, uint32_t& o_noOfIndices)
	{
		bool wereThereErrors = false;

		// Right now the asset table is at -1.
		// After the following table operation it will be at -2
		// and the "indices" table will be at -1:
		const char* const key = "indices";
		lua_pushstring(&io_luaState, key);
		lua_gettable(&io_luaState, -2);
		if (lua_istable(&io_luaState, -1))
		{
			const int indexCount = luaL_len(&io_luaState, -1);
			o_noOfIndices = indexCount;
			i_indexData = new uint32_t[indexCount];
#if defined(EAE6320_PLATFORM_D3D)
			for (int i = 1; i <= indexCount; ++i)
			{
				lua_pushinteger(&io_luaState, i);
				lua_gettable(&io_luaState, -2);
				if (i % 3 == 1) {
					i_indexData[i - 1] = static_cast<uint32_t>(lua_tonumber(&io_luaState, -1));
				}
				else if (i % 3 == 2) {
					i_indexData[i] = static_cast<uint32_t>(lua_tonumber(&io_luaState, -1));
				}
				else {
					i_indexData[i - 2] = static_cast<uint32_t>(lua_tonumber(&io_luaState, -1));
				}
				lua_pop(&io_luaState, 1);
			}
#elif defined(EAE6320_PLATFORM_GL)
			for (int i = 1; i <= indexCount; ++i)
			{
				lua_pushinteger(&io_luaState, i);
				lua_gettable(&io_luaState, -2);
				i_indexData[i - 1] = static_cast<uint32_t>(lua_tonumber(&io_luaState, -1));
				lua_pop(&io_luaState, 1);
			}
#endif
		}

		else
		{
			wereThereErrors = true;
			std::stringstream errorMessage;
			errorMessage << "The value at \"" << key << "\" must be a table "
				"(instead of a " << luaL_typename(&io_luaState, -1) << ")\n";
			eae6320::OutputErrorMessage(errorMessage.str().c_str());
		}

		// Pop the indices table
		lua_pop(&io_luaState, 1);

		return !wereThereErrors;
	}
}
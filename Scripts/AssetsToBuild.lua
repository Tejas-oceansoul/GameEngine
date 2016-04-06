return
{
	{
		BuildTool = "ShaderBuilder.exe",
		Directory = "Shader",
		Assets = 
		{
			"vertex.shader",
		},
		Dependency =
		{
			"shaders.inc",
		},
		Optional =
		{
			"vertex",
		},
	},

	{
		BuildTool = "ShaderBuilder.exe",
		Directory = "Shader",
		Assets = 
		{
			"fragment.shader",
			"transparencyFragment.shader",
		},
		Dependency =
		{
			"shaders.inc",
		},
		Optional =
		{
			"fragment",
		},
	},

	{
		BuildTool = "MeshBuilder.exe",
		Directory = "Mesh",
		Assets = 
		{
			"Mercury.lmesh",
			"Venus.lmesh",
			"Earth.lmesh",
			"Mars.lmesh",
			"Jupiter.lmesh",
			"Saturn.lmesh",
			"Uranus.lmesh",
			"Neptune.lmesh",
			"Pluto.lmesh",
			"Background.lmesh",
			"Pointer.lmesh",
			"Panel.lmesh"
		},
	},

	{
		BuildTool = "EffectBuilder.exe",
		Directory = "Effect",
		Assets = 
		{
			"Effect.lua",
			"transparencyEffect.lua",
		},

	},

	{
		BuildTool = "MaterialBuilder.exe",
		Directory = "Material",
		Assets = 
		{
			"Earth.mat",
			"Jupiter.mat",
			"Mars.mat",
			"Mercury.mat",
			"Venus.mat",
			"Saturn.mat",
			"Uranus.mat",
			"Neptune.mat",
			"Pluto.mat",
			"Background.mat",
			"Pointer.mat",
			"EarthPanel.mat",
			"JupiterPanel.mat",
			"MarsPanel.mat",
			"MercuryPanel.mat",
			"VenusPanel.mat",
			"SaturnPanel.mat",
			"UranusPanel.mat",
			"NeptunePanel.mat",
			"PlutoPanel.mat",
		},
		
	},

		{
		BuildTool = "TextureBuilder.exe",
		Directory = "Texture",
		Assets = 
		{
			"Jupiter.jpg",
			"Earth.jpg",
			"Mars.jpg",
			"Mercury.jpg",
			"Venus.jpg",
			"Saturn.jpg",
			"Uranus.jpg",
			"Neptune.jpg",
			"Pluto.jpg",
			"Stars.jpg",
			"Pointer.jpg",
			"JupiterPanel.jpg",
			"EarthPanel.jpg",
			"MarsPanel.jpg",
			"MercuryPanel.jpg",
			"VenusPanel.jpg",
			"SaturnPanel.jpg",
			"UranusPanel.jpg",
			"NeptunePanel.jpg",
			"PlutoPanel.jpg",
		},

	},
}
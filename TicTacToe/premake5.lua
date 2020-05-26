require "Libs/Net/NetworkLib/NetworkLib"
require "TicTacToe"

workspace "TicTacToe"
	configurations { "Debug", "Release" }
	architecture "x64"
	cppdialect "C++17"
	location("./tmp/builds/projects/" .. _ACTION)
		
CreateNetworkLib("Libs/Net/NetworkLib/", "./tmp/builds/files/" .. _ACTION .. "/%{cfg.buildcfg}")
CreateTicTacToe("./", "./builds/%{cfg.buildcfg}")
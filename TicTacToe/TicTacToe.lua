function CreateTicTacToe(baseFolder, outputFolder)
	print("TicTacToe : " .. baseFolder)
	project "TicTacToe"
		kind "WindowedApp"
		language "C++"
		cppdialect "c++17"
		-- architecture "x64"
		targetdir(outputFolder)
		filter {}
		targetname "TicTacToe"
		
		files {
			baseFolder .. "src/**"
		}
		includedirs { baseFolder .. "src" }
		
		filter "configurations:Debug"
			defines { "DEBUG" }
			symbols "On"
		
		filter "configurations:Release"
			defines { "NDEBUG" }
			optimize "On"
		
		filter {}
		location("./tmp/builds/projects/" .. _ACTION)
		-- filter "system:windows"
			-- systemversion "10.0.15063.0"
		filter {}
		
		defines {}
		includedirs {
			"Libs/SDL2/include",
			"Libs/Net/NetworkLib/src"
		}
		filter "toolset:msc*"
			libdirs {
				"Libs/SDL2/lib/vs/x64"
			}
			postbuildcommands {
				"robocopy \"$(TargetDir)../../Libs/SDL2/lib/vs/x64\" \"$(TargetDir).\" SDL2.dll"
			}
		filter {}
		filter "system:windows"
			postbuildcommands {
				"cmd /c (robocopy \"$(TargetDir)../../Data\" \"$(TargetDir).\" *.* /xx) ^& IF %ERRORLEVEL% LEQ 1 exit 0 "
			}
		filter {}
		libdirs { "./tmp/builds/files/" .. _ACTION .. "/%{cfg.buildcfg}" }
		links {
			"SDL2",
			"SDL2main",
			"Network"
		}
		debugdir "$(TargetDir)"
end

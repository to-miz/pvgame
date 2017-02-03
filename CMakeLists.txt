cmake_minimum_required( VERSION 3.7 )

project( pvgame LANGUAGES CXX )

function( target_common_settings target )
	target_compile_features( ${target}
		PRIVATE
			cxx_range_for
			cxx_auto_type
			cxx_variadic_templates
			cxx_constexpr
	)

	target_include_directories( ${target}
		PRIVATE
			"${PROJECT_SOURCE_DIR}/src"
			"${PROJECT_SOURCE_DIR}/lib/ssemath"
			"${PROJECT_SOURCE_DIR}/lib/tm"
			"${PROJECT_SOURCE_DIR}/lib/stb"
			"${PROJECT_SOURCE_DIR}/lib/dlmalloc"
	)

	target_compile_definitions( ${target}
		PUBLIC
			# debug
			"$<$<CONFIG:Debug>:GAME_DEBUG>"

			# release
			"$<$<CONFIG:Release>:NDEBUG>"
	)

	# architecture
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		target_compile_definitions( ${target} PUBLIC ARCHITECTURE_X64 )
	else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		target_compile_definitions( ${target} PUBLIC ARCHITECTURE_X86 )
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

	if( WIN32 )
		# architecture
		target_compile_definitions( ${target}
			PUBLIC
				"ARCHITECTURE_LITTLE_ENDIAN"
				"ARCHITECTURE_IEEE_754"
		)
		# game related
		target_compile_definitions( ${target}
			PUBLIC
				"GAME_FLIP_Z"
		)
		# win32 stuff
		target_compile_definitions( ${target}
			PUBLIC
				"NOMINMAX"
				"UNICODE"
		)
	endif( WIN32 )

	if( MSVC )
		# compile
		target_compile_definitions( ${target}
			PRIVATE
				"_CRT_SECURE_NO_WARNINGS"
				"_SCL_SECURE_NO_WARNINGS"
		)
		target_compile_options( ${target}
			PUBLIC
				"/FC" # full paths in errors
				"/EHsc-"
				"/W4"
				"/Oi" # use intrinsics

				# debug
				"$<$<CONFIG:Debug>:/Od>"
				"$<$<CONFIG:Debug>:/Zi>"
				"$<$<CONFIG:Debug>:/MDd>"

				# release
				"$<$<CONFIG:Release>:/MD>"
				"$<$<CONFIG:Release>:/GS->"
				"$<$<CONFIG:Release>:/Gy>"
				"$<$<CONFIG:Release>:/fp:fast>"
				"$<$<CONFIG:Release>:/Ox>"
				"$<$<CONFIG:Release>:/Oy->"
				"$<$<CONFIG:Release>:/GL>"
				"$<$<CONFIG:Release>:/O2>"
		)
		# link
		target_link_libraries( ${target}
			PRIVATE
				"-SUBSYSTEM:WINDOWS"

				# release
				"$<$<CONFIG:Release>:-LTCG>"
		)
	elseif( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
		target_compile_options( ${target} -Wall )
	endif()
endfunction()

add_library( game_dll
	SHARED
		"${PROJECT_SOURCE_DIR}/src/game_main.cpp"
)
add_executable( game
	WIN32
		"${PROJECT_SOURCE_DIR}/src/platform/win32/win32main.cpp"
		"${PROJECT_SOURCE_DIR}/src/platform/win32/win32malloc.cpp"
)

target_compile_definitions( game_dll PUBLIC GAME_DLL )
target_common_settings( game_dll )
target_common_settings( game )

if( WIN32 )
	# game
	target_link_libraries( game
		PRIVATE
			"opengl32.lib"
			"Shlwapi.lib"
	)

	# game dll
	target_link_libraries( game_dll
		PRIVATE
			"-EXPORT:initializeApp"
			"-EXPORT:updateAndRender"
			"-EXPORT:reloadApp"
	)
endif( WIN32 )
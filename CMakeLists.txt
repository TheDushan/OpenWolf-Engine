#
# Stellar Prey CMakeLists.txt
#
# cmake required (http://www.cmake.org/)
#

cmake_minimum_required( VERSION 3.16 )

if( POLICY CMP0017 )
  cmake_policy( SET CMP0072 NEW )
endif()

include( CMakeDependentOption )

set( GAME_APP_NAME "" CACHE            STRING "The Application name")
set( GAME_APP_VERSION "" CACHE         STRING "The Application version")
set( GAME_APP_STAGE "" CACHE           STRING "The Application stage")
set( CODENAME "" CACHE                 STRING "The Application codename")
set( URL_MASTER_SERVER "" CACHE        STRING "The URL for the Master Server")
set( URL_MOTD_SERVER "" CACHE          STRING "The URL for the MOTD Server")
set( URL_AUTHORIZE_SERVER "" CACHE     STRING "The URL for the Authorize Server")
set( URL_AUTOUPDATE_SERVER "" CACHE    STRING "The URL for the Autoupdate Server")
set( URL_COMMUNITY_SERVER "" CACHE     STRING "The URL for the Community Server. Example: xxx.xxx.xxx.xxx:port")

string( REPLACE " " "" GAME_APP_NAME_UPPER ${GAME_APP_NAME} )
string( TOLOWER ${GAME_APP_NAME} GAME_APP_NAME_LOWER )

if("${GAME_APP_NAME}" STREQUAL "")
	message(FATAL_ERROR "Please set GAME_APP_NAME first")
endif()

if("${GAME_APP_VERSION}" STREQUAL "")
	message(FATAL_ERROR "Please set GAME_APP_VERSION first")
endif()

if("${GAME_APP_STAGE}" STREQUAL "")
	message(FATAL_ERROR "Please set GAME_APP_STAGE first")
endif()

if("${CODENAME}" STREQUAL "")
	message(FATAL_ERROR "Please set CODENAME first")
endif()

if("${URL_MASTER_SERVER}" STREQUAL "")
	message(FATAL_ERROR "Please set URL_MASTER_SERVER first")
endif()

if("${URL_MOTD_SERVER}" STREQUAL "")
	message(FATAL_ERROR "Please set URL_MOTD_SERVER first")
endif()

if("${URL_AUTHORIZE_SERVER}" STREQUAL "")
	message(FATAL_ERROR "Please set URL_AUTHORIZE_SERVER first")
endif()

if("${URL_AUTOUPDATE_SERVER}" STREQUAL "")
	message(FATAL_ERROR "Please set URL_AUTOUPDATE_SERVER first")
endif()

if("${URL_COMMUNITY_SERVER}" STREQUAL "")
	message(FATAL_ERROR "Please set URL_COMMUNITY_SERVER first")
endif()

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set( MSVC_INCREMENTAL_DEFAULT ON )

set( CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)" )

project( ${GAME_APP_NAME} C CXX )

if( WIN32 )
  set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /fp:precise" )
  set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE" )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE /SUBSYSTEM:CONSOLE" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
else()
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2 -pipe -Wno-write-strings -Wno-narrowing -fpermissive -Wsizeof-array-argument -I/usr/include/freetype2" )
  set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -mcmodel=large" )
  set( CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -DNDEBUG" )
  set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -ffast-math -fomit-frame-pointer -fno-strict-aliasing" )
  set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -DNDEBUG -ffast-math -fomit-frame-pointer -fno-strict-aliasing -g" )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_C_FLAGS_DEBUG}" )
set( CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${CMAKE_C_FLAGS_MINSIZEREL}" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_C_FLAGS_RELEASE}" )
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CMAKE_C_FLAGS_RELWITHDEBINFO}" )
set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "" ) # Need this to get rid of -rdynamic linker flag set by default by cmake
set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

if( WIN32 )
  SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/windows )
  SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/windows) 
  SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/windows )
  SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/windows )
else()
  SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/unix )
  SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/unix )
  SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/unix )
  SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/unix )
endif()


# Options
option( BUILD_CLIENT                   "Build client"                             ON )
option( BUILD_SERVER                   "Build dedicated server"                   ON )
option( BUILD_AUTOUPDATE_SERVER        "Build AutoUpdate server"                  ON )
option( BUILD_MASTER_SERVER            "Build master server"                      ON )
option( BUILD_AUTH_SERVER              "Build authorization server"               ON )
option( BUILD_COMMUNITY_SERVER         "Build community server"                   ON )

# Package info
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Application client" )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/GPL.txt" )
include( CPack )

set( MOUNT_DIR ${CMAKE_SOURCE_DIR}/src/engine )
set( LIB_DIR ${CMAKE_SOURCE_DIR}/src/libs )
set( TOOLS_DIR ${CMAKE_SOURCE_DIR}/src/tools )

message(STATUS "writing ${MOUNT_DIR}/framework/appConfig.hpp")
CONFIGURE_FILE("cmake/appConfig.hpp.in" "${MOUNT_DIR}/framework/appConfig.hpp" @ONLY)

find_package( PkgConfig )

if( WIN32 )
	add_definitions( -D_CRT_SECURE_NO_WARNINGS )
endif()

set( CONSOLE "tty" )

# Determine platform
if( WIN32 )
	set( PLATFORM "win32" )
	set( CONSOLE "win32" )
elseif( UNIX )
	if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
		add_definitions( -DMACOS_X -DMACOS_X_ACCELERATION_HACK -D USE_FILE32API -DMACOS_X_CURSOR_HACK )
	endif()
	set( PLATFORM "unix" )
endif()

# Determine arch
if( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" )
	set( BUILD_ARCH "AMD64" )
else()
	set( BUILD_ARCH ${CMAKE_SYSTEM_PROCESSOR} )
endif()

######################
# Define Build Files #
######################

set( SHAREDLIST_HEADERS
    ${MOUNT_DIR}/framework/appConfig.hpp
	${MOUNT_DIR}/qcommon/q_platform.hpp
	${MOUNT_DIR}/qcommon/q_shared.hpp
)

set( SHAREDLIST_SOURCES
	${MOUNT_DIR}/qcommon/q_shared.cpp
	${MOUNT_DIR}/qcommon/q_math.cpp
)

set( GPURENDERER_HEADERS 
	${MOUNT_DIR}/GPURenderer/iqm.hpp
	${MOUNT_DIR}/GPURenderer/qgl.hpp
	${MOUNT_DIR}/GPURenderer/r_common.hpp
	${MOUNT_DIR}/GPURenderer/r_dsa.hpp
	${MOUNT_DIR}/GPURenderer/r_extramath.hpp
	${MOUNT_DIR}/GPURenderer/r_extratypes.hpp
	${MOUNT_DIR}/GPURenderer/r_fbo.hpp
	${MOUNT_DIR}/GPURenderer/r_local.hpp
	${MOUNT_DIR}/GPURenderer/r_postprocess.hpp
	${MOUNT_DIR}/GPURenderer/r_splash.hpp
	${MOUNT_DIR}/API/renderer_api.hpp
	${MOUNT_DIR}/GPURenderer/r_types.hpp
	${MOUNT_DIR}/GPURenderer/r_cmdsTemplate.hpp
	${MOUNT_DIR}/GPURenderer/r_ConsoleVars.hpp
)

set( GPURENDERER_SOURCES
	${MOUNT_DIR}/GPURenderer/r_animation.cpp
	${MOUNT_DIR}/GPURenderer/r_backend.cpp
	${MOUNT_DIR}/GPURenderer/r_bsp_tech3.cpp
	${MOUNT_DIR}/GPURenderer/r_cmds.cpp
	${MOUNT_DIR}/GPURenderer/r_curve.cpp
	${MOUNT_DIR}/GPURenderer/r_dsa.cpp
	${MOUNT_DIR}/GPURenderer/r_extensions.cpp
	${MOUNT_DIR}/GPURenderer/r_extramath.cpp
	${MOUNT_DIR}/GPURenderer/r_fbo.cpp
	${MOUNT_DIR}/GPURenderer/r_flares.cpp
	${MOUNT_DIR}/GPURenderer/r_font.cpp
	${MOUNT_DIR}/GPURenderer/r_glsl.cpp
	${MOUNT_DIR}/GPURenderer/r_image.cpp
	${MOUNT_DIR}/GPURenderer/r_image_dds.cpp
	${MOUNT_DIR}/GPURenderer/r_image_jpg.cpp
	${MOUNT_DIR}/GPURenderer/r_image_png.cpp
	${MOUNT_DIR}/GPURenderer/r_image_tga.cpp
	${MOUNT_DIR}/GPURenderer/r_init.cpp
	${MOUNT_DIR}/GPURenderer/r_light.cpp
	${MOUNT_DIR}/GPURenderer/r_main.cpp
	${MOUNT_DIR}/GPURenderer/r_marks.cpp
	${MOUNT_DIR}/GPURenderer/r_mesh.cpp
	${MOUNT_DIR}/GPURenderer/r_model.cpp
	${MOUNT_DIR}/GPURenderer/r_model_iqm.cpp
	${MOUNT_DIR}/GPURenderer/r_model_obj.cpp
	${MOUNT_DIR}/GPURenderer/r_noise.cpp
	${MOUNT_DIR}/GPURenderer/r_postprocess.cpp
	${MOUNT_DIR}/GPURenderer/r_scene.cpp
	${MOUNT_DIR}/GPURenderer/r_shade.cpp
	${MOUNT_DIR}/GPURenderer/r_shade_calc.cpp
	${MOUNT_DIR}/GPURenderer/r_shader.cpp
	${MOUNT_DIR}/GPURenderer/r_shadows.cpp
	${MOUNT_DIR}/GPURenderer/r_sky.cpp
	${MOUNT_DIR}/GPURenderer/r_surface.cpp
	${MOUNT_DIR}/GPURenderer/r_vbo.cpp
	${MOUNT_DIR}/GPURenderer/r_world.cpp
	${MOUNT_DIR}/GPURenderer/r_glimp.cpp
	${MOUNT_DIR}/GPURenderer/r_ConsoleVars.cpp
)

set( SERVERLIST_HEADERS
	${MOUNT_DIR}/server/server.hpp
	${MOUNT_DIR}/server/serverCcmds.hpp
	${MOUNT_DIR}/server/serverClient.hpp
	${MOUNT_DIR}/server/serverCommunity.hpp
	${MOUNT_DIR}/server/serverGame.hpp
	${MOUNT_DIR}/server/serverWorld.hpp
	${MOUNT_DIR}/server/serverSnapshot.hpp
	${MOUNT_DIR}/server/serverNetChan.hpp
	${MOUNT_DIR}/server/serverInit.hpp
	${MOUNT_DIR}/server/serverMain.hpp
	${MOUNT_DIR}/server/serverOACS.hpp
	${MOUNT_DIR}/server/serverWallhack.hpp
	${MOUNT_DIR}/API/sgame_api.hpp
	${MOUNT_DIR}/API/serverClient_api.hpp
	${MOUNT_DIR}/API/serverGame_api.hpp
	${MOUNT_DIR}/API/serverWorld_api.hpp
	${MOUNT_DIR}/API/serverSnapshot_api.hpp
	${MOUNT_DIR}/API/serverNetChan_api.hpp
	${MOUNT_DIR}/API/serverInit_api.hpp
	${MOUNT_DIR}/API/serverMain_api.hpp

)

set( SERVERLIST_SOURCES
	${MOUNT_DIR}/server/serverCcmds.cpp
	${MOUNT_DIR}/server/serverClient.cpp
	${MOUNT_DIR}/server/serverCommunity.cpp
	${MOUNT_DIR}/server/serverGame.cpp
	${MOUNT_DIR}/server/serverWorld.cpp
	${MOUNT_DIR}/server/serverSnapshot.cpp
	${MOUNT_DIR}/server/serverNetChan.cpp
	${MOUNT_DIR}/server/serverInit.cpp
	${MOUNT_DIR}/server/serverMain.cpp
	${MOUNT_DIR}/server/serverOACS.cpp
	${MOUNT_DIR}/server/serverWallhack.cpp
)

set( COLLISIONMODEL_HEADERS
	${MOUNT_DIR}/cm/cm_local.hpp
	${MOUNT_DIR}/cm/cm_patch.hpp
	${MOUNT_DIR}/cm/cm_polylib.hpp
	${MOUNT_DIR}/API/cm_api.hpp
)

set( COLLISIONMODEL_SOURCES
	${MOUNT_DIR}/cm/cm_load_tech3.cpp
	${MOUNT_DIR}/cm/cm_patch.cpp
	${MOUNT_DIR}/cm/cm_polylib.cpp
	${MOUNT_DIR}/cm/cm_test.cpp
	${MOUNT_DIR}/cm/cm_trace.cpp
	${MOUNT_DIR}/cm/cm_trisoup.cpp
)

set( DOWNLOADLIST_HEADERS
	${MOUNT_DIR}/download/downloadLocal.hpp
)

set( DOWNLOADLIST_SOURCES
	${MOUNT_DIR}/download/downloadMain.cpp
) 

set( QCOMMONLIST_HEADERS
	${MOUNT_DIR}/API/download_api.hpp
	${MOUNT_DIR}/qcommon/q_shared.hpp
	${MOUNT_DIR}/qcommon/qcommon.hpp
	${MOUNT_DIR}/qcommon/qfiles.hpp
)

set( QCOMMONLIST_SOURCES
	${MOUNT_DIR}/qcommon/common.cpp
	${MOUNT_DIR}/qcommon/msg.cpp
)

set( FRAMEWORKS_HEADERS
	${MOUNT_DIR}/API/FileSystem_api.hpp
	${MOUNT_DIR}/API/CVarSystem_api.hpp
	${MOUNT_DIR}/API/CmdSystem_api.hpp
	${MOUNT_DIR}/API/CmdBuffer_api.hpp
	${MOUNT_DIR}/API/CmdDelay_api.hpp
	${MOUNT_DIR}/API/MD4_api.hpp
	${MOUNT_DIR}/API/MD5_api.hpp
	${MOUNT_DIR}/API/Network_api.hpp
	${MOUNT_DIR}/API/NetworkChain_api.hpp
	${MOUNT_DIR}/API/Parse_api.hpp
	${MOUNT_DIR}/API/CmdCompletion_api.hpp
	${MOUNT_DIR}/framework/FileSystem.hpp
	${MOUNT_DIR}/framework/CVarSystem.hpp
	${MOUNT_DIR}/framework/CmdSystem.hpp
	${MOUNT_DIR}/framework/CmdBuffer.hpp
	${MOUNT_DIR}/framework/CmdDelay.hpp
	${MOUNT_DIR}/framework/Huffman.hpp
	${MOUNT_DIR}/framework/IOAPI.hpp
	${MOUNT_DIR}/framework/MD4.hpp
	${MOUNT_DIR}/framework/MD5.hpp
	${MOUNT_DIR}/framework/Puff.hpp
	${MOUNT_DIR}/framework/Unzip.hpp
	${MOUNT_DIR}/framework/SurfaceFlags_Tech3.hpp
	${MOUNT_DIR}/framework/Network.hpp
	${MOUNT_DIR}/framework/NetworkChain.hpp
	${MOUNT_DIR}/framework/Parse.hpp
	${MOUNT_DIR}/framework/CmdCompletion.hpp
	${MOUNT_DIR}/framework/CommonConsoleVars.hpp
	${MOUNT_DIR}/API/Memory_api.hpp
	${MOUNT_DIR}/framework/Memory.hpp
)

set( FRAMEWORKS_SOURCES
	${MOUNT_DIR}/framework/FileSystem.cpp
	${MOUNT_DIR}/framework/hash_table.cpp
	${MOUNT_DIR}/framework/CVarSystem.cpp
	${MOUNT_DIR}/framework/CmdSystem.cpp
	${MOUNT_DIR}/framework/CmdBuffer.cpp
	${MOUNT_DIR}/framework/CmdDelay.cpp
	${MOUNT_DIR}/framework/IOAPI.cpp
	${MOUNT_DIR}/framework/Huffman.cpp
	${MOUNT_DIR}/framework/MD4.cpp
	${MOUNT_DIR}/framework/MD5.cpp
	${MOUNT_DIR}/framework/Puff.cpp
	${MOUNT_DIR}/framework/tiger_hash.cpp
	${MOUNT_DIR}/framework/Unzip.cpp
	${MOUNT_DIR}/framework/Network.cpp
	${MOUNT_DIR}/framework/NetworkChain.cpp
	${MOUNT_DIR}/framework/Parse.cpp
	${MOUNT_DIR}/framework/CmdCompletion.cpp
	${MOUNT_DIR}/framework/CommonConsoleVars.cpp
	${MOUNT_DIR}/framework/Memory.cpp
)

set( CLIENTLIST_HEADERS
	${MOUNT_DIR}/client/clientLocal.hpp
	${MOUNT_DIR}/client/clientPublic.hpp
	${MOUNT_DIR}/client/clientAVI.hpp
	${MOUNT_DIR}/client/clientBrowser.hpp
	${MOUNT_DIR}/client/clientGame.hpp
	${MOUNT_DIR}/client/clientGUI.hpp
	${MOUNT_DIR}/client/clientLAN.hpp
	${MOUNT_DIR}/client/clientScreen.hpp
	${MOUNT_DIR}/client/keycodes.hpp
	${MOUNT_DIR}/API/clientScreen_api.hpp
	${MOUNT_DIR}/API/clientAVI_api.hpp
	${MOUNT_DIR}/API/clientGame_api.hpp
	${MOUNT_DIR}/API/clientGUI_api.hpp
	${MOUNT_DIR}/API/clientLAN_api.hpp
	${MOUNT_DIR}/client/clientParse.hpp
	${MOUNT_DIR}/API/clientNetChan_api.hpp
	${MOUNT_DIR}/client/clientNetChan.hpp
	${MOUNT_DIR}/API/clientCinema_api.hpp
	${MOUNT_DIR}/client/clientCinema.hpp
	${MOUNT_DIR}/API/clientLocalization_api.hpp
	${MOUNT_DIR}/client/clientLocalization.hpp
	${MOUNT_DIR}/API/clientConsole_api.hpp
	${MOUNT_DIR}/client/clientConsole.hpp
	${MOUNT_DIR}/API/clientInput_api.hpp
	${MOUNT_DIR}/client/clientInput.hpp
	${MOUNT_DIR}/API/clientKeys_api.hpp
	${MOUNT_DIR}/client/clientKeys.hpp
	${MOUNT_DIR}/API/clientMain_api.hpp
	${MOUNT_DIR}/client/clientMain.hpp
	${MOUNT_DIR}/client/clientAuthorization.hpp
	${MOUNT_DIR}/API/clientAutoUpdate_api.hpp
	${MOUNT_DIR}/client/clientAutoUpdate.hpp
	${MOUNT_DIR}/API/clientConsoleCommands_api.hpp
	${MOUNT_DIR}/client/clientConsoleCommands.hpp
	${MOUNT_DIR}/client/clientDemo.hpp
	${MOUNT_DIR}/API/clientDownload_api.hpp
	${MOUNT_DIR}/client/clientDownload.hpp
	${MOUNT_DIR}/client/clientGUID.hpp
	${MOUNT_DIR}/client/clientInput.hpp
	${MOUNT_DIR}/client/clientKeys.hpp
	${MOUNT_DIR}/client/clientMotd.hpp
	${MOUNT_DIR}/API/clientReliableCommands_api.hpp
	${MOUNT_DIR}/client/clientReliableCommands.hpp
	${MOUNT_DIR}/API/clientRenderer_api.hpp
	${MOUNT_DIR}/client/clientRenderer.hpp
	${MOUNT_DIR}/client/clientStartUpCache.hpp
	${MOUNT_DIR}/client/clientWave.hpp
)

set( CLIENTLIST_SOURCES
	${MOUNT_DIR}/client/clientAVI.cpp
	${MOUNT_DIR}/client/clientGame.cpp
	${MOUNT_DIR}/client/clientCinema.cpp
	${MOUNT_DIR}/client/clientConsole.cpp
	${MOUNT_DIR}/client/clientInput.cpp
	${MOUNT_DIR}/client/clientKeys.cpp
	${MOUNT_DIR}/client/clientMain.cpp
	${MOUNT_DIR}/client/clientNetChan.cpp
	${MOUNT_DIR}/client/clientParse.cpp
	${MOUNT_DIR}/client/clientScreen.cpp
	${MOUNT_DIR}/client/clientBrowser.cpp
	${MOUNT_DIR}/client/clientGUI.cpp
	${MOUNT_DIR}/client/clientLAN.cpp
	${MOUNT_DIR}/client/clientLocalization.cpp
	${MOUNT_DIR}/client/clientAuthorization.cpp
	${MOUNT_DIR}/client/clientAutoUpdate.cpp
	${MOUNT_DIR}/client/clientConsoleCommands.cpp
	${MOUNT_DIR}/client/clientDemo.cpp
	${MOUNT_DIR}/client/clientDownload.cpp
	${MOUNT_DIR}/client/clientGUID.cpp
	${MOUNT_DIR}/client/clientInput.cpp
	${MOUNT_DIR}/client/clientKeys.cpp
	${MOUNT_DIR}/client/clientMotd.cpp
	${MOUNT_DIR}/client/clientReliableCommands.cpp
	${MOUNT_DIR}/client/clientRenderer.cpp
	${MOUNT_DIR}/client/clientStartUpCache.cpp
	${MOUNT_DIR}/client/clientWave.cpp
)

set( AUDIO_HEADERS
	${MOUNT_DIR}/soundSystem/sndSystem_codec.hpp
	${MOUNT_DIR}/soundSystem/sndSystem_local.hpp
	${MOUNT_DIR}/API/soundSystem_api.hpp
)

set( AUDIO_SOURCES
	${MOUNT_DIR}/soundSystem/sndSystem_adpcm.cpp
	${MOUNT_DIR}/soundSystem/sndSystem_codec.cpp
	${MOUNT_DIR}/soundSystem/sndSystem_codec_wav.cpp
	${MOUNT_DIR}/soundSystem/sndSystem_codec_ogg.cpp
	${MOUNT_DIR}/soundSystem/sndSystem_dma.cpp
	${MOUNT_DIR}/soundSystem/sndSystem_load.cpp
	${MOUNT_DIR}/soundSystem/sndSystem_mem.cpp
	${MOUNT_DIR}/soundSystem/sndSystem_mix.cpp
	${MOUNT_DIR}/soundSystem/sndSystem_wavelet.cpp
)

set( AUDIO_OPENAL_HEADERS
	${MOUNT_DIR}/API/soundSystem_api.hpp
	${MOUNT_DIR}/soundSystemAL/qal.hpp
	${MOUNT_DIR}/soundSystemAL/sndSystemAL_local.hpp
)

set( AUDIO_OPENAL_SOURCES
  ${MOUNT_DIR}/soundSystemAL/qal.cpp
  ${MOUNT_DIR}/soundSystemAL/sndSystemAL_buffers.cpp
  ${MOUNT_DIR}/soundSystemAL/sndSystemAL_main.cpp
  ${MOUNT_DIR}/soundSystemAL/sndSystemAL_music.cpp
  ${MOUNT_DIR}/soundSystemAL/sndSystemAL_sources.cpp
  ${MOUNT_DIR}/soundSystemAL/sndSystemAL_stream.cpp
)

set( PLATFORMLIST_SOURCES
  ${MOUNT_DIR}/platform/Windows/systemWindows.cpp
  ${MOUNT_DIR}/platform/Linux/systemUnix.cpp
  ${MOUNT_DIR}/platform/systemMain.cpp
  ${MOUNT_DIR}/console/consoleCurses.cpp
  ${MOUNT_DIR}/console/consoleLogging.cpp
  ${MOUNT_DIR}/console/consoleHistory.cpp
)

SET ( PLATFORM_DEDICATED_SOURCES
    ${MOUNT_DIR}/platform/systemMain.cpp
	${MOUNT_DIR}/platform/Windows/systemWindows.cpp
    ${MOUNT_DIR}/console/consoleLogging.cpp
    ${MOUNT_DIR}/console/consoleCurses.cpp
    ${MOUNT_DIR}/platform/Linux/systemUnix.cpp
    ${MOUNT_DIR}/platform/systemMain.cpp
)

SET ( PLATFORMLIST_HEADERS
    ${MOUNT_DIR}/platform/Windows/resource.hpp
	${MOUNT_DIR}/platform/systemLocal.hpp
	${MOUNT_DIR}/API/system_api.hpp
	${MOUNT_DIR}/API/consoleCurses_api.hpp
    ${MOUNT_DIR}/console/consoleCurses.hpp
	${MOUNT_DIR}/API/consoleLogging_api.hpp
    ${MOUNT_DIR}/console/consoleLogging.hpp
	${MOUNT_DIR}/API/consoleHistory_api.hpp
	${MOUNT_DIR}/console/consoleHistory.hpp
)

set( SDLLIST
	${MOUNT_DIR}/platform/systemInput.cpp
	${MOUNT_DIR}/platform/systemSound.cpp
)

set( SERVER_HEADERS
	${MOUNT_DIR}/server/serverDedPrecompiled.hpp
)

set( SERVER_SOURCES
	${MOUNT_DIR}/server/serverDedPrecompiled.cpp
)

set( AUTOUPDATESERVER_HEADERS
	${MOUNT_DIR}/server/serverAutoPrecompiled.hpp
)

set( AUTOUPDATESERVER_SOURCES
	${MOUNT_DIR}/server/serverAutoPrecompiled.cpp
)

set( OWMASTERSERVERLIST_HEADERS
	${TOOLS_DIR}/master/clients.hpp
	${TOOLS_DIR}/master/games.hpp
	${TOOLS_DIR}/master/common.hpp
	${TOOLS_DIR}/master/messages.hpp
	${TOOLS_DIR}/master/servers.hpp
	${TOOLS_DIR}/master/system.hpp
)

set( OWMASTERSERVERLIST_SOURCES
	${TOOLS_DIR}/master/clients.cpp
	${TOOLS_DIR}/master/games.cpp
	${TOOLS_DIR}/master/common.cpp
	${TOOLS_DIR}/master/messages.cpp
	${TOOLS_DIR}/master/servers.cpp
	${TOOLS_DIR}/master/system.cpp
	${TOOLS_DIR}/master/dpmaster.cpp
)

set( OWAUTHSERVERLIST_HEADERS
	${TOOLS_DIR}/authserver/clients.hpp
	${TOOLS_DIR}/authserver/games.hpp
	${TOOLS_DIR}/authserver/common.hpp
	${TOOLS_DIR}/authserver/messages.hpp
	${TOOLS_DIR}/authserver/servers.hpp
	${TOOLS_DIR}/authserver/system.hpp
)

set( OWAUTHSERVERLIST_SOURCES
	${TOOLS_DIR}/authserver/clients.cpp
	${TOOLS_DIR}/authserver/games.cpp
	${TOOLS_DIR}/authserver/common.cpp
	${TOOLS_DIR}/authserver/messages.cpp
	${TOOLS_DIR}/authserver/servers.cpp
	${TOOLS_DIR}/authserver/system.cpp
	${TOOLS_DIR}/authserver/dpmaster.cpp
)

set( OWCOMMUNITYSERVERLIST_HEADERS
	${TOOLS_DIR}/communityserver/communityserver.hpp
	${TOOLS_DIR}/communityserver/json.hpp
)

set( OWCOMMUNITYSERVERLIST_SOURCES
	${TOOLS_DIR}/communityserver/communityserver.cpp
	${TOOLS_DIR}/communityserver/mySQLDatabase.cpp
	${TOOLS_DIR}/communityserver/net.cpp
	${TOOLS_DIR}/communityserver/parser.cpp
	${TOOLS_DIR}/communityserver/json.cpp
)

set( CLIENT_SOURCES
	${MOUNT_DIR}/platform/systemLauncher.cpp
)

####################
# End of file list #
####################

if( WIN32 )
	add_definitions( -DWINVER=0x501 )
	#set( OS_LIBRARIES m winmm wsock32 gdi32 ole32 ws2_32 psapi Iphlpapi advapi32 shell32 user32 )
	set( OS_LIBRARIES winmm ws2_32 psapi )
	set( EXTRALIBS dinput dsound )
elseif( UNIX )
	if( CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" )
		set( OS_LIBRARIES m z )
	elseif( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
		set( OS_LIBRARIES dl m z )
		# FIXME This looks wrong.
		set( OS_LIBRARIES ${OS_LIBRARIES} "-framework AGL -framework OpenGL -framework Carbon -framework IOKit" )
	else()
		set( OS_LIBRARIES ${CMAKE_DL_LIBS} m z rt )
	endif()
endif()


################
# Build OpenAL #
################

	find_package( SDL REQUIRED )
	find_package( OpenAL REQUIRED )

	# Dushan - This is how visual studio directories (or filters) are generated
	if( WIN32 )
		SOURCE_GROUP( Header\ Files\\SharedList                               FILES ${SHAREDLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\AudioOpenAL                              FILES ${AUDIO_OPENAL_HEADERS} )
		SOURCE_GROUP( Source\ Files\\SharedList                               FILES ${SHAREDLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\AudioOpenAL                              FILES ${AUDIO_OPENAL_SOURCES} )
	endif( WIN32 )

	add_library( soundSystemAL SHARED ${SHAREDLIST_HEADERS} ${AUDIO_OPENAL_HEADERS} ${SHAREDLIST_SOURCES} ${AUDIO_OPENAL_SOURCES} ${MOUNT_DIR}/soundSystemAL/sndSystemAL_precompiled.hpp ${MOUNT_DIR}/soundSystemAL/sndSystemAL_precompiled.cpp )

	TARGET_INCLUDE_DIRECTORIES( soundSystemAL PRIVATE ${OPENAL_INCLUDE_DIR} ${MOUNT_DIR} ${SDL2_INCLUDE_DIR} )
	target_link_libraries( soundSystemAL ${OPENAL_LIBRARY} ${SDL2_LIBRARY} ${CMAKE_DL_LIBS} )

	if( MSVC )
		SET_TARGET_PROPERTIES( soundSystemAL PROPERTIES LINK_FLAGS "/DEF:\"${MOUNT_DIR}/soundSystemAL/sndSystemAL.def" )
	endif(MSVC)
			
	target_precompile_headers( soundSystemAL PRIVATE "${MOUNT_DIR}/soundSystemAL/sndSystemAL_precompiled.hpp" )

	set_property( TARGET soundSystemAL PROPERTY COMPILE_DEFINITIONS )
	if(WIN32)
		set_property( TARGET soundSystemAL APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ WIN32 _AMD64 _WIN64 __WIN64__ NDEBUG _WINDOWS HAVE_CONFIG_H __STDC_CONSTANT_MACROS AL_NO_PROTOTYPES OALAUDIO )
	elseif( UNIX )
		set_property( TARGET soundSystemAL APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG __LINUX__ __linux__ __amd64__ __x86_64__ LINUX HAVE_CONFIG_H  __STDC_CONSTANT_MACROS AL_NO_PROTOTYPES OALAUDIO )
	endif()
	
	set_target_properties( soundSystemAL PROPERTIES OUTPUT_NAME "soundSystemAL.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )

################
# Build Client #
################

if( BUILD_CLIENT )
	# Setup target client

	# Dushan - This is how visual studio directories (or filters) are generated
	if( WIN32 )
		SOURCE_GROUP( Header\ Files\\SharedList                               FILES ${SHAREDLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\GPURenderer                              FILES ${GPURENDERER_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Server                                   FILES ${SERVERLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\CM                                       FILES ${COLLISIONMODEL_HEADERS} )
		SOURCE_GROUP( Header\ Files\\QCommon                                  FILES ${QCOMMONLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Download                                 FILES ${DOWNLOADLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Framework                                FILES ${FRAMEWORKS_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Client                                   FILES ${CLIENTLIST_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Audio                                    FILES ${AUDIO_HEADERS} )
		SOURCE_GROUP( Header\ Files\\Platform                                 FILES ${PLATFORMLIST_HEADERS} )
		SOURCE_GROUP( Source\ Files\\SharedList                               FILES ${SHAREDLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\GPURenderer                              FILES ${GPURENDERER_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Server                                   FILES ${SERVERLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\CM                                       FILES ${COLLISIONMODEL_SOURCES} )
		SOURCE_GROUP( Source\ Files\\QCommon                                  FILES ${QCOMMONLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Download                                 FILES ${DOWNLOADLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Framework                                FILES ${FRAMEWORKS_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Client                                   FILES ${CLIENTLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Audio                                    FILES ${AUDIO_SOURCES} )
		SOURCE_GROUP( Source\ Files\\Platform                                 FILES ${PLATFORMLIST_SOURCES} )
		SOURCE_GROUP( Source\ Files\\SDL                                      FILES ${SDLLIST} )
	endif( WIN32 )

	add_library( engine SHARED ${SHAREDLIST_HEADERS} ${GPURENDERER_HEADERS} ${FRAMEWORKS_HEADERS} ${QCOMMONLIST_HEADERS} ${SERVERLIST_HEADERS} ${COLLISIONMODEL_HEADERS} ${CLIENTLIST_HEADERS} ${AUDIO_HEADERS} ${SHAREDLIST_SOURCES} ${GPURENDERER_SOURCES} ${SERVERLIST_SOURCES} ${COLLISIONMODEL_SOURCES} ${QCOMMONLIST_SOURCES} ${CLIENTLIST_SOURCES} ${AUDIO_SOURCES} ${FRAMEWORKS_SOURCES} ${PLATFORMLIST} ${SDLLIST} ${PLATFORMLIST_SOURCES} ${PLATFORMLIST_HEADERS} ${MOUNT_DIR}/framework/precompiled.hpp ${MOUNT_DIR}/framework/precompiled.cpp ${DOWNLOADLIST_HEADERS} ${DOWNLOADLIST_SOURCES} )
	
if( MSVC )
	SET_TARGET_PROPERTIES( engine PROPERTIES LINK_FLAGS "/DEF:\"${MOUNT_DIR}/platform/engineMain.def" )
endif(MSVC)
	    
if(WIN32)
	set_property( TARGET engine APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ WIN32 _AMD64 _WIN64 __WIN64__ NDEBUG _WINDOWS HAVE_CONFIG_H USE_INCREASED_ENTITIES  __STDC_CONSTANT_MACROS  BUILD_FREETYPE HAVE_BOOLEAN HAVE_BZIP2 )
elseif( UNIX )
	set_property( TARGET engine APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG __LINUX__ __linux__ __amd64__ __x86_64__ LINUX HAVE_CONFIG_H USE_INCREASED_ENTITIES  __STDC_CONSTANT_MACROS  BUILD_FREETYPE HAVE_BZIP2 )
endif()

	target_precompile_headers( engine PRIVATE "${MOUNT_DIR}/framework/precompiled.hpp" )
  
  	set_target_properties( engine PROPERTIES OUTPUT_NAME "engine.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )

	# This section finds and provides REQUIRED dependencies to target client
	find_package( OpenGL REQUIRED )
	find_package( JPEG REQUIRED )
	find_package( SDL REQUIRED )
	find_package( Freetype REQUIRED )
	find_package( CURL REQUIRED )
	find_package( Zlib REQUIRED )
	find_package( PNG REQUIRED )
	find_package( BZ2 REQUIRED )
	find_package( OpenAL REQUIRED )
	find_package( OggVorbis REQUIRED )
	find_package( OpenSSL REQUIRED )

	set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
	# Determine platform
	if( WIN32 )
		find_package( PDCurses REQUIRED )
	elseif( UNIX )
		find_package( Curses REQUIRED )
	endif()

	TARGET_INCLUDE_DIRECTORIES( engine PRIVATE ${OPENSSL_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${PNG_INCLUDE_DIR} ${PDCURSES_INCLUDE_DIR} ${MOUNT_DIR} ${JPEG_INCLUDE_DIR} ${BZ2_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR} ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR} )

	TARGET_LINK_LIBRARIES( engine ${OPENSSL_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL2_LIBRARY} ${FREETYPE_LIBRARIES} ${ZLIB_LIBRARIES} ${CURL_LIBRARIES} ${PNG_LIBRARIES} ${PDCURSES_LIBRARY} ${JPEG_LIBRARIES} ${LINK_LIBRARY} ${BZ2_LIBRARY} ${OPENAL_LIBRARY} ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY} )

	if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
		link_directories( /opt/local/lib )
	endif()
 
	set_property( TARGET engine APPEND PROPERTY COMPILE_DEFINITIONS USE_CURSES )
	TARGET_INCLUDE_DIRECTORIES( engine PRIVATE ${CURSES_INCLUDE_DIR} )
	target_link_libraries( engine ${CURSES_LIBRARIES} )

	if ( WIN32)
		target_compile_options(engine PRIVATE "/MP")
		target_link_libraries( engine ${OS_LIBRARIES} )
	else ( UNIX )
		target_link_libraries( engine ${OS_LIBRARIES} -lfreetype )
	endif() 
	
	if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
		link_directories( /opt/local/lib )
	endif()
 
	set_property( TARGET engine APPEND PROPERTY COMPILE_DEFINITIONS USE_CURSES )
	TARGET_INCLUDE_DIRECTORIES( engine PRIVATE ${CURSES_INCLUDE_DIR} )
	target_link_libraries( engine ${CURSES_LIBRARIES} )

	if ( WIN32)
		target_link_libraries( engine ${OS_LIBRARIES} )
	else ( UNIX )
		target_link_libraries( engine ${OS_LIBRARIES} )
	endif() 
	
	# Dushan - This is how visual studio directories (or filters) are generated
	if( MSVC )
		SOURCE_GROUP( Source\ Files\\Client                               FILES ${CLIENT_SOURCES} )
	endif( MSVC )
  
	add_executable( client ${CLIENT_SOURCES} )

	TARGET_INCLUDE_DIRECTORIES( client PRIVATE ${SDL2_INCLUDE_DIR} )

	TARGET_LINK_LIBRARIES( client ${SDL2_LIBRARY}  )

if(MSVC)
	target_compile_options(client PRIVATE "/MP")
	set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ WIN32 _AMD64 _WIN64 __WIN64__ NDEBUG _WINDOWS HAVE_CONFIG_H  __STDC_CONSTANT_MACROS )
elseif( UNIX )
	set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG __LINUX__ __linux__ __amd64__ __x86_64__ LINUX __STDC_CONSTANT_MACROS )
endif()
  
	set_target_properties( client PROPERTIES OUTPUT_NAME "${GAME_APP_NAME_UPPER}.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )

if( MSVC )
	set_target_properties( client PROPERTIES LINK_FLAGS  /SUBSYSTEM:CONSOLE )
endif( MSVC )
	
endif()

################
# Build Server #
################

if( BUILD_SERVER )
	# Setup the target Server
	add_executable( server ${SHAREDLIST_HEADERS} ${QCOMMONLIST_HEADERS} ${FRAMEWORKS_HEADERS} ${DOWNLOADLIST_HEADERS} ${SERVERLIST_HEADERS} ${COLLISIONMODEL_HEADERS} ${CLIENTLIST_HEADERS}  ${SHAREDLIST_SOURCES} ${SERVERLIST_SOURCES} ${DOWNLOADLIST_SOURCES} ${COLLISIONMODEL_SOURCES} ${QCOMMONLIST_SOURCES} ${FRAMEWORKS_SOURCES} ${PLATFORMLIST_HEADERS} ${PLATFORMLIST_SOURCES} ${PLATFORM_DEDICATED_SOURCES} ${SERVER_SOURCES} ${SERVER_HEADERS} )

	if(WIN32)
		target_compile_options(server PRIVATE "/MP")
		set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ WIN32 _AMD64 _WIN64 __WIN64__ NDEBUG DEDICATED _WINDOWS HAVE_CONFIG_H USE_INCREASED_ENTITIES  __STDC_CONSTANT_MACROS  HAVE_BOOLEAN HAVE_BZIP2 )
	elseif( UNIX )
		set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG __LINUX__ __linux__ __amd64__ __x86_64__ LINUX DEDICATED HAVE_CONFIG_H USE_INCREASED_ENTITIES __STDC_CONSTANT_MACROS BUILD_FREETYPE HAVE_BZIP2 )
	endif()

	target_precompile_headers( server PRIVATE "${MOUNT_DIR}/server/serverDedPrecompiled.hpp" )
	
	set_target_properties( server PROPERTIES OUTPUT_NAME "${GAME_APP_NAME_UPPER}ded.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )

	# The following section finds the REQUIRED dependencies and makes them available to the target
	find_package( SDL REQUIRED )
	find_package( CURL REQUIRED )
	find_package( Zlib REQUIRED )
	find_package( BZ2 REQUIRED )
	find_package( OpenSSL REQUIRED )
	
	TARGET_INCLUDE_DIRECTORIES( server PRIVATE ${OPENSSL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${PDCURSES_INCLUDE_DIR} ${MOUNT_DIR} ${BZ2_INCLUDE_DIR} )
	target_link_libraries( server ${OS_LIBRARIES} ${OPENSSL_LIBRARIES} ${SDL2_LIBRARY}  ${ZLIB_LIBRARIES} ${CURL_LIBRARIES} ${PDCURSES_LIBRARY} ${PDCURSES_LIBRARY} ${BZ2_LIBRARY} )
	# The following section finds the OPTIONAL dependencies and makes them available to the target

	set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
	# Determine platform
	if( WIN32 )
		find_package( PDCurses REQUIRED )
	elseif( UNIX )
		find_package( Curses REQUIRED )
	endif()
	set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS USE_CURSES )
	TARGET_INCLUDE_DIRECTORIES( server PRIVATE ${CURSES_INCLUDE_DIR} )
	target_link_libraries( server ${CURSES_LIBRARIES} )
endif()

###########################
# Build Autoupdate Server #
###########################

if( BUILD_AUTOUPDATE_SERVER )
	# Setup the target Server
	add_executable( autoupdateserver ${SHAREDLIST_HEADERS} ${FRAMEWORKS_HEADERS} ${DOWNLOADLIST_HEADERS}
	${SERVERLIST_HEADERS} ${COLLISIONMODEL_HEADERS} ${CLIENTLIST_HEADERS} ${DOWNLOADLIST_SOURCES}
	${QCOMMONLIST_HEADERS} ${SHAREDLIST_SOURCES} ${SERVERLIST_SOURCES} ${COLLISIONMODEL_SOURCES} 
	${QCOMMONLIST_SOURCES} ${PLATFORMLIST_HEADERS} ${PLATFORMLIST_SOURCES} ${FRAMEWORKS_SOURCES}
	${PLATFORM_DEDICATED_SOURCES} ${AUTOUPDATESERVER_SOURCES} ${AUTOUPDATESERVER_HEADERS} )

	target_link_libraries( autoupdateserver ${OS_LIBRARIES} )
	if(WIN32)
		target_compile_options(autoupdateserver PRIVATE "/MP")
        set_property( TARGET autoupdateserver APPEND PROPERTY COMPILE_DEFINITIONS DEDICATED _AMD64_ WIN32 _AMD64 _WIN64 __WIN64__ UPDATE_SERVER )
	elseif( UNIX )
		set_property( TARGET autoupdateserver APPEND PROPERTY COMPILE_DEFINITIONS NDEBUG __LINUX__ __linux__ __amd64__ __x86_64__ LINUX DEDICATED UPDATE_SERVER )
	endif()

	target_precompile_headers( autoupdateserver PRIVATE "${MOUNT_DIR}/server/serverAutoPrecompiled.hpp" )

	# The following section finds the REQUIRED dependencies and makes them available to the target
	find_package( SDL REQUIRED )
	find_package( CURL REQUIRED )
	find_package( Zlib REQUIRED )
	find_package( OpenSSL REQUIRED )
	
	TARGET_INCLUDE_DIRECTORIES( autoupdateserver PRIVATE ${OPENSSL_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${PDCURSES_INCLUDE_DIR} ${MOUNT_DIR} )
	target_link_libraries( autoupdateserver ${SDL2_LIBRARY} ${OPENSSL_LIBRARIES}  ${ZLIB_LIBRARIES} ${CURL_LIBRARIES} ${PDCURSES_LIBRARY} ${PDCURSES_LIBRARY} )
	# The following section finds the OPTIONAL dependencies and makes them available to the target

	set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
	# Determine platform
	if( WIN32 )
		find_package( PDCurses REQUIRED )
	elseif( UNIX )
		find_package( Curses REQUIRED )
	endif()
	set_property( TARGET autoupdateserver APPEND PROPERTY COMPILE_DEFINITIONS USE_CURSES )
	TARGET_INCLUDE_DIRECTORIES( autoupdateserver PRIVATE ${CURSES_INCLUDE_DIR} )
	target_link_libraries( autoupdateserver ${CURSES_LIBRARIES} )
endif()

#########################
# Build OWMasterServer  #
#########################

if( BUILD_MASTER_SERVER )
	# Dushan - This is how visual studio directories (or filters) are generated
	if( MSVC )
		SOURCE_GROUP( Header\ Files\\OWMaster                                 FILES ${OWMASTERSERVERLIST_HEADERS} )
		SOURCE_GROUP( Source\ Files\\OWMaster                                 FILES ${OWMASTERSERVERLIST_SOURCES} )
	endif( MSVC )

	# Setup OWMaster target
	add_executable( OWMaster ${OWMASTERSERVERLIST_SOURCES} ${OWMASTERSERVERLIST_HEADERS} )
	if(WIN32)
		target_compile_options(OWMaster PRIVATE "/MP")
		set_property( TARGET OWMaster APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ NDEBUG _WIN32 WIN32 _AMD64 _WIN64 __WIN64__  )
	elseif( UNIX )
		set_property( TARGET OWMaster APPEND PROPERTY COMPILE_DEFINITIONS __LINUX__ _LINUX_ LINUX )
	endif() 

	find_package( GEOIP REQUIRED )
	
	TARGET_INCLUDE_DIRECTORIES( OWMaster PRIVATE ${TOOLS_DIR}/master ${GEOIP_INCLUDE_DIR})

	if(WIN32)
		target_link_libraries( OWMaster wsock32 ws2_32 ${GEOIP_LIBRARIES} )
	elseif( UNIX )
		target_link_libraries( OWMaster -lm ${GEOIP_LIBRARIES} )
	endif() 

	set_target_properties( OWMaster PROPERTIES OUTPUT_NAME "OWMaster.${BUILD_ARCH}" PREFIX "" )

endif()

#########################
# Build OWAuthServer  #
#########################

if( BUILD_AUTH_SERVER )
	# Dushan - This is how visual studio directories (or filters) are generated
	if( MSVC )
		SOURCE_GROUP( Header\ Files\\OWAuthServer                             FILES ${OWAUTHSERVERLIST_HEADERS} )
		SOURCE_GROUP( Source\ Files\\OWAuthServer                             FILES ${OWAUTHSERVERLIST_SOURCES} )
	endif( MSVC )

	# Setup OWAuthServer target
	add_executable( OWAuthServer ${OWAUTHSERVERLIST_SOURCES} ${OWAUTHSERVERLIST_HEADERS} )
	if(WIN32)
		target_compile_options(OWAuthServer PRIVATE "/MP")
		set_property( TARGET OWAuthServer APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ NDEBUG _WIN32 WIN32 _AMD64 _WIN64 __WIN64__  )
	elseif( UNIX )
		set_property( TARGET OWAuthServer APPEND PROPERTY COMPILE_DEFINITIONS __LINUX__ _LINUX_ LINUX )
	endif() 

	TARGET_INCLUDE_DIRECTORIES( OWAuthServer PRIVATE ${TOOLS_DIR}/authserver )

	if(WIN32)
		target_link_libraries( OWAuthServer wsock32 ws2_32 )
	elseif( UNIX )
		target_link_libraries( OWAuthServer -lm )
	endif() 

	set_target_properties( OWAuthServer PROPERTIES OUTPUT_NAME "OWAuthServer.${BUILD_ARCH}" PREFIX "" )

endif()

###########################
# Build OWCommunityServer #
###########################

if( BUILD_COMMUNITY_SERVER )
	# Dushan - This is how visual studio directories (or filters) are generated
	if( MSVC )
		SOURCE_GROUP( Header\ Files\\OWCommunityServer                        FILES ${OWCOMMUNITYSERVERLIST_HEADERS} )
		SOURCE_GROUP( Source\ Files\\OWCommunityServer                        FILES ${OWCOMMUNITYSERVERLIST_SOURCES} )
	endif( MSVC )

	# Setup OWCommunityServer target
	add_executable( OWCommunityServer ${OWCOMMUNITYSERVERLIST_SOURCES} ${OWCOMMUNITYSERVERLIST_HEADERS} )
	if(WIN32)
		target_compile_options( OWCommunityServer PRIVATE "/MP" )
		set_property( TARGET OWCommunityServer APPEND PROPERTY COMPILE_DEFINITIONS _AMD64_ NDEBUG _WIN32 WIN32 _AMD64 _WIN64 __WIN64__  )
	elseif( UNIX )
		set_property( TARGET OWCommunityServer APPEND PROPERTY COMPILE_DEFINITIONS __LINUX__ _LINUX_ LINUX )
	endif() 

    find_package( MySQL REQUIRED )

	TARGET_INCLUDE_DIRECTORIES( OWCommunityServer PRIVATE ${TOOLS_DIR}/communityserver ${MYSQL_INCLUDE_DIR} )
    
	target_link_libraries( OWCommunityServer ${MYSQL_LIBRARIES} )
	
	if(WIN32)
		target_link_libraries( OWCommunityServer wsock32 ws2_32 )
	elseif( UNIX )
		target_link_libraries( OWCommunityServer -lm )
	endif() 

	set_target_properties( OWCommunityServer PROPERTIES OUTPUT_NAME "OWCommunityServer.${BUILD_ARCH}" PREFIX "" )
endif()

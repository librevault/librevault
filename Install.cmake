if(WIN32)
	set(LV_PACKAGING_PATH NO CACHE PATH "Base path for packaging purposes")
	if(NOT LV_PACKAGING_PATH)
		message(WARNING "Packaging directory is not set")
	endif()

	set(LV_INTALLER_SOURCE "${LV_PACKAGING_PATH}/release")

	# Daemon
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(LV_DAEMON_PATH "${LV_INTALLER_SOURCE}/x64")
	else()
		set(LV_DAEMON_PATH "${LV_INTALLER_SOURCE}/x32")
	endif()

	install(PROGRAMS $<TARGET_FILE:librevault-daemon> DESTINATION ${LV_DAEMON_PATH} COMPONENT librevault-package)

	# GUI
	option(BUILD_64BIT_GUI "Build only 64-bit GUI instead of only 32-bit" OFF)
	if((CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT BUILD_64BIT_GUI) OR (CMAKE_SIZEOF_VOID_P EQUAL 8 AND BUILD_64BIT_GUI))
		install(PROGRAMS $<TARGET_FILE:librevault-gui> DESTINATION ${LV_INTALLER_SOURCE} COMPONENT librevault-package)
	endif()

	# Install script
	file(GLOB INSTALLSCRIPTS *)
	install(FILES ${INSTALLSCRIPTS} DESTINATION ${LV_PACKAGING_PATH} COMPONENT librevault-installscripts)

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	install(PROGRAMS $<TARGET_FILE:librevault-daemon> DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT librevault-package)
	install(PROGRAMS $<TARGET_FILE:librevault-gui> DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT librevault-package)

	install(FILES "gui/resources/Librevault.desktop" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
	install(FILES "gui/resources/librevault_icon.svg" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps RENAME "librevault.svg")
endif()

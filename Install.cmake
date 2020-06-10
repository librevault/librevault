install(CODE "set(CMAKE_INSTALL_LOCAL_ONLY ON)")

include(InstallQt5Plugin)

set(LIBRARY_SEARCH_PATHS "${CMAKE_LIBRARY_PATH};${CONAN_BIN_DIRS};$<$<PLATFORM_ID:Darwin>:${sparkle_SOURCE_DIR}>")

macro(lv_collect_libs INDEPENDENT_BINARIES INSTALL_LOCATION)
	install(CODE "
		include(GetPrerequisites)
		foreach(INSTALLED_BINARY ${INDEPENDENT_BINARIES})
			get_prerequisites(\"\${INSTALLED_BINARY}\" dependencies 1 1 \"\" \"${LIBRARY_SEARCH_PATHS}\")
			foreach(dependency \${dependencies})
				gp_resolve_item(\"\${INSTALLED_BINARY}\" \"\${dependency}\" \"\" \"${LIBRARY_SEARCH_PATHS}\" resolved_file)
				get_filename_component(resolved_file \${resolved_file} ABSOLUTE)
				gp_append_unique(PREREQUISITE_LIBS \${resolved_file})
				get_filename_component(file_canonical \${resolved_file} REALPATH)
				gp_append_unique(PREREQUISITE_LIBS \${file_canonical})
			endforeach()
		endforeach()

		list(SORT PREREQUISITE_LIBS)
		foreach(PREREQUISITE_LIB \${PREREQUISITE_LIBS})
			file(INSTALL \${PREREQUISITE_LIB} DESTINATION ${INSTALL_LOCATION})
		endforeach()
	")
endmacro()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if(BUILD_DAEMON)
		install(PROGRAMS $<TARGET_FILE:librevault-daemon> DESTINATION ${CMAKE_INSTALL_PREFIX})
		list(APPEND INSTALLED_BINARIES ${CMAKE_INSTALL_PREFIX}/librevault-daemon.exe)
	endif()
	if(BUILD_GUI)
		install(PROGRAMS $<TARGET_FILE:librevault-gui> DESTINATION ${CMAKE_INSTALL_PREFIX})
		list(APPEND INSTALLED_BINARIES ${CMAKE_INSTALL_PREFIX}/librevault-gui.exe)
	endif()

	# Prerequisites

	# Install Qt5 plugins
	set(BUNDLE_PLUGINS_PATH ${CMAKE_INSTALL_PREFIX}/plugins)
	install_qt5_plugin("Qt5::QMinimalIntegrationPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
	install_qt5_plugin("Qt5::QWindowsIntegrationPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
	install_qt5_plugin("Qt5::QSvgPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
	install_qt5_plugin("Qt5::QSvgIconPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
	install_qt5_plugin("Qt5::QSQLiteDriverPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
	install_qt5_plugin("Qt5::QWindowsVistaStylePlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")

	list(APPEND INSTALLED_BINARIES ${QT_PLUGIN})

	lv_collect_libs("${INSTALLED_BINARIES}" "${CMAKE_INSTALL_PREFIX}")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#	if(INSTALL_BUNDLE)
#		set(CMAKE_INSTALL_BINDIR opt/librevault/bin/elf)
#		set(CMAKE_INSTALL_SHIMDIR opt/librevault/bin)
#		set(CMAKE_INSTALL_LIBDIR opt/librevault/lib)
#		set(CMAKE_INSTALL_DATAROOTDIR usr/share)
#	endif()
	if(BUILD_DAEMON)
		install(PROGRAMS $<TARGET_FILE:librevault-daemon> DESTINATION ${CMAKE_INSTALL_BINDIR})
		list(APPEND INSTALLED_BINARIES ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/librevault-daemon)
	endif()
	if(BUILD_GUI)
		install(PROGRAMS $<TARGET_FILE:librevault-gui> DESTINATION ${CMAKE_INSTALL_BINDIR})
		install(FILES "src/gui/resources/Librevault.desktop" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
		install(FILES "src/gui/resources/librevault_icon.svg" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps RENAME "librevault.svg")
		list(APPEND INSTALLED_BINARIES ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/librevault-gui)
	endif()
	if(BUILD_CLI)
		install(PROGRAMS $<TARGET_FILE:librevault-cli> DESTINATION ${CMAKE_INSTALL_BINDIR})
		list(APPEND INSTALLED_BINARIES ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/librevault-cli)
	endif()

	if(INSTALL_BUNDLE)
		# Install Qt5 plugins
		set(BUNDLE_PLUGINS_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/qt5/plugins)
		install_qt5_plugin("Qt5::QMinimalIntegrationPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
		install_qt5_plugin("Qt5::QXcbIntegrationPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
		install_qt5_plugin("Qt5::QSvgPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
		install_qt5_plugin("Qt5::QSvgIconPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
		install_qt5_plugin("Qt5::QSQLiteDriverPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")

		list(APPEND INSTALLED_BINARIES ${QT_PLUGIN})
#		list(APPEND DEPENDENCY_RESOLVE_PATHS "${Qt5_DIR}/../..")
#		list(APPEND DEPENDENCY_RESOLVE_PATHS "/usr/lib")

		# Dependencies of targets and plugins
		lv_collect_libs("${INSTALLED_BINARIES}" "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

		# qt.conf
		install(FILES "packaging/appimage/qt.conf" DESTINATION ${CMAKE_INSTALL_BINDIR})

		# Install ld-linux.so runtime
#		install(CODE "
#			execute_process(
#				COMMAND ldd \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/librevault-daemon\"
#				COMMAND grep -oP \"(\\\\S*ld-linux\\\\S*)\"
#				OUTPUT_VARIABLE ELF_INTERPRETER_LINK
#				OUTPUT_STRIP_TRAILING_WHITESPACE
#			)
#			execute_process(COMMAND readlink -f \${ELF_INTERPRETER_LINK} OUTPUT_VARIABLE ELF_INTERPRETER OUTPUT_STRIP_TRAILING_WHITESPACE)
#		")
#		install(PROGRAMS "/\${ELF_INTERPRETER}" DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME "ld-linux.so")

		# Install ld-linux shims
#		install(PROGRAMS "packaging/linux-standalone/shim_daemon.sh" DESTINATION ${CMAKE_INSTALL_SHIMDIR} RENAME "librevault-daemon")
#		install(PROGRAMS "packaging/linux-standalone/shim_gui.sh" DESTINATION ${CMAKE_INSTALL_SHIMDIR} RENAME "librevault-gui")
#		install(PROGRAMS "packaging/linux-standalone/shim_cli.sh" DESTINATION ${CMAKE_INSTALL_SHIMDIR} RENAME "librevault-cli")
	endif()
elseif(OS_MAC)
#	set(CPACK_GENERATOR "Bundle")
#	set(CPACK_PACKAGE_FILE_NAME "Librevault")
	# DragNDrop
#	set(CPACK_DMG_FORMAT "ULFO")
#	set(CPACK_DMG_DS_STORE "${CMAKE_SOURCE_DIR}/packaging/osx/DS_Store.in")
#	set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/packaging/osx/background.tiff")
	# Bundle
#	set(CPACK_BUNDLE_NAME "Librevault")
#	set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/packaging/osx/Librevault.icns")

	set(APP_ROOT "${CMAKE_INSTALL_PREFIX}/Librevault.app")
	install(DIRECTORY DESTINATION ${APP_ROOT})
	configure_file("${CMAKE_SOURCE_DIR}/packaging/osx/Info.plist" "${CMAKE_BINARY_DIR}/Info.plist" @ONLY)

	if(BUILD_DAEMON)
		install(PROGRAMS $<TARGET_FILE:librevault-daemon> DESTINATION ${APP_ROOT}/Contents/MacOS)
	endif()
	if(BUILD_GUI)
		install(PROGRAMS $<TARGET_FILE:librevault-gui> DESTINATION ${APP_ROOT}/Contents/MacOS)
	endif()
	install(FILES "${CMAKE_BINARY_DIR}/Info.plist" DESTINATION "${APP_ROOT}/Contents")

	install(FILES "packaging/osx/qt.conf" DESTINATION ${APP_ROOT}/Contents/Resources)
	install(FILES "packaging/osx/dsa_pub.pem" DESTINATION ${APP_ROOT}/Contents/Resources)
	install(FILES "packaging/osx/Librevault.icns" DESTINATION ${APP_ROOT}/Contents/Resources)

	# Bundle plugin path
	set(BUNDLE_PLUGINS_PATH "${APP_ROOT}/Contents/PlugIns")

	# Qt5 plugins
	install_qt5_plugin("Qt5::QMinimalIntegrationPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
	install_qt5_plugin("Qt5::QCocoaIntegrationPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
	install_qt5_plugin("Qt5::QSvgPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
	install_qt5_plugin("Qt5::QSvgIconPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
	install_qt5_plugin("Qt5::QSQLiteDriverPlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")
	install_qt5_plugin("Qt5::QMacStylePlugin" QT_PLUGIN "${BUNDLE_PLUGINS_PATH}")

	install(CODE "
	include(BundleUtilities)
	set(BU_CHMOD_BUNDLE_ITEMS ON)
	fixup_bundle(\"${APP_ROOT}\" \"${QT_PLUGIN}\" \"${LIBRARY_SEARCH_PATHS}\")
	")
endif()

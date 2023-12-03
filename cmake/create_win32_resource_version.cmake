# example:
# if(WIN32)
# create_win32_resource_version(
# 		TARGET ${DA_LIB_NAME}
# 		FILENAME ${DA_LIB_NAME}
# 		VERSION ${DA_LIB_VERSION}
# 		EXT "dll"
# 		COMPANYNAME "DA"
# 		COPYRIGHT "czy"
# 		DESCRIPTION ${DA_LIB_DESCRIPTION}
# )
# endif()
macro(create_win32_resource_version)
	if(MSVC) # TODO: MinGW (http://www.mingw.org/wiki/MS_resource_compiler)
		set(_target)
		set(_filename)
		set(_version ${PROJECT_VERSION})
		set(_ext "ico")
		set(_companyname "https://github.com/czyt1988")
		set(_copyright "Copyright (C) 2023 by 尘中远 github:czyt1988")
		set(_description "DA")
		set(cmd "_target")
		foreach(arg ${ARGN})
			if(arg STREQUAL "TARGET")
				set(cmd "_target")
			elseif(arg STREQUAL "FILENAME")
				set(cmd "_filename")
			elseif(arg STREQUAL "VERSION")
				set(cmd "_version")
			elseif(arg STREQUAL "EXT")
				set(cmd "_ext")
			elseif(arg STREQUAL "COMPANYNAME")
				set(cmd "_companyname")
			elseif(arg STREQUAL "COPYRIGHT")
				set(cmd "_copyright")
			elseif(arg STREQUAL "DESCRIPTION")
				set(cmd "_description")
			else()
				if("${cmd}" STREQUAL "_target")
					set(_target ${arg})
				elseif("${cmd}" STREQUAL "_filename")
					set(_filename ${arg})
				elseif("${cmd}" STREQUAL "_version")
					set(_version ${arg})
				elseif("${cmd}" STREQUAL "_ext")
					set(_ext ${arg})
				elseif("${cmd}" STREQUAL "_companyname")
					set(_companyname ${arg})
				elseif("${cmd}" STREQUAL "_copyright")
					set(_copyright ${arg})
				elseif("${cmd}" STREQUAL "_description")
					set(_description ${arg})
				else()
				endif()
			endif()
		endforeach()
		string(REGEX MATCHALL "[.]" matches "${_version}")
		list(LENGTH matches n_matches)
		while(n_matches LESS 3)
			string(APPEND _version ".0")
			string(REGEX MATCHALL "[.]" matches "${_version}")
			list(LENGTH matches n_matches)
		endwhile()
		string(REPLACE "." "," PC ${_version})
		set(RC_FILE ${CMAKE_CURRENT_BINARY_DIR}/${_target}.rc)
		file(WRITE  ${RC_FILE} "#include <winres.h>\n\n")
		file(APPEND ${RC_FILE} "#define TARGET_NAME \"${_target}\"\n")
		file(APPEND ${RC_FILE} "#define FILE_VERSION_C ${PC}\n")
		file(APPEND ${RC_FILE} "#define FILE_VERSION_S \"${_version}\"\n")
		if("${_ext}" STREQUAL "exe")
			set(_filetype "0x1L")
			file(APPEND ${RC_FILE} "#define FILE_NAME \"${_filename}.exe\"\n")
		else()
			set(_filetype "0x2L")
			file(APPEND ${RC_FILE} "#ifdef _DEBUG\n")
			file(APPEND ${RC_FILE} "#define FILE_NAME \"${_filename}${CMAKE_DEBUG_POSTFIX}.dll\"\n")
			file(APPEND ${RC_FILE} "#else\n")
			file(APPEND ${RC_FILE} "#define FILE_NAME \"${_filename}.dll\"\n")
			file(APPEND ${RC_FILE} "#endif\n")
		endif()
		file(APPEND ${RC_FILE} "\nVS_VERSION_INFO VERSIONINFO\n")
		file(APPEND ${RC_FILE} " FILEVERSION FILE_VERSION_C\n")
		file(APPEND ${RC_FILE} " PRODUCTVERSION FILE_VERSION_C\n")
		file(APPEND ${RC_FILE} " FILEFLAGSMASK 0x3fL\n")
		file(APPEND ${RC_FILE} "#ifdef _DEBUG\n")
		file(APPEND ${RC_FILE} " FILEFLAGS 0x1L\n")
		file(APPEND ${RC_FILE} "#else\n")
		file(APPEND ${RC_FILE} " FILEFLAGS 0x0L\n")
		file(APPEND ${RC_FILE} "#endif\n")
		file(APPEND ${RC_FILE} " FILEOS 0x40004L\n")
		file(APPEND ${RC_FILE} " FILETYPE ${_filetype}\n")
		file(APPEND ${RC_FILE} " FILESUBTYPE 0x0L\n")
		file(APPEND ${RC_FILE} "BEGIN\n")
		file(APPEND ${RC_FILE} "    BLOCK \"StringFileInfo\"\n")
		file(APPEND ${RC_FILE} "    BEGIN\n")
		file(APPEND ${RC_FILE} "        BLOCK \"040704b0\"\n")
		file(APPEND ${RC_FILE} "        BEGIN\n")
		file(APPEND ${RC_FILE} "            VALUE \"CompanyName\", \"${_companyname}\"\n")
		file(APPEND ${RC_FILE} "            VALUE \"FileDescription\", \"${_description}\"\n")
		file(APPEND ${RC_FILE} "            VALUE \"FileVersion\", FILE_VERSION_S\n")
		file(APPEND ${RC_FILE} "            VALUE \"InternalName\", FILE_NAME\n")
		file(APPEND ${RC_FILE} "            VALUE \"LegalCopyright\", \"${_copyright}\"\n")
		file(APPEND ${RC_FILE} "            VALUE \"OriginalFilename\", FILE_NAME\n")
		file(APPEND ${RC_FILE} "            VALUE \"ProductName\", TARGET_NAME\n")
		file(APPEND ${RC_FILE} "            VALUE \"ProductVersion\", FILE_VERSION_S\n")
		file(APPEND ${RC_FILE} "        END\n")
		file(APPEND ${RC_FILE} "    END\n")
		file(APPEND ${RC_FILE} "    BLOCK \"VarFileInfo\"\n")
		file(APPEND ${RC_FILE} "    BEGIN\n")
		file(APPEND ${RC_FILE} "        VALUE \"Translation\", 0x409, 1200\n")
		file(APPEND ${RC_FILE} "    END\n")
		file(APPEND ${RC_FILE} "END\n")
		target_sources(${_target} PRIVATE ${RC_FILE})
	endif()
endmacro(create_win32_resource_version)

# example:
# set(__rc_path "${PROJECT_BINARY_DIR}/${DEMO_NAME}.rc")
# enerate_win32_rc_file(
# 	PATH "${__rc_path}"
# 	VERSION "${PROJECT_VERSION}"
# 	COMPANY "wangwenx190"
# 	DESCRIPTION "FramelessHelper Demo Application: MainWindow"
# 	COPYRIGHT "MIT License"
# 	PRODUCT "FramelessHelper Demo"
# 	ICONS "../shared/example.ico"
# )
function(generate_win32_rc_file)
    cmake_parse_arguments(RC_ARGS "LIBRARY" "PATH;COMMENTS;COMPANY;DESCRIPTION;VERSION;INTERNAL_NAME;COPYRIGHT;TRADEMARK;ORIGINAL_FILENAME;PRODUCT" "ICONS" ${ARGN})
    if(NOT RC_ARGS_PATH)
        message(AUTHOR_WARNING "generate_win32_rc_file: You need to specify where to put the generated rc file for this function!")
        return()
    endif()
    if(RC_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "generate_win32_rc_file: Unrecognized arguments: ${RC_ARGS_UNPARSED_ARGUMENTS}")
        return()
    endif()
    set(__file_type)
    if(RC_ARGS_LIBRARY)
        set(__file_type "VFT_DLL")
    else()
        set(__file_type "VFT_APP")
    endif()
    set(__icons)
    if(RC_ARGS_ICONS)
        set(__index 1)
        foreach(__icon IN LISTS RC_ARGS_ICONS)
            string(APPEND __icons "IDI_ICON${__index}    ICON    \"${__icon}\"\n")
            math(EXPR __index "${__index} +1")
        endforeach()
    endif()
    set(__comments)
    if(RC_ARGS_COMMENTS)
        set(__comments "${RC_ARGS_COMMENTS}")
    endif()
    set(__company)
    if(RC_ARGS_COMPANY)
        set(__company "${RC_ARGS_COMPANY}")
    endif()
    set(__description)
    if(RC_ARGS_DESCRIPTION)
        set(__description "${RC_ARGS_DESCRIPTION}")
    endif()
    set(__version)
    if(RC_ARGS_VERSION)
        if(RC_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
            set(__version "${RC_ARGS_VERSION}")
        elseif(RC_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
            set(__version "${RC_ARGS_VERSION}.0")
        elseif(RC_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+")
            set(__version "${RC_ARGS_VERSION}.0.0")
        elseif(RC_ARGS_VERSION MATCHES "[0-9]+")
            set(__version "${RC_ARGS_VERSION}.0.0.0")
        else()
            message(FATAL_ERROR "generate_win32_rc_file: Invalid version format: '${RC_ARGS_VERSION}'")
        endif()
    else()
        set(__version "0.0.0.0")
    endif()
    set(__version_comma)
    string(REPLACE "." "," __version_comma ${__version})
    set(__internal_name)
    if(RC_ARGS_INTERNAL_NAME)
        set(__internal_name "${RC_ARGS_INTERNAL_NAME}")
    endif()
    set(__copyright)
    if(RC_ARGS_COPYRIGHT)
        set(__copyright "${RC_ARGS_COPYRIGHT}")
    endif()
    set(__trademark)
    if(RC_ARGS_TRADEMARK)
        set(__trademark "${RC_ARGS_TRADEMARK}")
    endif()
    set(__original_filename)
    if(RC_ARGS_ORIGINAL_FILENAME)
        set(__original_filename "${RC_ARGS_ORIGINAL_FILENAME}")
    endif()
    set(__product)
    if(RC_ARGS_PRODUCT)
        set(__product "${RC_ARGS_PRODUCT}")
    endif()
    set(__contents "// This file is auto-generated by CMake. DO NOT EDIT! ALL MODIFICATIONS WILL BE LOST!

#include <windows.h> // Use lower-cased file names to be compatible with MinGW.

${__icons}

VS_VERSION_INFO VERSIONINFO
FILEVERSION     ${__version_comma}
PRODUCTVERSION  ${__version_comma}
FILEFLAGSMASK   0x3fL
#ifdef _DEBUG
    FILEFLAGS   VS_FF_DEBUG
#else // !_DEBUG
    FILEFLAGS   0x0L
#endif // _DEBUG
FILEOS          VOS_NT_WINDOWS32
FILETYPE        ${__file_type}
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK \"StringFileInfo\"
    BEGIN
        BLOCK \"040904b0\"
        BEGIN
            VALUE \"CompanyName\",      \"${__company}\\0\"
            VALUE \"FileDescription\",  \"${__description}\\0\"
            VALUE \"FileVersion\",      \"${__version}\\0\"
            VALUE \"LegalCopyright\",   \"${__copyright}\\0\"
            VALUE \"OriginalFilename\", \"${__original_filename}\\0\"
            VALUE \"ProductName\",      \"${__product}\\0\"
            VALUE \"ProductVersion\",   \"${__version}\\0\"
            VALUE \"Comments\",         \"${__comments}\\0\"
            VALUE \"LegalTrademarks\",  \"${__trademark}\\0\"
            VALUE \"InternalName\",     \"${__internal_name}\\0\"
        END
    END
    BLOCK \"VarFileInfo\"
    BEGIN
        VALUE \"Translation\", 0x0409, 1200
    END
END
")
    file(GENERATE OUTPUT "${RC_ARGS_PATH}" CONTENT "${__contents}")
endfunction()

function(generate_win32_manifest_file)
    cmake_parse_arguments(MF_ARGS "UTF8_CODEPAGE;VISTA_COMPAT;WIN7_COMPAT;WIN8_COMPAT;WIN8_1_COMPAT;WIN10_COMPAT;WIN11_COMPAT;XAML_ISLANDS_COMPAT;REQUIRE_ADMIN" "PATH;ID;VERSION;DESCRIPTION;ARCHITECTURE;LANGUAGE;PUBLIC_KEY_TOKEN" "" ${ARGN})
    if(NOT MF_ARGS_PATH)
        message(AUTHOR_WARNING "generate_win32_manifest_file: You need to specify where to put the generated rc file for this function!")
        return()
    endif()
    if(NOT MF_ARGS_ID)
        message(AUTHOR_WARNING "generate_win32_manifest_file: You need to specify your application identifier for this function!")
        return()
    endif()
    if(MF_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "generate_win32_manifest_file: Unrecognized arguments: ${MF_ARGS_UNPARSED_ARGUMENTS}")
        return()
    endif()
    set(__id "${MF_ARGS_ID}")
    set(__version "")
    if(MF_ARGS_VERSION)
        if(MF_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
            set(__version "${MF_ARGS_VERSION}")
        elseif(MF_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
            set(__version "${MF_ARGS_VERSION}.0")
        elseif(MF_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+")
            set(__version "${MF_ARGS_VERSION}.0.0")
        elseif(MF_ARGS_VERSION MATCHES "[0-9]+")
            set(__version "${MF_ARGS_VERSION}.0.0.0")
        else()
            message(FATAL_ERROR "generate_win32_manifest_file: Invalid version format: '${MF_ARGS_VERSION}'")
        endif()
    else()
        set(__version "0.0.0.0")
    endif()
    set(__architecture "")
    if(MF_ARGS_ARCHITECTURE)
        set(__architecture "processorArchitecture=\"${MF_ARGS_ARCHITECTURE}\"")
    endif()
    set(__language "")
    if(MF_ARGS_LANGUAGE)
        set(__language "language=\"${MF_ARGS_LANGUAGE}\"")
    endif()
    set(__public_key_token "")
    if(MF_ARGS_PUBLIC_KEY_TOKEN)
        set(__public_key_token "publicKeyToken=\"${MF_ARGS_PUBLIC_KEY_TOKEN}\"")
    endif()
    set(__description "")
    if(MF_ARGS_DESCRIPTION)
        set(__description "<description>${MF_ARGS_DESCRIPTION}</description>")
    endif()
    set(__execution_level "")
    if(MF_ARGS_REQUIRE_ADMIN)
        set(__execution_level "requireAdministrator")
    else()
        set(__execution_level "asInvoker")
    endif()
    set(__vista_compat "")
    if(MF_ARGS_VISTA_COMPAT)
        set(__vista_compat "<!-- Windows Vista and Windows Server 2008 -->
      <supportedOS Id=\"{e2011457-1546-43c5-a5fe-008deee3d3f0}\"/>")
    endif()
    set(__win7_compat "")
    if(MF_ARGS_WIN7_COMPAT)
        set(__win7_compat "<!-- Windows 7 and Windows Server 2008 R2 -->
      <supportedOS Id=\"{35138b9a-5d96-4fbd-8e2d-a2440225f93a}\"/>")
    endif()
    set(__win8_compat "")
    if(MF_ARGS_WIN8_COMPAT)
        set(__win8_compat "<!-- Windows 8 and Windows Server 2012 -->
      <supportedOS Id=\"{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}\"/>")
    endif()
    set(__win8_1_compat "")
    if(MF_ARGS_WIN8_1_COMPAT)
        set(__win8_1_compat "<!-- Windows 8.1 and Windows Server 2012 R2 -->
      <supportedOS Id=\"{1f676c76-80e1-4239-95bb-83d0f6d0da78}\"/>")
    endif()
    set(__win10_11_compat "")
    if(MF_ARGS_WIN10_COMPAT OR MF_ARGS_WIN11_COMPAT)
        set(__win10_11_compat "<!-- Windows 10, Windows 11, Windows Server 2016, Windows Server 2019 and Windows Server 2022 -->
      <supportedOS Id=\"{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}\"/>")
    endif()
    set(__xaml_islands_compat "")
    if(MF_ARGS_XAML_ISLANDS_COMPAT)
        set(__xaml_islands_compat "<!-- Windows 10 Version 1809 (October 2018 Update) -->
      <maxversiontested Id=\"10.0.17763.0\"/>
      <!-- Windows 10 Version 1903 (May 2019 Update) -->
      <maxversiontested Id=\"10.0.18362.0\"/>
      <!-- Windows 10 Version 1909 (November 2019 Update) -->
      <maxversiontested Id=\"10.0.18363.0\"/>
      <!-- Windows 10 Version 2004 (May 2020 Update) -->
      <maxversiontested Id=\"10.0.19041.0\"/>
      <!-- Windows 10 Version 20H2 (October 2020 Update) -->
      <maxversiontested Id=\"10.0.19042.0\"/>
      <!-- Windows 10 Version 21H1 (May 2021 Update) -->
      <maxversiontested Id=\"10.0.19043.0\"/>
      <!-- Windows 10 Version 21H2 (November 2021 Update) -->
      <maxversiontested Id=\"10.0.19044.0\"/>
      <!-- Windows 10 Version 22H2 (October 2022 Update) -->
      <maxversiontested Id=\"10.0.19045.0\"/>
      <!-- Windows 11 Version 21H2 -->
      <maxversiontested Id=\"10.0.22000.0\"/>
      <!-- Windows 11 Version 22H2 (October 2022 Update) -->
      <maxversiontested Id=\"10.0.22621.0\"/>")
    endif()
    set(__utf8_codepage "")
    if(MF_ARGS_UTF8_CODEPAGE)
        set(__utf8_codepage "<activeCodePage xmlns=\"http://schemas.microsoft.com/SMI/2019/WindowsSettings\">UTF-8</activeCodePage>")
    endif()
    set(__contents "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>

<!-- This file is auto-generated by CMake. DO NOT EDIT! ALL MODIFICATIONS WILL BE LOST! -->

<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">
  <assemblyIdentity type=\"win32\" name=\"${__id}\" version=\"${__version}\" ${__architecture} ${__public_key_token} ${__language}/>
  ${__description}
  <dependency>
    <dependentAssembly>
      <assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.Common-Controls\" version=\"6.0.0.0\" processorArchitecture=\"*\" publicKeyToken=\"6595b64144ccf1df\" language=\"*\"/>
    </dependentAssembly>
  </dependency>
  <trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v3\">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level=\"${__execution_level}\" uiAccess=\"false\"/>
      </requestedPrivileges>
    </security>
  </trustInfo>
  <compatibility xmlns=\"urn:schemas-microsoft-com:compatibility.v1\">
    <application>
      ${__xaml_islands_compat}
      ${__vista_compat}
      ${__win7_compat}
      ${__win8_compat}
      ${__win8_1_compat}
      ${__win10_11_compat}
    </application>
  </compatibility>
  <application xmlns=\"urn:schemas-microsoft-com:asm.v3\">
    <windowsSettings>
      <dpiAware xmlns=\"http://schemas.microsoft.com/SMI/2005/WindowsSettings\">True/PM</dpiAware>
      <printerDriverIsolation xmlns=\"http://schemas.microsoft.com/SMI/2011/WindowsSettings\">true</printerDriverIsolation>
      <disableWindowFiltering xmlns=\"http://schemas.microsoft.com/SMI/2011/WindowsSettings\">true</disableWindowFiltering>
      <highResolutionScrollingAware xmlns=\"http://schemas.microsoft.com/SMI/2013/WindowsSettings\">true</highResolutionScrollingAware>
      <ultraHighResolutionScrollingAware xmlns=\"http://schemas.microsoft.com/SMI/2013/WindowsSettings\">true</ultraHighResolutionScrollingAware>
      <dpiAwareness xmlns=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">PerMonitorV2, PerMonitor</dpiAwareness>
      <longPathAware xmlns=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">true</longPathAware>
      <!-- <gdiScaling xmlns=\"http://schemas.microsoft.com/SMI/2017/WindowsSettings\">true</gdiScaling> -->
      ${__utf8_codepage}
      <heapType xmlns=\"http://schemas.microsoft.com/SMI/2020/WindowsSettings\">SegmentHeap</heapType>
    </windowsSettings>
  </application>
</assembly>
")
    file(GENERATE OUTPUT "${MF_ARGS_PATH}" CONTENT "${__contents}")
endfunction()

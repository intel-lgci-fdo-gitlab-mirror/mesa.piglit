include_directories(
	${piglit_BINARY_DIR}/tests/util
	${piglit_SOURCE_DIR}/tests/util
)

file(
	GLOB PIGLIT_TESTS_METAFILES
	"*.meta.xml"
)

file(
	COPY ${PIGLIT_TESTS_METAFILES}
	DESTINATION ${piglit_BINARY_DIR}/tests
)

if(MSVC)
	include_directories(${piglit_SOURCE_DIR}/tests/util/msvc)
	add_definitions(-DSTATIC_GETOPT)
endif()

add_subdirectory (util)
add_subdirectory (fbo)
add_subdirectory (gbm)
add_subdirectory (general)
add_subdirectory (glx)
add_subdirectory (glslparsertest)
add_subdirectory (hiz)
add_subdirectory (asmparsertest)
add_subdirectory (security)
add_subdirectory (shaders)
add_subdirectory (texturing)
add_subdirectory (spec)
add_subdirectory (fast_color_clear)
add_subdirectory (llvmpipe)
add_subdirectory (perf)
add_subdirectory (wgl)

IF(EGL_FOUND)
	add_subdirectory (egl)
ENDIF(EGL_FOUND)

IF(PIGLIT_BUILD_CL_TESTS)
	add_subdirectory (cl)
ENDIF(PIGLIT_BUILD_CL_TESTS)

if(PIGLIT_USE_WAYLAND)
	add_subdirectory (wayland)
ENDIF(PIGLIT_USE_WAYLAND)

piglit_include_target_api()

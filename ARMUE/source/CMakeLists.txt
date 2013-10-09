#set executable and library output directory
set(EXECUTABLE_OUTPUT_PATH ${ARMUE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${ARMUE_SOURCE_DIR}/lib)

set(SOURCE_DIRS 
./
./core
./utils
./arch/arm
)

#include directory
include_directories(${SOURCE_DIRS})

aux_source_directory(./ MAIN_APP)
aux_source_directory(./core CORE_FILE)
aux_source_directory(./utils UTILS_FILE)
aux_source_directory(./arch/arm ARCH_ARM_FILE)

SET(CMAKE_C_FLAGS "$ENV{CFLAGS} -O2 -Wall -g -finline-functions")
ADD_DEFINITIONS(-D_DEBUG)

#executable and library name
add_executable(ARMUE 
${MAIN_APP}
${CORE_FILE}
${UTILS_FILE}
${ARCH_ARM_FILE}
)
#add_library(ADL_LIB STATIC ${SOURCES})

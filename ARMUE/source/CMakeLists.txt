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

#main app
aux_source_directory(./ MAIN_APP)

#tests
set(SPEED_TEST ./test/speed_test.c)

#armv7m instruction test
set(ARM_INS_TEST ./test/armv7m_instruction_test.c)

#source files
aux_source_directory(./core CORE_FILE)
aux_source_directory(./utils UTILS_FILE)
aux_source_directory(./arch/arm ARCH_ARM_FILE)

SET(CMAKE_C_FLAGS "$ENV{CFLAGS} -O0 -Wall -Wl,-Map,debug.map -g -ggdb3 -finline-functions")
ADD_DEFINITIONS(-D_DEBUG)

#executable and library name
add_executable(ARMUE
${MAIN_APP}
${CORE_FILE}
${UTILS_FILE}
${ARCH_ARM_FILE}
)

add_executable(Speed_test
${SPEED_TEST}
${CORE_FILE}
${UTILS_FILE}
${ARCH_ARM_FILE}
)

add_executable(arm_instruction_test
${ARM_INS_TEST}
${CORE_FILE}
${UTILS_FILE}
${ARCH_ARM_FILE}
)

#add_library(ADL_LIB STATIC ${SOURCES})


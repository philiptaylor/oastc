cmake_minimum_required(VERSION 2.8)

project(oastc)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

add_executable(oastc_dec oastc_dec.cpp)

add_executable(oastc_unit_tests unit_tests.cpp)

add_executable(oastc_testgen test_generator.cpp)

add_custom_target(testgen_images_dir
  COMMAND ${CMAKE_COMMAND} -E make_directory testgen_img
  COMMAND ${CMAKE_COMMAND} -E make_directory converted_img/0ad
  COMMAND ${CMAKE_COMMAND} -E make_directory converted_img/misc
)

set(TESTGEN_FILES "")
foreach(F
  4x4x1-0
  5x4x1-0
  5x5x1-0
  6x5x1-0
  6x6x1-0
  8x5x1-0 8x5x1-1
  8x6x1-0 8x6x1-1
  8x8x1-0 8x8x1-1
  10x5x1-0 10x5x1-1 10x5x1-2
  10x6x1-0 10x6x1-1 10x6x1-2
  10x8x1-0 10x8x1-1 10x8x1-2 10x8x1-3
  10x10x1-0 10x10x1-1 10x10x1-2 10x10x1-3
  12x10x1-0 12x10x1-1 12x10x1-2 12x10x1-3 12x10x1-4 12x10x1-5
  12x12x1-0 12x12x1-1 12x12x1-2 12x12x1-3 12x12x1-4 12x12x1-5
) # TODO: this is making unreasonable assumptions about the outputs of testgen
  set(TESTGEN_FILES ${TESTGEN_FILES} "testgen_img/testgen_${F}.astc")
endforeach()

add_custom_command(
  OUTPUT ${TESTGEN_FILES}
  COMMAND oastc_testgen
  WORKING_DIRECTORY testgen_img
  DEPENDS oastc_testgen testgen_images_dir)

set(TEST_ASTC_OUTPUTS "")
foreach(IMAGE misc/test1 0ad/tree_cypress)
  foreach(BLOCKSIZE 4x4 10x5 12x12)
    foreach(QUALITY veryfast fast medium thorough)
      add_custom_command(
        OUTPUT "converted_img/${IMAGE}.astcenc.${BLOCKSIZE}.${QUALITY}.astc"
        COMMAND astcenc -c "${CMAKE_CURRENT_SOURCE_DIR}/inputs/${IMAGE}.png" "converted_img/${IMAGE}.astcenc.${BLOCKSIZE}.${QUALITY}.astc" ${BLOCKSIZE} -${QUALITY}
        DEPENDS "inputs/${IMAGE}.png"
      )
      set(TEST_ASTC_OUTPUTS ${TEST_ASTC_OUTPUTS} "converted_img/${IMAGE}.astcenc.${BLOCKSIZE}.${QUALITY}.astc")
    endforeach()
  endforeach()
endforeach()

set(TEST_ASTC_OUTPUTS ${TEST_ASTC_OUTPUTS} ${TESTGEN_FILES})

set(TEST_ASTC_DECODED "")
foreach(ASTC ${TEST_ASTC_OUTPUTS})
  add_custom_command(
    OUTPUT "${ASTC}.astcenc.tga"
    COMMAND astcenc -d "${ASTC}" "${ASTC}.astcenc.tga"
    DEPENDS "${ASTC}"
  )
  add_custom_command(
    OUTPUT "${ASTC}.oastc_dec.tga"
    COMMAND oastc_dec -i "${ASTC}" -o "${ASTC}.oastc_dec.tga"
    DEPENDS "${ASTC}" oastc_dec
  )
  set(TEST_ASTC_DECODED ${TEST_ASTC_DECODED} "${ASTC}.astcenc.tga")
  set(TEST_ASTC_DECODED ${TEST_ASTC_DECODED} "${ASTC}.oastc_dec.tga")
endforeach()

add_custom_target(test
  DEPENDS oastc_unit_tests
  COMMAND ./oastc_unit_tests
)

add_custom_target(oastc_tests
  DEPENDS ${TEST_ASTC_DECODED}
)

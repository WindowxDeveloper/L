add_module(
  stb_perlin
  CONDITION ${DEV_DBG}
  SOURCES ${CMAKE_CURRENT_LIST_DIR}/stb_perlin.cpp
  SYSTEM_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}
)

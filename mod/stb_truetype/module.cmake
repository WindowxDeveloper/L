add_module(
  stb_truetype
  CONDITION ${DEV_DBG}
  SOURCES ${CMAKE_CURRENT_LIST_DIR}/stb_truetype.cpp
)
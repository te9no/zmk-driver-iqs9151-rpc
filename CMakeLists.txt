zephyr_include_directories(include)

add_subdirectory(drivers)

if(EXISTS "${APPLICATION_SOURCE_DIR}/include/drivers/behavior.h")
  add_subdirectory(behaviors)
endif()

if(EXISTS "${APPLICATION_SOURCE_DIR}/include/drivers/input_processor.h")
  add_subdirectory(input_processors)
endif()

if(CONFIG_ZMK_SPLIT_RELAY_EVENT AND (CONFIG_INPUT_IQS9151_STUDIO_RPC OR CONFIG_INPUT_IQS9151))
  target_sources(app PRIVATE src/studio/iqs9151_split.c)
endif()

if(CONFIG_INPUT_IQS9151_STUDIO_RPC)
  target_sources(app PRIVATE src/studio/iqs9151_handler.c)

  list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
  include(nanopb)
  set(NANOPB_GENERATE_CPP_APPEND_PATH TRUE)
  set(NANOPB_GENERATE_CPP_STANDALONE OFF)

  zephyr_library()
  file(GLOB_RECURSE PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

  nanopb_generate_cpp(proto_srcs proto_hdrs RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES})
  target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${NANOPB_INCLUDE_DIRS}
  )
  target_sources(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${proto_srcs} ${proto_hdrs})

  target_include_directories(app PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/proto
    ${NANOPB_INCLUDE_DIRS}
  )
  add_dependencies(app ${ZEPHYR_CURRENT_LIBRARY})
endif()

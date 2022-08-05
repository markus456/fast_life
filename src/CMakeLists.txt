add_executable(fast_life main.cc events.cc graphics.cc)
target_link_libraries(fast_life ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
install(TARGETS fast_life DESTINATION ${CMAKE_BINARY_DIR})
target_link_options(fast_life PRIVATE /SUBSYSTEM:windows /ENTRY:mainCRTStartup)
set(PROJECT_NAME unpaker)
project(${PROJECT_NAME})

# Not available in 64-bit mode.
set(CMAKE_CXX_FLAGS "-m32")

# Boost's locale required for CP1251 -> UFT-8 conversion.
find_package(Boost COMPONENTS locale REQUIRED)

add_executable(${PROJECT_NAME}
    decode.h
    pak.cpp
    pak.h
    unpaker.cpp
)

include_directories(../common)

target_link_libraries(${PROJECT_NAME} Boost::locale)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

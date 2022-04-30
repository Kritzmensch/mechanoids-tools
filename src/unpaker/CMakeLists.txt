set(PROJECT_NAME unpaker)
project(${PROJECT_NAME})

# Not available in 64-bit mode
set(CMAKE_CXX_FLAGS "-m32")

add_executable(${PROJECT_NAME}
    decode.h
    pak.cpp
    pak.h
    unpaker.cpp
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

set(PROJECT_NAME name_generator)
project(${PROJECT_NAME})

add_executable(${PROJECT_NAME}
    name_generator.cpp
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

cmake_minimum_required(VERSION 3.8)

project(xx)

include_directories(
	../xx/
)

set(XX_SRCS)
file(GLOB XX_SRCS
	*.h
	*.cpp
)

if (MSVC)
	list(APPEND XX_SRCS "xx.natvis")
endif()

add_library(xx STATIC
    ${XX_SRCS}
)

set(MQTTCLIENT_PACKAGE "mqtt-client")
set(MQTTCLIENT_PACKAGE_PATH "/usr/local/mqttclient")
set(MQTTCLIENT_PACKAGE_LIBS "")
set(MQTTCLIENT_PACKAGE_INC_DIRS "")
set(MQTTCLIENT_PACKAGE_LINK_DIRS "")

macro(find_depend_package package)
    find_package(${package} REQUIRED)
    if (NOT ${package}_FOUND)
        message(FATAL_ERROR "${package} not found!")
    endif (NOT ${package}_FOUND)
endmacro(find_depend_package package)

macro(find_depend_package_by_path package package_path)
    set(${package}_DIR ${package_path})
    find_depend_package(${package})
endmacro(find_depend_package_by_path package package_path)

foreach(depend_package ${MQTTCLIENT_PACKAGE})
    foreach(depend_package_path ${MQTTCLIENT_PACKAGE_PATH})
        find_depend_package_by_path(${depend_package} ${depend_package_path}/lib/cmake)
    endforeach()
    list(APPEND MQTTCLIENT_PACKAGE_LIBS ${${depend_package}_LIBRARIES})
    list(APPEND MQTTCLIENT_PACKAGE_INC_DIRS ${${depend_package}_INCLUDE_DIRS})
    list(APPEND MQTTCLIENT_PACKAGE_LINK_DIRS ${${depend_package}_LINK_DIRS})
endforeach()

message("\n------------ find package info  ------------")

message("--> depend packages : ")
foreach(depend_package ${MQTTCLIENT_PACKAGE_LIBS})
    message("    " ${depend_package})
endforeach()

message("--> depend packages : ")
foreach(depend_package_link_dir ${MQTTCLIENT_PACKAGE_LINK_DIRS})
    message("    " ${depend_package_link_dir})
endforeach()

message("--> depend packages : ")
foreach(depend_package_include_dir ${MQTTCLIENT_PACKAGE_INC_DIRS})
    message("    " ${depend_package_include_dir})
endforeach()

include_directories(${MQTTCLIENT_PACKAGE_INC_DIRS})
link_directories(${MQTTCLIENT_PACKAGE_LINK_DIRS})
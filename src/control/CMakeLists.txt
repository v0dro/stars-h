# @copyright (c) 2017 King Abdullah University of Science and
#                      Technology (KAUST). All rights reserved.
#
# STARS-H is a software package, provided by King Abdullah
#             University of Science and Technology (KAUST)
#
# @file src/control/CMakeLists.txt
# @version 1.3.0
# @author Aleksandr Mikhalev
# @date 2017-11-07


# set the values of the variable in the parent scope
set(STARSH_SRC "${CMAKE_CURRENT_SOURCE_DIR}/cluster.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/blrf.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/blrm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/array.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/problem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/init.c"
    ${STARSH_SRC})
set(STARSH_SRC ${STARSH_SRC} PARENT_SCOPE)

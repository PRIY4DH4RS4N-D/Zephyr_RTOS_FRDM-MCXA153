
# Copyright 2026 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

mcux_add_configuration(
    CC "-DSDK_DEBUGCONSOLE=1"
    CX "-DSDK_DEBUGCONSOLE=1"
)


mcux_add_source(
    SOURCES frdmmcxa153/board.c
            frdmmcxa153/board.h
)

mcux_add_include(
    INCLUDES frdmmcxa153
)

mcux_add_source(
    SOURCES frdmmcxa153/clock_config.c
            frdmmcxa153/clock_config.h
)

mcux_add_include(
    INCLUDES frdmmcxa153
)

mcux_add_source(
    SOURCES board/pin_mux.c
            board/pin_mux.h
)

mcux_add_include(
    INCLUDES board
)

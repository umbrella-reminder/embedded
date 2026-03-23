
set(source_dirs
    "src/main.c"
)

set(include_dirs
    "include"
)

idf_component_register(
    SRCS "${source_dirs}"
    INCLUDE_DIRS "${include_dirs}"
)
# Strict compiler warnings for project targets (early error detection).
#
#   mdbr_enable_warnings(<target>)
#
# MDBR_WARNINGS_AS_ERRORS=ON turns every warning into a build error. CI enables
# it so a warning can never be merged; keep it OFF locally if you are mid-change.
# Third-party code (md4c, Qt) is not affected: only targets passed here are.

option(MDBR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

function(mdbr_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4             # high warning level
            /permissive-    # standard-conforming mode
            /w14265         # class has virtual functions, but destructor is not virtual
            /w14826         # conversion is sign-extended
            /w14905         # wide string literal cast to LPSTR
        )
        if(MDBR_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wshadow                # local hides another variable
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            -Wcast-align
            -Wnull-dereference
            -Wimplicit-fallthrough
            -Wformat=2
        )
        if(MDBR_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

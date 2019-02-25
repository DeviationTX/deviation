# Get all subdirectories under ${current_dir} and store them
# in ${result} variable
macro(subdirlist result current_dir)
    file(GLOB children ${current_dir}/*)
    set(dirlist "")

    foreach(child ${children})
        if (IS_DIRECTORY ${child})
            list(APPEND dirlist ${child})
        endif()
    endforeach()

    set(${result} ${dirlist})
endmacro()

# Prepend ${CMAKE_CURRENT_SOURCE_DIR} to a ${directory} name
# and save it in PARENT_SCOPE ${variable}
macro(prepend_cur_dir variable directory)
    set(${variable} ${CMAKE_CURRENT_SOURCE_DIR}/${directory})
endmacro()

# Add custom command to print firmware size in Berkley format
function(firmware_size target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_SIZE_UTIL} -B
        "${CMAKE_CURRENT_BINARY_DIR}/${target}${CMAKE_EXECUTABLE_SUFFIX}"
    )
endfunction()

# Add a command to generate firmare in a provided format
function(generate_object target suffix type)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O${type}
        "${CMAKE_CURRENT_BINARY_DIR}/${target}${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_BINARY_DIR}/${target}${suffix}"
    )
endfunction()

# Add custom linker script to the linker flags
function(linker_script_add path_to_script)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -T ${path_to_script}")
endfunction()

# Update a target LINK_DEPENDS property with a custom linker script.
# That allows to rebuild that target if the linker script gets changed
function(linker_script_target_dependency target path_to_script)
    get_target_property(_cur_link_deps ${target} LINK_DEPENDS)
    string(APPEND _cur_link_deps " ${path_to_script}")
    set_target_properties(${target} PROPERTIES LINK_DEPENDS ${_cur_link_deps})
endfunction()

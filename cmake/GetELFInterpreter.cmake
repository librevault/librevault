function(get_elf_interpreter _elf _var)
    execute_process(
            COMMAND ldd \"${_elf}\"
            COMMAND grep -oP \"(\\S*ld-linux\\S*)\"
            OUTPUT_VARIABLE ELF_INTERPRETER_LINK
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(COMMAND readlink -f \${ELF_INTERPRETER_LINK} OUTPUT_VARIABLE ELF_INTERPRETER OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(NOT ELF_INTERPRETER)
        set(${_var} "ELF-NOTFOUND" PARENT_SCOPE)
        return()
    endif()

    set(${_var} "${ELF_INTERPRETER}" PARENT_SCOPE)
    return()
endfunction()
if (NOT EXISTS "C:/Users/Kimor/source/repos/0 - Ongoing/Insanity_Engine/Projects/SDL2-2.0.16/Project/install_manifest.txt")
    message(FATAL_ERROR "Cannot find install manifest: \"C:/Users/Kimor/source/repos/0 - Ongoing/Insanity_Engine/Projects/SDL2-2.0.16/Project/install_manifest.txt\"")
endif(NOT EXISTS "C:/Users/Kimor/source/repos/0 - Ongoing/Insanity_Engine/Projects/SDL2-2.0.16/Project/install_manifest.txt")

file(READ "C:/Users/Kimor/source/repos/0 - Ongoing/Insanity_Engine/Projects/SDL2-2.0.16/Project/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")
foreach (file ${files})
    message(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
    execute_process(
        COMMAND D:/Program Files/cmake-3.20.2-windows-x86_64/cmake-3.20.2-windows-x86_64/bin/cmake.exe -E remove "$ENV{DESTDIR}${file}"
        OUTPUT_VARIABLE rm_out
        RESULT_VARIABLE rm_retval
    )
    if(NOT ${rm_retval} EQUAL 0)
        message(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
    endif (NOT ${rm_retval} EQUAL 0)
endforeach(file)


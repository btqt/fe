# Run all Target or single Target
if( ${MACHINE_QEMU_ARM} MATCHES 1  )
    find_program( QEMU_ARM_BIN qemu-arm )

    if(NOT QEMU_ARM_BIN)
        message("qemu-arm not found!")
        message("Please install qemu")
        message(">> sudo apt install qemu")
        message(FATAL_ERROR "Aborting...")
    else()
        # convert variable from string to list
        separate_arguments(TARGET_BIN)
        foreach(BIN ${TARGET_BIN})
            message("Run : ${TARGET_PATH}/${BIN} --gtest_output=xml:reports/${BIN}-result.xml\n")
            execute_process(COMMAND ${QEMU_ARM_BIN} -L ${CMAKE_SYSROOT} ${TARGET_PATH}/${BIN} --gtest_output=xml:reports/${BIN}-result.xml)
        endforeach()
    endif() # NOT QEMU_ARM_BIN
else()
    # convert variable from string to list
    separate_arguments(TARGET_BIN)
    foreach(BIN ${TARGET_BIN})
        message("Run : ${TARGET_PATH}/${BIN} --gtest_output=xml:reports/${BIN}-result.xml\n")
        execute_process(COMMAND ${TARGET_PATH}/${BIN} --gtest_output=xml:reports/${BIN}-result.xml)
    endforeach()
endif() # NOT QEMU_ARM_BIN

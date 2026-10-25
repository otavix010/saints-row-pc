# What every Xbox 360 port's executable adds to the SDK's setup (x360-ports/comum, copied by sincronizar.py):
#   - the linker map, which names the frames of falha.log (scripts/pilha.py);
#   - PORT_CPU_NATIVO: made on the player's own PC (gerar.ps1), the translated code may use every instruction of that
#     processor (-march=native);
#   - the AMD FidelityFX runtime beside the executable, when the SDK was built with it: the runtime links it privately,
#     so the SDK's own copy of the runtime DLLs leaves it out, and without it the runtime does not load.

option(PORT_CPU_NATIVO "Compile for this PC's processor (-march=native)" OFF)

function(port_setup target)
    port_agility(${target})
    target_link_options(${target} PRIVATE "LINKER:/MAP:${CMAKE_CURRENT_BINARY_DIR}/${target}.map")
    if(PORT_CPU_NATIVO)
        target_compile_options(${target} PRIVATE -march=native)
    endif()
    if(WIN32 AND TARGET rex::amd_fidelityfx_dx12)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:rex::amd_fidelityfx_dx12>"
                    "$<TARGET_FILE_DIR:${target}>"
            VERBATIM)
    endif()
endfunction()

# Development: PORT_D3D12_AGILITY_DIR=<the Agility SDK's bin/x64> makes the executable take Direct3D 12 from a D3D12
# folder beside it (D3D12Core.dll, and d3d12SDKLayers.dll: the validation layer, which Windows lacks without the
# Graphics Tools), to read what the runtime does wrong (scripts/depuracao.py shows the messages).
set(PORT_D3D12_AGILITY_DIR "" CACHE PATH "Development: the Agility SDK's bin/x64, to validate Direct3D 12")
function(port_agility target)
    if(NOT PORT_D3D12_AGILITY_DIR)
        return()
    endif()
    target_sources(${target} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/port/agility.cpp")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/D3D12"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PORT_D3D12_AGILITY_DIR}/D3D12Core.dll"
                "${PORT_D3D12_AGILITY_DIR}/d3d12SDKLayers.dll" "$<TARGET_FILE_DIR:${target}>/D3D12"
        VERBATIM)
endfunction()

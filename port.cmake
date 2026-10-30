# What every Xbox 360 port's executable adds to the SDK's setup (x360-ports/comum, copied by sincronizar.py):
#   - the linker map, which names the frames of falha.log (scripts/pilha.py);
#   - PORT_CPU_NATIVO: made on the player's own PC (gerar.ps1), the translated code may use every instruction of that
#     processor (-march=native);
#   - the AMD FidelityFX runtime beside the executable, when the SDK was built with it: the runtime links it privately,
#     so the SDK's own copy of the runtime DLLs leaves it out, and without it the runtime does not load.

option(PORT_CPU_NATIVO "Compile for this PC's processor (-march=native)" OFF)

# the Otavix ports' standard performance meter (src/port/otavix_osd.c, from otavix010/silent-hill-pc, MIT) is C
enable_language(C)

function(port_setup target)
    port_agility(${target})
    # the GPU's vendor, for the render target path the port picks (src/port/port_options.h); the performance meter's
    # numbers from Windows (src/port/osd_stats.cpp); the frame limiter's wait (src/port/frame_limit.cpp)
    target_sources(${target} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/port/gpu_vendor.cpp"
                                     "${CMAKE_CURRENT_LIST_DIR}/src/port/osd_stats.cpp"
                                     "${CMAKE_CURRENT_LIST_DIR}/src/port/frame_limit.cpp"
                                     "${CMAKE_CURRENT_LIST_DIR}/src/port/otavix_osd.c")
    target_link_libraries(${target} PRIVATE dxgi pdh user32 gdi32)
    # the game's atomic sections locked by their address, not by the runtime's global lock (src/port/guest_lock.h,
    # after the generated header, whose macros it replaces)
    if(TARGET ${target}_recomp)
        target_precompile_headers(${target}_recomp PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/port/guest_lock.h")
    endif()
    target_link_options(${target} PRIVATE "LINKER:/MAP:${CMAKE_CURRENT_BINARY_DIR}/${target}.map")
    if(PORT_CPU_NATIVO)
        target_compile_options(${target} PRIVATE -march=native)
        target_compile_definitions(${target} PRIVATE PORT_CPU_NATIVO=1)  # said in the session's performance file
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

# the translated code's atomic sections take their lock below their address (scripts/reservas.ps1, guest_lock.h): a step
# of the code generation's own command, so it runs each time the files are made again
file(GLOB _port_manifest LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*_manifest.toml")
if(_port_manifest AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated/default/sources.cmake")
    add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/generated/default/codegen.build.stamp" APPEND
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_LIST_DIR}/scripts/reservas.ps1"
                -Pasta "${CMAKE_CURRENT_SOURCE_DIR}/generated/default")
endif()

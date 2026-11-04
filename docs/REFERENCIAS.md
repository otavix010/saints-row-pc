# Otavix — ports de Xbox 360: origem, licenças e referências

Os ports de Xbox 360 da Otavix rodam sobre o **ReXGlue SDK**, que recompila o código do jogo para PC e traz o runtime: a imitação da GPU Xenos, o kernel, o áudio e a entrada. Por cima dele, a Otavix escreveu uma base comum (`comum/`) e uma camada própria para cada jogo. Este documento explica de onde vem cada parte e o que serviu de referência, para que quem joga e quem fez esse caminho antes de nós possa conferir.

## 1. Base e componentes de terceiros (avisos exigidos por licença)

| Componente | Autor | Licença | Como entra |
|---|---|---|---|
| **ReXGlue SDK** 0.10.0.24 (`bd833a2`): recompilador e runtime | Tom Clay e colaboradores ([rexglue/rexglue-sdk](https://github.com/rexglue/rexglue-sdk)) | BSD 3-Clause | O tradutor (`rexglue.exe`) e o runtime (`rexruntime.dll`, `rexgpu-xenos.dll`) são distribuídos no pacote do SDK. O aviso original (`LICENSE-rexglue.txt`) acompanha o pacote. |
| **Xenia**, de onde o ReXGlue deriva boa parte do runtime | Ben Vanik e colaboradores do Xenia ([xenia.jp](https://xenia.jp)) | BSD 3-Clause | Dentro do runtime do ReXGlue. O aviso vem no `LICENSE-rexglue.txt`. |
| **AMD FidelityFX SDK** (FSR 1, CAS e o caminho temporal) | Advanced Micro Devices | MIT | `amd_fidelityfx_dx12.dll`, ao lado do executável. Aviso em `LICENSE-fidelityfx.txt`. |
| **FFmpeg** (decodificação do áudio XMA) | Projeto FFmpeg | LGPL 2.1 ou posterior | Dentro do `rexruntime.dll`, uma biblioteca dinâmica que o usuário pode substituir. O código-fonte do runtime, com o FFmpeg, é público (ver a seção 4). |
| **libmspack** (`lzxd.c`, descompressão LZX do executável do jogo) | Stuart Caie | LGPL 2.1 | Igual ao FFmpeg: dentro do `rexruntime.dll`, com o fonte público. |
| **Desmontador PowerPC do GNU binutils** (`thirdparty/disasm`) | Free Software Foundation | GPL | Só no **tradutor** (`rexglue.exe`), um programa separado que o gerador executa à parte. Não entra no runtime nem no executável do jogo. O fonte é público (seção 4). |
| SDL 3, fmt, spdlog, Dear ImGui, toml++, glslang, SPIRV-Tools, Vulkan, xxHash, simde, o1heap, CLI11, inja, Tracy, Catch2, utfcpp | Os autores de cada projeto | zlib, MIT, BSD, Apache 2.0 ou Boost | Embutidos no SDK. Os avisos vêm com o pacote do SDK. |
| **LLVM/Clang** (o compilador que o gerador baixa) | Projeto LLVM | Apache 2.0 com exceção LLVM | Uma ferramenta usada no PC do jogador. Não é redistribuída dentro do jogo. |

## 2. Referências de comportamento (sem código copiado)

Estas fontes ajudaram a entender **como cada jogo se comporta**. A implementação é nossa. Fatos medidos e documentados (endereços, valores, comportamento observado) não são código.

- **Patches de jogo do Xenia** (repositório game-patches). Os endereços do "Unlock FPS" do Asura's Wrath foram encontrados por **boma**. No port, eles viraram ganchos que seguem a opção de fps (`asuras-wrath/src/tick_rate.h`).
- **hells-gate-recomp**, de **florinp93**. A documentação de como a interface do Dante's Inferno avança guiou a técnica dos 120 fps (`dantes-inferno/src/ui_rate.h`). Os endereços foram achados de novo nesta versão do jogo, e nenhum código dele foi usado.
- **Notas de compatibilidade do Xenia** (xenia-canary/game-compatibility #326). O brilho do DiRT 2 depende da leitura de volta do resolve (`dirt-2/src/dirt2_app.h`).
- **Xenia Canary**. Foi referência para o comportamento da EDRAM e dos resolves nas nossas investigações. Nenhuma mudança dele foi portada para os ports.

Nosso agradecimento a essas pessoas e equipes, e em especial aos autores do ReXGlue e do Xenia: sem eles, levar estes jogos ao PC levaria anos a mais.

## 3. O que é autoral (Otavix)

- **A base comum dos ports** (`comum/`): opções de imagem e presets, o medidor de desempenho (Otavix OSD), o limitador e a taxa de quadros acima de 60, a trava de seções atômicas por endereço, a correção da corrida na entrada de controles, o relatório de falha (`falha.log`, com GPU perdida e DRED), a leitura da `.iso` e do jogo extraído, o controle roteirizado e a captura para bancadas, e o gerador (`gerar.ps1`).
- **A camada de cada jogo**: os 60 fps do Saints Row e os 120 fps do Asura, do Dante e do DiRT 2, o pool de áudio do Saints Row, a leitura de volta do brilho no DiRT 2 e as demais correções descritas no `NOVIDADES.md` de cada port.
- **As correções do SDK publicadas em `otavix010/rexglue-sdk-ffx`**: a textura 3D como 2D com escala e o diagnóstico de comandos. Seguem a licença BSD do SDK.

## 4. Onde está o código-fonte das partes LGPL e GPL

- O runtime e o tradutor são compilados do ReXGlue no commit `bd833a2` ([rexglue/rexglue-sdk](https://github.com/rexglue/rexglue-sdk)), com os submódulos do próprio repositório (o FFmpeg, o libmspack e o desmontador ficam em `thirdparty/`).
- As nossas mudanças e o modo de compilar ficam em [otavix010/rexglue-sdk-ffx](https://github.com/otavix010/rexglue-sdk-ffx) (`patches/` e `sdk-fidelityfx.cmd`).
- Quem quiser trocar o FFmpeg ou o libmspack pode recompilar o `rexruntime.dll` a partir desse fonte e colocá-lo no lugar do original.

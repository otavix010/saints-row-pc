# Saints Row — port nativo para PC

Port do **Saints Row** (Xbox 360) por **recompilação estática**: o código PowerPC do jogo é traduzido para C++ e compilado como um programa de Windows. Não há emulador de CPU rodando o jogo. A parte gráfica usa o runtime da [ReXGlue](https://github.com/rexglue/rexglue-sdk), derivado do [Xenia](https://xenia.jp).

**Nada do jogo vem neste repositório.** O port é gerado no seu PC a partir da sua própria cópia (a imagem `.iso` do disco). O código traduzido também é gerado aí e nunca é distribuído.

O runtime vem de uma compilação própria da ReXGlue com o AMD FidelityFX e correções do port ([otavix010/rexglue-sdk-ffx](https://github.com/otavix010/rexglue-sdk-ffx)), porque a oficial não traz o FidelityFX. Entre as correções está a da escala de resolução no D3D12, que derrubava o jogo acima de 1x.

O jeito mais fácil de jogar é pelo [Otavix Launcher](https://github.com/otavix010/otavix-launcher), que baixa este port, pede a sua `.iso`, gera tudo e cuida das opções.

## O que o port acrescenta

| | Xbox 360 | Port |
|--|--|--|
| Taxa de quadros | 30 fps | 30 ou **60** fps (quadros reais) |
| Resolução interna | 720p | 720p, **1440p** ou 4K |
| Ampliação para a tela | — | AMD FSR 1, AMD CAS, FSR 3 temporal (experimental) ou bilinear, com nitidez ajustável |
| Anti-aliasing | o do jogo | o do jogo, mais FXAA opcional |
| Filtro anisotrópico | o do jogo | até 16x |
| Contador de fps | — | opcional, no canto da tela |

DLSS e XeSS não entram: os dois precisam de vetores de movimento, que os jogos de Xbox 360 não fornecem. O ganho de nitidez vem da resolução interna (supersampling de verdade), e a fluidez vem dos fps reais.

## Estado dos testes

- Do início ao jogo livre a 60 fps; a velocidade do personagem a pé é a mesma a 30 e a 60.
- 120 fps não entra: o jogo seguiria a velocidade certa (a caminhada percorre a mesma distância a 60 e a 120), mas não passa de uns 70 fps, porque a thread do runtime que traduz os comandos de GPU do Xbox 360 fica a 98% de um núcleo com a quantidade de desenhos da cidade.
- Som funcionando, com o volume do port testado (25% dá 1/16 da potência, como previsto).

## Gerar à mão (sem o launcher)

Precisa do **Visual Studio 2022 Build Tools** com "Desenvolvimento para desktop com C++" e "C++ CMake tools for Windows". O gerador baixa o resto: o SDK da ReXGlue e o Clang (LLVM) portátil, numa pasta `_ferramentas` ao lado do port.

```powershell
.\gerar.ps1 -Iso "D:\Jogos\Saints Row (World).iso"
.\jogar.cmd
```

O port foi feito a partir do **Saints Row (World)**. O gerador confere o `default.xex` da sua `.iso` (SHA-1 `c2646093ff3926141fcf4fd630e876873d1caece`) e avisa se for outra versão.

## Opções

Ficam em `saintsrow.toml` ao lado do executável, ou no arquivo indicado pela variável `PORT_OPCOES`. O launcher grava em `opcoes.toml`. Cada uma também vale na linha de comando (`--fps 120`) ou como variável de ambiente (`REX_FPS=120`).

| Opção | Valores | Padrão |
|--|--|--|
| `fps` | 30, 60 | 60 |
| `resolution_scale` | 1 (720p), 2 (1440p), 3 (4K) | 2 |
| `filtro` | `fsr`, `cas`, `fsr3` (temporal, experimental), `bilinear` | `fsr` |
| `nitidez` | 0 a 100 | 70 |
| `swap_post_effect` | `none`, `fxaa`, `fxaa_extreme` | `none` |
| `anisotropic_override` | -1 (o do jogo), 3 (4x), 4 (8x), 5 (16x) | 5 |
| `fullscreen` | `true`, `false` | `true` |
| `monitor` | 0 (padrão), 1, 2... | 0 |
| `mostrar_fps` | `true`, `false` | `false` |
| `volume` | 0 a 100 | 100 |
| `alvos` | `auto`, `rov`, `rtv`: como a EDRAM é imitada; `auto` usa o caminho medido melhor para o jogo | `auto` |
| `vibration` | `true`, `false` | `true` |
| `zona_morta_esquerda`, `zona_morta_direita` | 0 a 50 (%) | 0 |
| `user_language` | 1 inglês, 3 alemão, 4 francês, 5 espanhol, 6 italiano | 1 |
| `iso` | caminho da `.iso` | — |

## Como funciona

- `config/funcoes.toml`: as funções que a análise estática não acha sozinha: chamadas só por ponteiro (nos dados ou com o endereço montado no código) ou fora do alcance da análise. `config/ganchos.toml`: os ganchos no código do jogo. `config/jogo.txt`: a versão do jogo que o gerador confere.
- `src/saintsrow_app.h`: a taxa de quadros. O jogo (de uma XDK antiga) apresenta um quadro a cada 2 vblanks e mede o jogo pelo relógio, não pelos quadros: o port acelera o vblank do console para o dobro do fps escolhido (60 fps = vblank a 120 Hz). Conferido: uma caminhada de 3 s chega ao mesmo ponto a 30 e a 60 fps, e a abertura segue no mesmo tempo.
- `src/port/audio_guard.h` (base comum): na abertura, o áudio do jogo mandava um quadro para um cliente que ele mesmo tinha acabado de desregistrar, e a ReXGlue caía; agora o quadro é descartado.
- `src/port/`: a base comum dos ports de Xbox 360 (a mesma em todos): opções de imagem, contador de fps, leitura da `.iso`, a trava que corrige uma corrida na entrada de controles da ReXGlue, `falha.log` com a pilha em caso de falha.

Para desenvolver: `build.cmd` compila; `scripts/rodar.py` roda numa janela com prints, o fps de cada segundo e um controle roteirizado (`PORT_PAD`; para começar um jogo novo: `--pad 18:A,22:A,26:A,32:Y,36:A,40-240@4:A`); `scripts/tecla.py` aperta botões no jogo em execução; `scripts/varrer_ponteiros.py` e `scripts/desmontar.py` ajudam a achar funções.

## Créditos

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk): recompilador estático e runtime de Xbox 360 (BSD-3).
- [Xenia](https://github.com/xenia-project/xenia): o emulador em que o runtime se baseia (BSD-3).

Saints Row é marca da Deep Silver. Este projeto não tem relação com ela nem com a Microsoft.

## Licença

O código deste repositório está sob a licença MIT (`LICENSE`). O SDK e o runtime têm a licença própria deles (BSD-3).

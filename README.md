<h1 align="center">Saints Row — port nativo para PC</h1>

<p align="center"><i>Saints Row (Xbox 360) — port nativo para PC por recompilação estática (Otavix Engine 360). Gera a partir da sua .iso.</i></p>

<p align="center">
  <a href="https://github.com/otavix010/saints-row-pc/commits/main"><img alt="Última atualização" src="https://img.shields.io/github/last-commit/otavix010/saints-row-pc?label=%C3%BAltima%20atualiza%C3%A7%C3%A3o"></a>
  <a href="LICENSE"><img alt="Licença" src="https://img.shields.io/github/license/otavix010/saints-row-pc?label=licen%C3%A7a"></a>
  <a href="https://github.com/otavix010/otavix-launcher/releases/latest"><img alt="Baixar o Otavix Launcher" src="https://img.shields.io/badge/Otavix%20Launcher-baixar-b00020"></a>
  <img alt="Windows 10/11" src="https://img.shields.io/badge/Windows-10%2F11-0078d4">
  <img alt="Linux: não suportado no momento" src="https://img.shields.io/badge/Linux-n%C3%A3o%20suportado-lightgrey">
</p>

<p align="center">
  <a href="#o-que-o-port-acrescenta">O que o port acrescenta</a> ·
  <a href="#estado-dos-testes">Estado dos testes</a> ·
  <a href="#plataformas">Plataformas</a> ·
  <a href="#requisitos">Requisitos</a> ·
  <a href="#desempenho-medido">Desempenho</a> ·
  <a href="#perguntas-frequentes">Perguntas frequentes</a> ·
  <a href="#gerar-à-mão-sem-o-launcher">Gerar à mão (sem o launcher)</a> ·
  <a href="#opções">Opções</a> ·
  <a href="#como-funciona">Como funciona</a>
</p>

> [!IMPORTANT]
> **Para jogar, baixe o [Otavix Launcher](https://github.com/otavix010/otavix-launcher/releases/latest).** Ele baixa este port, pede a sua `.iso`, gera o jogo no seu PC e o mantém atualizado. Para relatar um problema ou pedir algo, use **"Reportar problema"** na página do jogo no launcher: o relatório já vai com o log e as configurações, e é por ele que as correções saem mais rápido.
>
> **To play, download the [Otavix Launcher](https://github.com/otavix010/otavix-launcher/releases/latest).** It downloads this port, asks for your `.iso`, makes the game on your PC and keeps it updated. To report a problem or ask for something, use **"Report a problem"** on the game's page in the launcher: the report carries the log and the settings, and that is how fixes come out fastest.

Port do **Saints Row** (Xbox 360) por **recompilação estática**: o código PowerPC do jogo é traduzido para C++ e compilado como um programa de Windows. Não há emulador de CPU rodando o jogo. A parte gráfica usa o runtime da **Otavix Engine 360**, derivado do ReXGlue e do [Xenia](https://xenia.jp) (veja os Créditos).

**Nada do jogo vem neste repositório.** O port é gerado no seu PC a partir da sua própria cópia (a imagem `.iso` do disco). O código traduzido também é gerado aí e nunca é distribuído.

O runtime é a **Otavix Engine 360 1.0.0** (privada): o gerador baixa o pacote pronto do servidor do launcher e confere o SHA-256. Ela descende do ReXGlue e do Xenia (BSD-3, veja os Créditos) e traz o AMD FidelityFX e as correções do port; os avisos de licença vão no pacote.

O jeito mais fácil de jogar é pelo [Otavix Launcher](https://github.com/otavix010/otavix-launcher), que baixa este port, pede a sua `.iso`, gera tudo e cuida das opções.

![Stilwater](docs/jogo.jpg)

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

| Tela de carregamento | Os Saints |
|---|---|
| ![Tela de carregamento](docs/cena1.jpg) | ![Os Saints](docs/cena2.jpg) |

## Estado dos testes

- Jogo livre logo depois da criação do personagem, na engine 1.0.1: 4 minutos andando e correndo pelas ruas, a 60 fps em 2x (média 61,1; mínimo 59; GPU em 31%). A velocidade do personagem a pé é a mesma a 30 e a 60.
- 120 fps não entra: o jogo seguiria a velocidade certa (a caminhada percorre a mesma distância a 60 e a 120), mas não passa de uns 70 fps, porque a thread do runtime que traduz os comandos de GPU do Xbox 360 fica a 98% de um núcleo com a quantidade de desenhos da cidade.
- Som funcionando, com o volume do port testado (25% dá 1/16 da potência, como previsto).

## Plataformas

| | **Windows** | **Linux / SteamOS** |
|---|---|---|
| Situação | **funciona** (este port) | **não suportado e não testado** |
| Vídeo | Direct3D 12 (o Agility SDK vai ao lado do exe) | o runtime só tem o caminho D3D12 no Windows |
| Como jogar | pelo launcher, ou `gerar.ps1` e `jogar.cmd` | só o port de *God of War: Ascension* tem versão Linux (beta) por enquanto |

## Requisitos

- **Windows 10/11 64 bits** e GPU com **Direct3D 12**.
- **CPU x86-64 com SSE4.1.** AVX não é obrigatório: o gerador compila no seu PC com `-march=native` e usa o que o processador tiver.
- **Visual Studio 2022 Build Tools** com "Desenvolvimento para desktop com C++" e "C++ CMake tools for Windows" (o launcher confere).
- **A sua `.iso`** do *Saints Row* (Saints Row).
- **Memória:** o mínimo exato não foi medido. Um cuidado medido: a compilação completa com 16 jobs chegou a ~35 GB de memória comprometida; o gerador limita os jobs (`-Jobs 4` é seguro).
- **Disco:** não medido com precisão: a `.iso`, mais as ferramentas (a engine, ~14 MB, e o Clang portátil) e a pasta de compilação.

## Desempenho medido

FPS de jogo. **Só colocamos o que foi medido**, e com **um jogo só rodando** (com outra carga junto, o desempenho pode cair bastante). Os números de 2x são da engine 1.0.1 (a saída da GPU é **idêntica quadro a quadro** à da 1.0.0 nos 4 ports); o que veio do runtime anterior está indicado como tal.

| Máquina | CPU | GPU | FPS |
|---|---|---|---|
| **PC 1** | Ryzen 7 9800X3D | RTX 5080 | 2x (1440p): **60** do início ao jogo livre (média 61,1 nas ruas, mínimo 59, GPU em 31%) na engine 1.0.1. 120 não entra: o próprio jogo limita a física a 1/60 e o runtime não passa de ~70 na cidade (medido no runtime anterior) |
| **PC 2** | Core i5-11400F | RX 9060 XT | não medido |
| **Notebook** | Core i5-10300H (4/8) | GTX 1650 | não medido |
| **Portátil** | Legion Go S (Linux) | Radeon 780M | não suportado |

DLSS e XeSS não existem para estes jogos (o Xbox 360 não fornece vetores de movimento); o FSR e o CAS atuam só na saída.

## Problemas conhecidos

- Só 30 e 60 FPS (sem 120, pelo motivo acima).

## Gerar à mão (sem o launcher)

Precisa do **Visual Studio 2022 Build Tools** com "Desenvolvimento para desktop com C++" e "C++ CMake tools for Windows". O gerador baixa o resto: a Otavix Engine 360 e o Clang (LLVM) portátil, numa pasta `_ferramentas` ao lado do port.

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

## Perguntas frequentes

**Este repositório tem o jogo?** Não. Você precisa da sua cópia (a imagem `.iso` do disco); o gerador monta o port no seu PC.

**Roda no Linux ou no SteamOS?** Ainda não: só o port de *God of War: Ascension* tem versão Linux (beta). Este roda no Windows.

**Saiu atualização. O que acontece com o jogo e os saves?** Atualize pelo launcher: ele refaz a tradução e a compilação uma vez (o gerador guarda a versão do runtime em `out\gerar\sdk.txt`). O jogo e os saves ficam.

**A `.iso` está danificada ou cortada.** O port agora recusa a imagem e diz o motivo no log, em vez de fechar.

**Onde relato um problema?** No launcher, em **"Reportar problema"** na página do jogo.

## Como funciona

- `config/funcoes.toml`: as funções que a análise estática não acha sozinha: chamadas só por ponteiro (nos dados ou com o endereço montado no código) ou fora do alcance da análise. `config/ganchos.toml`: os ganchos no código do jogo. `config/jogo.txt`: a versão do jogo que o gerador confere.
- `src/saintsrow_app.h`: a taxa de quadros. O jogo (de uma XDK antiga) apresenta um quadro a cada 2 vblanks e mede o jogo pelo relógio, não pelos quadros: o port acelera o vblank do console para o dobro do fps escolhido (60 fps = vblank a 120 Hz). Conferido: uma caminhada de 3 s chega ao mesmo ponto a 30 e a 60 fps, e a abertura segue no mesmo tempo.
- `src/port/audio_guard.h` (base comum): na abertura, o áudio do jogo mandava um quadro para um cliente que ele mesmo tinha acabado de desregistrar, e o runtime caía; agora o quadro é descartado.
- `src/port/`: a base comum dos ports de Xbox 360 (a mesma em todos): opções de imagem, contador de fps, leitura da `.iso`, a trava que corrige uma corrida na entrada de controles do runtime, `falha.log` com a pilha em caso de falha.

Para desenvolver: `build.cmd` compila; `scripts/rodar.py` roda numa janela com prints, o fps de cada segundo e um controle roteirizado (`PORT_PAD`; para começar um jogo novo: `--pad 18:A,22:A,26:A,32:Y,36:A,40-240@4:A`); `scripts/tecla.py` aperta botões no jogo em execução; `scripts/varrer_ponteiros.py` e `scripts/desmontar.py` ajudam a achar funções.

## Créditos

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk): recompilador estático e runtime de Xbox 360 (BSD-3).
- [Xenia](https://github.com/xenia-project/xenia): o emulador em que o runtime se baseia (BSD-3).

Saints Row é marca da Deep Silver. Este projeto não tem relação com ela nem com a Microsoft.

## Licença

O código deste repositório está sob a licença MIT (`LICENSE`). O SDK e o runtime têm a licença própria deles (BSD-3).

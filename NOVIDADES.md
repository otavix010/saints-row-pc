# Novidades do port

O Otavix Launcher mostra este arquivo na página "Novidades" do jogo. Cada atualização tem um título
"## <data> · <versão>" e o texto em português (### pt), inglês (### en) e, quando houver, espanhol (### es).
As mais novas ficam em cima.

## 29/10/2026 · 0cf67cc
### pt
- **Medidor de desempenho no F10:** o F10 agora mostra e esconde o medidor em qualquer configuração (antes só funcionava com "Mostrar fps" ou o medidor do launcher ligados). A linha traz fps, tempo de quadro (ms), uso e temperatura da CPU e da GPU, VRAM e RAM. Num processador sem sensor exposto pelo Windows (como os Ryzen de mesa), a temperatura da CPU aparece quando o MSI Afterburner está aberto.
- **Cursor do mouse:** some depois de um instante parado sobre o jogo e volta quando o mouse se mexe.
- **Download do gerador:** as ferramentas do port agora vêm do servidor do Otavix Launcher; o gerador não para mais no erro 404 ao preparar as ferramentas.
- **Disco não encontrado:** um jogo gerado fora do launcher podia abrir com erro de disco; agora o port usa a .iso com que foi gerado, e os saves continuam na pasta certa.
### en
- **Performance meter on F10:** F10 now shows and hides the meter with any settings (before, it only worked with "Show fps" or the launcher's meter on). The line has fps, frame time (ms), CPU and GPU use and temperature, VRAM and RAM. On a processor whose sensor Windows does not expose (such as desktop Ryzens), the CPU temperature shows while MSI Afterburner is open.
- **Mouse cursor:** it hides after a moment still over the game and comes back when the mouse moves.
- **Generator download:** the port's tools now come from the Otavix Launcher's server; the generator no longer stops with a 404 error while preparing the tools.
- **Disc not found:** a game generated outside the launcher could open with a disc error; the port now uses the .iso it was generated from, and the saves stay in the right folder.
### es
- **Medidor de rendimiento en F10:** F10 ahora muestra y oculta el medidor con cualquier configuración (antes solo funcionaba con "Mostrar fps" o el medidor del launcher activados). La línea trae fps, tiempo de fotograma (ms), uso y temperatura de la CPU y la GPU, VRAM y RAM. En un procesador cuyo sensor Windows no expone (como los Ryzen de escritorio), la temperatura de la CPU aparece con MSI Afterburner abierto.
- **Cursor del ratón:** se oculta tras un momento quieto sobre el juego y vuelve al mover el ratón.
- **Descarga del generador:** las herramientas del port ahora vienen del servidor de Otavix Launcher; el generador ya no se detiene con el error 404 al preparar las herramientas.
- **Disco no encontrado:** un juego generado fuera del launcher podía abrir con error de disco; ahora el port usa la .iso con la que se generó, y las partidas guardadas siguen en la carpeta correcta.

## 28/10/2026 · d6e8d7d
### pt
- **"Acesso negado" ao preparar as ferramentas:** em alguns PCs o gerador parava com "Move-Item: O acesso ao caminho ... llvm.tmp ... foi negado", porque o antivírus ainda estava examinando o compilador recém-baixado. Agora o gerador espera e tenta de novo e, se continuar preso, copia os arquivos. Basta gerar de novo; o download não é repetido.
### en
- **"Access denied" while preparing the tools:** on some PCs the generator stopped with "Move-Item: Access to the path ... llvm.tmp ... is denied", because the antivirus was still scanning the freshly downloaded compiler. The generator now waits and tries again and, if it is still locked, copies the files. Just generate again; the download is not repeated.
### es
- **"Acceso denegado" al preparar las herramientas:** en algunos PC el generador se detenía con "Move-Item: Acceso denegado a la ruta ... llvm.tmp ...", porque el antivirus aún estaba revisando el compilador recién descargado. Ahora el generador espera y vuelve a intentarlo y, si sigue bloqueado, copia los archivos. Basta con generar de nuevo; la descarga no se repite.

## 27/10/2026 · 23bb933
### pt
- **Pasta no lugar da .iso:** escolher a pasta onde a .iso está (mesmo numa subpasta) agora funciona; o gerador acha a .iso sozinho. Se a pasta não tiver uma .iso (por exemplo, o jogo já extraído ou no formato Games on Demand), aparece uma mensagem explicando o que escolher, em vez de "acesso negado".
### en
- **A folder instead of the .iso:** picking the folder the .iso is in (even in a subfolder) now works; the generator finds the .iso by itself. If the folder has no .iso (an unpacked game or a Games on Demand folder, for example), a message says what to pick, instead of "access denied".
### es
- **Carpeta en lugar de la .iso:** elegir la carpeta donde está la .iso (incluso en una subcarpeta) ahora funciona; el generador encuentra la .iso solo. Si la carpeta no tiene una .iso (el juego ya extraído o en formato Games on Demand, por ejemplo), aparece un mensaje que explica qué elegir, en lugar de "acceso denegado".

## 27/10/2026 · 6836420
### pt
- **Câmera lenta quando o fps cai:** abaixo de 40 fps o jogo inteiro ficava lento (andar, carros, tiros), porque a física dava no máximo 3 passos curtos por quadro. Agora são 6, como no Xbox 360, e o jogo mantém a velocidade real até 20 fps. A 60 fps nada muda.
### en
- **Slow motion when the fps drops:** below 40 fps the whole game slowed down (walking, cars, gunfire), because the physics took at most 3 short steps a frame. It now takes 6, as on the Xbox 360, and the game keeps its real speed down to 20 fps. At 60 fps nothing changes.

## 26/10/2026 · eb398d3
### pt
- **Tela preta na abertura:** às vezes (cerca de 1 em 10 aberturas) o jogo parava na tela preta logo ao abrir e precisava ser fechado. Durante a abertura, o jogo desliga e religa o seu motor de som, e o aviso de desligar podia chegar no meio de um trabalho de áudio e se perder. Agora o desligamento espera esse trabalho terminar. Em 48 aberturas de teste, nenhuma travou.
### en
- **Black screen on start:** sometimes (about 1 in 10 starts) the game stopped on a black screen right after opening and had to be closed. While starting, the game shuts down and restarts its sound engine, and the shutdown notice could arrive in the middle of an audio job and get lost. The shutdown now waits for that job to finish. In 48 test starts, none froze.

## 26/10/2026 · 9335785
### pt
- **Estabilidade:** corrigido um defeito de tradução no tratamento de erro do jogo (o mesmo que fechava o Dante's Inferno ao salvar).
- **Relatório de erro mais completo:** quando o jogo fecha por erro, o arquivo de falha que vai junto no report registra também os fechamentos do tipo 0xC0000409, com o ponto exato do problema.
### en
- **Stability:** fixed a translation defect in the game's error handling (the same one that closed Dante's Inferno when saving).
- **More complete error report:** when the game closes with an error, the crash file sent with the report now also records 0xC0000409 closes, with the exact point of the problem.

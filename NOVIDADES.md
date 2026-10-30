# Novidades do port

O Otavix Launcher mostra este arquivo na página "Novidades" do jogo. Cada atualização tem um título
"## <data> · <versão>" e o texto em português (### pt), inglês (### en) e, quando houver, espanhol (### es).
As mais novas ficam em cima.

## 29/10/2026 · 6284472
### pt
- **Jogo extraído com atualização (title update):** se a pasta do jogo tinha o `default.xexp` da atualização, o port aplicava a atualização por cima do jogo do disco e fechava na hora (erro 0xC0000374). O port é o código do disco; agora a atualização é ignorada.
### en
- **Unpacked game with a title update:** if the game folder had the update's `default.xexp`, the port applied the update over the disc's game and closed right away (error 0xC0000374). The port is the disc's code; the update is now ignored.
### es
- **Juego extraído con actualización (title update):** si la carpeta del juego tenía el `default.xexp` de la actualización, el port la aplicaba sobre el juego del disco y se cerraba al instante (error 0xC0000374). El port es el código del disco; ahora la actualización se ignora.

## 29/10/2026 · 9f78d87
### pt
- **Desempenho em processadores com menos núcleos:** cada operação atômica do jogo passava por uma única trava do runtime, a mesma que a placa de vídeo usa a cada desenho. Num processador com menos núcleos que threads ocupadas (como um Ryzen 5 5600X), a thread que segurava essa trava era pausada pelo Windows e todas as outras esperavam. Agora cada operação trava só o endereço que altera: menos quedas de fps e menos travadas.
- **Log mais leve:** um aviso repetido centenas de vezes por segundo (Saints Row: "invalid type" de textura) era gravado no disco a cada vez; agora uma linha igual aparece 20 vezes e depois só uma a cada 10000.
### en
- **Performance on processors with fewer cores:** every atomic operation of the game went through a single lock of the runtime, the same one the graphics card's thread takes on every draw. On a processor with fewer cores than busy threads (such as a Ryzen 5 5600X), the thread holding that lock was paused by Windows and all the others waited. Each operation now locks only the address it changes: fewer frame rate drops and stutters.
- **Lighter log:** a warning repeated hundreds of times a second (Saints Row: a texture's "invalid type") was written to the disk each time; the same line now shows 20 times and then once in 10000.
### es
- **Rendimiento en procesadores con menos núcleos:** cada operación atómica del juego pasaba por un único bloqueo del runtime, el mismo que usa la tarjeta gráfica en cada dibujo. En un procesador con menos núcleos que hilos ocupados (como un Ryzen 5 5600X), el hilo que tenía ese bloqueo era pausado por Windows y todos los demás esperaban. Ahora cada operación bloquea solo la dirección que cambia: menos caídas de fps y menos tirones.
- **Log más ligero:** un aviso repetido cientos de veces por segundo (Saints Row: "invalid type" de textura) se escribía en el disco cada vez; ahora una línea igual aparece 20 veces y luego solo una de cada 10000.

## 29/10/2026 · 12295d0
### pt
- **Jogo extraído do disco:** além da .iso, agora serve a pasta com os arquivos do disco já extraídos (a que tem o default.xex). Pode escolher essa pasta, uma subpasta dela (como a $SystemUpdate) ou a pasta de cima: o gerador acha o jogo, confere a versão e o port lê os arquivos direto dali.
### en
- **Game unpacked from its disc:** besides the .iso, the folder with the disc's files already unpacked (the one with default.xex) now works. Pick that folder, one inside it (such as $SystemUpdate) or the one above: the generator finds the game, checks its version, and the port reads the files right there.
### es
- **Juego extraído del disco:** además de la .iso, ahora sirve la carpeta con los archivos del disco ya extraídos (la que tiene el default.xex). Puedes elegir esa carpeta, una subcarpeta (como $SystemUpdate) o la carpeta de arriba: el generador encuentra el juego, comprueba la versión y el port lee los archivos directamente de ahí.

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

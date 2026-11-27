# Novidades do port

O Otavix Launcher mostra este arquivo na página "Novidades" do jogo. Cada atualização tem um título
"## <data> · <versão>" e o texto em português (### pt), inglês (### en) e, quando houver, espanhol (### es).
As mais novas ficam em cima.

## 27/11/2026 · gerador
### pt
- **Gerador:** quando a geração para, o launcher mostra a mensagem inteira, sem as linhas técnicas do PowerShell que cortavam o começo dela.
- **Gerador:** se a .iso escolhida for de outro console (PS3, PS2, PSP ou Xbox original), diz qual é e pede a .iso do Xbox 360.
### en
- **Generator:** when the build stops, the launcher shows the whole message, without the PowerShell technical lines that cut off its beginning.
- **Generator:** if the chosen .iso is from another console (PS3, PS2, PSP or the original Xbox), it says which one and asks for the Xbox 360 .iso.
### es
- **Generador:** cuando la generación se detiene, el launcher muestra el mensaje completo, sin las líneas técnicas de PowerShell que cortaban su comienzo.
- **Generador:** si el .iso elegido es de otra consola (PS3, PS2, PSP o Xbox original), dice cuál es y pide el .iso de Xbox 360.

## 26/11/2026 · gerador
### pt
- **Gerador:** se o Visual Studio foi instalado sem o SDK do Windows, avisa logo no começo e diz o que marcar no Visual Studio Installer, em vez de parar no meio da compilação.
### en
- **Generator:** if Visual Studio was installed without the Windows SDK, it says so right at the start and tells what to tick in the Visual Studio Installer, instead of stopping halfway through the build.
### es
- **Generador:** si Visual Studio se instaló sin el SDK de Windows, avisa al principio y dice qué marcar en el Visual Studio Installer, en lugar de detenerse a mitad de la compilación.

## 25/11/2026 · gerador
### pt
- **Gerador:** a compilação agora funciona também com versões mais antigas do Visual Studio 2022 Build Tools (que traziam o CMake 3.26 e paravam com "Invalid character escape").
### en
- **Generator:** the build now also works with older versions of the Visual Studio 2022 Build Tools (which brought CMake 3.26 and stopped with "Invalid character escape").
### es
- **Generador:** la compilación ahora también funciona con versiones más antiguas de Visual Studio 2022 Build Tools (que traían CMake 3.26 y se detenían con "Invalid character escape").

## 24/11/2026 · gerador
### pt
- **Gerador:** antes de baixar e compilar, confere se a placa de vídeo tem DirectX 12 e, se não tiver, avisa na hora.
- **Gerador:** confere a .iso inteira uma vez e avisa se ela parecer danificada ou incompleta.
- **Gerador:** se a .iso for do disco certo mas vier com a atualização do jogo embutida no executável ("com update"), a mensagem agora diz isso e pede a .iso do disco original.
### en
- **Generator:** before downloading and building, checks that the video card has DirectX 12 and, if it does not, says so right away.
- **Generator:** checks the whole .iso once and warns if it looks damaged or incomplete.
- **Generator:** if the .iso is the right disc but came with the game's update built into the executable ("with the update"), the message now says so and asks for the .iso of the original disc.
### es
- **Generador:** antes de descargar y compilar, comprueba que la tarjeta de video tenga DirectX 12 y, si no la tiene, avisa enseguida.
- **Generador:** comprueba la .iso completa una vez y avisa si parece dañada o incompleta.
- **Generador:** si el .iso es del disco correcto pero vino con la actualización del juego incluida en el ejecutable ("con update"), el mensaje ahora lo dice y pide el .iso del disco original.

## 22/11/2026 · gerador
### pt
- **Gerador:** ao escolher a pasta da versão de PlayStation 3 do jogo, a mensagem agora diz isso direto e explica que o port precisa do disco do Xbox 360.
### en
- **Generator:** when the folder of the PlayStation 3 edition of the game is picked, the message now says so plainly and explains that the port needs the Xbox 360 disc.
### es
- **Generador:** al elegir la carpeta de la versión de PlayStation 3 del juego, el mensaje ahora lo dice directamente y explica que el port necesita el disco de Xbox 360.

## 20/11/2026 · gerador
### pt
- **Gerador:** a compilação não falha mais com "\NVIDIA was unexpected at this time" em PCs com uma entrada entre aspas no PATH do Windows (deixada por alguns instaladores).
### en
- **Generator:** the build no longer fails with "\NVIDIA was unexpected at this time" on PCs with a quoted entry in the Windows PATH (left by some installers).
### es
- **Generador:** la compilación ya no falla con "\NVIDIA was unexpected at this time" en PCs con una entrada entre comillas en el PATH de Windows (dejada por algunos instaladores).

## 19/11/2026 · gerador
### pt
- **Gerador:** a compilação não para mais com "build.ninja still dirty" quando os arquivos da engine chegam com data no futuro (acontecia por algumas horas depois de cada atualização, em PCs com fuso horário a oeste do Brasil).
### en
- **Generator:** the build no longer stops with "build.ninja still dirty" when the engine's files arrive dated in the future (it happened for a few hours after each update, on PCs in time zones west of Brazil).
### es
- **Generador:** la compilación ya no se detiene con "build.ninja still dirty" cuando los archivos de la engine llegan con fecha en el futuro (pasaba durante algunas horas después de cada actualización, en PCs con huso horario al oeste de Brasil).

## 18/11/2026 · teclas
### pt
- **Teclas padrão:** as teclas do teclado e do mouse passam a seguir as ações do jogo a pé: ataque principal no botão esquerdo do mouse, secundário no direito, recarregar no R, pular no Espaço, ação e entrar no carro no E, correr no Shift esquerdo, chute no F, inventário no Q e agachar no Ctrl esquerdo. Quem já trocou teclas pelo launcher fica com as suas.
### en
- **Default keys:** the keyboard and mouse keys now follow the game's on-foot actions: primary attack on the left mouse button, secondary on the right one, reload on R, jump on Space, action and enter car on E, sprint on left Shift, kick on F, inventory on Q and crouch on left Ctrl. Keys you changed in the launcher stay yours.
### es
- **Teclas por defecto:** las teclas del teclado y del ratón ahora siguen las acciones del juego a pie: ataque principal en el botón izquierdo del ratón, secundario en el derecho, recargar en R, saltar en Espacio, acción y entrar al coche en E, correr en Mayús izquierda, patada en F, inventario en Q y agacharse en Ctrl izquierdo. Las teclas que cambiaste en el launcher siguen siendo las tuyas.

## 18/11/2026 · engine 1.0.8
### pt
- **Teclado e mouse:** o jogo agora aceita teclado e mouse junto com o controle (ou sem ele), com o movimento do mouse no analógico direito e os botões dele no mapa de teclas. As teclas padrão estão na seção Teclado e mouse do README; para trocá-las, desligar o teclado ou o mouse e ajustar a sensibilidade, use Opções avançadas → Controles na versão mais nova do launcher.
### en
- **Keyboard and mouse:** the game now takes keyboard and mouse together with the controller (or without one), with the mouse's movement on the right stick and its buttons in the key map. The default keys are in the README's Keyboard and mouse section; to change them, turn the keyboard or the mouse off and set the sensitivity, use Advanced options → Controls in the newest version of the launcher.
### es
- **Teclado y ratón:** el juego ahora acepta teclado y ratón junto con el mando (o sin él), con el movimiento del ratón en el stick derecho y sus botones en el mapa de teclas. Las teclas por defecto están en la sección Teclado e mouse del README; para cambiarlas, desactivar el teclado o el ratón y ajustar la sensibilidad, usa Opciones avanzadas → Controles en la versión más nueva del launcher.

## 18/11/2026 · engine 1.0.7
### pt
- **Engasgos de shader:** a primeira vez que um efeito aparece não segura mais o quadro: os shaders novos são compilados em segundo plano enquanto o jogo segue (report 1347). O objeto ou efeito novo pode levar alguns quadros para aparecer nessa primeira vez; nas seguintes ele já está guardado.
### en
- **Shader stutter:** the first time an effect shows up no longer holds the frame: new shaders are compiled in the background while the game goes on (report 1347). The new object or effect may take a few frames to appear that first time; after that it is already stored.
### es
- **Tirones de shaders:** la primera vez que aparece un efecto ya no detiene el cuadro: los shaders nuevos se compilan en segundo plano mientras el juego sigue (reporte 1347). El objeto o efecto nuevo puede tardar algunos cuadros en aparecer esa primera vez; en las siguientes ya está guardado.

## 18/11/2026 · gerador
### pt
- **Gerador:** o download das ferramentas continua de onde parou quando a conexão cai, em vez de falhar no meio.
- **Gerador:** o Visual Studio 2019 (e os anteriores) não serve para compilar o port e era aceito por engano, o que terminava em erro de compilação; agora o gerador pede o Visual Studio 2022 Build Tools e mostra o comando para instalar.
### en
- **Generator:** the tools download resumes where it stopped when the connection drops, instead of failing midway.
- **Generator:** Visual Studio 2019 (and older) cannot build the port and was accepted by mistake, which ended in a build error; the generator now asks for Visual Studio 2022 Build Tools and shows the command to install it.
### es
- **Generador:** la descarga de las herramientas continúa donde se quedó cuando la conexión se cae, en lugar de fallar a la mitad.
- **Generador:** Visual Studio 2019 (y los anteriores) no sirve para compilar el port y se aceptaba por error, lo que terminaba en un error de compilación; ahora el generador pide Visual Studio 2022 Build Tools y muestra el comando para instalarlo.

## 17/11/2026 · engine 1.0.6
### pt
- **Ícone:** o executável do jogo passa a ter o ícone do próprio jogo, tirado da sua cópia: ele aparece no Explorer e nos atalhos, e não só na janela.
### en
- **Icon:** the game's executable now carries the game's own icon, taken from your copy: it shows in Explorer and in shortcuts, not only on the window.
### es
- **Icono:** el ejecutable del juego ahora lleva el icono del propio juego, sacado de tu copia: aparece en el Explorador y en los accesos directos, y no solo en la ventana.

## 16/11/2026 · engine 1.0.5
### pt
- **Abertura:** a janela do jogo continua respondendo enquanto os shaders são preparados na abertura. Em computadores mais fracos essa etapa leva minutos, e o Windows chegava a oferecer fechar o jogo (erro 0xCFFFFFFF).
- **Gerador:** quando a .iso escolhida é de outro jogo dos nossos ports, a mensagem diz qual é o jogo e o que escolher; para outras cópias diferentes da esperada, o log descreve o arquivo encontrado.
### en
- **Startup:** the game window keeps responding while shaders are prepared at startup. On slower computers this step takes minutes, and Windows would offer to close the game (error 0xCFFFFFFF).
- **Generator:** when the .iso picked belongs to another game of our ports, the message names the game and says what to pick; for other copies that differ from the expected one, the log describes the file found.
### es
- **Inicio:** la ventana del juego sigue respondiendo mientras se preparan los shaders al inicio. En computadoras más lentas este paso tarda minutos, y Windows llegaba a ofrecer cerrar el juego (error 0xCFFFFFFF).
- **Generador:** cuando la .iso elegida es de otro juego de nuestros ports, el mensaje dice cuál es el juego y qué elegir; para otras copias distintas de la esperada, el registro describe el archivo encontrado.

## 14/11/2026 · engine 1.0.4
### pt
- **Teclado na tela:** onde o jogo pede um texto (como o nome do piloto), a janela de digitação aparece e aceita o teclado; Enter ou A confirmam, Esc ou B cancelam. Antes, o texto voltava sozinho e ilegível.
- **Imagem:** corrigido um piscar leve em objetos ao fundo (como prédios distantes) que veio com a engine 1.0.2.
- **Desempenho:** menos trabalho de GPU nas cópias de profundidade e stencil e nos desenhos que não mudam a imagem.
### en
- **On-screen keyboard:** where the game asks for text (such as the driver's name), the typing window appears and takes the keyboard; Enter or A confirm, Esc or B cancel. Before, the text came back on its own and unreadable.
- **Picture:** fixed a slight flicker on background objects (such as distant buildings) that came with engine 1.0.2.
- **Performance:** less GPU work in depth and stencil copies and in draws that do not change the picture.
### es
- **Teclado en pantalla:** donde el juego pide un texto (como el nombre del piloto), la ventana de escritura aparece y acepta el teclado; Enter o A confirman, Esc o B cancelan. Antes, el texto volvía solo e ilegible.
- **Imagen:** corregido un leve parpadeo en objetos del fondo (como edificios lejanos) que llegó con el motor 1.0.2.
- **Rendimiento:** menos trabajo de GPU en las copias de profundidad y stencil y en los dibujos que no cambian la imagen.

## 13/11/2026 · engine 1.0.3
### pt
- **Fechar o jogo:** fechar o jogo com ALT+F4 ou pelo X da janela não termina mais com o erro 0xC0000409. O defeito veio na engine 1.0.2.
### en
- **Closing the game:** closing the game with ALT+F4 or the window's X no longer ends with error 0xC0000409. The defect came with engine 1.0.2.
### es
- **Cerrar el juego:** cerrar el juego con ALT+F4 o con la X de la ventana ya no termina con el error 0xC0000409. El defecto llegó con el motor 1.0.2.

## 13/11/2026 · engine 1.0.2
### pt
- **Otavix Engine 360 1.0.2:** nova versão da engine, voltada ao desempenho da placa de vídeo. A imagem do jogo sai idêntica à da 1.0.1, quadro a quadro. O launcher refaz a tradução e a compilação na atualização; o jogo e os saves continuam onde estavam.
- **Envio à placa de vídeo em paralelo:** os comandos de cada quadro passam a ser enviados à placa de vídeo por uma thread própria, enquanto a engine já prepara o quadro seguinte.
- **Imagens que o jogo não lê:** a engine deixa de copiar para a memória do jogo as imagens renderizadas que ele não lê. Quando o jogo volta a ler uma delas, a cópia volta na hora.
- **Desenhos de tela cheia:** antes de um desenho que cobre a tela inteira, a engine deixa de copiar a parte da imagem que ele vai sobrescrever.
### en
- **Otavix Engine 360 1.0.2:** a new engine version, focused on graphics card performance. The game's image is identical to 1.0.1's, frame by frame. The launcher redoes the translation and the build on the update; the game and the saves stay where they were.
- **Parallel submission to the graphics card:** each frame's commands are now sent to the graphics card by a dedicated thread, while the engine already prepares the next frame.
- **Images the game doesn't read:** the engine no longer copies rendered images the game doesn't read into the game's memory. When the game reads one of them again, the copy comes back right away.
- **Full-screen draws:** before a draw that covers the whole screen, the engine no longer copies the part of the image that the draw will overwrite.
### es
- **Otavix Engine 360 1.0.2:** nueva versión del motor, centrada en el rendimiento de la tarjeta gráfica. La imagen del juego sale idéntica a la de la 1.0.1, cuadro a cuadro. El launcher rehace la traducción y la compilación al actualizar; el juego y las partidas guardadas siguen donde estaban.
- **Envío a la tarjeta gráfica en paralelo:** los comandos de cada cuadro ahora se envían a la tarjeta gráfica desde un hilo propio, mientras el motor ya prepara el cuadro siguiente.
- **Imágenes que el juego no lee:** el motor deja de copiar a la memoria del juego las imágenes renderizadas que este no lee. Cuando el juego vuelve a leer una de ellas, la copia vuelve al instante.
- **Dibujos de pantalla completa:** antes de un dibujo que cubre toda la pantalla, el motor deja de copiar la parte de la imagen que ese dibujo va a sobrescribir.

## 12/11/2026 · engine 1.0.1
### pt
- **Otavix Engine 360 1.0.1:** nova versão da engine, com mais uma parte do código de origem reescrita pela Otavix. A imagem do jogo sai idêntica à da 1.0.0, quadro a quadro. O launcher refaz a tradução e a compilação na atualização; o jogo e os saves continuam onde estavam.
- **Fila de tarefas do sistema:** ao colocar a primeira tarefa na fila vazia ou ao tirar a última, a engine escrevia 4 bytes num endereço fora da fila, na memória do jogo. Isso não acontece mais.
- **Nome da engine:** a janela do jogo e o log passam a mostrar "Otavix Engine 360 v1.0.1".
### en
- **Otavix Engine 360 1.0.1:** a new engine version, with another part of the original code rewritten by Otavix. The game's image is identical to 1.0.0's, frame by frame. The launcher redoes the translation and the build on the update; the game and the saves stay where they were.
- **System task queue:** when the first task went into the empty queue, or the last one left it, the engine wrote 4 bytes to an address outside the queue, in the game's memory. This no longer happens.
- **Engine name:** the game window and the log now show "Otavix Engine 360 v1.0.1".
### es
- **Otavix Engine 360 1.0.1:** nueva versión del motor, con otra parte del código de origen reescrita por Otavix. La imagen del juego sale idéntica a la de la 1.0.0, cuadro a cuadro. El launcher rehace la traducción y la compilación al actualizar; el juego y las partidas guardadas siguen donde estaban.
- **Cola de tareas del sistema:** al poner la primera tarea en la cola vacía o al quitar la última, el motor escribía 4 bytes en una dirección fuera de la cola, en la memoria del juego. Esto ya no ocurre.
- **Nombre del motor:** la ventana del juego y el log ahora muestran "Otavix Engine 360 v1.0.1".

## 11/11/2026 · engine 1.0.0
### pt
- **Otavix Engine 360 1.0.0:** o port agora roda sobre a engine própria da Otavix, que substitui o runtime de origem (ReXGlue/Xenia). O launcher refaz a tradução e a compilação na atualização; o jogo e os saves continuam onde estavam.
- **Imagens de disco danificadas:** uma .iso corrompida ou cortada no fim fazia o port fechar sem aviso ao ler as pastas do disco. Agora ela é recusada com o motivo no log, e um arquivo cortado é lido só até onde existe.
- **Áudio ao fechar o jogo:** se o decodificador de áudio demorasse a parar, a memória dele era devolvida enquanto ainda estava em uso, o que podia fechar o port com erro ao sair.
### en
- **Otavix Engine 360 1.0.0:** the port now runs on Otavix's own engine, which replaces the runtime it started from (ReXGlue/Xenia). The launcher redoes the translation and the build on the update; the game and the saves stay where they were.
- **Damaged disc images:** a corrupted .iso, or one cut short at the end, made the port close without warning while reading the disc's folders. It is now refused with the reason in the log, and a cut-short file is read only as far as it exists.
- **Audio when closing the game:** if the audio decoder took long to stop, its memory was given back while still in use, which could make the port close with an error on exit.
### es
- **Otavix Engine 360 1.0.0:** el port ahora funciona sobre el motor propio de Otavix, que reemplaza el runtime de origen (ReXGlue/Xenia). El launcher rehace la traducción y la compilación al actualizar; el juego y las partidas guardadas siguen donde estaban.
- **Imágenes de disco dañadas:** una .iso corrupta o cortada al final hacía que el port se cerrara sin aviso al leer las carpetas del disco. Ahora se rechaza con el motivo en el log, y un archivo cortado se lee solo hasta donde existe.
- **Audio al cerrar el juego:** si el decodificador de audio tardaba en detenerse, su memoria se devolvía mientras aún estaba en uso, lo que podía cerrar el port con un error al salir.

## 30/10/2026 · 1745a6a
### pt
- **Relatório de erro da placa de vídeo:** quando o driver de vídeo derrubava o jogo ("Graphics device lost"), o port fechava sem registrar nada. Agora o arquivo de falha que vai junto no report traz o motivo informado pelo driver, o ponto em que a placa parou e a VRAM em uso.
### en
- **Graphics card error report:** when the video driver brought the game down ("Graphics device lost"), the port closed without recording anything. The crash file sent with the report now has the reason given by the driver, the point where the card stopped and the VRAM in use.
### es
- **Informe de error de la tarjeta gráfica:** cuando el controlador de vídeo tiraba el juego ("Graphics device lost"), el port se cerraba sin registrar nada. Ahora el archivo de fallo que va con el report trae el motivo indicado por el controlador, el punto en que la tarjeta se detuvo y la VRAM en uso.

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

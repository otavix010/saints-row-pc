# Novidades do port

O Otavix Launcher mostra este arquivo na página "Novidades" do jogo. Cada atualização tem um título
"## <data> · <versão>" e o texto em português (### pt), inglês (### en) e, quando houver, espanhol (### es).
As mais novas ficam em cima.

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

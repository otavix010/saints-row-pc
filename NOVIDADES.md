# Novidades do port

O Otavix Launcher mostra este arquivo na página "Novidades" do jogo. Cada atualização tem um título
"## <data> · <versão>" e o texto em português (### pt), inglês (### en) e, quando houver, espanhol (### es).
As mais novas ficam em cima.

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

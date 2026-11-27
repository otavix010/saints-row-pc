<#
.SYNOPSIS
  Gera e compila o port a partir da SUA cópia do jogo (a .iso do Xbox 360).

.DESCRIPTION
  Faz tudo em ordem, e cada etapa pula se já foi feita (use -RefazerDe N para refazer da etapa N em diante):
    1. ferramentas: o SDK ReXGlue e o LLVM (Clang) portátil, baixados para a pasta _ferramentas ao lado do port
       (compartilhada pelos ports de Xbox 360); confere o Visual Studio Build Tools (C++ e CMake);
    2. jogo: lê o default.xex de dentro da .iso e confere a versão (a .iso fica onde está; o jogo a lê direto);
    3. tradução: o código PowerPC do jogo vira C++ (generated\);
    4. compilação;
    5. pronto para jogar (jogar.cmd).
  Nos jogos com conteúdo extra (DLC), os pacotes deixados na pasta da .iso (soltos ou em .zip, .7z, .rar...) são
  achados a cada vez que isto roda; os compactados são extraídos para a pasta dlc do port.
  Os logs de cada etapa ficam em out\gerar\. O jogo e a versão esperada estão em config\jogo.txt.

.PARAMETER Iso
  A .iso do jogo (Xbox 360), ou a pasta com o jogo extraído do disco (default.xex).
.PARAMETER RefazerDe
  Refaz a partir desta etapa (por exemplo 3 depois de uma versão nova do port).
.PARAMETER Jobs
  Quantos núcleos usar na compilação (padrão: todos).

.EXAMPLE
  .\gerar.ps1 -Iso "D:\Jogos\jogo.iso"
#>
param(
    [string]$Iso = '',
    [int]$RefazerDe = 0,
    [int]$Jobs = 0
)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
# a failure ends the generator with its message alone: PowerShell's own error record (the "No ...\gerar.ps1:<line>"
# position, "+ throw ...", "+ CategoryInfo") filled the last lines the launcher shows and cut off the start of
# the message (reports 1357, 1400, 1453)
trap { Write-Host $_.Exception.Message -ForegroundColor Red; exit 1 }
$R = $PSScriptRoot
if (-not $Jobs) { $Jobs = [Environment]::ProcessorCount }
$OUT = Join-Path $R 'out\gerar'
New-Item -ItemType Directory -Force $OUT | Out-Null

# the project (from its manifest) and the game it was made from (config\jogo.txt: titulo, versao, sha1_xex)
$PROJETO = (Get-ChildItem $R -Filter '*_manifest.toml' | Select-Object -First 1).Name -replace '_manifest\.toml$', ''
$JOGO = @{}
foreach ($linha in Get-Content (Join-Path $R 'config\jogo.txt') -Encoding UTF8) {
    if ($linha -match '^\s*([a-z0-9_]+)\s*=\s*(.*?)\s*$') { $JOGO[$Matches[1]] = $Matches[2] }
}
# the tools, pinned
# the Otavix Engine 360 1.0.8 (the runtime, the translator and their headers and libraries, in the same layout as the
# ReXGlue SDK it started from); its binaries come from the launcher's server only
$SDK_VERSAO = 'otx-1.0.8'
$SDK_URLS = @(
    'https://otavix-launcher.otavix-launcher-servidor.workers.dev/v1/arquivos/releases/otavix010/otavix-engine-360/v1.0.8/otavix-engine-360-1.0.8-win-amd64.zip')
$SDK_SHA256 = '488d0f6bf5579e921b9cf83047db50dbbdc11302fa6d6581c547a0b9a8c47b41'
$LLVM_VERSAO = '23.1.3'
$LLVM_URL = "https://github.com/llvm/llvm-project/releases/download/llvmorg-$LLVM_VERSAO/clang%2Bllvm-$LLVM_VERSAO-x86_64-pc-windows-msvc.tar.zst"
$LLVM_SHA256 = '7e69dc07a4536b79ff24a651a2611990bf8eaab17ea560f1092707c4f31ba01d'
# the package's zstd frames need a long window, more than tar's own decoder takes: the official zstd unpacks it
$ZSTD_URL = 'https://github.com/facebook/zstd/releases/download/v1.5.7/zstd-v1.5.7-win64.zip'
$ZSTD_SHA256 = 'acb4e8111511749dc7a3ebedca9b04190e37a17afeb73f55d4425dbf0b90fad9'

$FERRAMENTAS = Join-Path (Split-Path $R) '_ferramentas'
$SDK = Join-Path $FERRAMENTAS "rexglue-sdk-$SDK_VERSAO"
$LLVM = Join-Path $FERRAMENTAS "llvm-$LLVM_VERSAO"

function Msg($t) { Write-Host "[gerar] $t" -ForegroundColor Cyan }

# a line into a step's log, the file shared: the launcher reads the log for its progress, and PowerShell 5.1's
# Add-Content and Set-Content fail ("being used by another process") while any other process has it open; -Novo starts
# the file over
function Anotar([string]$arq, [string]$texto, [switch]$Novo) {
    $b = [Text.Encoding]::UTF8.GetBytes("$texto`r`n")
    for ($i = 0; ; $i++) {
        try {
            $fs = New-Object IO.FileStream($arq, $(if ($Novo) { 'Create' } else { 'Append' }), 'Write', 'ReadWrite')
            try { $fs.Write($b, 0, $b.Length) } finally { $fs.Dispose() }
            return
        } catch [IO.IOException] { if ($i -ge 50) { throw }; Start-Sleep -Milliseconds 100 }
    }
}

# runs a native command through cmd (stdout and stderr into the step's log); a failure throws with the log's end
function Exec([string]$linha) {
    Anotar $script:LOG "`r`n> $linha"
    cmd /d /c "$linha >> `"$script:LOG`" 2>&1"
    if ($LASTEXITCODE -ne 0) {
        Write-Host (Get-Content $script:LOG -Tail 25 | Out-String) -ForegroundColor DarkGray
        throw "falhou (código $LASTEXITCODE): $linha`r`nlog completo: $script:LOG"
    }
}
function Etapa([int]$n, [string]$nome, [scriptblock]$corpo) {
    $marca = Join-Path $OUT "$nome.ok"
    if ((Test-Path $marca) -and ($RefazerDe -eq 0 -or $n -lt $RefazerDe)) { Msg "${nome}: já feita (pulando)"; return }
    Remove-Item $marca -ErrorAction SilentlyContinue
    $script:LOG = Join-Path $OUT "$nome.log"
    Anotar $script:LOG "== $nome $(Get-Date)" -Novo
    Msg "$nome ..."
    $t0 = Get-Date
    & $corpo
    Set-Content -Path $marca -Value (Get-Date) -Encoding UTF8
    Msg ("{0}: ok ({1:N0} s)" -f $nome, ((Get-Date) - $t0).TotalSeconds)
}
function Baixar([string]$url, [string]$destino) {
    Msg "baixando $(Split-Path $destino -Leaf) ..."
    $tmp = "$destino.parcial"
    $extra = ''
    for ($vez = 1; ; $vez++) {
        $havia = Test-Path -LiteralPath $tmp
        try { Exec "curl.exe -L --fail --retry 3 -C - $extra-o `"$tmp`" `"$url`""; break }
        catch {
            # curl 35 (CRYPT_E_NO_REVOCATION_CHECK): a rede ou o antivírus do jogador não deixa o schannel consultar a
            # revogação do certificado (report 47 do launcher); de novo sem essa consulta, o certificado ainda conferido
            if ("$_" -match 'código 35\b' -and -not $extra) { $extra = '--ssl-no-revoke '; continue }
            # a piece left from before that the server will not continue: from the start
            if ($havia -and "$_" -match 'código (22|33|36)\b') { Remove-Item -Force -LiteralPath $tmp -ErrorAction SilentlyContinue; continue }
            # the connection dropped midway (report 1346): the same file again, from where it stopped
            if ($vez -ge 5 -or "$_" -notmatch 'código (6|7|18|28|52|55|56|92)\b') { throw }
            Msg "a conexão caiu; continuando o download (tentativa $($vez + 1) de 5) ..."
            Start-Sleep -Seconds 3
        }
    }
    Move-Item -Force $tmp $destino
}
# the first of the addresses that gives the file with this SHA-256; one that fails or gives another file is skipped
function Baixar-De([string[]]$urls, [string]$destino, [string]$sha256) {
    foreach ($u in $urls) {
        try {
            Baixar $u $destino
            if ((Get-FileHash $destino -Algorithm SHA256).Hash -eq $sha256) { return }
            Anotar $script:LOG "SHA-256 diferente: $u"
        }
        catch { Anotar $script:LOG "falhou: $u ($_)" }
        Msg 'tentando o próximo endereço ...'
    }
    throw "não deu para baixar $(Split-Path $destino -Leaf) de nenhum endereço (veja a internet ou o antivírus)"
}
# a folder just unpacked into its place. Report 167: "Move-Item: acesso negado" on the LLVM, an antivirus still
# scanning the new .exe files. Try again for a while, then copy and leave the source for a later clean-up.
function Mover-Pasta([string]$origem, [string]$destino) {
    for ($i = 1; $i -le 10; $i++) {
        try { Move-Item -LiteralPath $origem $destino -ErrorAction Stop; return }
        catch {
            Anotar $script:LOG "Move-Item ($i/10): $_"
            if (Test-Path -LiteralPath $destino) { Remove-Item -Recurse -Force -LiteralPath $destino -ErrorAction SilentlyContinue }
            Start-Sleep -Seconds 3
        }
    }
    Msg 'a pasta continua presa (antivírus?); copiando no lugar de mover ...'
    Copy-Item -Recurse -LiteralPath $origem $destino -ErrorAction Stop
}
# a temporary file or folder; one an antivirus still holds stays behind and is removed on the next run
function Apagar([string[]]$caminhos) {
    foreach ($c in $caminhos) { Remove-Item -Recurse -Force -LiteralPath $c -ErrorAction SilentlyContinue }
}

# the Visual Studio with the C++ tools (MSVC's headers and libraries, the Windows SDK, CMake and Ninja): the 2022
# (17.1) or a newer one, whose C++ library has what the engine's headers use (report 1343: the 2019's has not)
function Achar-VS {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { return $null }
    $p = & $vswhere -latest -version '[17.1,)' -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.CMake.Project -property installationPath
    if ($p) { return ($p | Select-Object -First 1) }
    return $null
}
# the Windows SDK (kernel32.lib and the rest of Windows' libraries and headers): a Visual Studio installed without it
# gets as far as CMake's first test of the compiler, "could not open 'kernel32.lib'" (report 1503). Looked for where
# VsDevCmd.bat looks: the Windows Kits root in the registry, a version with both the x64 libraries and the headers.
function Tem-SdkWindows {
    $raizes = @()
    foreach ($k in 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots', 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots') {
        $v = (Get-ItemProperty -Path $k -Name KitsRoot10 -ErrorAction SilentlyContinue).KitsRoot10
        if ($v) { $raizes += $v }
    }
    $raizes += "${env:ProgramFiles(x86)}\Windows Kits\10"
    foreach ($raiz in ($raizes | Select-Object -Unique)) {
        $lib = Join-Path $raiz 'Lib'
        if (-not (Test-Path -LiteralPath $lib)) { continue }
        foreach ($v in (Get-ChildItem -LiteralPath $lib -Directory -Filter '10.*' -ErrorAction SilentlyContinue)) {
            if ((Test-Path -LiteralPath (Join-Path $v.FullName 'um\x64\kernel32.lib')) -and
                (Test-Path -LiteralPath (Join-Path $raiz "Include\$($v.Name)\um\windows.h"))) { return $true }
        }
    }
    return $false
}
# "Windows SDK" in it is what the launcher looks for to explain it in English and Spanish
$ERRO_SDK_WINDOWS = "falta o SDK do Windows (Windows SDK) no Visual Studio: sem ele o compilador não acha as bibliotecas do Windows (kernel32.lib). Abra o Visual Studio Installer, clique em Modificar no Build Tools, marque `"Desenvolvimento para desktop com C++`" com o `"SDK do Windows 11`", instale e gere de novo."

# ---------------------------------------------------------------------------- Xbox 360 disc images (XDVDFS)
# The volume descriptor sits 0x10000 into the game partition, which starts at one of these offsets
# (a trimmed xiso, XGD1, XGD2, XGD3).
# The game partition of an image in none of the usual places: the descriptor looked for at every sector of the
# first 512 MB, read in 8 MB blocks (a second or two). Its base, or $null.
function Achar-Xdvdfs([IO.FileStream]$f) {
    $marca = 'MICROSOFT*XBOX*MEDIA'
    $latin = [Text.Encoding]::GetEncoding(28591)
    $bloco = 8MB
    $buf = New-Object byte[] ($bloco + 20)
    $fim = [Math]::Min($f.Length, 0x20000000 + 0x10000)
    for ($p = [int64]0x10000; $p -lt $fim; $p += $bloco) {
        $f.Position = $p
        $n = $f.Read($buf, 0, $buf.Length)
        if ($n -lt 20) { break }
        $txt = $latin.GetString($buf, 0, $n)
        $i = $txt.IndexOf($marca, [StringComparison]::Ordinal)
        while ($i -ge 0) {
            $pos = $p + $i
            if ($pos % 2048 -eq 0) { return $pos - 0x10000 }
            $i = $txt.IndexOf($marca, $i + 1, [StringComparison]::Ordinal)
        }
    }
    return $null
}
# A disc of another console, in the player's words, or $null (report 1340: the PS3 Asura's Wrath, BLUS30721, picked
# for this port). A PS3 disc has "PlayStation3" and the game's code at its second sector and the volume PS3VOLUME;
# PS2 and PSP discs are ISO 9660 with PLAYSTATION or PSP GAME as the system.
function Outro-Console([IO.FileStream]$f) {
    $latin = [Text.Encoding]::GetEncoding(28591)
    $setor = New-Object byte[] 2048
    $s1 = ''; $pvd = ''
    if ($f.Length -ge 0x8800) {
        $f.Position = 0x800; [void]$f.Read($setor, 0, 2048); $s1 = $latin.GetString($setor)
        $f.Position = 0x8000; [void]$f.Read($setor, 0, 2048); $pvd = $latin.GetString($setor)
    }
    $iso9660 = $pvd.Length -eq 2048 -and $pvd.Substring(1, 5) -eq 'CD001'
    $sistema = if ($iso9660) { $pvd.Substring(8, 32).Trim() } else { '' }
    $volume = if ($iso9660) { $pvd.Substring(40, 32).Trim() } else { '' }
    if ($s1.StartsWith('PlayStation3') -or $volume -eq 'PS3VOLUME') {
        $codigo = if ($s1.StartsWith('PlayStation3') -and $s1.Substring(16, 16).Trim([char]0, [char]32) -match '^[A-Z]{4}-?\d{5}') { " ($($Matches[0]))" } else { '' }
        return "um disco de PlayStation 3$codigo"
    }
    if ($sistema -eq 'PSP GAME') { return 'um disco de PSP (UMD)' }
    if ($sistema -eq 'PLAYSTATION') { return $(if ($f.Length -gt 800MB) { 'um disco de PlayStation 2' } else { 'um disco de PlayStation' }) }
    return $null
}
# Why a file is not a disc image, in the player's words: a compressed file renamed, an incomplete download, or
# another format (report 860)
function Explicar-Iso([IO.FileStream]$f, [string]$iso) {
    $gb = '{0:N2}' -f ($f.Length / 1GB)
    $f.Position = 0
    $cab = New-Object byte[] 8
    [void]$f.Read($cab, 0, 8)
    $sig = [Text.Encoding]::ASCII.GetString($cab)
    $tipo = $null
    if ($sig.StartsWith('Rar!')) { $tipo = 'um arquivo .rar' }
    elseif ($cab[0] -eq 0x37 -and $cab[1] -eq 0x7A -and $cab[2] -eq 0xBC) { $tipo = 'um arquivo .7z' }
    elseif ($sig.StartsWith('PK')) { $tipo = 'um arquivo .zip' }
    elseif ($sig.StartsWith('CISO') -or $sig.StartsWith('ZISO')) { $tipo = 'uma imagem comprimida (CSO/ZSO)' }
    $msg = "não é uma imagem de disco do Xbox 360: $iso ($gb GB)."
    if ($tipo) { return "$msg É $tipo com o nome trocado para .iso: extraia-o primeiro e escolha a .iso de dentro." }
    $outro = Outro-Console $f
    if ($outro) { return "$msg É $outro; este port é feito a partir do disco de Xbox 360 do jogo: escolha a .iso do Xbox 360." }
    if ($f.Length -lt 6GB) { return "$msg É menor que um disco de Xbox 360 completo (7 a 8 GB): o download pode estar incompleto." }
    return "$msg Não achamos o sistema de arquivos do disco; o download pode estar incompleto ou corrompido, ou não ser uma cópia do disco (GOD/XBLA não servem)."
}
function Ler-Xex([string]$iso, [string]$destino) {
    if (Test-Path -LiteralPath $iso -PathType Container) {
        Copy-Item -LiteralPath (Join-Path $iso 'default.xex') $destino -Force
        return
    }
    $f = [IO.File]::OpenRead($iso)
    try {
        $r = New-Object IO.BinaryReader($f)
        $base = $null
        # the known layouts first, then any sector in the first 512 MB (report 860: an image none of them matched)
        foreach ($b in @(0, 0x18300000, 0xFD90000, 0x2080000, 0xFB20, 0x20600)) {
            if ($b + 0x10000 + 28 -gt $f.Length) { continue }
            $f.Position = $b + 0x10000
            if ([Text.Encoding]::ASCII.GetString($r.ReadBytes(20)) -eq 'MICROSOFT*XBOX*MEDIA') { $base = $b; break }
        }
        if ($null -eq $base) { $base = Achar-Xdvdfs $f }
        if ($null -eq $base) { throw (Explicar-Iso $f $iso) }
        $f.Position = $base + 0x10000 + 20
        $raizSetor = $r.ReadUInt32(); $raizTam = $r.ReadUInt32()
        $f.Position = $base + [int64]$raizSetor * 2048
        $dir = $r.ReadBytes($raizTam)
        # the root directory is a binary tree of entries: left, right (in dwords), sector, size, attributes, name
        $pilha = New-Object System.Collections.Stack
        $pilha.Push(0)
        $vistos = @{}
        $xbe = $false
        while ($pilha.Count) {
            $o = $pilha.Pop()
            if ($vistos[$o] -or $o + 14 -gt $dir.Length) { continue }
            $vistos[$o] = $true
            $esq = [BitConverter]::ToUInt16($dir, $o); $dirt = [BitConverter]::ToUInt16($dir, $o + 2)
            if ($esq -eq 0xFFFF -and $dirt -eq 0xFFFF) { continue }
            $setor = [BitConverter]::ToUInt32($dir, $o + 4); $tam = [BitConverter]::ToUInt32($dir, $o + 8)
            $nome = [Text.Encoding]::ASCII.GetString($dir, $o + 14, $dir[$o + 13])
            if ($nome -ieq 'default.xbe') { $xbe = $true }
            if ($nome -ieq 'default.xex') {
                $f.Position = $base + [int64]$setor * 2048
                $saida = [IO.File]::Create($destino)
                try {
                    $buf = New-Object byte[] (1MB); $falta = [int64]$tam
                    while ($falta -gt 0) {
                        $n = $f.Read($buf, 0, [int][Math]::Min($falta, $buf.Length))
                        if ($n -le 0) { throw 'a .iso acabou antes do default.xex' }
                        $saida.Write($buf, 0, $n); $falta -= $n
                    }
                } finally { $saida.Close() }
                return
            }
            if ($esq) { $pilha.Push($esq * 4) }
            if ($dirt) { $pilha.Push($dirt * 4) }
        }
        # the same file system as the Xbox 360's, with the original Xbox's executable
        if ($xbe) { throw "não é uma imagem de disco do Xbox 360: $iso. É um disco do Xbox original (tem default.xbe, não default.xex); este port é feito a partir do disco de Xbox 360 do jogo: escolha a .iso do Xbox 360." }
        throw "a .iso não tem default.xex: $iso"
    } finally { $f.Close() }
}

# a default.xex that is not the port's: what its header says (format, version, title) and the SHA-1 of its decoded
# image (the game's code itself, the same in a copy whose .xex was only decrypted or decompressed by a download site),
# so a report tells another edition apart from the same game written differently; it reads, never changes the file
function Descrever-Xex([string]$xex) {
    $d = [IO.File]::ReadAllBytes($xex)
    # big-endian (PowerShell's -shl and -bor share one precedence, left to right: plain arithmetic instead)
    $u32 = { param($o) [uint32]([int64]$d[$o] * 16777216 + [int64]$d[$o + 1] * 65536 + [int64]$d[$o + 2] * 256 + $d[$o + 3]) }
    $u16 = { param($o) [uint16]([int]$d[$o] * 256 + $d[$o + 1]) }
    $linhas = New-Object System.Collections.Generic.List[string]
    $linhas.Add("default.xex: $($d.Length) bytes")
    if ($d.Length -lt 0x18 -or [Text.Encoding]::ASCII.GetString($d, 0, 4) -ne 'XEX2') {
        $linhas.Add('não é um XEX2 (cabeçalho ' + [Text.Encoding]::ASCII.GetString($d, 0, [Math]::Min(4, $d.Length)) + ')')
        return $linhas
    }
    $dados = & $u32 8; $seg = & $u32 0x10; $n = & $u32 0x14
    $formato = $null; $exec = $null
    for ($i = 0; $i -lt $n -and 0x18 + $i * 8 + 8 -le $d.Length; $i++) {
        $chave = & $u32 (0x18 + $i * 8); $valor = & $u32 (0x18 + $i * 8 + 4)
        if ($chave -eq 0x000003FF) { $formato = $valor }
        if ($chave -eq 0x00040006) { $exec = $valor }
    }
    $linhas.Add(('module_flags 0x{0:X8}, dados em 0x{1:X}' -f (& $u32 4), $dados))
    $ver = { param($v) '{0}.{1}.{2}.{3} (0x{4:X8})' -f ($v -shr 28), (($v -shr 24) -band 0xF), (($v -shr 8) -band 0xFFFF), ($v -band 0xFF), $v }
    if ($exec -and $exec + 0x18 -le $d.Length) {
        $linhas.Add(('title_id {0:X8}, media_id {1:X8}, versão {2}, versão base {3}, disco {4}/{5}' -f (& $u32 ($exec + 0xC)),
            (& $u32 $exec), (& $ver (& $u32 ($exec + 4))), (& $ver (& $u32 ($exec + 8))), $d[$exec + 0x12], $d[$exec + 0x13]))
        $script:XEX_MIDIA = '{0:X8}' -f (& $u32 $exec)
        $script:XEX_VERSAO = & $u32 ($exec + 4)
    }
    if ($seg + 0x160 -le $d.Length) {
        $linhas.Add(('imagem: {0} bytes, carregada em 0x{1:X8}, image_flags 0x{2:X8}' -f (& $u32 ($seg + 4)), (& $u32 ($seg + 0x110)), (& $u32 ($seg + 0x10C))))
    }
    if (-not $formato -or $formato + 8 -gt $d.Length) { $linhas.Add('sem FILE_FORMAT_INFO'); return $linhas }
    $cifra = & $u16 ($formato + 4); $comp = & $u16 ($formato + 6)
    $nomeComp = @{ 0 = 'sem compressão'; 1 = 'compressão básica'; 2 = 'compressão LZX'; 3 = 'delta (patch)' }[[int]$comp]
    $linhas.Add("formato: $(if ($cifra) { 'cifrado' } else { 'sem cifra' }), $nomeComp")
    if ($comp -gt 1) { $linhas.Add('hash da imagem: não calculado neste formato'); return $linhas }
    try {
        $corpo = New-Object byte[] ($d.Length - $dados)
        [Array]::Copy($d, $dados, $corpo, 0, $corpo.Length)
        $claro = $corpo
        if ($cifra) {
            # the session key opened with the retail console key (then the devkit one, all zeros); the image is AES-128-CBC,
            # IV zero, as one stream
            $chaveCifrada = New-Object byte[] 16; [Array]::Copy($d, $seg + 0x150, $chaveCifrada, 0, 16)
            $tam = $corpo.Length - ($corpo.Length % 16)
            foreach ($console in @([byte[]](0x20, 0xB1, 0x85, 0xA5, 0x9D, 0x28, 0xFD, 0xC3, 0x40, 0x58, 0x3F, 0xBB, 0x08, 0x96, 0xBF, 0x91), (New-Object byte[] 16))) {
                $aes = [Security.Cryptography.Aes]::Create(); $aes.Padding = 'None'
                $aes.Mode = 'ECB'; $aes.Key = $console
                $sessao = $aes.CreateDecryptor().TransformFinalBlock($chaveCifrada, 0, 16)
                $aes.Mode = 'CBC'; $aes.Key = $sessao; $aes.IV = New-Object byte[] 16
                $claro = New-Object byte[] $corpo.Length
                [void]$aes.CreateDecryptor().TransformBlock($corpo, 0, $tam, $claro, 0)
                $aes.Dispose()
                if ($comp -eq 1 -or ($claro[0] -eq 0x4D -and $claro[1] -eq 0x5A)) { break }
            }
        }
        $imagem = $claro
        if ($comp -eq 1) {
            # basic compression: (data size, zeros after it) pairs after the format header
            $blocos = [int](((& $u32 $formato) - 8) / 8)
            $ms = New-Object IO.MemoryStream
            $p = 0
            for ($b = 0; $b -lt $blocos; $b++) {
                $dt = [int](& $u32 ($formato + 8 + $b * 8)); $zt = [int](& $u32 ($formato + 12 + $b * 8))
                $dt = [Math]::Min($dt, $claro.Length - $p)
                $ms.Write($claro, $p, $dt); $p += $dt
                if ($zt -gt 0) { $ms.Write((New-Object byte[] $zt), 0, $zt) }
            }
            $imagem = $ms.ToArray()
        }
        # the image at the size the security header gives (the zeros at its end may or may not be in the file)
        $tamImagem = [int](& $u32 ($seg + 4))
        if ($tamImagem -gt 0 -and $imagem.Length -ne $tamImagem) {
            $justa = New-Object byte[] $tamImagem
            [Array]::Copy($imagem, $justa, [Math]::Min($imagem.Length, $tamImagem))
            $imagem = $justa
        }
        $pe = if ($imagem.Length -ge 2 -and $imagem[0] -eq 0x4D -and $imagem[1] -eq 0x5A) { 'MZ ok' } else { 'sem MZ: chave ou formato desconhecido' }
        $sha = [Security.Cryptography.SHA1]::Create()
        $h = ([BitConverter]::ToString($sha.ComputeHash($imagem)) -replace '-', '').ToLower()
        $linhas.Add("imagem decodificada: $($imagem.Length) bytes, $pe, SHA-1 $h")
        $script:SHA1_IMAGEM_LIDA = $h
        if ($JOGO['sha1_imagem']) {
            $linhas.Add($(if ($h -eq $JOGO['sha1_imagem']) { 'o código é o MESMO da versão do port (só o arquivo .xex foi regravado)' }
                          else { "o código é de outra edição (a do port: $($JOGO['sha1_imagem']))" }))
        }
    } catch { $linhas.Add("hash da imagem: falhou ($_)") }
    return $linhas
}

# ---------------------------------------------------------------------------- the video card
# The game draws with Direct3D 12 (feature level 11_0 at least) and gives up at its start without it (report 1468: an
# Intel HD Graphics 4000, which has no Direct3D 12 driver, after the whole download and build). Asked of Windows the
# way the engine asks (every adapter DXGI lists, the software ones left out, a device of level 11_0 tested without
# being made) before anything is downloaded or built. A check that cannot run says nothing and the generation goes on.
function Adaptadores-D3D12 {
    if (-not ('OtxD3D12' -as [type])) {
        Add-Type -Language CSharp -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class OtxD3D12 {
    [DllImport("dxgi.dll")] static extern int CreateDXGIFactory1(ref Guid riid, out IntPtr factory);
    [DllImport("d3d12.dll")] static extern int D3D12CreateDevice(IntPtr adapter, int level, ref Guid riid, IntPtr device);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int EnumAdapters1(IntPtr self, uint i, out IntPtr adapter);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int GetDesc1(IntPtr self, IntPtr desc);
    [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate uint Release(IntPtr self);
    static T Slot<T>(IntPtr obj, int i) where T : class {
        IntPtr fn = Marshal.ReadIntPtr(Marshal.ReadIntPtr(obj), i * IntPtr.Size);
        return Marshal.GetDelegateForFunctionPointer(fn, typeof(T)) as T;
    }
    // one line per adapter: name|vendor|device|software (0/1)|Direct3D 12 level 11_0 (0/1)
    public static string[] Listar() {
        Guid fabricaId = new Guid("770aae78-f26f-4dba-a829-253c83d1b387");   // IDXGIFactory1
        Guid dispositivoId = new Guid("189819f1-1db6-4b57-be54-1821339b85f7"); // ID3D12Device
        IntPtr fabrica;
        int hr = CreateDXGIFactory1(ref fabricaId, out fabrica);
        if (hr < 0) throw new Exception("CreateDXGIFactory1 0x" + hr.ToString("X8"));
        List<string> linhas = new List<string>();
        try {
            for (uint i = 0; i < 16; i++) {
                IntPtr adaptador;
                if (Slot<EnumAdapters1>(fabrica, 12)(fabrica, i, out adaptador) != 0) break;
                IntPtr desc = Marshal.AllocHGlobal(512);
                try {
                    string nome = "?"; int fornecedor = 0, aparelho = 0, flags = 0;
                    if (Slot<GetDesc1>(adaptador, 10)(adaptador, desc) >= 0) {
                        nome = Marshal.PtrToStringUni(desc);
                        fornecedor = Marshal.ReadInt32(desc, 256);
                        aparelho = Marshal.ReadInt32(desc, 260);
                        flags = Marshal.ReadInt32(desc, 304);
                    }
                    int d3d12 = D3D12CreateDevice(adaptador, 0xb000, ref dispositivoId, IntPtr.Zero);
                    linhas.Add(string.Format("{0}|{1:X4}|{2:X4}|{3}|{4}", nome, fornecedor, aparelho,
                                             (flags & 2) != 0 ? 1 : 0, d3d12 >= 0 ? 1 : 0));
                } finally {
                    Marshal.FreeHGlobal(desc);
                    Slot<Release>(adaptador, 2)(adaptador);
                }
            }
        } finally {
            Slot<Release>(fabrica, 2)(fabrica);
        }
        return linhas.ToArray();
    }
}
'@
    }
    return [OtxD3D12]::Listar()
}
function Conferir-Video {
    try { $lista = @(Adaptadores-D3D12 | Select-Object -Unique) }
    catch { Msg "placa de vídeo: não foi possível conferir o Direct3D 12 ($($_.Exception.Message)); seguindo"; return }
    $placas = @($lista | ForEach-Object { $c = $_.Split('|'); [pscustomobject]@{ Nome = $c[0]; Software = $c[3] -eq '1'; D3D12 = $c[4] -eq '1' } } |
                Where-Object { -not $_.Software })
    foreach ($p in $placas) { Msg "placa de vídeo: $($p.Nome) (Direct3D 12: $(if ($p.D3D12) { 'sim' } else { 'não' }))" }
    if (@($placas | Where-Object { $_.D3D12 }).Count) { return }
    $nomes = if ($placas.Count) { ($placas | ForEach-Object { $_.Nome }) -join ', ' } else { 'nenhuma (só o vídeo básico do Windows)' }
    throw "a placa de vídeo deste PC não tem Direct3D 12, que o jogo precisa para abrir: $nomes. O port precisa de uma placa de vídeo com DirectX 12 (nível 11_0 ou mais). Se o PC tem outra placa de vídeo, confira se o driver dela está instalado e atualizado. A geração parou aqui, antes de baixar e compilar."
}
Conferir-Video

# ---------------------------------------------------------------------------- 1. ferramentas
# done before but the tools this generator wants are not there (a newer SDK, a deleted folder): again
if (-not (Test-Path (Join-Path $SDK 'bin\rexglue.exe')) -or -not (Test-Path (Join-Path $LLVM 'bin\clang++.exe'))) {
    Remove-Item (Join-Path $OUT '1_ferramentas.ok') -ErrorAction SilentlyContinue
}
Etapa 1 '1_ferramentas' {
    $vs = Achar-VS
    if (-not $vs) {
        throw "falta o Visual Studio 2022 Build Tools (ou mais novo) com C++ e CMake; o 2019 e os anteriores não servem. Instale com:`r`n  winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override `"--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.CMake.Project --includeRecommended`""
    }
    Anotar $script:LOG "Visual Studio: $vs"
    if (-not (Tem-SdkWindows)) { throw $ERRO_SDK_WINDOWS }
    New-Item -ItemType Directory -Force $FERRAMENTAS | Out-Null
    if (-not (Test-Path (Join-Path $SDK 'bin\rexglue.exe'))) {
        $zip = Join-Path $FERRAMENTAS "rexglue-sdk-$SDK_VERSAO.zip"
        if (-not (Test-Path $zip) -or (Get-FileHash $zip -Algorithm SHA256).Hash -ne $SDK_SHA256) { Baixar-De $SDK_URLS $zip $SDK_SHA256 }
        if ((Get-FileHash $zip -Algorithm SHA256).Hash -ne $SDK_SHA256) { throw "o SDK baixado não confere (SHA-256): $zip" }
        $tmp = Join-Path $FERRAMENTAS 'sdk.tmp'
        Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
        Expand-Archive $zip $tmp
        Remove-Item -Recurse -Force $SDK -ErrorAction SilentlyContinue
        Mover-Pasta (Join-Path $tmp 'win-amd64') $SDK
        Apagar $tmp, $zip
    }
    if (-not (Test-Path (Join-Path $LLVM 'bin\clang++.exe'))) {
        $zstd = Join-Path $FERRAMENTAS 'zstd\zstd.exe'
        if (-not (Test-Path $zstd)) {
            $zip = Join-Path $FERRAMENTAS 'zstd.zip'
            Baixar $ZSTD_URL $zip
            if ((Get-FileHash $zip -Algorithm SHA256).Hash -ne $ZSTD_SHA256) { throw "o zstd baixado não confere (SHA-256): $zip" }
            $tmp = Join-Path $FERRAMENTAS 'zstd.tmp'
            Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
            Expand-Archive $zip $tmp
            New-Item -ItemType Directory -Force (Split-Path $zstd) | Out-Null
            Move-Item (Get-ChildItem $tmp -Recurse -Filter zstd.exe | Select-Object -First 1).FullName $zstd
            Apagar $tmp, $zip
        }
        $pacote = Join-Path $FERRAMENTAS "llvm-$LLVM_VERSAO.tar.zst"
        if (-not (Test-Path $pacote) -or (Get-FileHash $pacote -Algorithm SHA256).Hash -ne $LLVM_SHA256) { Baixar $LLVM_URL $pacote }
        if ((Get-FileHash $pacote -Algorithm SHA256).Hash -ne $LLVM_SHA256) { throw "o LLVM baixado não confere (SHA-256): $pacote" }
        $tmp = Join-Path $FERRAMENTAS 'llvm.tmp'
        Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
        New-Item -ItemType Directory -Force $tmp | Out-Null
        # only what a port's build runs: the compiler, the linker, the archiver and the compiler's own headers
        # (about 370 MB of the package's 4 GB)
        Msg 'extraindo o LLVM ...'
        $quais = @('*/bin/clang.exe', '*/bin/clang++.exe', '*/bin/lld-link.exe', '*/bin/llvm-ar.exe', '*/bin/llvm-ranlib.exe', '*/lib/clang/*') |
            ForEach-Object { "--include `"$_`"" }
        Exec "`"$zstd`" -dc --long=31 `"$pacote`" | `"$env:SystemRoot\System32\tar.exe`" -xf - -C `"$tmp`" $($quais -join ' ')"
        Remove-Item -Recurse -Force $LLVM -ErrorAction SilentlyContinue
        Mover-Pasta (Get-ChildItem $tmp -Directory | Select-Object -First 1).FullName $LLVM
        if (-not (Test-Path (Join-Path $LLVM 'bin\clang++.exe'))) { throw 'o LLVM extraído não tem o clang++' }
        Apagar $tmp, $pacote
    }
}

# ---------------------------------------------------------------------------- 2. jogo
# a folder picked instead of the .iso (report 148: the folder a download was unpacked to): the one image in it, two
# folders down at most; else the game unpacked from its disc (report 214: the files of the disc, default.xex at the
# top), the folder picked, one of the two above it (a folder of the disc picked, as its $SystemUpdate) or one two
# folders down at most; neither, a message saying what the port reads (a Games on Demand folder is neither)
function Achar-Extraido([string]$pasta) {
    $acima = $pasta
    for ($i = 0; $i -le 2 -and $acima; $i++) {
        if (Test-Path -LiteralPath (Join-Path $acima 'default.xex') -PathType Leaf) { return @($acima) }
        $acima = [IO.Path]::GetDirectoryName($acima)
    }
    return @(Get-ChildItem -LiteralPath $pasta -File -Recurse -Depth 2 -Filter 'default.xex' -ErrorAction SilentlyContinue |
             ForEach-Object { $_.DirectoryName })
}
if ($Iso -and (Test-Path -LiteralPath $Iso -PathType Container)) {
    $pasta = $Iso
    $imagens = @(Get-ChildItem -LiteralPath $pasta -File -Recurse -Depth 2 -ErrorAction SilentlyContinue |
                 Where-Object { $_.Extension -in '.iso', '.xiso' })
    $extraidos = @(if ($imagens.Count -eq 0) { Achar-Extraido $pasta })
    if ($imagens.Count -eq 1) {
        $Iso = $imagens[0].FullName
        Msg "a .iso dentro da pasta escolhida: $Iso"
    } elseif ($imagens.Count -gt 1) {
        throw "a pasta escolhida tem $($imagens.Count) arquivos .iso; escolha o arquivo .iso do jogo (Escolher arquivo...): $(($imagens | ForEach-Object { $_.Name }) -join ', ')"
    } elseif ($extraidos.Count -eq 1) {
        $Iso = $extraidos[0]
        Msg "o jogo extraído do disco: $Iso"
    } elseif ($extraidos.Count -gt 1) {
        throw "a pasta escolhida tem $($extraidos.Count) jogos extraídos (default.xex); escolha a pasta do jogo: $($extraidos -join ', ')"
    } else {
        # the PlayStation 3 edition of the game picked (report 1444: EBOOT.BIN, PIC1.PNG, SND0.AT3, LIC.DAT): said plainly
        $ps3 = @(Get-ChildItem -LiteralPath $pasta -Recurse -Depth 2 -ErrorAction SilentlyContinue |
                 Where-Object { $_.Name -in 'EBOOT.BIN', 'PARAM.SFO', 'PS3_DISC.SFB', 'PS3_GAME' } |
                 ForEach-Object { $_.Name.ToUpper() } | Select-Object -Unique)
        if ($ps3.Count -or (Split-Path -Leaf $pasta) -in 'PS3_GAME', 'USRDIR') {
            $vistos = if ($ps3.Count) { " (tem $($ps3 -join ', '))" } else { '' }
            throw "a pasta escolhida é a versão de PlayStation 3 do jogo$vistos`: $pasta. Este port é feito a partir do jogo de Xbox 360: ele precisa da imagem do disco do Xbox 360 (um arquivo .iso, ou o .zip/.7z/.rar baixado com ela dentro) ou da pasta com os arquivos desse disco extraídos (com o default.xex). A versão de PS3 não serve."
        }
        # what the folder holds, the largest first, said in the message (report 1348: nothing told what was there)
        $tem = @(Get-ChildItem -LiteralPath $pasta -File -Recurse -Depth 2 -ErrorAction SilentlyContinue |
                 Sort-Object Length -Descending | Select-Object -First 6 |
                 ForEach-Object { '{0} ({1:N0} MB)' -f $_.Name, ($_.Length / 1MB) })
        $tem = if ($tem.Count) { "Os maiores arquivos dela: $($tem -join ', ')." } else { 'Ela não tem arquivos.' }
        throw "a pasta escolhida não tem uma .iso nem o jogo extraído (default.xex): $pasta. $tem O port lê a imagem do disco do Xbox 360 (um arquivo .iso, ou o .zip/.7z/.rar baixado com ela dentro) ou a pasta com os arquivos do disco extraídos. Uma pasta no formato Games on Demand não serve."
    }
}
# the .iso picked again (moved, or a copy elsewhere): checked again and written down, the jogar.cmd made again; the
# translated and compiled code is kept (the same default.xex, which the check makes sure of)
$isoTxt = Join-Path $OUT 'iso.txt'
if ($Iso -and (Test-Path $isoTxt)) {
    $antes = Get-Content $isoTxt -Encoding UTF8 | Select-Object -First 1
    if ($antes -and -not [string]::Equals([IO.Path]::GetFullPath($antes), [IO.Path]::GetFullPath($Iso),
                                          [StringComparison]::OrdinalIgnoreCase)) {
        Msg "a .iso mudou: $Iso"
        Remove-Item (Join-Path $OUT '2_jogo.ok'), (Join-Path $OUT '5_jogar.ok') -ErrorAction SilentlyContinue
    }
}
Etapa 2 '2_jogo' {
    if (-not $Iso) { throw 'falta a .iso do jogo (-Iso)' }
    if (-not (Test-Path -LiteralPath $Iso)) { throw "a .iso (ou a pasta do jogo) não existe: $Iso" }
    $assets = Join-Path $R 'assets'
    New-Item -ItemType Directory -Force $assets | Out-Null
    $xex = Join-Path $assets 'default.xex'
    # read beside it and checked first: a wrong .iso leaves the good default.xex in place
    $lido = "$xex.novo"
    Ler-Xex $Iso $lido
    $sha = (Get-FileHash $lido -Algorithm SHA1).Hash.ToLower()
    if ($sha -ne $JOGO['sha1_xex']) {
        # the disc of another port's game picked by mistake (report 1266: the Saints Row image in DiRT 2's port):
        # named, with what to pick instead
        $outros = @{ 'bcaec56eabad135071ee879b4bbcc6aa1e3362b8' = "Asura's Wrath"
                     '98bff97ab205fc9893bf49e8c79b26711f6b18db' = "Dante's Inferno"
                     'cdcbe29702fbf18e3e63002096d94d081cbf488f' = 'DiRT 2'
                     'c2646093ff3926141fcf4fd630e876873d1caece' = 'Saints Row' }
        if ($outros.ContainsKey($sha)) {
            Remove-Item $lido -ErrorAction SilentlyContinue
            throw "a .iso escolhida é do jogo $($outros[$sha]), não do $($JOGO['titulo']). Escolha a .iso do $($JOGO['titulo']) ($($JOGO['versao'])); a que foi escolhida serve no port do $($outros[$sha])."
        }
        # what this default.xex is, in the step's log (and so in a report): another edition, or the same game re-encoded
        $script:SHA1_IMAGEM_LIDA = $null; $script:XEX_MIDIA = $null; $script:XEX_VERSAO = $null
        try { foreach ($l in (Descrever-Xex $lido)) { Anotar $script:LOG "xex: $l"; Msg "xex: $l" } }
        catch { Anotar $script:LOG "xex: não foi possível descrever ($_)" }
        # the same code in a default.xex written differently (decrypted or decompressed by a download site; reports
        # 868, 968, 969, 996, 1120, 1162): the decoded image is the port's, so the translation is the same and the
        # runtime loads that file as it is (it reads every XEX2 format)
        if (-not ($JOGO['sha1_imagem'] -and $script:SHA1_IMAGEM_LIDA -eq $JOGO['sha1_imagem'])) {
            Remove-Item $lido -ErrorAction SilentlyContinue
            # the port's own disc with a title update put into its default.xex (report 1476: some downloaded images come
            # "with the update"; same media_id, another version): the code is the update's, so it is said plainly
            if ($JOGO['media_id'] -and $JOGO['versao_xex'] -and $script:XEX_MIDIA -eq $JOGO['media_id'] -and
                $null -ne $script:XEX_VERSAO -and $script:XEX_VERSAO -ne [uint32]$JOGO['versao_xex']) {
                $v = { param($x) '{0}.{1}.{2}.{3}' -f ($x -shr 28), (($x -shr 24) -band 0xF), (($x -shr 8) -band 0xFFFF), ($x -band 0xFF) }
                throw "a .iso escolhida é do disco certo ($($JOGO['versao'])), mas veio com a atualização do jogo embutida no default.xex (versão $(& $v $script:XEX_VERSAO); a do disco é $(& $v ([uint32]$JOGO['versao_xex']))). Algumas cópias baixadas vêm assim, `"com update`". O port é feito a partir do executável original do disco: use uma .iso do disco original, sem a atualização embutida (o gerador confere a .iso inteira e avisa se ela é idêntica à cópia conhecida do disco)."
            }
            throw "este jogo não é a versão do port ($($JOGO['versao']), default.xex SHA-1 $($JOGO['sha1_xex']); a sua: $sha)"
        }
        Msg "o default.xex desta cópia foi regravado, mas o código é o mesmo da versão do port: aceito"
    }
    Move-Item -Force $lido $xex
    Set-Content -Path (Join-Path $OUT 'iso.txt') -Value $Iso -Encoding UTF8
}

# The game's extra content (DLC), for the games whose config\jogo.txt says dlc=sim: looked for beside the disc image
# every time this runs. Packages lying there (Xbox Live files with no extension, in any subfolder) are read by the
# game where they are; compressed files (.zip, .7z, .rar and the others Windows opens) that hold packages of this
# game are taken out into the port's dlc folder; another .iso there is opened when it is a plain data image. Nothing
# of it is needed: without packages the game is the disc's.
function Ler-Pacote([string]$arq) {
    # the name a content package gives itself, when the file is one of this game's (marketplace content, its title);
    # $null otherwise
    try {
        $f = [IO.File]::OpenRead($arq)
        try {
            if ($f.Length -lt 0xA000) { return $null }
            $b = New-Object byte[] 0x520
            if ($f.Read($b, 0, $b.Length) -ne $b.Length) { return $null }
        } finally { $f.Close() }
    } catch { return $null }
    $nome = Nome-Do-Pacote $b
    if ($nome -eq '') { $nome = [IO.Path]::GetFileName($arq) }
    return $nome
}
function Nome-Do-Pacote([byte[]]$b) {
    # from a package's first 0x520 bytes: its name ('' when it has none), $null when it is not this game's
    $sig = [Text.Encoding]::ASCII.GetString($b, 0, 4)
    if ($sig -ne 'LIVE' -and $sig -ne 'PIRS' -and $sig -ne 'CON ') { return $null }
    $tipo = ([int]$b[0x344] -shl 24) -bor ([int]$b[0x345] -shl 16) -bor ([int]$b[0x346] -shl 8) -bor [int]$b[0x347]
    $titulo = '{0:X2}{1:X2}{2:X2}{3:X2}' -f $b[0x360], $b[0x361], $b[0x362], $b[0x363]
    if ($tipo -ne 2 -or $titulo -ne $JOGO['title_id'].ToUpper()) { return $null }
    return [Text.Encoding]::BigEndianUnicode.GetString($b, 0x411, 0x100).Split([char]0)[0].Trim()
}
function Zip-De-Outro-Jogo([string]$arq) {
    # $true when the .zip was read and none of the packages in it is this game's
    try {
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $z = [IO.Compression.ZipFile]::OpenRead($arq)
        try {
            foreach ($e in $z.Entries) {
                if ($e.Name -notmatch '^[0-9A-Fa-f]{42}$') { continue }
                $b = New-Object byte[] 0x520
                $f = $e.Open()
                try {
                    $lidos = 0
                    while ($lidos -lt $b.Length) {
                        $n = $f.Read($b, $lidos, $b.Length - $lidos)
                        if ($n -le 0) { break }
                        $lidos += $n
                    }
                } finally { $f.Dispose() }
                if ($lidos -eq $b.Length -and $null -ne (Nome-Do-Pacote $b)) { return $false }
            }
        } finally { $z.Dispose() }
        return $true
    } catch { return $false }
}
function Achar-7z {
    foreach ($p in "$env:ProgramFiles\7-Zip\7z.exe", "${env:ProgramFiles(x86)}\7-Zip\7z.exe") {
        if ($p -and (Test-Path -LiteralPath $p)) { return $p }
    }
    return $null
}
function Listar-Compactado([string]$arq) {
    # the names inside a compressed file (or a data .iso), $null when nothing here opens it
    $ext = [IO.Path]::GetExtension($arq).ToLower()
    if ($ext -eq '.zip') {
        try {
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            $z = [IO.Compression.ZipFile]::OpenRead($arq)
            try { return @($z.Entries | ForEach-Object { $_.FullName }) } finally { $z.Dispose() }
        } catch { }
    }
    $tar = "$env:SystemRoot\System32\tar.exe"
    if (Test-Path $tar) {
        $nomes = & $tar -tf $arq 2>$null
        if ($LASTEXITCODE -eq 0 -and $nomes) { return @($nomes) }
    }
    $z7 = Achar-7z
    if ($z7) {
        $linhas = & $z7 l -slt -ba $arq 2>$null
        if ($LASTEXITCODE -eq 0) { return @($linhas | Where-Object { $_ -like 'Path = *' } | ForEach-Object { $_.Substring(7) }) }
    }
    return $null
}
function Extrair-Compactado([string]$arq, [string]$destino) {
    New-Item -ItemType Directory -Force $destino | Out-Null
    $ext = [IO.Path]::GetExtension($arq).ToLower()
    if ($ext -eq '.zip') {
        try {
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            $z = [IO.Compression.ZipFile]::OpenRead($arq)
            try {
                foreach ($e in $z.Entries) {
                    if ($e.Name -notmatch '^[0-9A-Fa-f]{42}$') { continue }
                    [IO.Compression.ZipFileExtensions]::ExtractToFile($e, (Join-Path $destino $e.Name), $true)
                }
            } finally { $z.Dispose() }
            return $true
        } catch { }
    }
    $tar = "$env:SystemRoot\System32\tar.exe"
    if (Test-Path $tar) {
        & $tar -xf $arq -C $destino 2>$null
        if ($LASTEXITCODE -eq 0) { return $true }
    }
    $z7 = Achar-7z
    if ($z7) {
        & $z7 x -y "-o$destino" $arq 2>$null | Out-Null
        if ($LASTEXITCODE -eq 0) { return $true }
    }
    return $false
}
function Achar-Dlc([string]$iso) {
    $dlc = Join-Path $R 'dlc'
    $pasta = if (Test-Path -LiteralPath $iso -PathType Container) { $iso } else { [IO.Path]::GetDirectoryName($iso) }
    if (-not $pasta -or -not (Test-Path -LiteralPath $pasta)) { return }
    # a disc's root or a whole drive is not searched in depth
    $fundo = if ([IO.Path]::GetPathRoot($pasta) -eq $pasta) { 0 } else { 2 }
    $arquivos = @(Get-ChildItem -LiteralPath $pasta -File -Recurse -Depth $fundo -ErrorAction SilentlyContinue |
                  Where-Object { -not $_.FullName.StartsWith($R, [StringComparison]::OrdinalIgnoreCase) })
    $isoCheia = if (Test-Path -LiteralPath $iso -PathType Leaf) { [IO.Path]::GetFullPath($iso) } else { '' }
    $compactados = '.zip', '.7z', '.rar', '.tar', '.gz', '.tgz', '.bz2', '.xz', '.zst', '.cab', '.iso', '.001'
    foreach ($a in $arquivos) {
        if ($a.Extension.ToLower() -notin $compactados -or $a.FullName -eq $isoCheia) { continue }
        if ($a.Extension -eq '.iso') {
            # a console's disc image is not a data image: only one with the ISO 9660 mark is opened
            try {
                $f = [IO.File]::OpenRead($a.FullName)
                try {
                    $m = New-Object byte[] 5
                    $f.Position = 0x8001
                    [void]$f.Read($m, 0, 5)
                } finally { $f.Close() }
                if ([Text.Encoding]::ASCII.GetString($m) -ne 'CD001') { continue }
            } catch { continue }
        }
        $destino = Join-Path $dlc ($a.BaseName -replace '[^\w\-\. \(\)'']', '_')
        $marca = "$destino.ok"
        $assinatura = "$($a.FullName)|$($a.Length)|$($a.LastWriteTimeUtc.Ticks)"
        if ((Test-Path -LiteralPath $marca) -and (Get-Content -LiteralPath $marca -Encoding UTF8 | Select-Object -First 1) -eq $assinatura) { continue }
        $nomes = Listar-Compactado $a.FullName
        if ($null -eq $nomes) {
            if ($a.Extension -ne '.iso') { Msg "DLC: não consegui abrir $($a.Name) para ver se tem DLC (se tiver, extraia para $dlc)" }
            continue
        }
        if (-not ($nomes | Where-Object { ($_ -replace '[\\/]+$', '').Split('\/')[-1] -match '^[0-9A-Fa-f]{42}$' })) { continue }
        New-Item -ItemType Directory -Force $dlc | Out-Null
        if ($a.Extension -eq '.zip' -and (Zip-De-Outro-Jogo $a.FullName)) {
            Set-Content -LiteralPath $marca -Value $assinatura -Encoding UTF8
            continue
        }
        Msg "DLC: extraindo de $($a.Name) ..."
        if (-not (Extrair-Compactado $a.FullName $destino)) {
            Msg "DLC: não consegui extrair $($a.Name); extraia para $dlc"
            continue
        }
        # what came out is another game's: not kept
        if (-not (Get-ChildItem -LiteralPath $destino -File -Recurse -ErrorAction SilentlyContinue |
                  Where-Object { $_.Name -match '^[0-9A-Fa-f]{42}$' -and (Ler-Pacote $_.FullName) })) {
            Msg "DLC: $($a.Name) é de outro jogo; deixado de lado"
            Apagar @($destino)
        }
        Set-Content -LiteralPath $marca -Value $assinatura -Encoding UTF8
    }
    # the packages: those lying beside the disc image stay there (their folders written down for the game), those in
    # the port's dlc folder are found by the game itself
    $pastas = New-Object System.Collections.Generic.List[string]
    $achados = New-Object System.Collections.Generic.List[string]
    $soltos = @($arquivos | Where-Object { $_.Name -match '^[0-9A-Fa-f]{42}$' })
    $extraidos = @(if (Test-Path -LiteralPath $dlc) { Get-ChildItem -LiteralPath $dlc -File -Recurse -ErrorAction SilentlyContinue |
                                                      Where-Object { $_.Name -match '^[0-9A-Fa-f]{42}$' } })
    $vistos = @{}
    foreach ($a in @($extraidos) + @($soltos)) {
        $nome = Ler-Pacote $a.FullName
        if (-not $nome -or $vistos.ContainsKey($a.Name.ToUpper())) { continue }
        $vistos[$a.Name.ToUpper()] = $true
        $achados.Add($nome)
        if (-not $a.FullName.StartsWith($dlc, [StringComparison]::OrdinalIgnoreCase) -and -not $pastas.Contains($a.DirectoryName)) {
            $pastas.Add($a.DirectoryName)
        }
    }
    if ($achados.Count -eq 0) { return }
    New-Item -ItemType Directory -Force $dlc | Out-Null
    [IO.File]::WriteAllLines((Join-Path $dlc 'pastas.txt'), $pastas, (New-Object Text.UTF8Encoding $false))
    Msg "DLC: $($achados.Count) pacote(s) do jogo: $($achados -join ', ')"
}
if ($JOGO['dlc'] -eq 'sim') {
    $isoDlc = if ($Iso) { $Iso } elseif (Test-Path $isoTxt) { Get-Content $isoTxt -Encoding UTF8 | Select-Object -First 1 } else { '' }
    if ($isoDlc) {
        try { Achar-Dlc $isoDlc } catch { Msg "DLC: a procura falhou ($_); o jogo segue sem elas" }
    }
}

# the SDK changed since the last translation (a new engine): translation, build and the launch script again, even
# for whoever runs this by hand without -RefazerDe
$sdkTxt = Join-Path $OUT 'sdk.txt'
$sdkAntes = if (Test-Path $sdkTxt) { Get-Content $sdkTxt -Encoding UTF8 | Select-Object -First 1 } else { '' }
if ($sdkAntes -ne $SDK_VERSAO) {
    if (Test-Path (Join-Path $OUT '3_traducao.ok')) { Msg "o SDK mudou ($sdkAntes -> $SDK_VERSAO): tradução e compilação de novo" }
    Remove-Item (Join-Path $OUT '3_traducao.ok'), (Join-Path $OUT '4_compilar.ok'), (Join-Path $OUT '5_jogar.ok') -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force $OUT | Out-Null
    Set-Content -Path $sdkTxt -Value $SDK_VERSAO -Encoding UTF8
}

# ---------------------------------------------------------------------------- the disc image, checked whole
# The generator reads only the default.xex from the disc image; the game reads the rest of it when it plays, so a
# damaged image (a corrupted download, a bad read off the drive) gets through the generation and closes the game as
# it loads (report 1463, likely). The image is compared once with the known dumps of the disc (config\jogo.txt
# iso_ref = size:SHA-1 of each Redump dump of the game, separated by spaces; the port was tested with the first): the
# same, or what differs said plainly. Nothing is stopped by it: a trimmed image (smaller, the empty space taken out)
# or one made by another dumping program plays the same. The result is kept in out\gerar\iso_conferida.txt, which the
# launcher sends along with a report.
function Conferir-Iso([string]$iso, [string]$saida) {
    $conhecidas = @($JOGO['iso_ref'] -split '\s+' | Where-Object { $_ } | ForEach-Object {
        $t, $s = $_.Split(':'); [pscustomobject]@{ Tamanho = [int64]$t; Sha1 = $s.ToLower() } })
    $arq = Get-Item -LiteralPath $iso
    $gb = '{0:N2} GB' -f ($arq.Length / 1GB)
    $tamanhos = ($conhecidas | ForEach-Object { $_.Tamanho } | Select-Object -Unique) -join ' ou '
    $linhas = @($iso, "tamanho: $($arq.Length) bytes (das cópias conhecidas do disco: $tamanhos)")
    $mesmoTamanho = @($conhecidas | Where-Object { $_.Tamanho -eq $arq.Length })
    if (-not $mesmoTamanho.Count) {
        $res = "a .iso tem outro tamanho que a cópia completa do disco ($gb; a completa tem {0:N2} GB): pode ser uma .iso reduzida, que funciona igual, ou um download incompleto. Se o jogo fechar ao carregar, baixe a .iso de novo." -f ($conhecidas[0].Tamanho / 1GB)
    } else {
        Msg "conferindo a .iso inteira ($gb; pode levar alguns minutos) ..."
        $sha = [Security.Cryptography.SHA1]::Create()
        $f = [IO.File]::Open($iso, 'Open', 'Read', 'ReadWrite')
        try {
            $buf = New-Object byte[] (8MB)
            $lido = 0L; $proximo = 0.25
            while (($n = $f.Read($buf, 0, $buf.Length)) -gt 0) {
                [void]$sha.TransformBlock($buf, 0, $n, $null, 0)
                $lido += $n
                if ($lido / $arq.Length -ge $proximo) { Msg ('conferindo a .iso: {0:P0}' -f $proximo); $proximo += 0.25 }
            }
            [void]$sha.TransformFinalBlock($buf, 0, 0)
        } finally { $f.Dispose() }
        $hash = -join ($sha.Hash | ForEach-Object { $_.ToString('x2') })
        $linhas += "sha1: $hash (das cópias conhecidas com esse tamanho: $(($mesmoTamanho | ForEach-Object { $_.Sha1 }) -join ', '))"
        $res = if (@($mesmoTamanho | Where-Object { $_.Sha1 -eq $hash }).Count) { 'a .iso é idêntica a uma cópia conhecida do disco (Redump): está inteira.' }
               else { 'aviso: a .iso tem o tamanho do disco completo, mas o conteúdo é diferente das cópias conhecidas do disco (Redump). Cópias feitas por outros programas podem funcionar; mas, se o jogo fechar ao carregar, ela pode estar danificada (download corrompido ou erro de leitura): baixe a .iso de novo.' }
    }
    Msg $res
    Set-Content -Path $saida -Value (@($linhas[0], $res) + $linhas[1..($linhas.Count - 1)]) -Encoding UTF8
}
$isoConferida = Join-Path $OUT 'iso_conferida.txt'
$isoUsada = if ($Iso) { $Iso } elseif (Test-Path $isoTxt) { Get-Content $isoTxt -Encoding UTF8 | Select-Object -First 1 }
if ($JOGO['iso_ref'] -and $isoUsada -and [IO.Path]::GetExtension($isoUsada) -ieq '.iso' -and
    (Test-Path -LiteralPath $isoUsada -PathType Leaf) -and
    -not ((Test-Path $isoConferida) -and (Get-Content $isoConferida -Encoding UTF8 | Select-Object -First 1) -eq $isoUsada)) {
    try { Conferir-Iso $isoUsada $isoConferida }
    catch { Msg "não foi possível conferir a .iso inteira ($($_.Exception.Message)); seguindo" }
}

# ---------------------------------------------------------------------------- 3. tradução
Etapa 3 '3_traducao' {
    Exec "cd /d `"$R`" && `"$SDK\bin\rexglue.exe`" codegen ${PROJETO}_manifest.toml"
    # the translated code's atomic sections locked by their address, not by the runtime's global lock (guest_lock.h)
    Exec "powershell -NoProfile -ExecutionPolicy Bypass -File `"$R\scripts\reservas.ps1`" -Pasta `"$R\generated\default`""
}

# ---------------------------------------------------------------------------- 4. compilação
Etapa 4 '4_compilar' {
    $vs = Achar-VS
    if (-not (Tem-SdkWindows)) { throw $ERRO_SDK_WINDOWS }
    $build = Join-Path $R 'out\build\win-amd64-release'
    # files dated in the future (a zip keeps its maker's local time, with no time zone: west of it, the SDK's files
    # came out up to hours ahead; report 1359) keep build.ninja always out of date and ninja gives up: they are dated
    # now. The port's own files and the generated ones too, should the clock have been wrong when they were written.
    $agora = Get-Date
    $futuros = 0
    foreach ($pasta in $SDK, (Join-Path $R 'generated'), (Join-Path $R 'src'), (Join-Path $R 'config')) {
        if (-not (Test-Path -LiteralPath $pasta)) { continue }
        Get-ChildItem -LiteralPath $pasta -File -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.LastWriteTime -gt $agora } |
            ForEach-Object { $_.LastWriteTime = $agora; $futuros++ }
    }
    foreach ($f in 'CMakeLists.txt', 'CMakePresets.json', 'port.cmake') {
        $i = Get-Item -LiteralPath (Join-Path $R $f) -ErrorAction SilentlyContinue
        if ($i -and $i.LastWriteTime -gt $agora) { $i.LastWriteTime = $agora; $futuros++ }
    }
    if ($futuros) { Anotar $script:LOG "$futuros arquivo(s) com data no futuro: datados de agora" }
    # configured again (the SDK or the compiler may have changed), the rest of the folder kept: objects still good
    # are reused, and nothing the game wrote there is lost
    Remove-Item -Recurse -Force (Join-Path $build 'CMakeCache.txt'), (Join-Path $build 'CMakeFiles') -ErrorAction SilentlyContinue
    # a script of its own, one command a line: cmd reads each line when it runs it, so the PATH the Visual Studio's
    # environment sets is the one the next lines see (and the whole build's output goes to the step's log)
    $script = Join-Path $OUT 'compilar.cmd'
    # the paths CMake is given go with forward slashes: it writes the compiler's path as given into a .cmake file of its
    # own, and CMake 3.26 (the one older Visual Studio 2022 Build Tools bring) reads a backslash there as an escape,
    # "Invalid character escape '\O'" (report 1491). Forward slashes serve every version.
    $llvmCm = $LLVM -replace '\\', '/'
    $sdkCm = $SDK -replace '\\', '/'
    # a PATH entry in quotes ("C:\Program Files (x86)\...", left by some installers) turns the parentheses loose in
    # VsDevCmd.bat's if blocks: "\NVIDIA was unexpected at this time" (report 1392). Windows reads PATH without them.
    $linhas = @(
        '@echo off',
        'set "PATH=%PATH:"=%"',
        "call `"$vs\Common7\Tools\VsDevCmd.bat`" -arch=amd64 -host_arch=amd64 >nul",
        "set `"PATH=$LLVM\bin;%PATH%`"",
        "cd /d `"$R`"",
        "cmake --preset win-amd64-release -DCMAKE_C_COMPILER=`"$llvmCm/bin/clang.exe`" -DCMAKE_CXX_COMPILER=`"$llvmCm/bin/clang++.exe`" -DCMAKE_PREFIX_PATH=`"$sdkCm`" -DPORT_CPU_NATIVO=ON || exit /b 1",
        "cmake --build out\build\win-amd64-release --parallel $Jobs || exit /b 1"
    )
    [IO.File]::WriteAllLines($script, $linhas, (New-Object Text.UTF8Encoding($false)))
    Exec "`"$script`""
    if (-not (Test-Path (Join-Path $build "$PROJETO.exe"))) { throw "a compilação não gerou o $PROJETO.exe" }
}

# ---------------------------------------------------------------------------- 5. pronto
Etapa 5 '5_jogar' {
    $iso = (Get-Content (Join-Path $OUT 'iso.txt') -Encoding UTF8 | Select-Object -First 1)
    $jogar = Join-Path $R 'jogar.cmd'
    $linhas = @(
        '@echo off',
        "rem $($JOGO['titulo']): o port, lendo a .iso direto. Opções: opcoes.toml aqui, ou --nome valor. Saves em saves\.",
        'chcp 65001 >nul',
        'set "PORT_OPCOES=%~dp0opcoes.toml"',
        "start `"`" `"%~dp0out\build\win-amd64-release\$PROJETO.exe`" --game_data_root `"%~dp0assets`" --iso `"$iso`" --user_data_root `"%~dp0saves`" --cache_root `"%~dp0cache`" %*"
    )
    [IO.File]::WriteAllLines($jogar, $linhas, (New-Object Text.UTF8Encoding($false)))
    Msg "pronto: $jogar"
}

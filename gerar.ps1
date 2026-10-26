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
  Os logs de cada etapa ficam em out\gerar\. O jogo e a versão esperada estão em config\jogo.txt.

.PARAMETER Iso
  A .iso do jogo (Xbox 360).
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
# the ReXGlue SDK of the nightly 0.10.0.24 (bd833a2) built with AMD FidelityFX (FSR, CAS), which the official one
# lacks, and the ports' fixes (otavix010/rexglue-sdk-ffx: the 3D-as-2D texture at a resolution scale above 1x)
$SDK_VERSAO = '0.10.0.24-ffx2'
$SDK_URL = 'https://github.com/otavix010/rexglue-sdk-ffx/releases/download/v0.10.0.24-ffx2/rexglue-sdk-0.10.0.24-ffx2-win-amd64.zip'
$SDK_SHA256 = '9aad4b00c032ebe79ac6487ab88c255f7902f37ed3899306f2692caf2820af4e'
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
    Exec "curl.exe -L --fail --retry 3 -o `"$tmp`" `"$url`""
    Move-Item -Force $tmp $destino
}

# the Visual Studio with the C++ tools (MSVC's headers and libraries, the Windows SDK, CMake and Ninja)
function Achar-VS {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { return $null }
    $p = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 Microsoft.VisualStudio.Component.VC.CMake.Project -property installationPath
    if ($p) { return ($p | Select-Object -First 1) }
    return $null
}

# ---------------------------------------------------------------------------- Xbox 360 disc images (XDVDFS)
# The volume descriptor sits 0x10000 into the game partition, which starts at one of these offsets
# (a trimmed xiso, XGD1, XGD2, XGD3).
function Ler-Xex([string]$iso, [string]$destino) {
    $f = [IO.File]::OpenRead($iso)
    try {
        $r = New-Object IO.BinaryReader($f)
        $base = $null
        foreach ($b in @(0, 0x18300000, 0xFD90000, 0x2080000)) {
            if ($b + 0x10000 + 28 -gt $f.Length) { continue }
            $f.Position = $b + 0x10000
            if ([Text.Encoding]::ASCII.GetString($r.ReadBytes(20)) -eq 'MICROSOFT*XBOX*MEDIA') { $base = $b; break }
        }
        if ($null -eq $base) { throw "não é uma imagem de disco do Xbox 360: $iso" }
        $raizSetor = $r.ReadUInt32(); $raizTam = $r.ReadUInt32()
        $f.Position = $base + [int64]$raizSetor * 2048
        $dir = $r.ReadBytes($raizTam)
        # the root directory is a binary tree of entries: left, right (in dwords), sector, size, attributes, name
        $pilha = New-Object System.Collections.Stack
        $pilha.Push(0)
        $vistos = @{}
        while ($pilha.Count) {
            $o = $pilha.Pop()
            if ($vistos[$o] -or $o + 14 -gt $dir.Length) { continue }
            $vistos[$o] = $true
            $esq = [BitConverter]::ToUInt16($dir, $o); $dirt = [BitConverter]::ToUInt16($dir, $o + 2)
            if ($esq -eq 0xFFFF -and $dirt -eq 0xFFFF) { continue }
            $setor = [BitConverter]::ToUInt32($dir, $o + 4); $tam = [BitConverter]::ToUInt32($dir, $o + 8)
            $nome = [Text.Encoding]::ASCII.GetString($dir, $o + 14, $dir[$o + 13])
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
        throw "a .iso não tem default.xex: $iso"
    } finally { $f.Close() }
}

# ---------------------------------------------------------------------------- 1. ferramentas
# done before but the tools this generator wants are not there (a newer SDK, a deleted folder): again
if (-not (Test-Path (Join-Path $SDK 'bin\rexglue.exe')) -or -not (Test-Path (Join-Path $LLVM 'bin\clang++.exe'))) {
    Remove-Item (Join-Path $OUT '1_ferramentas.ok') -ErrorAction SilentlyContinue
}
Etapa 1 '1_ferramentas' {
    $vs = Achar-VS
    if (-not $vs) {
        throw "falta o Visual Studio Build Tools com C++ e CMake. Instale com:`r`n  winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override `"--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.CMake.Project --includeRecommended`""
    }
    Anotar $script:LOG "Visual Studio: $vs"
    New-Item -ItemType Directory -Force $FERRAMENTAS | Out-Null
    if (-not (Test-Path (Join-Path $SDK 'bin\rexglue.exe'))) {
        $zip = Join-Path $FERRAMENTAS "rexglue-sdk-$SDK_VERSAO.zip"
        if (-not (Test-Path $zip) -or (Get-FileHash $zip -Algorithm SHA256).Hash -ne $SDK_SHA256) { Baixar $SDK_URL $zip }
        if ((Get-FileHash $zip -Algorithm SHA256).Hash -ne $SDK_SHA256) { throw "o SDK baixado não confere (SHA-256): $zip" }
        $tmp = Join-Path $FERRAMENTAS 'sdk.tmp'
        Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
        Expand-Archive $zip $tmp
        Remove-Item -Recurse -Force $SDK -ErrorAction SilentlyContinue
        Move-Item (Join-Path $tmp 'win-amd64') $SDK
        Remove-Item -Recurse -Force $tmp, $zip
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
            Remove-Item -Recurse -Force $tmp, $zip
        }
        $pacote = Join-Path $FERRAMENTAS "llvm-$LLVM_VERSAO.tar.zst"
        if (-not (Test-Path $pacote) -or (Get-FileHash $pacote -Algorithm SHA256).Hash -ne $LLVM_SHA256) { Baixar $LLVM_URL $pacote }
        if ((Get-FileHash $pacote -Algorithm SHA256).Hash -ne $LLVM_SHA256) { throw "o LLVM baixado não confere (SHA-256): $pacote" }
        $tmp = Join-Path $FERRAMENTAS 'llvm.tmp'
        Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
        New-Item -ItemType Directory $tmp | Out-Null
        # only what a port's build runs: the compiler, the linker, the archiver and the compiler's own headers
        # (about 370 MB of the package's 4 GB)
        Msg 'extraindo o LLVM ...'
        $quais = @('*/bin/clang.exe', '*/bin/clang++.exe', '*/bin/lld-link.exe', '*/bin/llvm-ar.exe', '*/bin/llvm-ranlib.exe', '*/lib/clang/*') |
            ForEach-Object { "--include `"$_`"" }
        Exec "`"$zstd`" -dc --long=31 `"$pacote`" | `"$env:SystemRoot\System32\tar.exe`" -xf - -C `"$tmp`" $($quais -join ' ')"
        Remove-Item -Recurse -Force $LLVM -ErrorAction SilentlyContinue
        Move-Item (Get-ChildItem $tmp -Directory | Select-Object -First 1).FullName $LLVM
        if (-not (Test-Path (Join-Path $LLVM 'bin\clang++.exe'))) { throw 'o LLVM extraído não tem o clang++' }
        Remove-Item -Recurse -Force $tmp, $pacote
    }
}

# ---------------------------------------------------------------------------- 2. jogo
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
    if (-not (Test-Path -LiteralPath $Iso)) { throw "a .iso não existe: $Iso" }
    $assets = Join-Path $R 'assets'
    New-Item -ItemType Directory -Force $assets | Out-Null
    $xex = Join-Path $assets 'default.xex'
    # read beside it and checked first: a wrong .iso leaves the good default.xex in place
    $lido = "$xex.novo"
    Ler-Xex $Iso $lido
    $sha = (Get-FileHash $lido -Algorithm SHA1).Hash.ToLower()
    if ($sha -ne $JOGO['sha1_xex']) {
        Remove-Item $lido -ErrorAction SilentlyContinue
        throw "esta .iso não é a versão do port ($($JOGO['versao']), default.xex SHA-1 $($JOGO['sha1_xex']); a sua: $sha)"
    }
    Move-Item -Force $lido $xex
    Set-Content -Path (Join-Path $OUT 'iso.txt') -Value $Iso -Encoding UTF8
}

# ---------------------------------------------------------------------------- 3. tradução
Etapa 3 '3_traducao' {
    Exec "cd /d `"$R`" && `"$SDK\bin\rexglue.exe`" codegen ${PROJETO}_manifest.toml"
}

# ---------------------------------------------------------------------------- 4. compilação
Etapa 4 '4_compilar' {
    $vs = Achar-VS
    $build = Join-Path $R 'out\build\win-amd64-release'
    # configured again (the SDK or the compiler may have changed), the rest of the folder kept: objects still good
    # are reused, and nothing the game wrote there is lost
    Remove-Item -Recurse -Force (Join-Path $build 'CMakeCache.txt'), (Join-Path $build 'CMakeFiles') -ErrorAction SilentlyContinue
    # a script of its own, one command a line: cmd reads each line when it runs it, so the PATH the Visual Studio's
    # environment sets is the one the next lines see (and the whole build's output goes to the step's log)
    $script = Join-Path $OUT 'compilar.cmd'
    $linhas = @(
        '@echo off',
        "call `"$vs\Common7\Tools\VsDevCmd.bat`" -arch=amd64 -host_arch=amd64 >nul",
        "set `"PATH=$LLVM\bin;%PATH%`"",
        "cd /d `"$R`"",
        "cmake --preset win-amd64-release -DCMAKE_C_COMPILER=`"$LLVM\bin\clang.exe`" -DCMAKE_CXX_COMPILER=`"$LLVM\bin\clang++.exe`" -DCMAKE_PREFIX_PATH=`"$SDK`" -DPORT_CPU_NATIVO=ON || exit /b 1",
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

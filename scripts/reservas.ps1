<#
.SYNOPSIS
The translated code's atomic sections locked by the address they update, not by the runtime's global lock (see
src/port/guest_lock.h). Run by the build right after the code generation (port.cmake): in each generated file, a
section's REX_ENTER_GLOBAL_LOCK() moves below its lwarx/ldarx's address and becomes REX_ENTER_RESERVATION(ea):

    REX_ENTER_GLOBAL_LOCK();         // lwarx r9,0,r10
    // lwarx r9,0,r10           ->   ea = ctx.r10.u32;
    ea = ctx.r10.u32;                REX_ENTER_RESERVATION(ea);

A section of another shape keeps the global lock. A file is written only when it changes.
#>
param([Parameter(Mandatory = $true)][string]$Pasta)

$ErrorActionPreference = 'Stop'
$padrao = [regex]'(?m)^([ \t]*)REX_ENTER_GLOBAL_LOCK\(\);(\r?\n)([ \t]*// l[wd]arx [^\r\n]*\r?\n)([ \t]*ea = [^\r\n;]*;\r?\n)'
$utf8 = New-Object System.Text.UTF8Encoding($false)
$feitas = 0
$restam = 0
foreach ($f in Get-ChildItem -LiteralPath $Pasta -Filter '*.cpp') {
    $texto = [IO.File]::ReadAllText($f.FullName)
    if ($texto.IndexOf('REX_ENTER_GLOBAL_LOCK();') -lt 0) { continue }
    $n = 0
    $novo = $padrao.Replace($texto, {
        param($m)
        $script:n++
        $m.Groups[3].Value + $m.Groups[4].Value + $m.Groups[1].Value + 'REX_ENTER_RESERVATION(ea);' + $m.Groups[2].Value
    })
    $feitas += $n
    $restam += ([regex]::Matches($novo, 'REX_ENTER_GLOBAL_LOCK\(\);')).Count
    if ($n -gt 0) { [IO.File]::WriteAllText($f.FullName, $novo, $utf8) }
}
Write-Output "reservas: $feitas secoes pelo endereco, $restam com a trava global"

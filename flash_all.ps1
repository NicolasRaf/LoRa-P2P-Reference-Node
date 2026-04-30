$picotool = "${env:USERPROFILE}\.pico-sdk\picotool\2.2.0-a4\picotool\picotool.exe"

Write-Host "Procurando placas Raspberry Pi Pico..."
$info = & $picotool info 2>&1
$infoString = $info | Out-String

$regex = 'bus (\d+), address (\d+)'
$matches = [regex]::Matches($infoString, $regex)

if ($matches.Count -eq 0) {
    Write-Host "Nenhuma placa encontrada (ou todas ja estao em modo BOOTSEL)!"
    
    # Se ja estiverem em BOOTSEL, tenta gravar pelo menos em uma
    Write-Host "Tentando gravar de forma generica..."
    & $picotool load "build/LoRa_P2P.elf" -x
    exit 0
}

Write-Host "Encontradas $($matches.Count) placa(s) em modo normal."

foreach ($match in $matches) {
    $bus = $match.Groups[1].Value
    $addr = $match.Groups[2].Value
    Write-Host "---------------------------------------------------"
    Write-Host "Processando placa em bus $bus address $addr..."
    
    # 1. Coloca apenas ESTA placa em modo BOOTSEL
    & $picotool reboot --bus $bus --address $addr -f -u | Out-Null
    
    # 2. Aguarda o Windows montar a placa em modo BOOTSEL
    Write-Host "   Aguardando entrada em modo de gravacao..."
    Start-Sleep -Seconds 2
    
    # 3. Grava o firmware (como so tem ela em BOOTSEL, o load funciona direto)
    Write-Host "   Gravando firmware..."
    $loadResult = & $picotool load "build/LoRa_P2P.elf" -x 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "   Sucesso! A placa foi reiniciada e o codigo esta rodando."
    } else {
        Write-Host "   Erro ao gravar: $loadResult"
    }
    
    # 4. Aguarda ela religar antes de passar pra proxima (para nao embolar os USBs)
    Start-Sleep -Seconds 2
}

Write-Host "---------------------------------------------------"
Write-Host "Todas as placas foram atualizadas com sucesso!"

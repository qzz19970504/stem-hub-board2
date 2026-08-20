$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$outputDirectory = Join-Path $repositoryRoot 'build\host-tests'
$includeDirectory = Join-Path $repositoryRoot 'App\Inc'
$sourceDirectory = Join-Path $repositoryRoot 'App\Src'
$compiler = (Get-Command gcc -ErrorAction Stop).Source

New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

$testCases = @(
    @{ Name = 'test_at_protocol'; Sources = @('app_at_protocol.c') },
    @{ Name = 'test_line_reader'; Sources = @('app_line_reader.c') },
    @{ Name = 'test_ring_buffer'; Sources = @('app_ring_buffer.c') },
    @{ Name = 'test_output_math'; Sources = @('app_output_math.c') },
    @{ Name = 'test_pwm_fade'; Sources = @('app_pwm_fade.c') },
    @{ Name = 'test_settings_record'; Sources = @('app_settings_record.c') },
    @{ Name = 'test_uart_tunnel'; Sources = @('app_uart_tunnel.c') },
    @{ Name = 'test_transparent_mode'; Sources = @('app_transparent_mode.c') },
    @{ Name = 'test_output_state'; Sources = @('app_output_state.c') },
    @{ Name = 'test_status'; Sources = @('app_status.c', 'app_output_state.c') }
)

foreach ($testCase in $testCases) {
    $testSource = Join-Path $PSScriptRoot ($testCase.Name + '.c')
    $applicationSources = foreach ($source in $testCase.Sources) {
        Join-Path $sourceDirectory $source
    }
    $executable = Join-Path $outputDirectory ($testCase.Name + '.exe')
    $arguments = @(
        '-std=c11',
        '-Wall',
        '-Wextra',
        '-Werror',
        '-I', $includeDirectory,
        $testSource
    ) + $applicationSources + @('-o', $executable)

    & $compiler @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed for $($testCase.Name)"
    }

    & $executable
    if ($LASTEXITCODE -ne 0) {
        throw "Test failed: $($testCase.Name)"
    }

    Write-Output "PASS $($testCase.Name)"
}

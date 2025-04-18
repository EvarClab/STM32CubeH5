@ECHO OFF
call ../env.bat
:: Environment variable for setting postbuild command with AppliCfg
set "projectdir=%~dp0"

:: Enable delayed expansion
setlocal EnableDelayedExpansion

:: External script
set ob_flash_programming="ob_flash_programming.bat"

if "%isGeneratedByCubeMX%" == "true" (
set appli_dir=%stirot_boot_path_project%
) else (
set appli_dir=../../%stirot_boot_path_project%
)

:: IAR project Appli TrustZone files
set icf_tz_s_file="%appli_dir%\EWARM\Secure\stm32h533xx_flash_s.icf"
set icf_tz_ns_file="%appli_dir%\EWARM\NonSecure\stm32h533xx_flash_ns.icf"

:: CubeIDE project Appli TrustZone files
set ld_tz_s_file="%appli_dir%\STM32CubeIDE\Secure\STM32H533RETx_FLASH.ld"
set ld_tz_ns_file="%appli_dir%\STM32CubeIDE\NonSecure\STM32H533RETx_FLASH.ld"

:: KEIL project Appli TrustZone files
set sct_tz_s_file="%appli_dir%\MDK-ARM\Secure\stm32h533xx_s.sct"
set sct_tz_ns_file="%appli_dir%\MDK-ARM\NonSecure\stm32h533xx_ns.sct"

:: CubeIDE project Appli Full Secure files
set cube_appli_postbuild="%appli_dir%\STM32CubeIDE\postbuild.sh"
set ld_file="%appli_dir%\STM32CubeIDE\STM32H533RETx_FLASH.ld"

:: IAR project Appli Full Secure files
set iar_appli_postbuild="%appli_dir%\EWARM\postbuild.bat"
set icf_file="%appli_dir%\EWARM\stm32h533xx_flash.icf"

:: KEIL project Appli Full Secure files
set keil_appli_postbuild="%appli_dir%\MDK-ARM\postbuild.bat"
set sct_file="%appli_dir%\MDK-ARM\stm32h5xx_app.sct"

set main_h="%appli_dir%\Inc\main.h"
set s_main_h="%appli_dir%\Secure\Inc\main.h"

set obk_cfg_file="%projectdir%Config\STiRoT_Config.xml"

:: Switch use case project Application TrustZone or Full secure
set Full_secure=1

:: General section need
set code_size="Firmware area size"
set code_offset="Firmware execution area offset"
set data_image_en="Number of images managed"
set secure_code_size="Size of the secure area"

:start
goto exe:
goto py:
:exe
::line for Windows executable
set "applicfg=%cube_fw_path%\Utilities\PC_Software\ROT_AppliConfig\dist\AppliCfg.exe"
set "python="
if exist %applicfg% (
echo run config Appli with Windows executable
goto update
)
:py
::called if we just want to use AppliCfg python (think to comment "goto exe:")
set "applicfg=%cube_fw_path%\Utilities\PC_Software\ROT_AppliConfig\AppliCfg.py"
set "python=python "

:update
set "AppliCfg=%python%%applicfg%"

:: ================================================ Updating test Application files ========================================================

if /i "%Full_secure%" == "1" (

if exist "%icf_file%" (
    set "action=Updating Linker .icf file"
    echo !action!
    echo ICF %icf_file%

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %code_size% -n S_CODE_SIZE --vb %icf_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET --vb %icf_file%
    if !errorlevel! neq 0 goto :error
)

if exist "%ld_file%" (
    set "action=Updating Linker .ld file"
    echo !action!
    echo LD %ld_file%

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %code_size% -n S_CODE_SIZE --vb %ld_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET --vb %ld_file%
    if !errorlevel! neq 0 goto :error
)

if exist "%sct_file%" (
    set "action=Updating Linker .sct file"
    echo !action!
    echo SCT %sct_file%

    %AppliCfg% definevalue -xml %obk_cfg_file% -nxml %code_size% -n S_CODE_SIZE --vb %sct_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% definevalue -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET --vb %sct_file%
    if !errorlevel! neq 0 goto :error
)

set "action=Updating Data Image Define Flag"
%AppliCfg% setdefine -xml %obk_cfg_file% -nxml %data_image_en% -n DATA_IMAGE_EN -v 0x02 --vb %main_h%
REM Only applicable for STIROT_Appli example => error status is not checked

%AppliCfg% definevalue -xml %obk_cfg_file% -nxml %code_size% -n S_CODE_SIZE --vb %main_h%
if !errorlevel! neq 0 goto :error

%AppliCfg% definevalue -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET --vb %main_h%
if !errorlevel! neq 0 goto :error

echo stm32h533xx_flash.icf successfully updated according to STiRoT_Config.obk
IF [%1] NEQ [AUTO] cmd /k
exit 0
)

if /i "%Full_secure%" == "0" (

set "action=Update appli postbuild"
%AppliCfg% flash -xml %obk_cfg_file% -nxml %secure_code_size% -b image_size %iar_appli_postbuild% --vb
if !errorlevel! neq 0 goto :error

%AppliCfg% flash -xml %obk_cfg_file% -nxml %secure_code_size% -b image_size %cube_appli_postbuild% --vb
if !errorlevel! neq 0 goto :error

%AppliCfg% flash -xml %obk_cfg_file% -nxml %secure_code_size% -b image_size %keil_appli_postbuild% --vb
if !errorlevel! neq 0 goto :error

if exist "%icf_tz_s_file%" (
    set "action=Updating Linker .icf secure file"
    echo !action!
    echo ICF %icf_tz_s_file%

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %secure_code_size% -n S_CODE_SIZE --vb %icf_tz_s_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET --vb %icf_tz_s_file%
    if !errorlevel! neq 0 goto :error
)

if exist "%ld_tz_s_file%" (
    set "action=Updating Linker .ld secure file"
    echo !action!
    echo LD %ld_tz_s_file%

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %secure_code_size% -n S_CODE_SIZE --vb %ld_tz_s_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET --vb %ld_tz_s_file%
    if !errorlevel! neq 0 goto :error
)

if exist "%sct_tz_s_file%" (
    set "action=Updating Linker .sct secure file"
    echo !action!
    echo SCT %sct_tz_s_file%

    %AppliCfg% definevalue -xml %obk_cfg_file% -nxml %secure_code_size% -n S_CODE_SIZE --vb %sct_tz_s_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% definevalue -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET --vb %sct_tz_s_file%
    if !errorlevel! neq 0 goto :error
)

if exist "%icf_tz_ns_file%" (
    set "action=Updating Linker .icf non secure file"
    echo !action!
    echo ICF %icf_tz_ns_file%

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %secure_code_size% -n S_CODE_SIZE --vb %icf_tz_ns_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET --vb %icf_tz_ns_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %secure_code_size% -n NS_CODE_SIZE -e "(cons1 - val1)" -cons "0x00020000" --vb %icf_tz_ns_file%
    if !errorlevel! neq 0 goto :error
)

if exist "%ld_tz_ns_file%" (
    set "action=Updating Linker .ld non secure file"
    echo !action!
    echo LD %ld_tz_ns_file%

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %secure_code_size% -n S_CODE_SIZE --vb %ld_tz_ns_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET --vb %ld_tz_ns_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% linker -xml %obk_cfg_file% -nxml %secure_code_size% -n NS_CODE_SIZE -e "(cons1 - val1)" -cons "0x00020000" --vb %ld_tz_ns_file%
    if !errorlevel! neq 0 goto :error
)

if exist "%sct_tz_ns_file%" (
    set "action=Updating Linker .sct non secure file"
    echo !action!
    echo SCT %sct_tz_ns_file%

    %AppliCfg% definevalue -xml %obk_cfg_file% -nxml %secure_code_size% -n S_CODE_SIZE --vb %sct_tz_ns_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% definevalue -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET --vb %sct_tz_ns_file%
    if !errorlevel! neq 0 goto :error

    %AppliCfg% definevalue -xml %obk_cfg_file% -nxml %secure_code_size% -n NS_CODE_SIZE -e "(cons1 - val1)" -cons "0x00020000" --vb %sct_tz_ns_file%
    if !errorlevel! neq 0 goto :error
)

%AppliCfg% definevalue -xml %obk_cfg_file% -nxml %secure_code_size% -n S_CODE_SIZE %s_main_h% --vb
if !errorlevel! neq 0 goto :error

%AppliCfg% definevalue -xml %obk_cfg_file% -nxml %code_offset% -n S_CODE_OFFSET %s_main_h% --vb
if !errorlevel! neq 0 goto :error

%AppliCfg% definevalue -xml %obk_cfg_file% -nxml %secure_code_size% -n NS_CODE_SIZE -e "(cons1 - val1)" -cons "0x00020000" %s_main_h% --vb
if !errorlevel! neq 0 goto :error

echo stm32h533xx_flash.icf, STM32H533RETx_FLASH.ld and main.h successfully updated according to STiRoT_Config.obk
IF [%1] NEQ [AUTO] cmd /k
exit 0
)



:error
echo        Error when trying to "%action%" >CON
echo        Update script aborted >CON
IF [%1] NEQ [AUTO] cmd /k
exit 1

################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
F:/githublocally/zigbeeember572/app/framework/plugin/heartbeat/heartbeat.c 

OBJS += \
./heartbeat/heartbeat.o 

C_DEPS += \
./heartbeat/heartbeat.d 


# Each subdirectory must supply rules for building sources it contributes
heartbeat/heartbeat.o: F:/githublocally/zigbeeember572/app/framework/plugin/heartbeat/heartbeat.c
	@echo 'Building file: $<'
	@echo 'Invoking: IAR C/C++ Compiler for ARM'
	iccarm "$<" -o "$@" --no_path_in_file_macros --separate_cluster_for_initialized_variables --no_wrap_diagnostics -I"F:/githublocally/zigbeeember572//submodules/Device/SiliconLabs/EFR32MG1P/Include" -I"F:/githublocally/zigbeeember572//submodules/kits/common/bsp" -I"F:/githublocally/zigbeeember572//submodules/kits/EFR32MG1_BRD4151A/config" -I"F:/githublocally/zigbeeember572//app/builder/test_ha" -I"F:/githublocally/zigbeeember572//stack" -I"F:/githublocally/zigbeeember572//app/framework/include" -I"F:/githublocally/zigbeeember572//app/util" -I"F:/githublocally/zigbeeember572//hal" -I"F:/githublocally/zigbeeember572/" -I"F:/githublocally/zigbeeember572//hal//micro/cortexm3/efm32" -I"F:/githublocally/zigbeeember572//hal//micro/cortexm3/efm32/efr32" -I"F:/githublocally/zigbeeember572//submodules/CMSIS/Include" -I"F:/githublocally/zigbeeember572//submodules/emlib/inc" -I"F:/githublocally/zigbeeember572//submodules/emdrv/common/inc" -I"F:/githublocally/zigbeeember572//submodules/emdrv/sleep/inc" -I"F:/githublocally/zigbeeember572//submodules/emdrv/aesdrv/inc" -I"F:/githublocally/zigbeeember572//submodules/emdrv/aesdrv/src" -I"F:/githublocally/zigbeeember572//submodules/emdrv/trace/inc" -I"F:/githublocally/zigbeeember572//submodules/emdrv/gpiointerrupt/inc" -I"F:/githublocally/zigbeeember572//submodules/emdrv/ecode" -I"F:/githublocally/zigbeeember572//hal//micro/cortexm3/efm32/config" -I"F:/githublocally/zigbeeember572//submodules/emdrv/uartdrv/inc" -I"F:/githublocally/zigbeeember572//submodules/emdrv/spidrv/inc" -I"F:/githublocally/zigbeeember572//submodules/emdrv/dmadrv/inc" -I"F:/githublocally/zigbeeember572//submodules/emdrv/ustimer/inc" -I"F:/githublocally/zigbeeember572//submodules/emdrv/rtcdrv/inc" -I"F:/githublocally/zigbeeember572//submodules/kits/SLWSTK6100A_EFR32MG/config" -I"F:/githublocally/zigbeeember572//submodules/kits/common/drivers" -I"F:/githublocally/zigbeeember572//submodules/efrseq/rfprotocol/ieee802154/cortex" -I"F:/githublocally/zigbeeember572//submodules/efrseq/common/cortex" -I"F:/githublocally/zigbeeember572//submodules/s029_efr4fpga_drv/inc" -I"F:/githublocally/zigbeeember572//submodules/reptile/glib" -I"F:/githublocally/zigbeeember572//submodules/reptile/glib/glib" -I"F:/githublocally/zigbeeember572//submodules/reptile/glib/dmd" -I"F:/githublocally/zigbeeember572//submodules/emdrv/tempdrv/inc" -e --use_c++_inline --cpu Cortex-M4F --fpu VFPv4_sp --debug --dlib_config "C:\Program Files (x86)\IAR Systems\Embedded Workbench 7.2\arm\inc\c\DLib_Config_Normal.h" --endian little --cpu_mode thumb -Ohz '-DCORTEXM3_EFR32MG1P232F256GM48_MICRO=1' '-DCORTEXM3_EFR32MG1P232F256GM48=1' '-DPLATFORM_HEADER="hal/micro/cortexm3/compiler/iar.h"' '-DZA_GENERATED_HEADER="app/builder/test_ha/test_ha.h"' '-DGENERATED_TOKEN_HEADER="app/builder/test_ha/test_ha_tokens.h"' '-DATTRIBUTE_STORAGE_CONFIGURATION="app/builder/test_ha/test_ha_endpoint_config.h"' '-DBOARD_HEADER="app/builder/test_ha/test_ha_board.h"' '-DNULL_BTL=1' '-DEFR32MG1P=1' '-DEFR32MG1P232F256GM48=1' '-DBOARD_BRD4151A=1' '-DEMBER_AF_API_EMBER_TYPES="stack/include/ember-types.h"' '-DCORTEXM3=1' '-DCONFIGURATION_HEADER="app/framework/util/config.h"' '-DAPPLICATION_TOKEN_HEADER="app/framework/util/tokens.h"' '-DCORTEXM3_EFM32_MICRO=1' '-DCORTEXM3_EFR32_MICRO=1' '-DCORTEXM3_EFR32=1' '-DPHY_EFR32=1' --diag_suppress Pa050 --dependencies=m "$(basename $(notdir $<)).d"
	@echo 'Finished building: $<'
	@echo ' '



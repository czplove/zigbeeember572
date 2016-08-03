################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include simple-main/subdir.mk
-include sim-eeprom1/EFR32/subdir.mk
-include scenes/subdir.mk
-include scan-dispatch/subdir.mk
-include network-find/subdir.mk
-include idle-sleep/subdir.mk
-include identify-feedback/subdir.mk
-include identify/subdir.mk
-include heartbeat/subdir.mk
-include groups-server/subdir.mk
-include form-and-join/subdir.mk
-include end-device-support/subdir.mk
-include emlib/subdir.mk
-include ember-minimal-printf/subdir.mk
-include efr32/subdir.mk
-include door-lock-server/subdir.mk
-include counters/subdir.mk
-include antenna-stub/subdir.mk
-include address-table/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(S79_DEPS)),)
-include $(S79_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(MSA_UPPER_DEPS)),)
-include $(MSA_UPPER_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(MSA_DEPS)),)
-include $(MSA_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S79_UPPER_DEPS)),)
-include $(S79_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: test_ha.out

# Tool invocations
test_ha.out: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: IAR Linker for ARM'
	ilinkarm $(OBJS) $(USER_OBJS) -o "test_ha.out" --config "F:/githublocally/zigbeeember572/hal/micro/cortexm3/efm32/efr32/iar-cfg.icf" --config_def NULL_BTL=1 --no_wrap_diagnostics "F:/githublocally/zigbeeember572//build/debug-basic-library-cortexm3-iar-efr32mg1p-efr32/debug-basic-library.a" "F:/githublocally/zigbeeember572//build/zll-stub-library-cortexm3-iar-efr32mg1p-efr32/zll-stub-library.a" "F:/githublocally/zigbeeember572//build/cbke-library-dsa-sign-stub-cortexm3-iar-efr32mg1p-efr32/cbke-library-dsa-sign-stub.a" "F:/githublocally/zigbeeember572//build/cbke-library-dsa-verify-stub-cortexm3-iar-efr32mg1p-efr32/cbke-library-dsa-verify-stub.a" "F:/githublocally/zigbeeember572//build/alarm-stub-library-cortexm3-iar-efr32mg1p-efr32/alarm-stub-library.a" "F:/githublocally/zigbeeember572//build/debug-extended-stub-library-cortexm3-iar-efr32mg1p-efr32/debug-extended-stub-library.a" "F:/githublocally/zigbeeember572//build/end-device-bind-stub-library-cortexm3-iar-efr32mg1p-efr32/end-device-bind-stub-library.a" "F:/githublocally/zigbeeember572//build/cbke-283k1-stub-library-cortexm3-iar-efr32mg1p-efr32/cbke-283k1-stub-library.a" "F:/githublocally/zigbeeember572//build/packet-validate-library-cortexm3-iar-efr32mg1p-efr32/packet-validate-library.a" "F:/githublocally/zigbeeember572//build/zigbee-pro-stack-cortexm3-iar-efr32mg1p-efr32/zigbee-pro-stack.a" "F:/githublocally/zigbeeember572//build/hal-library-cortexm3-iar-efr32mg1p-efr32/hal-library.a" "F:/githublocally/zigbeeember572//build/gp-stub-library-cortexm3-iar-efr32mg1p-efr32/gp-stub-library.a" "F:/githublocally/zigbeeember572//build/install-code-library-stub-cortexm3-iar-efr32mg1p-efr32/install-code-library-stub.a" "F:/githublocally/zigbeeember572//build/cbke-163k1-stub-library-cortexm3-iar-efr32mg1p-efr32/cbke-163k1-stub-library.a" "F:/githublocally/zigbeeember572//build/cbke-stub-library-cortexm3-iar-efr32mg1p-efr32/cbke-stub-library.a" "F:/githublocally/zigbeeember572//build/security-library-core-cortexm3-iar-efr32mg1p-efr32/security-library-core.a" "F:/githublocally/zigbeeember572//build/cbke-library-dsa-verify-283k1-stub-cortexm3-iar-efr32mg1p-efr32/cbke-library-dsa-verify-283k1-stub.a" "F:/githublocally/zigbeeember572//build/binding-table-library-cortexm3-iar-efr32mg1p-efr32/binding-table-library.a" "F:/githublocally/zigbeeember572//build/sim-eeprom1-library-cortexm3-iar-efr32mg1p-efr32/sim-eeprom1-library.a" "F:/githublocally/zigbeeember572//build/multi-network-stub-library-cortexm3-iar-efr32mg1p-efr32/multi-network-stub-library.a" "F:/githublocally/zigbeeember572//build/security-library-link-keys-stub-cortexm3-iar-efr32mg1p-efr32/security-library-link-keys-stub.a" "F:/githublocally/zigbeeember572//build/enhanced-beacon-request-stub-library-cortexm3-iar-efr32mg1p-efr32/enhanced-beacon-request-stub-library.a" "F:/githublocally/zigbeeember572//build/rf4ce-stack-stub-library-cortexm3-iar-efr32mg1p-efr32/rf4ce-stack-stub-library.a" --entry halEntryPoint --map "F:/githublocally/zigbeeember572/app/builder/test_ha/IAR ARM - Debug/test_ha.map" --log_file "F:/githublocally/zigbeeember572/app/builder/test_ha/IAR ARM - Debug/test_ha.log" --log initialization,modules,sections,veneers --diag_suppress=Lp012 --redirect _Printf=_PrintfFull --redirect _Scanf=_ScanfFull  
	ielftool --ihex --verbose "test_ha.out" "test_ha.hex"  
	ielftool --ihex "test_ha.out" "test_ha.hex" 
	ielftool --bin "test_ha.out" "test_ha.bin" 
	ielftool --srec "test_ha.out" "test_ha.s37"
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(S79_DEPS)$(S_DEPS)$(CXX_DEPS)$(MSA_UPPER_DEPS)$(S_UPPER_DEPS)$(ASM_DEPS)$(CC_DEPS)$(MSA_DEPS)$(ASM_UPPER_DEPS)$(S79_UPPER_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS) test_ha.out
	-@echo ' '

post-build:
	-cmd //c F:\githublocally\zigbeeember572/app/builder/test_ha/test_ha.bat test_ha ..
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets

################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: GNU Compiler'
	"C:/ti/ccsv6/tools/compiler/gcc_msp430_4.9.14r1_364/bin/msp430-elf-gcc.exe" -c -mcpu=msp430 -mmcu=msp430g2553 -I"C:/ti/ccsv6/ccs_base/msp430/include_gcc" -I"C:/ti/ccsv6/tools/compiler/gcc_msp430_4.9.14r1_364/msp430-elf/include" -Os -g -gdwarf-3 -gstrict-dwarf -Wall -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '



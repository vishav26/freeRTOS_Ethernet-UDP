# freeRTOS_Ethernet-UDP
This is the implementation of the data exchange via Ethernet/UDP on freeRTOS
The project contents here is the implementation of a real-time system (RTOS). The speed data captured from the fan is transferred via Ethernet/UDP to a simulated PID Controller which generates the output accordingly. This output is fed back to the board via Ethernet/UDP which in turn is used to drive the fan. The complete implementation has been done using open source freeRTOS. The Micro-controller used is a Xilinx Zynq 7000 Soc FPGA. 
The Counter and PWM used in the project are designed using verilog. Vivado 2016.4 is used for hardware design and Xilinx SDK for writing source code and debugging. 

#include <stdio.h>                   // Standard I/O operations
#include <complex.h>                 // For complex number operations
#include <stdlib.h>                  // General utilities
#include "platform.h"                // Platform-specific initialization
#include "xil_printf.h"              // Xilinx-specific printf implementation
#include <xtime_l.h>                 // Time measurement utilities
#include "xparameters.h"             // Definitions for Xilinx hardware parameters
#include "xaxidma.h"                 // AXI DMA driver
#include "dma_init.h"                // DMA initialization function

#define N 8                          // Number of FFT points

// Reverse bit order for 8-point FFT
const int rev8[N] = {0, 4, 2, 6, 1, 5, 3, 7}; // Permutation for bit-reversed order
const float complex W[N / 2] = {               // Twiddle factors for 8-point FFT
    1 - 0 * I,
    0.7071067811865476 - 0.7071067811865475 * I,
    0.0 - 1 * I,
    -0.7071067811865476 - 0.7071067811865475 * I
};

// Performs bit-reversed ordering of the input data
void bitreverse(const float complex dataIn[N], float complex dataOut[N]) {
    for (int i = 0; i < N; i++) {
        dataOut[i] = dataIn[rev8[i]]; // Rearrange data based on the reverse bit order
    }
}

// Performs the FFT computation stages
void FFT_stages(float complex FFT_input[N], float complex FFT_output[N]) {
    float complex temp1[N], temp2[N];




    // Stage 1: Butterfly computation with stride 2
    for (int i = 0; i < N; i += 2) {
        temp1[i] = FFT_input[i] + FFT_input[i + 1];      // Sum
        temp1[i + 1] = FFT_input[i] - FFT_input[i + 1];  // Difference
    }

    // Stage 2: Butterfly computation with stride 4
    for (int i = 0; i < N; i += 4) {
        for (int j = 0; j < 2; ++j) {
            temp2[i + j] = temp1[i + j] + W[2 * j] * temp1[i + j + 2];      // Sum with twiddle factor
            temp2[i + 2 + j] = temp1[i + j] - W[2 * j] * temp1[i + j + 2];  // Difference with twiddle factor
        }
    }

    // Stage 3: Final butterfly computation with stride 8
    for (int i = 0; i < N / 2; i++) {
        FFT_output[i] = temp2[i] + W[i] * temp2[i + 4];      // Sum with twiddle factor
        FFT_output[i + 4] = temp2[i] - W[i] * temp2[i + 4];  // Difference with twiddle factor
    }
}

int main() {
    init_platform(); // Initialize hardware platform

    // Initialize variables for time measurement and FFT data
    XTime PL_start_time, PL_end_time; // Time for hardware (PL) execution
    XTime PS_start_time, PS_end_time; // Time for software (PS) execution
    const float complex FFT_input[N] = { // Input data for FFT
        11 + 23 * I, 32 + 10 * I, 91 + 94 * I, 15 + 69 * I,
        47 + 96 * I, 44 + 12 * I, 96 + 17 * I, 49 + 58 * I
    };
    float complex FFT_output_sw[N], FFT_output_hw[N] = {0}; // FFT outputs
    float complex FFT_rev_sw[N];                           // Bit-reversed data for software FFT

    // Initialize DMA
    XAxiDma AxiDMA; // DMA instance
    int status = DMA_Init(&AxiDMA, XPAR_AXI_DMA_0_DEVICE_ID);
    if (status != XST_SUCCESS) {
        printf("\n\rDMA Initialization Failed! Exiting...\n\r");
        return XST_FAILURE;
    }

    // Perform FFT in software (PS)
    XTime_SetTime(0);               // Reset timer
    XTime_GetTime(&PS_start_time);  // Start time
    bitreverse(FFT_input, FFT_rev_sw);     // Rearrange input in bit-reversed order
    FFT_stages(FFT_rev_sw, FFT_output_sw); // Perform FFT stages
    XTime_GetTime(&PS_end_time);    // End time

    // Perform FFT in hardware (PL) using DMA
    XTime_SetTime(0);               // Reset timer
    XTime_GetTime(&PL_start_time);  // Start time

    // Transfer input data to hardware
    status = XAxiDma_SimpleTransfer(&AxiDMA, (UINTPTR)FFT_input, sizeof(float complex) * N, XAXIDMA_DMA_TO_DEVICE);
    if (status != XST_SUCCESS) {
        printf("\n\rDMA Input Transfer Failed!");
        return XST_FAILURE;
    }
    while (XAxiDma_Busy(&AxiDMA, XAXIDMA_DMA_TO_DEVICE)); // Wait for transfer to complete

    // Retrieve output data from hardware
    status = XAxiDma_SimpleTransfer(&AxiDMA, (UINTPTR)FFT_output_hw, sizeof(float complex) * N, XAXIDMA_DEVICE_TO_DMA);
    if (status != XST_SUCCESS) {
        printf("\n\rDMA Output Transfer Failed!");
        return XST_FAILURE;
    }
    while (XAxiDma_Busy(&AxiDMA, XAXIDMA_DEVICE_TO_DMA)); // Wait for transfer to complete

    XTime_GetTime(&PL_end_time);    // End time

    // Compare software and hardware results
    for (int i = 0; i < N; i++) {
        printf("\n\rPS Output: %.6f + %.6fI, PL Output: %.6f + %.6fI", crealf(FFT_output_sw[i]), cimagf(FFT_output_sw[i]), crealf(FFT_output_hw[i]), cimagf(FFT_output_hw[i]));
        float diff1 = fabsf(crealf(FFT_output_sw[i]) - crealf(FFT_output_hw[i])); // Real part difference
        float diff2 = fabsf(cimagf(FFT_output_sw[i]) - cimagf(FFT_output_hw[i])); // Imaginary part difference
        if (diff1 >= 0.0001 || diff2 >= 0.0001) {
            printf("\n\rData Mismatch found at index %d!", i); // Flag mismatch
        } else {
            printf(" - DMA Transfer Successful!"); // Outputs match
        }
    }

    // Display execution time comparison
    printf("\n\r------- Execution Time Comparison --------");
    printf("\n\rExecution time for PS in Microseconds: %.6f", (float)(PS_end_time - PS_start_time) / (COUNTS_PER_SECOND / 1e6));
    printf("\n\rExecution time for PL in Microseconds: %.6f", (float)(PL_end_time - PL_start_time) / (COUNTS_PER_SECOND / 1e6));

    cleanup_platform(); // Clean up platform resources
    return 0;
}

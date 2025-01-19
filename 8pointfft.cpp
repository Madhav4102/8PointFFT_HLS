#include "information.h"
#include <hls_stream.h>
#include <complex>
#include <stdint.h>

#include <math.h>


// Struct definition for axis_data




void fft(hls::stream<axis_data> &input_vector, hls::stream<axis_data> &output_vector)
{
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis register both port=output_vector
#pragma HLS INTERFACE axis register both port=input_vector

    // the input array in which we will place the extracted input for the fft
    // operation
    std::complex<float> input_array[N];
    std::complex<float> reversed_array[N];
    std::complex<float> FFT_output[N];

    // this loop is used for storing the input data into an array which is
    // then used to perform the fft operation
    for (int i = 0; i < 8; i++)
    {

        axis_data input_data = input_vector.read();
        uint64_t raw_data = input_data.data.to_uint64();

        uint32_t real_part = static_cast<uint32_t>(raw_data & 0xFFFFFFFF);        // Lower 32 bits
        uint32_t imag_part = static_cast<uint32_t>((raw_data >> 32) & 0xFFFFFFFF); // Upper 32 bits

        float real = *reinterpret_cast<float *>(&real_part);
        float imag = *reinterpret_cast<float *>(&imag_part);

        input_array[i] = std::complex<float>(real, imag);
        if (input_data.last && i < 7)
        {
            // Break early if the last signal arrives before all 8 elements are read
            break;
        }
    }

    // Array of twiddle factors
    const std::complex<float> W[4] = {
        std::complex<float>(1, 0),
        std::complex<float>(std::cos(-2 * M_PI / 8), std::sin(-2 * M_PI / 8)),
        std::complex<float>(std::cos(-4 * M_PI / 8), std::sin(-4 * M_PI / 8)),
        std::complex<float>(std::cos(-6 * M_PI / 8), std::sin(-6 * M_PI / 8))
    };

    const int rev8[N] = {0, 4, 2, 6, 1, 5, 3, 7}; // Permutation for bit-reversed order

    for (int i = 0; i < N; i++) {

        reversed_array[i] = input_array[rev8[i]]; // Rearrange data based on the reverse bit order
    }

    std::complex<float> temp1[N];
    std::complex<float> temp2[N];

    // Stage 1: Butterfly computation with stride 2
    for (int i = 0; i < N; i += 2) {

        temp1[i] = reversed_array[i] + reversed_array[i + 1];      // Sum
        temp1[i + 1] = reversed_array[i] - reversed_array[i + 1];  // Difference
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

    // Writing FFT output to the output_vector stream
    for (int i = 0; i < N; i++) {
    	float real_part = FFT_output[i].real();
    	float imag_part = FFT_output[i].imag();

    	uint32_t real_part_int = *reinterpret_cast<uint32_t *>(&real_part);
    	uint32_t imag_part_int = *reinterpret_cast<uint32_t *>(&imag_part);
    	uint64_t packed_data = (static_cast<uint64_t>(imag_part_int) << 32) | real_part_int;
        axis_data output_data;
        output_data.data = (ap_uint<64>)packed_data;
        output_data.last = (ap_uint<1>)((i == N - 1) ? 1 : 0); // Set last signal for the last element

        // Write to the output stream
        output_vector.write(output_data);
       }
}

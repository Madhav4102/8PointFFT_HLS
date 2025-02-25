// Copyright 1986-2019 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2019.1 (win64) Build 2552052 Fri May 24 14:49:42 MDT 2019
// Date        : Mon Dec 16 00:27:23 2024
// Host        : LAPTOP-H94SON88 running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode synth_stub -rename_top decalper_eb_ot_sdeen_pot_pi_dehcac_xnilix -prefix
//               decalper_eb_ot_sdeen_pot_pi_dehcac_xnilix_ design_1_fft_0_0_stub.v
// Design      : design_1_fft_0_0
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7z020clg484-1
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
(* X_CORE_INFO = "fft,Vivado 2019.1" *)
module decalper_eb_ot_sdeen_pot_pi_dehcac_xnilix(ap_clk, ap_rst_n, input_vector_TVALID, 
  input_vector_TREADY, input_vector_TDATA, input_vector_TLAST, output_vector_TVALID, 
  output_vector_TREADY, output_vector_TDATA, output_vector_TLAST)
/* synthesis syn_black_box black_box_pad_pin="ap_clk,ap_rst_n,input_vector_TVALID,input_vector_TREADY,input_vector_TDATA[63:0],input_vector_TLAST[0:0],output_vector_TVALID,output_vector_TREADY,output_vector_TDATA[63:0],output_vector_TLAST[0:0]" */;
  input ap_clk;
  input ap_rst_n;
  input input_vector_TVALID;
  output input_vector_TREADY;
  input [63:0]input_vector_TDATA;
  input [0:0]input_vector_TLAST;
  output output_vector_TVALID;
  input output_vector_TREADY;
  output [63:0]output_vector_TDATA;
  output [0:0]output_vector_TLAST;
endmodule

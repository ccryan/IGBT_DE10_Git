// (C) 2001-2018 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files from any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Intel Program License Subscription 
// Agreement, Intel FPGA IP License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Intel and sold by 
// Intel or its authorized distributors.  Please refer to the applicable 
// agreement for further details.



// Your use of Altera Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Altera Program License Subscription 
// Agreement, Altera MegaCore Function License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Altera and sold by 
// Altera or its authorized distributors.  Please refer to the applicable 
// agreement for further details.


// $Id: //acds/rel/18.1std/ip/merlin/altera_merlin_router/altera_merlin_router.sv.terp#1 $
// $Revision: #1 $
// $Date: 2018/07/18 $
// $Author: psgswbuild $

// -------------------------------------------------------
// Merlin Router
//
// Asserts the appropriate one-hot encoded channel based on 
// either (a) the address or (b) the dest id. The DECODER_TYPE
// parameter controls this behaviour. 0 means address decoder,
// 1 means dest id decoder.
//
// In the case of (a), it also sets the destination id.
// -------------------------------------------------------

`timescale 1 ns / 1 ns

module soc_system_mm_interconnect_0_router_default_decode
  #(
     parameter DEFAULT_CHANNEL = 3,
               DEFAULT_WR_CHANNEL = -1,
               DEFAULT_RD_CHANNEL = -1,
               DEFAULT_DESTID = 9 
   )
  (output [108 - 103 : 0] default_destination_id,
   output [52-1 : 0] default_wr_channel,
   output [52-1 : 0] default_rd_channel,
   output [52-1 : 0] default_src_channel
  );

  assign default_destination_id = 
    DEFAULT_DESTID[108 - 103 : 0];

  generate
    if (DEFAULT_CHANNEL == -1) begin : no_default_channel_assignment
      assign default_src_channel = '0;
    end
    else begin : default_channel_assignment
      assign default_src_channel = 52'b1 << DEFAULT_CHANNEL;
    end
  endgenerate

  generate
    if (DEFAULT_RD_CHANNEL == -1) begin : no_default_rw_channel_assignment
      assign default_wr_channel = '0;
      assign default_rd_channel = '0;
    end
    else begin : default_rw_channel_assignment
      assign default_wr_channel = 52'b1 << DEFAULT_WR_CHANNEL;
      assign default_rd_channel = 52'b1 << DEFAULT_RD_CHANNEL;
    end
  endgenerate

endmodule


module soc_system_mm_interconnect_0_router
(
    // -------------------
    // Clock & Reset
    // -------------------
    input clk,
    input reset,

    // -------------------
    // Command Sink (Input)
    // -------------------
    input                       sink_valid,
    input  [133-1 : 0]    sink_data,
    input                       sink_startofpacket,
    input                       sink_endofpacket,
    output                      sink_ready,

    // -------------------
    // Command Source (Output)
    // -------------------
    output                          src_valid,
    output reg [133-1    : 0] src_data,
    output reg [52-1 : 0] src_channel,
    output                          src_startofpacket,
    output                          src_endofpacket,
    input                           src_ready
);

    // -------------------------------------------------------
    // Local parameters and variables
    // -------------------------------------------------------
    localparam PKT_ADDR_H = 67;
    localparam PKT_ADDR_L = 36;
    localparam PKT_DEST_ID_H = 108;
    localparam PKT_DEST_ID_L = 103;
    localparam PKT_PROTECTION_H = 123;
    localparam PKT_PROTECTION_L = 121;
    localparam ST_DATA_W = 133;
    localparam ST_CHANNEL_W = 52;
    localparam DECODER_TYPE = 0;

    localparam PKT_TRANS_WRITE = 70;
    localparam PKT_TRANS_READ  = 71;

    localparam PKT_ADDR_W = PKT_ADDR_H-PKT_ADDR_L + 1;
    localparam PKT_DEST_ID_W = PKT_DEST_ID_H-PKT_DEST_ID_L + 1;



    // -------------------------------------------------------
    // Figure out the number of bits to mask off for each slave span
    // during address decoding
    // -------------------------------------------------------
    localparam PAD0 = log2ceil(64'h40000 - 64'h0); 
    localparam PAD1 = log2ceil(64'h40010 - 64'h40000); 
    localparam PAD2 = log2ceil(64'h41010 - 64'h41000); 
    localparam PAD3 = log2ceil(64'h42010 - 64'h42000); 
    localparam PAD4 = log2ceil(64'h43010 - 64'h43000); 
    localparam PAD5 = log2ceil(64'h44010 - 64'h44000); 
    localparam PAD6 = log2ceil(64'h45040 - 64'h45000); 
    localparam PAD7 = log2ceil(64'h46040 - 64'h46000); 
    localparam PAD8 = log2ceil(64'h47040 - 64'h47000); 
    localparam PAD9 = log2ceil(64'h50010 - 64'h50000); 
    localparam PAD10 = log2ceil(64'h50110 - 64'h50100); 
    localparam PAD11 = log2ceil(64'h50210 - 64'h50200); 
    localparam PAD12 = log2ceil(64'h50310 - 64'h50300); 
    localparam PAD13 = log2ceil(64'h50410 - 64'h50400); 
    localparam PAD14 = log2ceil(64'h50510 - 64'h50500); 
    localparam PAD15 = log2ceil(64'h50610 - 64'h50600); 
    localparam PAD16 = log2ceil(64'h50710 - 64'h50700); 
    localparam PAD17 = log2ceil(64'h50810 - 64'h50800); 
    localparam PAD18 = log2ceil(64'h50910 - 64'h50900); 
    localparam PAD19 = log2ceil(64'h50a10 - 64'h50a00); 
    localparam PAD20 = log2ceil(64'h50b10 - 64'h50b00); 
    localparam PAD21 = log2ceil(64'h50c10 - 64'h50c00); 
    localparam PAD22 = log2ceil(64'h50d10 - 64'h50d00); 
    localparam PAD23 = log2ceil(64'h50e10 - 64'h50e00); 
    localparam PAD24 = log2ceil(64'h50f10 - 64'h50f00); 
    localparam PAD25 = log2ceil(64'h51010 - 64'h51000); 
    localparam PAD26 = log2ceil(64'h51110 - 64'h51100); 
    localparam PAD27 = log2ceil(64'h51210 - 64'h51200); 
    localparam PAD28 = log2ceil(64'h51310 - 64'h51300); 
    localparam PAD29 = log2ceil(64'h51410 - 64'h51400); 
    localparam PAD30 = log2ceil(64'h51510 - 64'h51500); 
    localparam PAD31 = log2ceil(64'h51610 - 64'h51600); 
    localparam PAD32 = log2ceil(64'h51710 - 64'h51700); 
    localparam PAD33 = log2ceil(64'h51810 - 64'h51800); 
    localparam PAD34 = log2ceil(64'h51910 - 64'h51900); 
    localparam PAD35 = log2ceil(64'h51a10 - 64'h51a00); 
    localparam PAD36 = log2ceil(64'h51b10 - 64'h51b00); 
    localparam PAD37 = log2ceil(64'h51c10 - 64'h51c00); 
    localparam PAD38 = log2ceil(64'h51d10 - 64'h51d00); 
    localparam PAD39 = log2ceil(64'h51e10 - 64'h51e00); 
    localparam PAD40 = log2ceil(64'h51f10 - 64'h51f00); 
    localparam PAD41 = log2ceil(64'h52010 - 64'h52000); 
    localparam PAD42 = log2ceil(64'h52110 - 64'h52100); 
    localparam PAD43 = log2ceil(64'h52210 - 64'h52200); 
    localparam PAD44 = log2ceil(64'h52310 - 64'h52300); 
    // -------------------------------------------------------
    // Work out which address bits are significant based on the
    // address range of the slaves. If the required width is too
    // large or too small, we use the address field width instead.
    // -------------------------------------------------------
    localparam ADDR_RANGE = 64'h52310;
    localparam RANGE_ADDR_WIDTH = log2ceil(ADDR_RANGE);
    localparam OPTIMIZED_ADDR_H = (RANGE_ADDR_WIDTH > PKT_ADDR_W) ||
                                  (RANGE_ADDR_WIDTH == 0) ?
                                        PKT_ADDR_H :
                                        PKT_ADDR_L + RANGE_ADDR_WIDTH - 1;

    localparam RG = RANGE_ADDR_WIDTH-1;
    localparam REAL_ADDRESS_RANGE = OPTIMIZED_ADDR_H - PKT_ADDR_L;

      reg [PKT_ADDR_W-1 : 0] address;
      always @* begin
        address = {PKT_ADDR_W{1'b0}};
        address [REAL_ADDRESS_RANGE:0] = sink_data[OPTIMIZED_ADDR_H : PKT_ADDR_L];
      end   

    // -------------------------------------------------------
    // Pass almost everything through, untouched
    // -------------------------------------------------------
    assign sink_ready        = src_ready;
    assign src_valid         = sink_valid;
    assign src_startofpacket = sink_startofpacket;
    assign src_endofpacket   = sink_endofpacket;
    wire [PKT_DEST_ID_W-1:0] default_destid;
    wire [52-1 : 0] default_src_channel;






    soc_system_mm_interconnect_0_router_default_decode the_default_decode(
      .default_destination_id (default_destid),
      .default_wr_channel   (),
      .default_rd_channel   (),
      .default_src_channel  (default_src_channel)
    );

    always @* begin
        src_data    = sink_data;
        src_channel = default_src_channel;
        src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = default_destid;

        // --------------------------------------------------
        // Address Decoder
        // Sets the channel and destination ID based on the address
        // --------------------------------------------------

    // ( 0x0 .. 0x40000 )
    if ( {address[RG:PAD0],{PAD0{1'b0}}} == 19'h0   ) begin
            src_channel = 52'b000000000000000000000000000000000000000001000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 9;
    end

    // ( 0x40000 .. 0x40010 )
    if ( {address[RG:PAD1],{PAD1{1'b0}}} == 19'h40000   ) begin
            src_channel = 52'b000000000000000000000000000000000000000010000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 2;
    end

    // ( 0x41000 .. 0x41010 )
    if ( {address[RG:PAD2],{PAD2{1'b0}}} == 19'h41000   ) begin
            src_channel = 52'b000000000000000000000000000000000000000100000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 50;
    end

    // ( 0x42000 .. 0x42010 )
    if ( {address[RG:PAD3],{PAD3{1'b0}}} == 19'h42000   ) begin
            src_channel = 52'b000000000000000000000000000000000000001000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 11;
    end

    // ( 0x43000 .. 0x43010 )
    if ( {address[RG:PAD4],{PAD4{1'b0}}} == 19'h43000   ) begin
            src_channel = 52'b000000000000000000000000000000000000010000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 12;
    end

    // ( 0x44000 .. 0x44010 )
    if ( {address[RG:PAD5],{PAD5{1'b0}}} == 19'h44000   ) begin
            src_channel = 52'b000000000000000000000000000000000000100000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 13;
    end

    // ( 0x45000 .. 0x45040 )
    if ( {address[RG:PAD6],{PAD6{1'b0}}} == 19'h45000   ) begin
            src_channel = 52'b000000000000000000000000000000000000000000001;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 6;
    end

    // ( 0x46000 .. 0x46040 )
    if ( {address[RG:PAD7],{PAD7{1'b0}}} == 19'h46000   ) begin
            src_channel = 52'b000000000000000000000000000000000000000000010;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 5;
    end

    // ( 0x47000 .. 0x47040 )
    if ( {address[RG:PAD8],{PAD8{1'b0}}} == 19'h47000   ) begin
            src_channel = 52'b000000000000000000000000000000000000000000100;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 4;
    end

    // ( 0x50000 .. 0x50010 )
    if ( {address[RG:PAD9],{PAD9{1'b0}}} == 19'h50000   ) begin
            src_channel = 52'b000000000000000000000000000000000001000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 14;
    end

    // ( 0x50100 .. 0x50110 )
    if ( {address[RG:PAD10],{PAD10{1'b0}}} == 19'h50100   ) begin
            src_channel = 52'b000000000000000000000000000000000010000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 25;
    end

    // ( 0x50200 .. 0x50210 )
    if ( {address[RG:PAD11],{PAD11{1'b0}}} == 19'h50200   ) begin
            src_channel = 52'b000000000000000000000000000000000100000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 36;
    end

    // ( 0x50300 .. 0x50310 )
    if ( {address[RG:PAD12],{PAD12{1'b0}}} == 19'h50300   ) begin
            src_channel = 52'b000000000000000000000000000000001000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 43;
    end

    // ( 0x50400 .. 0x50410 )
    if ( {address[RG:PAD13],{PAD13{1'b0}}} == 19'h50400   ) begin
            src_channel = 52'b000000000000000000000000000000010000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 44;
    end

    // ( 0x50500 .. 0x50510 )
    if ( {address[RG:PAD14],{PAD14{1'b0}}} == 19'h50500   ) begin
            src_channel = 52'b000000000000000000000000000000100000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 45;
    end

    // ( 0x50600 .. 0x50610 )
    if ( {address[RG:PAD15],{PAD15{1'b0}}} == 19'h50600   ) begin
            src_channel = 52'b000000000000000000000000000001000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 46;
    end

    // ( 0x50700 .. 0x50710 )
    if ( {address[RG:PAD16],{PAD16{1'b0}}} == 19'h50700   ) begin
            src_channel = 52'b000000000000000000000000000010000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 47;
    end

    // ( 0x50800 .. 0x50810 )
    if ( {address[RG:PAD17],{PAD17{1'b0}}} == 19'h50800   ) begin
            src_channel = 52'b000000000000000000000000000100000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 48;
    end

    // ( 0x50900 .. 0x50910 )
    if ( {address[RG:PAD18],{PAD18{1'b0}}} == 19'h50900   ) begin
            src_channel = 52'b000000000000000000000000001000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 49;
    end

    // ( 0x50a00 .. 0x50a10 )
    if ( {address[RG:PAD19],{PAD19{1'b0}}} == 19'h50a00   ) begin
            src_channel = 52'b000000000000000000000000010000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 15;
    end

    // ( 0x50b00 .. 0x50b10 )
    if ( {address[RG:PAD20],{PAD20{1'b0}}} == 19'h50b00   ) begin
            src_channel = 52'b000000000000000000000000100000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 16;
    end

    // ( 0x50c00 .. 0x50c10 )
    if ( {address[RG:PAD21],{PAD21{1'b0}}} == 19'h50c00   ) begin
            src_channel = 52'b000000000000000000000001000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 17;
    end

    // ( 0x50d00 .. 0x50d10 )
    if ( {address[RG:PAD22],{PAD22{1'b0}}} == 19'h50d00   ) begin
            src_channel = 52'b000010000000000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 18;
    end

    // ( 0x50e00 .. 0x50e10 )
    if ( {address[RG:PAD23],{PAD23{1'b0}}} == 19'h50e00   ) begin
            src_channel = 52'b000000000000000000000010000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 19;
    end

    // ( 0x50f00 .. 0x50f10 )
    if ( {address[RG:PAD24],{PAD24{1'b0}}} == 19'h50f00   ) begin
            src_channel = 52'b000000000000000000000100000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 20;
    end

    // ( 0x51000 .. 0x51010 )
    if ( {address[RG:PAD25],{PAD25{1'b0}}} == 19'h51000   ) begin
            src_channel = 52'b000000000000000000001000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 21;
    end

    // ( 0x51100 .. 0x51110 )
    if ( {address[RG:PAD26],{PAD26{1'b0}}} == 19'h51100   ) begin
            src_channel = 52'b000000000000000000010000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 22;
    end

    // ( 0x51200 .. 0x51210 )
    if ( {address[RG:PAD27],{PAD27{1'b0}}} == 19'h51200   ) begin
            src_channel = 52'b000000000000000000100000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 23;
    end

    // ( 0x51300 .. 0x51310 )
    if ( {address[RG:PAD28],{PAD28{1'b0}}} == 19'h51300   ) begin
            src_channel = 52'b000000000000000001000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 24;
    end

    // ( 0x51400 .. 0x51410 )
    if ( {address[RG:PAD29],{PAD29{1'b0}}} == 19'h51400   ) begin
            src_channel = 52'b000000000000000010000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 26;
    end

    // ( 0x51500 .. 0x51510 )
    if ( {address[RG:PAD30],{PAD30{1'b0}}} == 19'h51500   ) begin
            src_channel = 52'b000000000000000100000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 27;
    end

    // ( 0x51600 .. 0x51610 )
    if ( {address[RG:PAD31],{PAD31{1'b0}}} == 19'h51600   ) begin
            src_channel = 52'b000000000000001000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 28;
    end

    // ( 0x51700 .. 0x51710 )
    if ( {address[RG:PAD32],{PAD32{1'b0}}} == 19'h51700   ) begin
            src_channel = 52'b000000000000010000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 29;
    end

    // ( 0x51800 .. 0x51810 )
    if ( {address[RG:PAD33],{PAD33{1'b0}}} == 19'h51800   ) begin
            src_channel = 52'b000000000000100000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 30;
    end

    // ( 0x51900 .. 0x51910 )
    if ( {address[RG:PAD34],{PAD34{1'b0}}} == 19'h51900   ) begin
            src_channel = 52'b000000000001000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 31;
    end

    // ( 0x51a00 .. 0x51a10 )
    if ( {address[RG:PAD35],{PAD35{1'b0}}} == 19'h51a00   ) begin
            src_channel = 52'b000000010000000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 32;
    end

    // ( 0x51b00 .. 0x51b10 )
    if ( {address[RG:PAD36],{PAD36{1'b0}}} == 19'h51b00   ) begin
            src_channel = 52'b000000100000000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 33;
    end

    // ( 0x51c00 .. 0x51c10 )
    if ( {address[RG:PAD37],{PAD37{1'b0}}} == 19'h51c00   ) begin
            src_channel = 52'b000001000000000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 34;
    end

    // ( 0x51d00 .. 0x51d10 )
    if ( {address[RG:PAD38],{PAD38{1'b0}}} == 19'h51d00   ) begin
            src_channel = 52'b000000001000000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 35;
    end

    // ( 0x51e00 .. 0x51e10 )
    if ( {address[RG:PAD39],{PAD39{1'b0}}} == 19'h51e00   ) begin
            src_channel = 52'b000000000100000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 37;
    end

    // ( 0x51f00 .. 0x51f10 )
    if ( {address[RG:PAD40],{PAD40{1'b0}}} == 19'h51f00   ) begin
            src_channel = 52'b000000000010000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 38;
    end

    // ( 0x52000 .. 0x52010 )
    if ( {address[RG:PAD41],{PAD41{1'b0}}} == 19'h52000   ) begin
            src_channel = 52'b000100000000000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 39;
    end

    // ( 0x52100 .. 0x52110 )
    if ( {address[RG:PAD42],{PAD42{1'b0}}} == 19'h52100   ) begin
            src_channel = 52'b001000000000000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 40;
    end

    // ( 0x52200 .. 0x52210 )
    if ( {address[RG:PAD43],{PAD43{1'b0}}} == 19'h52200   ) begin
            src_channel = 52'b010000000000000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 41;
    end

    // ( 0x52300 .. 0x52310 )
    if ( {address[RG:PAD44],{PAD44{1'b0}}} == 19'h52300   ) begin
            src_channel = 52'b100000000000000000000000000000000000000000000;
            src_data[PKT_DEST_ID_H:PKT_DEST_ID_L] = 42;
    end

end


    // --------------------------------------------------
    // Ceil(log2()) function
    // --------------------------------------------------
    function integer log2ceil;
        input reg[65:0] val;
        reg [65:0] i;

        begin
            i = 1;
            log2ceil = 0;

            while (i < val) begin
                log2ceil = log2ceil + 1;
                i = i << 1;
            end
        end
    endfunction

endmodule



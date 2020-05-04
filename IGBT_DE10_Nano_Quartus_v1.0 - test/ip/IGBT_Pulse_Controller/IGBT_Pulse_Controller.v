module IGBT_Pulse_Controller(
	CLK,
	RESET,
	

	START,
	FSMSTAT,
	
	PULSE_LENGTH,
	
	OUTPUT_PULSE
);
	//parameter PULSE_LENGTH = {32'd10; // fixed pulse length for now, 10 us
	
	parameter PULSE_WIDTH = 32; // Maximum pulse width
	parameter SYSTEM_TIMER_WIDTH = 32;
	
	// control signals
	input 		START;
	input [PULSE_WIDTH-1:0] PULSE_LENGTH;
	output reg 	FSMSTAT;
	
	// system clk
	input CLK; 
	input RESET;
	
	output 	 OUTPUT_PULSE;


	// system clock 
	reg [SYSTEM_TIMER_WIDTH-1:0] SYSTEM_TIMER_CNT;
	reg OUT_EN;
	
	reg [PULSE_WIDTH-1:0]	PULSE_CNT;
	
	reg [9:0] State; // synthesis keep = 1;
	localparam [9:0]
		S0 = 10'b0000000001,
		S1 = 10'b0000000010,
		S2 = 10'b0000000100,
		S3 = 10'b0000001000;
			
	initial begin
		OUT_EN	<= 1'b0;
		FSMSTAT	<=1'b0;
		
		State 	<= S0;
		//PULSE_LENGTH <= {2'b00, {(30){1'b1}}};
	end
		

// state machine for pulse length
	always @(posedge CLK, posedge RESET) begin // add reset and start? after
	
	
	if (RESET)
		begin
		
			FSMSTAT	<= 1'b0;
			OUT_EN	<= 1'b0;
			State	<= S0;
		end
		
	else 
		begin
			case (State)
				S0:
				begin
					OUT_EN	<= 1'b0;
					FSMSTAT	<=1'b0;
					
				// Wait for the Start signal
					if (START)
						State <= S1;
						
				end
			
				S1:
				begin
					PULSE_CNT		<= {1'b1,{ (PULSE_WIDTH-1) {1'b0} }} - PULSE_LENGTH + 1'b1;
					FSMSTAT <= 1'b1;
					State <= S2;
				end
				
				S2: // output pulse
				begin			
			
					OUT_EN <= 1'b1;
					PULSE_CNT <= PULSE_CNT + 1'b1;
							
					if (PULSE_CNT[PULSE_WIDTH - 1 ])
						State <= S3;
				end
				
				S3 : // end of sequence
				begin
					FSMSTAT <= 1'b1; //FSMSTAT <= 1'b0; for the moment, need a proof the system is working.
					OUT_EN <= 1'b0;
					//State <= S0; /// for now no loop 

				end
				
				default:
					$display(" Error");
				
			endcase
			
		end	
	end	
 assign	OUTPUT_PULSE = OUT_EN;

//	assign OUTPUT_PULSE <= OUT_EN;
	//test
	//assign OUTPUT_PULSE = 1'b1;

endmodule


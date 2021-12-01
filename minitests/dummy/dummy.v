module top();
	// This will be removed after in process
	(* keep *) wire c;
	EG_PHY_OSC osc(.osc_clk(c));
endmodule

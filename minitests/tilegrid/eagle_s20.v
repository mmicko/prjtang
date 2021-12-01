module top(input clk,input p1, input p2);

/*
genvar i;
generate for (i = 0; i < 64; i = i + 1)
begin: part1
	(* keep *)
	EG_PHY_BRAM bram_i(.clka(clk));
end
endgenerate

genvar j;
generate for (j = 0; j < 16; j = j + 1)
begin: part2
	(* keep *)
	EG_PHY_BRAM32K  bram32_j(.clka(clk));
end
endgenerate

genvar k;
generate for (k = 0; k < 29; k = k + 1)
begin: part3
	(* keep *)
	EG_PHY_MULT18 mult_k_ (.clk(clk));
end
endgenerate


genvar l;
generate for (l = 0; l < 4; l = l + 1)
begin: part4
	(* keep *)
	EG_PHY_PLL pll_l(.refclk(clk));
end
endgenerate

genvar m;
generate for (m = 0; m < 29; m = m + 1)
begin: part5
	(* keep *)
	EG_PHY_FIFO fifo_m_ (.clkw(clk),.clkr(clk));
end
endgenerate
*/
(* keep *) wire c;
EG_PHY_OSC osc(.osc_clk(c));

(* keep *) wire p3;
EG_PHY_PWRMNT(.sel_pwr(p1), .pwr_mnt_pd(p2), .pwr_dwn_n(p3));
endmodule

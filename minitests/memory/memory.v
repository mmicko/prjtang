module top(input clk,input p1, input p2);

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

endmodule

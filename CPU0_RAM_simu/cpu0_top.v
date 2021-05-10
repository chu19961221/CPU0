
module cpu0_top(
input clk, input rst, 
output [31:0] pc, mar, mdr, dbus, SP_o,
output m_rw, m_en,
output [3:0] itype
);

/*
//cpu0
wire [3:0] itype;
wire m_rw, m_en;
wire [31:0] mar, mdr, dbus;
*/

//RAM
wire r_clk, r_rw;
wire [31:0] r_addr, r_mdr, r_data;


cpu0 cpu(
.clock(clk),
.reset(rst),
.itype(itype),
.pc(pc),
.tick(),
.ir(),
.mar(mar),
.mdr(mdr),
.dbus(dbus),
.m_en(m_en),
.m_rw(m_rw),
.m_w1(),
.SP_o(SP_o)
);

ram ram0(
.address(mar/4),
.clock(~clk),
.data(mdr),
.wren(~m_rw),
.q(dbus)
);

endmodule

all: report.typ conf.typ plot-pdata.png plot-rdata1.png plot-rdata2.png plot-rdata3.png
	typst compile report.typ

plot-pdata.png: plot-pdata.gp pdata.txt
	gnuplot plot-pdata.gp

plot-rdata1.png: plot-rdata1.gp rdata1.txt
	gnuplot plot-rdata1.gp

plot-rdata2.png: plot-rdata2.gp rdata2.txt
	gnuplot plot-rdata2.gp

plot-rdata3.png: plot-rdata3.gp rdata3.txt
	gnuplot plot-rdata3.gp

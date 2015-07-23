################################################################################
#   OpenAirInterface
#   Copyright(c) 1999 - 2014 Eurecom
#
#    (at your option) any later version.
#
#
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#
#    along with OpenAirInterface.The full GNU General Public License is
#
#  Contact Information
#  OpenAirInterface Admin: openair_admin@eurecom.fr
#  OpenAirInterface Tech : openair_tech@eurecom.fr
#  OpenAirInterface Dev  : openair4g-devel@eurecom.fr
#
#  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.
#
################################################################################
MYFILE=EPC

all:
	pdflatex $(MYFILE).tex
	pdflatex $(MYFILE).tex
	makeindex $(MYFILE).nlo -s nomencl.ist -o $(MYFILE).nls
	pdflatex $(MYFILE).tex

.PHONY: all

#!/bin/bash
#

cd engine/src
make clean; make install
cd ../..

PATH=$PWD/engine/pub:$PATH; export PATH
echo "Single core"
time MaBoSS -c engine/examples/cell_fate/CellFateModel_1.cfg -o cell_fate_result engine/examples/cell_fate/CellFateModel.bnd
echo "Two cores"
time MaBoSS -c engine/examples/cell_fate/CellFateModel_2.cfg -o cell_fate_result engine/examples/cell_fate/CellFateModel.bnd
echo "Four cores"
time MaBoSS -c engine/examples/cell_fate/CellFateModel_4.cfg -o cell_fate_result engine/examples/cell_fate/CellFateModel.bnd
echo "Eight cores"
time MaBoSS -c engine/examples/cell_fate/CellFateModel_8.cfg -o cell_fate_result engine/examples/cell_fate/CellFateModel.bnd

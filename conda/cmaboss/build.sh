cd engine/src
make grammars
cd ../python
cp -r ../src cmaboss
$PYTHON -m pip install . --no-deps --ignore-installed --no-cache-dir -vvv
mkdir -p ${PREFIX}/tests
cp -r ../tests/* ${PREFIX}/tests


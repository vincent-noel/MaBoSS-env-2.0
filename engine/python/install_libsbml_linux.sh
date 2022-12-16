wget https://sourceforge.net/projects/sbml/files/libsbml/5.19.0/stable/libSBML-5.19.0-core-plus-packages-src.tar.gz
tar -zxf libSBML-5.19.0-core-plus-packages-src.tar.gz
cd libSBML-5.19.0-Source
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr \
-DCMAKE_INSTALL_LIBDIR=/usr/lib \
-DCMAKE_CXX_COMPILER=g++ \
-DCMAKE_C_COMPILER=gcc \
-DCMAKE_CXX_STANDARD_LIBRARIES=-lxml2 \
-DWITH_SWIG=OFF \
-DLIBXML_LIBRARY=/usr/lib \
-DLIBXML_INCLUDE_DIR=/usr/include/libxml2 \
-DENABLE_COMP=ON \
-DENABLE_FBC=ON \
-DENABLE_GROUPS=ON \
-DENABLE_LAYOUT=ON \
-DENABLE_MULTI=ON \
-DENABLE_QUAL=ON \
-DENABLE_RENDER=ON \
-DENABLE_DISTRIB=ON \
-DWITH_CPP_NAMESPACE=ON \
..
make
make install
#!/bin/bash

mycmake                                            \
    -DBUILD_OSG_DEPRECATED_SERIALIZERS:BOOLEAN=OFF \
    -DOSG_USE_DEPRECATED_API:BOOLEAN=OFF           \
    -DBUILD_OSG_EXAMPLES:BOOLEAN=ON                \
    -DOSG_GL1_AVAILABLE:BOOLEAN=OFF                \
    -DOSG_GL_CONTEXT_VERSION:STRING=2.1            \
    -DBUILD_SHARED_LIBS:BOOLEAN=ON                 \
    -DCMAKE_INSTALL_PREFIX:STRING=/usr/local/osg   \
    -DCMAKE_CXX_FLAGS:STRING=

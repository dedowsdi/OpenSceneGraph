SET(TARGET_SRC
    osgbloom.cpp
    GaussianBlur.cpp
    TextureFilter.cpp
)
SET(TARGET_ADDED_LIBRARIES osgGA osgFX)
SETUP_EXAMPLE(osgbloom)

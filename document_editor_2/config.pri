#Require at least Qt 4.4.5
QT_VERSION = $$[QT_VERSION]
QT_VERSION = $$split(QT_VERSION, ".")
QT_VER_MAJ = $$member(QT_VERSION, 0) 
QT_VER_MIN = $$member(QT_VERSION, 1) 
QT_VER_PAT = $$member(QT_VERSION, 2) 

message(version asplit $$QT_VERSION )
message(version bsplit $$QT_VER_MAJ )
message(version csplit $$QT_VER_MIN )
message(version dsplit $$QT_VER_PAT )


#### discovery qt5 or qt4 
equals(QT_VER_MAJ,5) {
#### qt5 fix  /Users/pro/qt/qt5lang/5.1.1/clang_64/include
#### cache()
LOCALQTDIR = /Users/pro/qt/qt51/qtbase/include
LOCALQTDIR55555555 = /Users/pro/qt/qt5lang/5.1.1/clang_64/include
DEPENDPATH +=  $$LOCALQTDIR
INCLUDEPATH += $$LOCALQTDIR
#### if need!!! 
QT += printsupport
message(Use qt5 from $$LOCALQTDIR config.pri setting )
}

equals(QT_VER_MAJ,4) {
#### http://www.qtcentre.org/wiki/index.php?title=Undocumented_qmake
#### qt4 fix  lessThan << minore 
message(Use qt4 setting edit config.pri )

}


win32 {
    ########## window no pwd  #############
    BUILD_TREE_PATH = $$PWD
    message(Window path  $$BUILD_TREE_PATH)
}


unix {
    BUILD_TREE_PATH = $$PWD
    message(Unix path  $$BUILD_TREE_PATH)
}

macx {
    BUILD_TREE_PATH = $$PWD
    message(Mac path $$BUILD_TREE_PATH)
}
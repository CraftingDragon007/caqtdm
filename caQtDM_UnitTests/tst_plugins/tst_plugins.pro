include(../../caQtDM_Viewer/qtdefs.pri)

TEMPLATE = subdirs

SUBDIRS += tst_internal

bsread: {
    SUBDIRS += tst_bsread
}

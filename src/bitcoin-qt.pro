

TEMPLATE = app
VERSION = 1.1.1
TARGET = bitcoin-qt
INCLUDEPATH += ../src ../src/json ../src/cryptopp ../src/univalue/include config
DEFINES += QT_GUI HAVE_ENDIAN_H
DEFINES += SSL  HAVE_CONFIG_H
CONFIG += no_include_pwd
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++0x
DEFINES += QT_GUI BOOST_THREAD_USE_LIB BITCOIN_COMPAT_ENDIAN_H
Qt += network
OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build
#PACKAGE_NAME=bitcoin-qt
DEFINES += HAVE_WORKING_BOOST_SLEEP ENABLE_WALLET _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS

# use: qmake "RELEASE=1"
contains(RELEASE, 1) {
    # Mac: compile for maximum compatibility (10.5, 32-bit)
    macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.5 -arch i386 -isysroot /Developer/SDKs/MacOSX10.5.sdk

    !windows:!macx {
        # Linux: static link
        LIBS += -Wl,-Bstatic
    }
}
# use: qmake "USE_QRCODE=1"
# libqrencode (http://fukuchi.org/works/qrencode/index.en.html) must be installed for support
contains(USE_QRCODE, 1) {
    message(Building with QRCode support)
    DEFINES += USE_QRCODE
    LIBS += -libqrencode
}
LIBS += -libzbarqt
LIBS += -libqrencode

DEFINES += USE_QRCODE
LIBS += -libqrencode

# use: qmake "USE_DBUS=1"
contains(USE_DBUS, 1) {
    message(Building with DBUS (Freedesktop notifications) support)
    DEFINES += USE_DBUS
    QT += dbus
}



# use: qmake "USE_SSL=1"
contains(USE_SSL, 1) {
    message(Building with SSL support for RPC)
    DEFINES += USE_SSL
}

contains(BITCOIN_NEED_QT_PLUGINS, 1) {
    DEFINES += BITCOIN_NEED_QT_PLUGINS
    QTPLUGIN += qcncodecs qjpcodecs qtwcodecs qkrcodecs
}

!windows {
    # for extra security against potential buffer overflows
    QMAKE_CXXFLAGS += -fstack-protector
    QMAKE_LFLAGS += -fstack-protector
    # do not enable this on windows, as it will result in a non-working executable!
}




# disable quite some warnings because bitcoin core "sins" a lot
QMAKE_CXXFLAGS_WARN_ON = -fdiagnostics-show-option -Wall -Wno-invalid-offsetof -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare -Wno-char-subscripts  -Wno-unused-value -Wno-sequence-point -Wno-parentheses -Wno-unknown-pragmas -Wno-switch

# for boost 1.37, add -mt to the boost libraries
unix:LIBS += -lssl -lcrypto -lboost_system -lboost_filesystem -lboost_program_options -lboost_thread -ldb_cxx
macx:DEFINES += __WXMAC_OSX__ MSG_NOSIGNAL=0 BOOST_FILESYSTEM_VERSION=3
macx:LIBS += -lboost_thread-mt
windows:DEFINES += __WXMSW__
windows:LIBS += -lssl -lcrypto -lboost_system-mgw44-mt-1_43 -lboost_filesystem-mgw44-mt-1_43 -lboost_program_options-mgw44-mt-1_43 -lboost_thread-mgw44-mt-1_43 -ldb_cxx -lws2_32 -lgdi32

# Input
DEPENDPATH += src/qt src src/cryptopp src json/include
OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build
bin_PROGRAMS += qt/bitcoin-qt
EXTRA_LIBRARIES += qt/libbitcoinqt.a


RESOURCES += \
    qt/bitcoin.qrc \
    qt/bitcoin_locale.qrc

CONFIG += no_keywords

CODECFORTR = UTF-8
TRANSLATIONS = qt/locale/bitcoin_nl.ts qt/locale/bitcoin_de.ts
INCLUDEPATH += ../src /usr/include/boost   src/qt/forms

FORMS += \
    ../src/qt/forms/aboutdialog.ui \
    ../src/qt/forms/addressbookpage.ui \
    ../src/qt/forms/askpassphrasedialog.ui \
    ../src/qt/forms/coincontroldialog.ui \
    ../src/qt/forms/editaddressdialog.ui \
    ../src/qt/forms/helpmessagedialog.ui \
    ../src/qt/forms/intro.ui \
    ../src/qt/forms/openuridialog.ui \
    ../src/qt/forms/optionsdialog.ui \
    ../src/qt/forms/overviewpage.ui \
    ../src/qt/forms/receivecoinsdialog.ui \
    ../src/qt/forms/receiverequestdialog.ui \
    ../src/qt/forms/debugwindow.ui \
    ../src/qt/forms/sendcoinsdialog.ui \
    ../src/qt/forms/sendcoinsentry.ui \
    ../src/qt/forms/signverifymessagedialog.ui \
    ../src/qt/forms/transactiondescdialog.ui \
    qt/forms/logon.ui \
    qt/forms/logon.ui \
    qt/forms/logondlg.ui \
    qt/forms/setdialog.ui \
    qt/forms/ipcdialog.ui \
    qt/forms/exportdialog.ui \
    qt/forms/ipcdetails.ui \
    qt/forms/ipcregister.ui \
    qt/forms/ipcregisterinformation.ui \
    qt/forms/successfultrade.ui \
    qt/forms/ipcinspectiontag.ui \
    qt/forms/ipctransfertransaction.ui \
    qt/forms/settingwidget.ui \
    qt/forms/passwordsettingwidget.ui \
    qt/forms/sendcoinsaffrimwidget.ui \
    qt/forms/addbookwidget.ui \
    qt/forms/recvhistory.ui \
    qt/forms/sendhistory.ui \
    qt/forms/editadddialog.ui \
    qt/forms/infowidget.ui \
    qt/forms/feewidget.ui \
    qt/forms/sendresultwidget.ui \
    qt/forms/ipcauthorizationtransaction.ui \
    qt/forms/setrecovery.ui \
    qt/forms/setmessageauthentication.ui \
    qt/forms/setmessageauthenticationtab.ui \
    qt/forms/setmessagesignature.ui \
    qt/forms/walletpagebuttons.ui \
    qt/forms/ipcselectaddress.ui \
    qt/forms/tallydscribe.ui \
    qt/forms/tallyclause.ui \
    qt/forms/tallyapply.ui \
    qt/forms/tallyaccount.ui \
    qt/forms/tallyoutaccount.ui \
    qt/forms/walletpassword.ui \
    qt/forms/adddetail.ui \
    qt/forms/ecoincreatedialog.ui \
    qt/forms/ecoindialog.ui \
    qt/forms/ecoinsendaffrimdialog.ui \
    qt/forms/ecoinsenddialog.ui \
    qt/forms/ecoinsendresultdialog.ui \
    qt/forms/modaloverlay.ui \
    qt/forms/recvipchistory.ui \
    qt/forms/recvtokenhistory.ui \
    qt/forms/sendipchistory.ui \
    qt/forms/sendtokenhistory.ui \
    qt/forms/ecoinaddressdialog.ui \
    qt/forms/cmessagebox.ui


RESOURCES += \
    ../src/qt/bitcoin.qrc

HEADERS += \
    bench/data/block413567.raw.h \
    bench/bench.h \
    bench/perf.h \
    compat/byteswap.h \
    compat/endian.h \
    compat/sanity.h \
    config/bitcoin-config.h \
    consensus/consensus.h \
    consensus/merkle.h \
    consensus/params.h \
    consensus/validation.h \
    crypto/ctaes/ctaes.h \
    crypto/aes.h \
    crypto/common.h \
    crypto/hmac_sha256.h \
    crypto/hmac_sha512.h \
    crypto/ripemd160.h \
    crypto/sha1.h \
    crypto/sha256.h \
    crypto/sha512.h \
    leveldb/db/builder.h \
    leveldb/db/db_impl.h \
    leveldb/db/db_iter.h \
    leveldb/db/dbformat.h \
    leveldb/db/filename.h \
    leveldb/db/log_format.h \
    leveldb/db/log_reader.h \
    leveldb/db/log_writer.h \
    leveldb/db/memtable.h \
    leveldb/db/skiplist.h \
    leveldb/db/snapshot.h \
    leveldb/db/table_cache.h \
    leveldb/db/version_edit.h \
    leveldb/db/version_set.h \
    leveldb/db/write_batch_internal.h \
    leveldb/helpers/memenv/memenv.h \
    leveldb/include/leveldb/c.h \
    leveldb/include/leveldb/cache.h \
    leveldb/include/leveldb/comparator.h \
    leveldb/include/leveldb/db.h \
    leveldb/include/leveldb/dumpfile.h \
    leveldb/include/leveldb/env.h \
    leveldb/include/leveldb/filter_policy.h \
    leveldb/include/leveldb/iterator.h \
    leveldb/include/leveldb/options.h \
    leveldb/include/leveldb/slice.h \
    leveldb/include/leveldb/status.h \
    leveldb/include/leveldb/table.h \
    leveldb/include/leveldb/table_builder.h \
    leveldb/include/leveldb/write_batch.h \
    leveldb/port/win/stdint.h \
    leveldb/port/atomic_pointer.h \
    leveldb/port/port.h \
    leveldb/port/port_example.h \
    leveldb/port/port_posix.h \
    leveldb/port/port_win.h \
    leveldb/port/thread_annotations.h \
    leveldb/table/block.h \
    leveldb/table/block_builder.h \
    leveldb/table/filter_block.h \
    leveldb/table/format.h \
    leveldb/table/iterator_wrapper.h \
    leveldb/table/merger.h \
    leveldb/table/two_level_iterator.h \
    leveldb/util/arena.h \
    leveldb/util/coding.h \
    leveldb/util/crc32c.h \
    leveldb/util/hash.h \
    leveldb/util/histogram.h \
    leveldb/util/logging.h \
    leveldb/util/mutexlock.h \
    leveldb/util/posix_logger.h \
    leveldb/util/random.h \
    leveldb/util/testharness.h \
    leveldb/util/testutil.h \
    obj/build.h \
    policy/fees.h \
    policy/policy.h \
    policy/rbf.h \
    primitives/block.h \
    primitives/transaction.h \
    qt/test/compattests.h \
    qt/test/paymentrequestdata.h \
    qt/test/paymentservertests.h \
    qt/test/rpcnestedtests.h \
    qt/test/uritests.h \
    qt/addressbookpage.h \
    qt/addresstablemodel.h \
    qt/askpassphrasedialog.h \
    qt/bantablemodel.h \
    qt/bitcoinaddressvalidator.h \
    qt/bitcoinamountfield.h \
    qt/bitcoingui.h \
    qt/bitcoinunits.h \
    qt/clientmodel.h \
    qt/coincontroldialog.h \
    qt/coincontroltreewidget.h \
    qt/csvmodelwriter.h \
    qt/editaddressdialog.h \
    qt/guiconstants.h \
    qt/guiutil.h \
    qt/intro.h \
    qt/macdockiconhandler.h \
    qt/macnotificationhandler.h \
    qt/modaloverlay.h \
    qt/networkstyle.h \
    qt/notificator.h \
    qt/openuridialog.h \
    qt/optionsdialog.h \
    qt/optionsmodel.h \
    qt/overviewpage.h \
    qt/paymentrequest.pb.h \
    qt/paymentrequestplus.h \
    qt/paymentserver.h \
    qt/peertablemodel.h \
    qt/platformstyle.h \
    qt/qvalidatedlineedit.h \
    qt/qvaluecombobox.h \
    qt/receivecoinsdialog.h \
    qt/receiverequestdialog.h \
    qt/recentrequeststablemodel.h \
    qt/rpcconsole.h \
    qt/sendcoinsdialog.h \
    qt/sendcoinsentry.h \
    qt/signverifymessagedialog.h \
    qt/splashscreen.h \
    qt/trafficgraphwidget.h \
    qt/transactiondesc.h \
    qt/transactiondescdialog.h \
    qt/transactionfilterproxy.h \
    qt/transactionrecord.h \
    qt/transactiontablemodel.h \
    qt/transactionview.h \
    qt/utilitydialog.h \
    qt/walletframe.h \
    qt/walletmodel.h \
    qt/walletmodeltransaction.h \
    qt/walletview.h \
    qt/winshutdownmonitor.h \
    rpc/client.h \
    rpc/protocol.h \
    rpc/register.h \
    rpc/server.h \
    script/bitcoinconsensus.h \
    script/interpreter.h \
    script/ismine.h \
    script/script.h \
    script/script_error.h \
    script/sigcache.h \
    script/sign.h \
    script/standard.h \
    secp256k1/contrib/lax_der_parsing.h \
    secp256k1/contrib/lax_der_privatekey_parsing.h \
    secp256k1/include/secp256k1.h \
    secp256k1/include/secp256k1_ecdh.h \
    secp256k1/include/secp256k1_recovery.h \
    secp256k1/src/java/org_bitcoin_NativeSecp256k1.h \
    secp256k1/src/java/org_bitcoin_Secp256k1Context.h \
    secp256k1/src/modules/ecdh/main_impl.h \
    secp256k1/src/modules/ecdh/tests_impl.h \
    secp256k1/src/modules/recovery/main_impl.h \
    secp256k1/src/modules/recovery/tests_impl.h \
    secp256k1/src/basic-config.h \
    secp256k1/src/bench.h \
    secp256k1/src/ecdsa.h \
    secp256k1/src/ecdsa_impl.h \
    secp256k1/src/eckey.h \
    secp256k1/src/eckey_impl.h \
    secp256k1/src/ecmult.h \
    secp256k1/src/ecmult_const.h \
    secp256k1/src/ecmult_const_impl.h \
    secp256k1/src/ecmult_gen.h \
    secp256k1/src/ecmult_gen_impl.h \
    secp256k1/src/ecmult_impl.h \
    secp256k1/src/ecmult_static_context.h \
    secp256k1/src/field.h \
    secp256k1/src/field_10x26.h \
    secp256k1/src/field_10x26_impl.h \
    secp256k1/src/field_5x52.h \
    secp256k1/src/field_5x52_asm_impl.h \
    secp256k1/src/field_5x52_impl.h \
    secp256k1/src/field_5x52_int128_impl.h \
    secp256k1/src/field_impl.h \
    secp256k1/src/group.h \
    secp256k1/src/group_impl.h \
    secp256k1/src/hash.h \
    secp256k1/src/hash_impl.h \
    secp256k1/src/libsecp256k1-config.h \
    secp256k1/src/num.h \
    secp256k1/src/num_gmp.h \
    secp256k1/src/num_gmp_impl.h \
    secp256k1/src/num_impl.h \
    secp256k1/src/scalar.h \
    secp256k1/src/scalar_4x64.h \
    secp256k1/src/scalar_4x64_impl.h \
    secp256k1/src/scalar_8x32.h \
    secp256k1/src/scalar_8x32_impl.h \
    secp256k1/src/scalar_impl.h \
    secp256k1/src/scalar_low.h \
    secp256k1/src/scalar_low_impl.h \
    secp256k1/src/testrand.h \
    secp256k1/src/testrand_impl.h \
    secp256k1/src/util.h \
    support/allocators/secure.h \
    support/allocators/zeroafterfree.h \
    support/cleanse.h \
    support/events.h \
    support/lockedpool.h \
    test/data/base58_encode_decode.json.h \
    test/data/base58_keys_invalid.json.h \
    test/data/base58_keys_valid.json.h \
    test/data/script_tests.json.h \
    test/data/sighash.json.h \
    test/data/tx_invalid.json.h \
    test/data/tx_valid.json.h \
    test/scriptnum10.h \
    test/test_bitcoin.h \
    test/test_random.h \
    test/testutil.h \
    univalue/include/univalue.h \
    univalue/lib/univalue_escapes.h \
    univalue/lib/univalue_utffilter.h \
    univalue/univalue-config.h \
    wallet/test/wallet_test_fixture.h \
    wallet/coincontrol.h \
    wallet/crypter.h \
    wallet/db.h \
    wallet/rpcwallet.h \
    wallet/wallet.h \
    wallet/walletdb.h \
    zmq/zmqabstractnotifier.h \
    zmq/zmqconfig.h \
    zmq/zmqnotificationinterface.h \
    zmq/zmqpublishnotifier.h \
    addrdb.h \
    addrman.h \
    amount.h \
    arith_uint256.h \
    base58.h \
    blockencodings.h \
    bloom.h \
    chain.h \
    chainparams.h \
    chainparamsbase.h \
    chainparamsseeds.h \
    checkpoints.h \
    checkqueue.h \
    clientversion.h \
    coins.h \
    compat.h \
    compressor.h \
    core_io.h \
    core_memusage.h \
    cuckoocache.h \
    dbwrapper.h \
    hash.h \
    httprpc.h \
    httpserver.h \
    indirectmap.h \
    init.h \
    key.h \
    keystore.h \
    limitedmap.h \
    memusage.h \
    merkleblock.h \
    miner.h \
    net.h \
    net_processing.h \
    netaddress.h \
    netbase.h \
    netmessagemaker.h \
    noui.h \
    pow.h \
    prevector.h \
    protocol.h \
    pubkey.h \
    random.h \
    reverselock.h \
    scheduler.h \
    serialize.h \
    streams.h \
    sync.h \
    threadinterrupt.h \
    threadsafety.h \
    timedata.h \
    tinyformat.h \
    torcontrol.h \
    txdb.h \
    txmempool.h \
    ui_interface.h \
    uint256.h \
    undo.h \
    util.h \
    utilmoneystr.h \
    utilstrencodings.h \
    utiltime.h \
    validation.h \
    validationinterface.h \
    version.h \
    versionbits.h \
    warnings.h \
    qt/logon.h \
    qt/logondlg.h \
    qt/setdialog.h \
    qt/ipcdialog.h \
    qt/exportdialog.h \
    qt/ipcdetails.h \
    qt/upgradewidget.h \
    qt/ipcregister.h \
    qt/ipcregisterinformation.h \
    qt/successfultrade.h \
    qt/ipcinspectiontag.h \
    qt/ipctransfertransaction.h \
    qt/ipcauthorizationtransaction.h \
    qt/settingwidget.h \
    qt/passwordsettingwidget.h \
    qt/sendcoinsaffrimwidget.h \
    qt/addbookwidget.h \
    qt/recvhistory.h \
    qt/sendhistory.h \
    qt/CDateEdit.h \
    qt/editadddialog.h \
    qt/infowidget.h \
    qt/common.h \
    qt/feewidget.h \
    qt/sendresultwidget.h \
    qt/CDateEdit.h \
    qt/setrecovery.h \
    qt/setmessageauthentication.h \
    qt/setmessageauthenticationtab.h \
    qt/setmessagesignature.h \
    qt/titlestyle.h \
    qt/walletpagebuttons.h \
    qt/addfilterproxy.h \
    qt/addmodel.h \
    qt/AskPassphrase.h \
    qt/md5thread.h \
    qt/ipcselectaddress.h \
    qt/tallydscribe.h \
    qt/tallyclause.h \
    qt/tallyapply.h \
    qt/tallyaccount.h \
    qt/walletpassword.h \
    qt/contract.h \
    qt/ecoinaddressdialog.h \
    qt/clickqlabel.h \
    qt/cmessagebox.h \
    qt/myscrollarea.h \
    qt/qthyperlink.h \
    qt/tallyoutaccount.h \
    dpoc/CarditConsensusMeeting.h \
    dpoc/ConsensusAccount.h \
    dpoc/ConsensusAccountPool.h \
    dpoc/DpocInfo.h \
    dpoc/DpocMining.h \
    dpoc/MeetingItem.h \
    dpoc/testlog.h \
    dpoc/TimeService.h \
    qt/ecoincreatedialog.h \
    qt/ecoindialog.h \
    qt/ecoinsendaffrimdialog.h \
    qt/ecoinsenddialog.h \
    qt/ecoinsendresultdialog.h \
    qt/recvipchistory.h \
    qt/sendipchistory.h \
    qt/log/log.h \
    qt/MyLabel.h






SOURCES += \
    bench/base58.cpp \
    bench/bench.cpp \
    bench/bench_bitcoin.cpp \
    bench/ccoins_caching.cpp \
    bench/checkblock.cpp \
    bench/checkqueue.cpp \
    bench/coin_selection.cpp \
    bench/crypto_hash.cpp \
    bench/Examples.cpp \
    bench/lockedpool.cpp \
    bench/mempool_eviction.cpp \
    bench/perf.cpp \
    bench/rollingbloom.cpp \
    bench/verify_script.cpp \
    compat/glibc_compat.cpp \
    compat/glibc_sanity.cpp \
    compat/glibcxx_sanity.cpp \
    compat/strnlen.cpp \
    consensus/merkle.cpp \
    crypto/aes.cpp \
    crypto/hmac_sha256.cpp \
    crypto/hmac_sha512.cpp \
    crypto/ripemd160.cpp \
    crypto/sha1.cpp \
    crypto/sha256.cpp \
    crypto/sha512.cpp \
    leveldb/db/autocompact_test.cc \
    leveldb/db/builder.cc \
    leveldb/db/c.cc \
    leveldb/db/corruption_test.cc \
    leveldb/db/db_bench.cc \
    leveldb/db/db_impl.cc \
    leveldb/db/db_iter.cc \
    leveldb/db/db_test.cc \
    leveldb/db/dbformat.cc \
    leveldb/db/dbformat_test.cc \
    leveldb/db/dumpfile.cc \
    leveldb/db/fault_injection_test.cc \
    leveldb/db/filename.cc \
    leveldb/db/filename_test.cc \
    leveldb/db/leveldbutil.cc \
    leveldb/db/log_reader.cc \
    leveldb/db/log_test.cc \
    leveldb/db/log_writer.cc \
    leveldb/db/memtable.cc \
    leveldb/db/recovery_test.cc \
    leveldb/db/repair.cc \
    leveldb/db/skiplist_test.cc \
    leveldb/db/table_cache.cc \
    leveldb/db/version_edit.cc \
    leveldb/db/version_edit_test.cc \
    leveldb/db/version_set.cc \
    leveldb/db/version_set_test.cc \
    leveldb/db/write_batch.cc \
    leveldb/db/write_batch_test.cc \
    leveldb/doc/bench/db_bench_sqlite3.cc \
    leveldb/doc/bench/db_bench_tree_db.cc \
    leveldb/helpers/memenv/memenv.cc \
    leveldb/helpers/memenv/memenv_test.cc \
    leveldb/issues/issue178_test.cc \
    leveldb/issues/issue200_test.cc \
    leveldb/port/port_posix.cc \
    leveldb/port/port_win.cc \
    leveldb/table/block.cc \
    leveldb/table/block_builder.cc \
    leveldb/table/filter_block.cc \
    leveldb/table/filter_block_test.cc \
    leveldb/table/format.cc \
    leveldb/table/iterator.cc \
    leveldb/table/merger.cc \
    leveldb/table/table.cc \
    leveldb/table/table_builder.cc \
    leveldb/table/table_test.cc \
    leveldb/table/two_level_iterator.cc \
    leveldb/util/arena.cc \
    leveldb/util/arena_test.cc \
    leveldb/util/bloom.cc \
    leveldb/util/bloom_test.cc \
    leveldb/util/cache.cc \
    leveldb/util/cache_test.cc \
    leveldb/util/coding.cc \
    leveldb/util/coding_test.cc \
    leveldb/util/comparator.cc \
    leveldb/util/crc32c.cc \
    leveldb/util/crc32c_test.cc \
    leveldb/util/env.cc \
    leveldb/util/env_posix.cc \
    leveldb/util/env_test.cc \
    leveldb/util/env_win.cc \
    leveldb/util/filter_policy.cc \
    leveldb/util/hash.cc \
    leveldb/util/hash_test.cc \
    leveldb/util/histogram.cc \
    leveldb/util/logging.cc \
    leveldb/util/options.cc \
    leveldb/util/status.cc \
    leveldb/util/testharness.cc \
    leveldb/util/testutil.cc \
    policy/fees.cpp \
    policy/policy.cpp \
    policy/rbf.cpp \
    primitives/block.cpp \
    primitives/transaction.cpp \
    qt/test/compattests.cpp \
    qt/test/moc_compattests.cpp \
    qt/test/moc_paymentservertests.cpp \
    qt/test/moc_rpcnestedtests.cpp \
    qt/test/moc_uritests.cpp \
    qt/test/paymentservertests.cpp \
    qt/test/rpcnestedtests.cpp \
    qt/test/test_main.cpp \
    qt/test/uritests.cpp \
    qt/addressbookpage.cpp \
    qt/addresstablemodel.cpp \
    qt/askpassphrasedialog.cpp \
    qt/bantablemodel.cpp \
    qt/bitcoin.cpp \
    qt/bitcoinaddressvalidator.cpp \
    qt/bitcoinamountfield.cpp \
    qt/bitcoingui.cpp \
    qt/bitcoinstrings.cpp \
    qt/bitcoinunits.cpp \
    qt/clientmodel.cpp \
    qt/coincontroldialog.cpp \
    qt/coincontroltreewidget.cpp \
    qt/csvmodelwriter.cpp \
    qt/editaddressdialog.cpp \
    qt/guiutil.cpp \
    qt/intro.cpp \
    qt/modaloverlay.cpp \
    qt/networkstyle.cpp \
    qt/notificator.cpp \
    qt/openuridialog.cpp \
    qt/optionsdialog.cpp \
    qt/optionsmodel.cpp \
    qt/overviewpage.cpp \
    qt/paymentrequest.pb.cc \
    qt/paymentrequestplus.cpp \
    qt/paymentserver.cpp \
    qt/peertablemodel.cpp \
    qt/platformstyle.cpp \
    qt/qrc_bitcoin.cpp \
    qt/qrc_bitcoin_locale.cpp \
    qt/qvalidatedlineedit.cpp \
    qt/qvaluecombobox.cpp \
    qt/receivecoinsdialog.cpp \
    qt/receiverequestdialog.cpp \
    qt/recentrequeststablemodel.cpp \
    qt/rpcconsole.cpp \
    qt/sendcoinsdialog.cpp \
    qt/sendcoinsentry.cpp \
    qt/signverifymessagedialog.cpp \
    qt/splashscreen.cpp \
    qt/trafficgraphwidget.cpp \
    qt/transactiondesc.cpp \
    qt/transactiondescdialog.cpp \
    qt/transactionfilterproxy.cpp \
    qt/transactionrecord.cpp \
    qt/transactiontablemodel.cpp \
    qt/transactionview.cpp \
    qt/utilitydialog.cpp \
    qt/walletframe.cpp \
    qt/walletmodel.cpp \
    qt/walletmodeltransaction.cpp \
    qt/walletview.cpp \
    qt/winshutdownmonitor.cpp \
    rpc/blockchain.cpp \
    rpc/client.cpp \
    rpc/mining.cpp \
    rpc/misc.cpp \
    rpc/net.cpp \
    rpc/protocol.cpp \
    rpc/rawtransaction.cpp \
    rpc/server.cpp \
    script/bitcoinconsensus.cpp \
    script/interpreter.cpp \
    script/ismine.cpp \
    script/script.cpp \
    script/script_error.cpp \
    script/sigcache.cpp \
    script/sign.cpp \
    script/standard.cpp \
    support/cleanse.cpp \
    support/lockedpool.cpp \
    test/addrman_tests.cpp \
    test/allocator_tests.cpp \
    test/amount_tests.cpp \
    test/arith_uint256_tests.cpp \
    test/base32_tests.cpp \
    test/base58_tests.cpp \
    test/base64_tests.cpp \
    test/bip32_tests.cpp \
    test/blockencodings_tests.cpp \
    test/bloom_tests.cpp \
    test/bswap_tests.cpp \
    test/coins_tests.cpp \
    test/compress_tests.cpp \
    test/crypto_tests.cpp \
    test/cuckoocache_tests.cpp \
    test/dbwrapper_tests.cpp \
    test/DoS_tests.cpp \
    test/getarg_tests.cpp \
    test/hash_tests.cpp \
    test/key_tests.cpp \
    test/limitedmap_tests.cpp \
    test/main_tests.cpp \
    test/mempool_tests.cpp \
    test/merkle_tests.cpp \
    test/miner_tests.cpp \
    test/multisig_tests.cpp \
    test/net_tests.cpp \
    test/netbase_tests.cpp \
    test/pmt_tests.cpp \
    test/policyestimator_tests.cpp \
    test/pow_tests.cpp \
    test/prevector_tests.cpp \
    test/raii_event_tests.cpp \
    test/reverselock_tests.cpp \
    test/rpc_tests.cpp \
    test/sanity_tests.cpp \
    test/scheduler_tests.cpp \
    test/script_P2SH_tests.cpp \
    test/script_tests.cpp \
    test/scriptnum_tests.cpp \
    test/serialize_tests.cpp \
    test/sighash_tests.cpp \
    test/sigopcount_tests.cpp \
    test/skiplist_tests.cpp \
    test/streams_tests.cpp \
    test/test_bitcoin.cpp \
    test/test_bitcoin_fuzzy.cpp \
    test/testutil.cpp \
    test/timedata_tests.cpp \
    test/transaction_tests.cpp \
    test/txvalidationcache_tests.cpp \
    test/uint256_tests.cpp \
    test/univalue_tests.cpp \
    test/util_tests.cpp \
    test/versionbits_tests.cpp \
    univalue/gen/gen.cpp \
    univalue/lib/univalue.cpp \
    univalue/lib/univalue_read.cpp \
    univalue/lib/univalue_write.cpp \
    univalue/test/unitester.cpp \
    wallet/test/accounting_tests.cpp \
    wallet/test/crypto_tests.cpp \
    wallet/test/wallet_test_fixture.cpp \
    wallet/test/wallet_tests.cpp \
    wallet/crypter.cpp \
    wallet/db.cpp \
    wallet/rpcdump.cpp \
    wallet/rpcwallet.cpp \
    wallet/wallet.cpp \
    wallet/walletdb.cpp \
    zmq/zmqabstractnotifier.cpp \
    zmq/zmqnotificationinterface.cpp \
    zmq/zmqpublishnotifier.cpp \
    addrdb.cpp \
    addrman.cpp \
    amount.cpp \
    arith_uint256.cpp \
    base58.cpp \
    bitcoin-cli.cpp \
    bitcoin-tx.cpp \
    bitcoind.cpp \
    blockencodings.cpp \
    bloom.cpp \
    chain.cpp \
    chainparams.cpp \
    chainparamsbase.cpp \
    checkpoints.cpp \
    clientversion.cpp \
    coins.cpp \
    compressor.cpp \
    core_read.cpp \
    core_write.cpp \
    dbwrapper.cpp \
    hash.cpp \
    httprpc.cpp \
    httpserver.cpp \
    init.cpp \
    key.cpp \
    keystore.cpp \
    merkleblock.cpp \
    miner.cpp \
    net.cpp \
    net_processing.cpp \
    netaddress.cpp \
    netbase.cpp \
    noui.cpp \
    pow.cpp \
    protocol.cpp \
    pubkey.cpp \
    random.cpp \
    rest.cpp \
    scheduler.cpp \
    sync.cpp \
    threadinterrupt.cpp \
    timedata.cpp \
    torcontrol.cpp \
    txdb.cpp \
    txmempool.cpp \
    ui_interface.cpp \
    uint256.cpp \
    util.cpp \
    utilmoneystr.cpp \
    utilstrencodings.cpp \
    utiltime.cpp \
    validation.cpp \
    validationinterface.cpp \
    versionbits.cpp \
    warnings.cpp \
    crypto/ctaes/bench.c \
    crypto/ctaes/ctaes.c \
    crypto/ctaes/test.c \
    leveldb/db/c_test.c \
    secp256k1/contrib/lax_der_parsing.c \
    secp256k1/contrib/lax_der_privatekey_parsing.c \
    secp256k1/src/java/org_bitcoin_NativeSecp256k1.c \
    secp256k1/src/java/org_bitcoin_Secp256k1Context.c \
    secp256k1/src/bench_ecdh.c \
    secp256k1/src/bench_internal.c \
    secp256k1/src/bench_recover.c \
    secp256k1/src/bench_schnorr_verify.c \
    secp256k1/src/bench_sign.c \
    secp256k1/src/bench_verify.c \
    secp256k1/src/gen_context.c \
    secp256k1/src/secp256k1.c \
    secp256k1/src/tests.c \
    secp256k1/src/tests_exhaustive.c \
    qt/logon.cpp \
    qt/logondlg.cpp \
    qt/setdialog.cpp \
    qt/ipcdialog.cpp \
    qt/exportdialog.cpp \
    qt/ipcdetails.cpp \
    qt/upgradewidget.cpp \
    qt/ipcregister.cpp \
    qt/ipcregisterinformation.cpp \
    qt/successfultrade.cpp \
    qt/ipcinspectiontag.cpp \
    qt/ipctransfertransaction.cpp \
    qt/ipcauthorizationtransaction.cpp \
    qt/settingwidget.cpp \
    qt/passwordsettingwidget.cpp \
    qt/sendcoinsaffrimwidget.cpp \
    qt/addbookwidget.cpp \
    qt/recvhistory.cpp \
    qt/sendhistory.cpp \
    qt/editadddialog.cpp \
    qt/CDateEdit.cpp \
    qt/infowidget.cpp \
    qt/common.cpp \
    qt/feewidget.cpp \
    qt/sendresultwidget.cpp \
    qt/CDateEdit.cpp \
    qt/setrecovery.cpp \
    qt/setmessageauthentication.cpp \
    qt/setmessageauthenticationtab.cpp \
    qt/setmessagesignature.cpp \
    qt/titlestyle.cpp \
    qt/walletpagebuttons.cpp \
    qt/addfilterproxy.cpp \
    qt/addmodel.cpp \
    qt/AskPassphrase.cpp \
    qt/md5thread.cpp \
    qt/ipcselectaddress.cpp \
    qt/tallydscribe.cpp \
    qt/tallyclause.cpp \
    qt/tallyapply.cpp \
    qt/tallyaccount.cpp \
    qt/walletpassword.cpp \
    qt/tallyoutaccount.cpp \
    dpoc/CarditConsensusMeeting.cpp \
    dpoc/ConsensusAccount.cpp \
    dpoc/ConsensusAccountPool.cpp \
    dpoc/DpocInfo.cpp \
    dpoc/DpocMining.cpp \
    dpoc/MeetingItem.cpp \
    dpoc/TimeService.cpp \
    qt/ecoincreatedialog.cpp \
    qt/ecoindialog.cpp \
    qt/ecoinsendaffrimdialog.cpp \
    qt/ecoinsenddialog.cpp \
    qt/ecoinsendresultdialog.cpp \
    qt/recvipchistory.cpp \
    qt/sendipchistory.cpp \
    qt/log/log.cpp \
    qt/ecoinaddressdialog.cpp \
    qt/clickqlabel.cpp \
    qt/cmessagebox.cpp \
    qt/myscrollarea.cpp \
    qt/qthyperlink.cpp \
    qt/MyLabel.cpp



DISTFILES += \
    qt/res/png/sendlogo.png \
    qt/res/png/wallettab/ipcdialogdown.png \
    qt/res/png/wallettab/ipcdialogup.png \
    qt/res/png/wallettab/recivedown.png \
    qt/res/png/wallettab/reciveup.png \
    qt/res/png/wallettab/senddialogdown.png \
    qt/res/png/wallettab/senddialogup.png \
    qt/res/png/wallettab/setdialogdown.png \
    qt/res/png/wallettab/setdialogup.png \
    qt/res/png/wallettab/walletdialogdown.png \
    qt/res/png/wallettab/walletdialogup.png \
    qt/res/png/buttons/createwallet.png \
    qt/res/png/no1.png \
    qt/res/png/no2.png \
    qt/res/png/no3.png \
    qt/res/png/no4.png \
    qt/res/png/no5.png \
    qt/res/png/no6.png \
    qt/res/png/no7.png \
    qt/res/png/no8.png \
    qt/res/png/no9.png \
    qt/res/png/no0.png \
    qt/res/png/wallettab/ipcdialogdown.png \
    qt/res/png/wallettab/ipcdialogup.png \
    qt/res/png/wallettab/recivedown.png \
    qt/res/png/wallettab/reciveup.png \
    qt/res/png/wallettab/senddialogdown.png \
    qt/res/png/wallettab/senddialogup.png \
    qt/res/png/wallettab/setdialogdown.png \
    qt/res/png/wallettab/setdialogup.png \
    qt/res/png/wallettab/walletdialogdown.png \
    qt/res/png/wallettab/walletdialogup.png \
    qt/res/png/buttons/createwallet.png \
    qt/res/png/hight.png

OBJECTIVE_SOURCES += \
    qt/macdockiconhandler.mm \
    qt/macnotificationhandler.mm


RC_FILE += \
        ../src/qt/res/bitcoin-qt-res.rc
OTHER_FILES += \
    ../src/qt/res/bitcoin-qt-res.rc


QT += webkit
QT += webkitwidgets
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

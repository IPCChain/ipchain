// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "AskPassphrase.h"

#include "guiconstants.h"
#include "walletmodel.h"

#include "support/allocators/secure.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>

AskPassphrase::AskPassphrase(Mode _mode)
// mode(_mode),model(0)
{

    switch(mode)
    {
    case Encrypt: // Ask passphrase x2

        break;
    case Unlock: // Ask passphrase

        break;
    case Decrypt:   // Ask passphrase

        break;
    case ChangePass: // Ask old passphrase + new passphrase x2

        break;
    }

}

AskPassphrase::~AskPassphrase()
{
    //  secureClearPassFields();
    //  delete ui;
}

void AskPassphrase::setModel(WalletModel *_model)
{
    this->model = _model;
}

int AskPassphrase::getModel()
{

    if(!model)
        return -1;
    switch(mode)
    {
    case Encrypt: {
        return 1;
        break;
    case Unlock:
            return 2;
            break;
        case Decrypt:
            return 3;
            break;
        case ChangePass:
            return 4;
            break;
        }
    }


}

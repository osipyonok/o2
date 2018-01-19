#include <QDataStream>
#include <QDebug>
#include <QUrlQuery>

#include "o0baseauth.h"
#include "o0globals.h"
#include "o0settingsstore.h"
#include "o2replyserver.h"

static const quint16 DefaultLocalPort = 1965;

O0BaseAuth::O0BaseAuth(QObject *parent, O0AbstractStore *store, bool inUseExternalInterceptor): QObject(parent), store_(0), useExternalInterceptor_(inUseExternalInterceptor) {
    localPort_ = DefaultLocalPort;
    
    if(!inUseExternalInterceptor) {
        replyServer_ = new O2ReplyServer(this);
    }
    
    setStore(store);
}

void O0BaseAuth::processOAuthCallbackFromExternalInterceptor(const QString &inURLString)
{
    QUrl getTokenUrl(inURLString);
    QUrlQuery query(getTokenUrl);
    QList< QPair<QString, QString> > tokens = query.queryItems();
    
    QMultiMap<QString, QString> queryParams;
    QPair<QString, QString> tokenPair;
    foreach (tokenPair, tokens) {
        // FIXME: We are decoding key and value again. This helps with Google OAuth, but is it mandated by the standard?
        QString key = QUrl::fromPercentEncoding(QByteArray().append(tokenPair.first.trimmed().toLatin1()));
        QString value = QUrl::fromPercentEncoding(QByteArray().append(tokenPair.second.trimmed().toLatin1()));
        queryParams.insert(key, value);
    }
    
    processParamsFromExternalInterceptor(queryParams);
}

void O0BaseAuth::setStore(O0AbstractStore *store) {
    if (store_) {
        store_->deleteLater();
    }
    if (store) {
        store_ = store;
        store_->setParent(this);
    } else {
        store_ = new O0SettingsStore(O2_ENCRYPTION_KEY, this);
        return;
    }
}

bool O0BaseAuth::linked() {
    QString key = QString(O2_KEY_LINKED).arg(clientId_);
    bool result = !store_->value(key).isEmpty();
    qDebug() << "O0BaseAuth::linked:" << (result? "Yes": "No");
    return result;
}

void O0BaseAuth::setLinked(bool v) {
    qDebug() << "O0BaseAuth::setLinked:" << (v? "true": "false");
    bool oldValue = linked();
    QString key = QString(O2_KEY_LINKED).arg(clientId_);
    store_->setValue(key, v? "1": "");
    if (oldValue != v) {
        Q_EMIT linkedChanged();
    }
}

QString O0BaseAuth::tokenSecret() {
    QString key = QString(O2_KEY_TOKEN_SECRET).arg(clientId_);
    return store_->value(key);
}

void O0BaseAuth::setTokenSecret(const QString &v) {
    QString key = QString(O2_KEY_TOKEN_SECRET).arg(clientId_);
    store_->setValue(key, v);
    Q_EMIT tokenSecretChanged();
}

QString O0BaseAuth::token() {
    QString key = QString(O2_KEY_TOKEN).arg(clientId_);
    return store_->value(key);
}

void O0BaseAuth::setToken(const QString &v) {
    QString key = QString(O2_KEY_TOKEN).arg(clientId_);
    store_->setValue(key, v);
    Q_EMIT tokenChanged();
}

QString O0BaseAuth::clientId() {
    return clientId_;
}

void O0BaseAuth::setClientId(const QString &value) {
    clientId_ = value;
    Q_EMIT clientIdChanged();
}

QString O0BaseAuth::clientSecret() {
    return clientSecret_;
}

void O0BaseAuth::setClientSecret(const QString &value) {
    clientSecret_ = value;
    Q_EMIT clientSecretChanged();
}

QByteArray O0BaseAuth::replyContent() const {
    if(replyServer_ != NULL) {
        return replyServer_->replyContent();
    }
    
    return QByteArray();
}

void O0BaseAuth::setReplyContent(const QByteArray &value) {
    if(replyServer_ != NULL) {
        return replyServer_->setReplyContent(value);
    }
}

int O0BaseAuth::localPort() {
    return localPort_;
}

void O0BaseAuth::setLocalPort(int value) {
    qDebug() << "O0BaseAuth::setLocalPort:" << value;
    localPort_ = value;
    Q_EMIT localPortChanged();
}

QVariantMap O0BaseAuth::extraTokens() {
    QString key = QString(O2_KEY_EXTRA_TOKENS).arg(clientId_);
    QString value = store_->value(key);
    QByteArray bytes = QByteArray::fromBase64(value.toLatin1());
    QDataStream stream(&bytes, QIODevice::ReadOnly);
    stream >> extraTokens_;
    return extraTokens_;
}

void O0BaseAuth::setExtraTokens(QVariantMap extraTokens) {
    extraTokens_ = extraTokens;
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << extraTokens;
    QString key = QString(O2_KEY_EXTRA_TOKENS).arg(clientId_);
    store_->setValue(key, bytes.toBase64());
    Q_EMIT extraTokensChanged();
}

QByteArray O0BaseAuth::createQueryParameters(const QList<O0RequestParameter> &parameters) {
    QByteArray ret;
    bool first = true;
    foreach (O0RequestParameter h, parameters) {
        if (first) {
            first = false;
        } else {
            ret.append("&");
        }
        ret.append(QUrl::toPercentEncoding(h.name) + "=" + QUrl::toPercentEncoding(h.value));
    }
    return ret;
}

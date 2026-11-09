import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Sign in to the current Lemmy server. Only a session token is kept, never the password.
Popup {
    id: root
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - 32 : 420, 440)
    padding: 22

    onOpened: {
        Account.clearError()
        password.text = ""
        totp.text = ""
        user.forceActiveFocus()
    }
    Connections {
        target: Account
        function onSignedInChanged() { if (Account.signedIn) root.close() }
    }

    function submit() {
        if (!Account.busy)
            Account.signIn(user.text, password.text, totp.text)
    }

    background: Rectangle {
        radius: Theme.radiusCard + 2
        color: Theme.popup
        border.width: 1
        border.color: Theme.border
    }
    Overlay.modal: Rectangle { color: Theme.scrim }

    contentItem: ColumnLayout {
        spacing: 12

        RowLayout {
            spacing: 12
            Image {
                source: Theme.logo
                sourceSize: Qt.size(44, 44)
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
            }
            ColumnLayout {
                spacing: 0
                Text {
                    text: qsTr("Sign in")
                    color: Theme.text
                    font.pixelSize: Theme.fontHuge
                    font.weight: Font.Bold
                }
                Text {
                    text: qsTr("to your %1 account").arg(Api.instance)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontMeta
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Use the account you created on %1. Accounts from other Lemmy servers work only "
                       + "there: switch server in the menu first.").arg(Api.instance)
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.Wrap
        }

        Field {
            id: user
            placeholder: qsTr("Username or email")
            onAccepted: password.forceActiveFocus()
        }
        Field {
            id: password
            placeholder: qsTr("Password")
            secret: true
            onAccepted: Account.needsTotp && totp.text === "" ? totp.forceActiveFocus() : root.submit()
        }
        Field {
            id: totp
            visible: Account.needsTotp
            placeholder: qsTr("Two-factor code")
            digits: true
            onAccepted: root.submit()
        }

        Text {
            visible: Account.error !== ""
            Layout.fillWidth: true
            text: Account.error
            color: Theme.nsfw
            font.pixelSize: Theme.fontMeta
            font.weight: Font.DemiBold
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Quokka keeps a sign-in token on this device so you stay signed in. "
                       + "Your password is not stored.")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSmall
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Chip {
                text: qsTr("Create an account")
                clickable: true
                fontSize: Theme.fontSmall
                onClicked: Qt.openUrlExternally(Account.signupUrl)
            }
            Item { Layout.fillWidth: true }
            Chip {
                text: qsTr("Cancel")
                clickable: true
                onClicked: root.close()
            }
            Chip {
                text: Account.busy ? qsTr("Signing in…") : qsTr("Sign in")
                iconName: Account.busy ? "" : "check"
                selected: true
                clickable: !Account.busy
                onClicked: root.submit()
            }
        }
    }

    component Field: Rectangle {
        id: field
        property alias text: input.text
        property string placeholder
        property bool secret: false
        property bool digits: false
        signal accepted()
        function forceActiveFocus() { input.forceActiveFocus() }

        Layout.fillWidth: true
        Layout.preferredHeight: visible ? 46 : 0
        radius: 23
        color: Theme.surfaceAlt
        border.width: input.activeFocus ? 2 : 1
        border.color: input.activeFocus ? Theme.accent : Theme.border
        PlainField {
            id: input
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            placeholderText: field.placeholder
            placeholderTextColor: Theme.textMuted
            color: Theme.text
            font.pixelSize: Theme.fontBody
            echoMode: field.secret ? TextInput.Password : TextInput.Normal
            inputMethodHints: field.digits ? Qt.ImhDigitsOnly
                            : field.secret ? Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                            : Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            onAccepted: field.accepted()
        }
    }
}

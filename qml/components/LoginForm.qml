import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string serverUrl: ""
    property string username: ""
    property string password: ""
    property bool busy: false

    signal loginRequested(string serverUrl, string username, string password)

    implicitHeight: content.implicitHeight
    implicitWidth: content.implicitWidth

    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 10

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 10
            rowSpacing: 10

            Label { text: "Server URL" }
            TextField {
                Layout.fillWidth: true
                text: root.serverUrl
                placeholderText: "http://localhost:3000"
                onTextChanged: root.serverUrl = text
            }

            Label { text: "Username" }
            TextField {
                Layout.fillWidth: true
                text: root.username
                placeholderText: "username"
                onTextChanged: root.username = text
            }

            Label { text: "Password" }
            TextField {
                Layout.fillWidth: true
                text: root.password
                echoMode: TextInput.Password
                placeholderText: "password"
                onTextChanged: root.password = text
            }
        }

        Button {
            text: root.busy ? "Authorizing..." : "Login"
            enabled: !root.busy && root.serverUrl.length > 0 && root.username.length > 0 && root.password.length > 0
            onClicked: root.loginRequested(root.serverUrl, root.username, root.password)
        }
    }
}

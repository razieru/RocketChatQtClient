import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RocketChat.Client 1.0

ApplicationWindow {
    id: root
    width: 980
    height: 760
    visible: true
    title: "Rocket.Chat Login (QML)"

    AuthViewModel {
        id: vm
        Component.onCompleted: restoreSession()
    }

    readonly property int stateLogin: 0
    readonly property int stateTwoFactor: 1
    readonly property int stateAuthenticated: 2
    property int screenState: stateLogin

    function updateScreenState() {
        if (vm.authenticated) {
            screenState = stateAuthenticated
            return
        }

        if (vm.twoFactorRequired) {
            screenState = stateTwoFactor
            return
        }

        screenState = stateLogin
    }

    Connections {
        target: vm
        function onAuthenticatedChanged() { root.updateScreenState() }
        function onTwoFactorStateChanged() { root.updateScreenState() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: "Rocket.Chat QML Client"
            font.pixelSize: 24
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Restore Session"
                enabled: !vm.busy
                onClicked: vm.restoreSession()
            }

            Item {
                Layout.fillWidth: true
            }

            Label { text: screenState === stateLogin ? "State: Login" :
                          screenState === stateTwoFactor ? "State: TwoFactor" :
                          "State: Authenticated" }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: screenState

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                LoginForm {
                    anchors.fill: parent
                    busy: vm.busy
                    serverUrl: vm.serverUrl
                    username: vm.username
                    password: vm.password
                    onServerUrlChanged: vm.serverUrl = serverUrl
                    onUsernameChanged: vm.username = username
                    onPasswordChanged: vm.password = password
                    onLoginRequested: function(serverUrl, username, password) {
                        vm.serverUrl = serverUrl
                        vm.username = username
                        vm.password = password
                        vm.login()
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                GroupBox {
                    anchors.fill: parent
                    title: "Two-Factor Authentication"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            text: vm.twoFactorInvalid ? "Invalid 2FA code. Try again." : "2FA required."
                        }
                        Label {
                            text: "Method: " + vm.twoFactorMethod + (vm.twoFactorHint.length > 0 ? " (" + vm.twoFactorHint + ")" : "")
                        }
                        TextField {
                            Layout.fillWidth: true
                            text: vm.twoFactorCode
                            placeholderText: "2FA code"
                            onTextChanged: vm.twoFactorCode = text
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Button {
                                text: "Submit 2FA code"
                                enabled: !vm.busy && vm.twoFactorCode.length > 0
                                onClicked: vm.submitTwoFactorCode()
                            }
                            Button {
                                text: "Back to Login"
                                enabled: !vm.busy
                                onClicked: {
                                    vm.logout()
                                    root.screenState = root.stateLogin
                                }
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8
                    property int authTabIndex: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: "Logout"
                            enabled: !vm.busy
                            onClicked: vm.logout()
                        }
                        Label { text: "Authenticated" }
                    }

                    TabBar {
                        Layout.fillWidth: true
                        currentIndex: parent.authTabIndex
                        onCurrentIndexChanged: {
                            parent.authTabIndex = currentIndex
                            if (currentIndex === 0) {
                                vm.reloadChats()
                            }
                        }

                        TabButton { text: "Chats" }
                        TabButton { text: "Profile" }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: parent.authTabIndex

                        GroupBox {
                            title: "Chats"
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Item {
                                anchors.fill: parent

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 8

                                    Item {
                                        Layout.fillHeight: true
                                        Layout.preferredWidth: 340

                                        ChatListPanel {
                                            id: chatsPanel
                                            anchors.fill: parent
                                            chatsModel: vm.chatsModel
                                            currentChatIndex: vm.selectedChatIndex
                                            delegateWidth: width
                                            delegateHeight: 56
                                            onChatSelected: function(chatIndex) {
                                                vm.selectChat(chatIndex)
                                            }
                                        }
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true

                                        MessageListPanel {
                                            anchors.fill: parent
                                            messagesModel: vm.messagesModel
                                            selectedChatId: vm.selectedChatId
                                            hasChats: chatsPanel.chatsCount > 0
                                            delegateWidth: width
                                            useImplicitDelegateHeight: true
                                        }
                                    }
                                }
                            }
                        }

                        GroupBox {
                            title: "User Info JSON"
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 8

                                CheckBox {
                                    text: "Use human-readable chat names"
                                    checked: vm.preferHumanReadableChatNames
                                    onToggled: vm.preferHumanReadableChatNames = checked
                                }

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true

                                    TextArea {
                                        readOnly: true
                                        text: vm.userInfoJson.length > 0 ? vm.userInfoJson : "{}"
                                        wrapMode: TextArea.NoWrap
                                        font.family: "Consolas"
                                        selectByMouse: true
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Label {
            visible: vm.errorMessage.length > 0
            color: "tomato"
            text: vm.errorMessage
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}

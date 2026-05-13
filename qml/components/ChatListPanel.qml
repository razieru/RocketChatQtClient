import QtQuick
import QtQuick.Controls

Item {
    id: root
    property var chatsModel
    property string currentChatId: ""
    property real delegateWidth: 0
    property real delegateHeight: 56
    property var onChatSelected
    readonly property int chatsCount: chatsList.count

    function syncCurrentIndexFromId() {
        var row = -1
        if (root.currentChatId.length > 0 && root.chatsModel && typeof root.chatsModel.rowForChatId === "function") {
            row = root.chatsModel.rowForChatId(root.currentChatId)
        }
        if (chatsList.currentIndex !== row) {
            chatsList.currentIndex = row
        }
    }

    onCurrentChatIdChanged: Qt.callLater(syncCurrentIndexFromId)

    Component.onCompleted: syncCurrentIndexFromId()

    Connections {
        target: root.chatsModel
        function onModelReset() {
            root.syncCurrentIndexFromId()
        }
    }

    ListView {
        id: chatsList
        anchors.fill: parent
        clip: true
        model: root.chatsModel
        spacing: 4
        onCountChanged: root.syncCurrentIndexFromId()
        section.property: "section"
        section.criteria: ViewSection.FullString
        section.delegate: Rectangle {
            width: ListView.view ? ListView.view.width : 0
            height: section ? 28 : 0
            visible: section.length > 0
            color: "transparent"

            Label {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 2
                anchors.rightMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                text: section
                color: "#9aa4b2"
                font.bold: true
                elide: Label.ElideRight
            }
        }

        delegate: ChatListItemDelegate {
            width: root.delegateWidth
            height: root.delegateHeight
            onChatClicked: function(chatId) {
                if (root.onChatSelected) {
                    root.onChatSelected(chatId)
                }
            }
        }
    }

    Label {
        anchors.centerIn: parent
        visible: chatsList.count === 0
        text: "No chats available"
    }
}

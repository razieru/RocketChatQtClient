import QtQuick
import QtQuick.Controls

Item {
    id: root
    property var chatsModel
    property int currentChatIndex: -1
    property real delegateWidth: 0
    property real delegateHeight: 56
    property var onChatSelected
    readonly property int chatsCount: chatsList.count

    ListView {
        id: chatsList
        anchors.fill: parent
        clip: true
        model: root.chatsModel
        spacing: 4
        currentIndex: root.currentChatIndex
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
            onChatClicked: function(chatIndex) {
                if (root.onChatSelected) {
                    root.onChatSelected(chatIndex)
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

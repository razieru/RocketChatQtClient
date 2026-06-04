import QtQuick
import QtQuick.Controls

Item {
    id: root
    property var messagesModel
    property var usersModel
    property bool hideUsernames: false
    property string currentUsername: ""
    property string selectedChatId: ""
    property bool hasChats: false
    property bool useImplicitDelegateHeight: true

    ListView {
        id: messagesList
        anchors.fill: parent
        clip: true
        spacing: 6
        model: root.messagesModel

        onCountChanged: {
            if (count > 0) {
                positionViewAtEnd()
            }
        }

        section.property: "dateSection"
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
                horizontalAlignment: Text.AlignHCenter
                text: section
                color: "#9aa4b2"
                font.bold: true
                elide: Label.ElideRight
            }
        }

        delegate: MessageListItemDelegate {
            width: messagesList.width
            usersModel: root.usersModel
            hideUsernames: root.hideUsernames
            currentUsername: root.currentUsername
        }
    }

    Label {
        anchors.centerIn: parent
        visible: root.hasChats && root.selectedChatId.length === 0
        text: "Select a chat"
    }

    Label {
        anchors.centerIn: parent
        visible: root.selectedChatId.length > 0 && messagesList.count === 0
        text: "No messages"
    }
}

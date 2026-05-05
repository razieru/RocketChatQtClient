import QtQuick
import QtQuick.Controls

Item {
    id: root
    property var messagesModel
    property string selectedChatId: ""
    property bool hasChats: false
    property real delegateWidth: 0
    property bool useImplicitDelegateHeight: true
    property real delegateHeight: 0

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

        delegate: MessageListItemDelegate {
            width: root.delegateWidth
            height: root.useImplicitDelegateHeight ? implicitHeight : root.delegateHeight
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
